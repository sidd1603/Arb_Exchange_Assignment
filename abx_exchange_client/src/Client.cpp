#include "Client.hpp"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <chrono>
#include <thread>
#include <set>

#ifdef _WIN32
    #pragma comment(lib, "Ws2_32.lib")
#endif

Client::Client() {
    setupNetwork();
}


Client::~Client() {
    if (networkSocket != -1) {
        #ifdef _WIN32
            shutdown(networkSocket, SD_BOTH);
            closesocket(networkSocket);
        #else
            shutdown(networkSocket, SHUT_RDWR);
            close(networkSocket);
        #endif
    }
    #ifdef _WIN32
        WSACleanup();
    #endif
}

void Client::setupNetwork() {
    #ifdef _WIN32
        if (WSAStartup(MAKEWORD(2, 2), &networkData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
    #endif
}

void Client::establishConnection() {
    std::cout << "Attempting to connect to server..." << std::endl;
    #ifdef _WIN32
        if ((networkSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
            throw std::runtime_error("Socket creation error");
        }
    #else
        if ((networkSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            throw std::runtime_error("Socket creation error");
        }
    #endif

    std::memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(3000);

    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address");
    }

    if (connect(networkSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        throw std::runtime_error("Connection failed");
    }
    
    // Set receive timeout to 5 seconds
    #ifdef _WIN32
        DWORD timeoutDuration = 5000;
        setsockopt(networkSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutDuration, sizeof(timeoutDuration));
    #else
        struct timeval timeoutDuration;
        timeoutDuration.tv_sec = 5;
        timeoutDuration.tv_usec = 0;
        setsockopt(networkSocket, SOL_SOCKET, SO_RCVTIMEO, &timeoutDuration, sizeof(timeoutDuration));
    #endif

    std::cout << "Successfully connected to server" << std::endl;
}

void Client::requestAllData() {
    std::cout << "Requesting all packets..." << std::endl;
    char dataRequest[2] = {1, 0};
    if (send(networkSocket, dataRequest, 2, 0) != 2) {
        throw std::runtime_error("Failed to send request");
    }
    
    // Wait a bit for server to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    receiveData();
}

void Client::receiveData() {
    const int DATA_SIZE = 17; // 4 + 1 + 4 + 4 + 4
    char dataBuffer[DATA_SIZE];
    int packetCount = 0;
    int noDataReads = 0;
    const int MAX_NO_DATA_READS = 5;  // Increased from 3 to 5
    
    std::cout << "Waiting to receive packets..." << std::endl;
    while (noDataReads < MAX_NO_DATA_READS) {
        int bytesRead = recv(networkSocket, dataBuffer, DATA_SIZE, 0);
        if (bytesRead == 0) {
            std::cout << "Server closed connection" << std::endl;
            break;
        } else if (bytesRead < 0) {
            #ifdef _WIN32
                int errorCode = WSAGetLastError();
                if (errorCode == WSAETIMEDOUT) {
                    std::cout << "Receive timeout, trying again..." << std::endl;
                    noDataReads++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Add delay between retries
                    continue;
                }
            #else
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::cout << "Receive timeout, trying again..." << std::endl;
                    noDataReads++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Add delay between retries
                    continue;
                }
            #endif
            throw std::runtime_error("Error receiving data");
        }
        
        noDataReads = 0; // Reset counter on successful read
        std::cout << "Received " << bytesRead << " bytes" << std::endl;
        
        if (bytesRead == DATA_SIZE) {
            Packet dataPacket;
            int dataOffset = 0;
            
            // Read symbol (4 bytes)
            std::memcpy(dataPacket.symbol, dataBuffer + dataOffset, 4);
            dataPacket.symbol[4] = '\0';
            dataOffset += 4;
            
            // Read buy/sell indicator (1 byte)
            dataPacket.buySellindicator = dataBuffer[dataOffset];
            dataOffset += 1;
            
            // Read quantity (4 bytes)
            uint32_t networkQuantity;
            std::memcpy(&networkQuantity, dataBuffer + dataOffset, 4);
            dataPacket.quantity = ntohl(networkQuantity);
            dataOffset += 4;
            
            // Read price (4 bytes)
            uint32_t networkPrice;
            std::memcpy(&networkPrice, dataBuffer + dataOffset, 4);
            dataPacket.price = ntohl(networkPrice);
            dataOffset += 4;
            
            // Read sequence number (4 bytes)
            uint32_t networkSeq;
            std::memcpy(&networkSeq, dataBuffer + dataOffset, 4);
            dataPacket.packetSequence = ntohl(networkSeq);
            
            // Check if we already have this packet
            bool isDuplicate = false;
            for (const auto& existingPacket : packetList) {
                if (existingPacket.packetSequence == dataPacket.packetSequence) {
                    isDuplicate = true;
                    break;
                }
            }
            
            if (!isDuplicate) {
                packetList.push_back(dataPacket);
                packetCount++;
                std::cout << "Processed packet " << packetCount << ": Symbol=" << dataPacket.symbol 
                          << " Seq=" << dataPacket.packetSequence << std::endl;
            } else {
                std::cout << "Skipping duplicate packet with sequence " << dataPacket.packetSequence << std::endl;
            }
        }
    }
    
    if (packetCount == 0) {
        throw std::runtime_error("No packets received from server");
    }
    
    std::cout << "Total packets received: " << packetCount << std::endl;
}

void Client::identifyMissingSequences() {
    if (packetList.empty()) return;
    
    // First, find the maximum sequence number
    int maxSequence = 1;
    for (const auto& packet : packetList) {
        maxSequence = std::max(maxSequence, static_cast<int>(packet.packetSequence));
    }
    
    // Create a set of received sequences for quick lookup
    std::set<int> receivedSequences;
    for (const auto& packet : packetList) {
        receivedSequences.insert(packet.packetSequence);
    }
    
    // Find all missing sequences from 1 to maxSequence-1 (last packet is never missed)
    missingSequenceList.clear();
    for (int seq = 1; seq < maxSequence; ++seq) {
        if (receivedSequences.find(seq) == receivedSequences.end()) {
            missingSequenceList.push_back(seq);
        }
    }
    
    if (!missingSequenceList.empty()) {
        std::cout << "Found " << missingSequenceList.size() << " missing sequences: ";
        for (int seq : missingSequenceList) {
            std::cout << seq << " ";
        }
        std::cout << std::endl;
    }
}

void Client::requestMissingData() {
    if (missingSequenceList.empty()) return;
    
    std::cout << "Requesting " << missingSequenceList.size() << " missing packets..." << std::endl;
    
    // Keep track of sequences we still need to get
    std::set<int> remainingSequences(missingSequenceList.begin(), missingSequenceList.end());
    const int MAX_RETRIES_PER_PACKET = 10;  // Increased retries per packet
    
    // Try each sequence multiple times
    for (int seq : missingSequenceList) {
        int retryCount = 0;
        bool packetReceived = false;
        
        while (!packetReceived && retryCount < MAX_RETRIES_PER_PACKET) {
            try {
                // New connection for each attempt
                establishConnection();
                
                std::cout << "Requesting missing packet " << seq << " (attempt " << (retryCount + 1) << "/" << MAX_RETRIES_PER_PACKET << ")" << std::endl;
                
                // Send request for this sequence
                char dataRequest[2] = {2, static_cast<char>(seq)};
                if (send(networkSocket, dataRequest, 2, 0) != 2) {
                    std::cerr << "Warning: Failed to send request for packet " << seq << std::endl;
                    continue;
                }
                
                // Store current packet count
                size_t previousCount = packetList.size();
                
                try {
                    // Wait a bit before receiving
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    
                    // Try to receive the packet
                    receiveData();
                    
                    // Check if we got any new packets
                    if (packetList.size() > previousCount) {
                        // Look for our sequence in the new packets
                        for (size_t i = previousCount; i < packetList.size(); ++i) {
                            if (packetList[i].packetSequence == seq) {
                                std::cout << "Successfully received packet " << seq << std::endl;
                                packetReceived = true;
                                remainingSequences.erase(seq);
                                break;
                            }
                        }
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Warning: Failed to receive packet " << seq << ": " << e.what() << std::endl;
                }
                
                // Close connection after each attempt
                #ifdef _WIN32
                    closesocket(networkSocket);
                    networkSocket = INVALID_SOCKET;
                #else
                    close(networkSocket);
                    networkSocket = -1;
                #endif
                
                if (!packetReceived) {
                    retryCount++;
                    if (retryCount < MAX_RETRIES_PER_PACKET) {
                        std::cout << "Retrying packet " << seq << " after a short delay..." << std::endl;
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));  // Wait between retries
                    }
                }
                
            } catch (const std::exception& e) {
                std::cerr << "Warning: Connection error while requesting packet " << seq << ": " << e.what() << std::endl;
                retryCount++;
                if (retryCount < MAX_RETRIES_PER_PACKET) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Longer wait after connection error
                }
            }
        }
        
        if (!packetReceived) {
            std::cerr << "Warning: Failed to retrieve packet " << seq << " after " << MAX_RETRIES_PER_PACKET << " attempts" << std::endl;
        }
    }
    
    if (!remainingSequences.empty()) {
        std::cerr << "Warning: Failed to retrieve " << remainingSequences.size() << " packets after all retries." << std::endl;
        std::cerr << "Missing sequences: ";
        for (int seq : remainingSequences) {
            std::cerr << seq << " ";
        }
        std::cerr << std::endl;
    }
}

void Client::exportToJson() {
    if (packetList.empty()) {
        throw std::runtime_error("No packets to write to JSON");
    }

    // Sort packets by sequence number
    std::sort(packetList.begin(), packetList.end(), 
        [](const Packet& a, const Packet& b) {
            return a.packetSequence < b.packetSequence;
        });
    
    // Verify sequence continuity
    int expectedSequence = 1;
    std::vector<int> missingSequences;
    for (const auto& packet : packetList) {
        while (expectedSequence < packet.packetSequence) {
            missingSequences.push_back(expectedSequence);
            expectedSequence++;
        }
        expectedSequence = packet.packetSequence + 1;
    }
    
    if (!missingSequences.empty()) {
        std::cerr << "Warning: Still missing sequences in final output: ";
        for (int seq : missingSequences) {
            std::cerr << seq << " ";
        }
        std::cerr << std::endl;
    }
    
    std::cout << "Writing " << packetList.size() << " packets to output.json" << std::endl;
    nlohmann::json jsonOutput = nlohmann::json::array();
    
    for (const auto& packet : packetList) {
        nlohmann::json packetJson;
        packetJson["symbol"] = std::string(packet.symbol);
        packetJson["buySellindicator"] = std::string(1, packet.buySellindicator);
        packetJson["quantity"] = packet.quantity;
        packetJson["price"] = packet.price;
        packetJson["packetSequence"] = packet.packetSequence;
        jsonOutput.push_back(packetJson);
    }
    
    // Get current working directory
    char currentDirectory[1024];
    #ifdef _WIN32
        if (_getcwd(currentDirectory, sizeof(currentDirectory)) == nullptr) {
    #else
        if (getcwd(currentDirectory, sizeof(currentDirectory)) == nullptr) {
    #endif
        throw std::runtime_error("Failed to get current working directory");
    }
    
    std::string directoryPath(currentDirectory);
    std::replace(directoryPath.begin(), directoryPath.end(), '\\', '/');
    std::string jsonFilePath = directoryPath + "/output.json";
    std::cout << "Writing to file: " << jsonFilePath << std::endl;
    
    std::ofstream jsonFile(jsonFilePath.c_str(), std::ios::out | std::ios::binary);
    if (!jsonFile.is_open()) {
        throw std::runtime_error("Failed to open output.json for writing at " + jsonFilePath);
    }
    
    std::string jsonString = jsonOutput.dump(4);
    std::cout << "JSON content to write (" << jsonString.length() << " bytes):" << std::endl;
    std::cout << jsonString << std::endl;
    
    jsonFile.write(jsonString.c_str(), jsonString.length());
    jsonFile.flush();
    jsonFile.close();
    
    if (jsonFile.fail()) {
        throw std::runtime_error("Error writing to output.json");
    }
    
    std::cout << "Successfully wrote to output.json at " << jsonFilePath << std::endl;
}

void Client::execute() {
    try {
        establishConnection();
        requestAllData();
        
        // Store the initially received packets
        std::vector<Packet> initialPacketList = packetList;
        
        identifyMissingSequences();
        if (!missingSequenceList.empty()) {
            std::cout << "Missing sequences: ";
            for (int seq : missingSequenceList) {
                std::cout << seq << " ";
            }
            std::cout << std::endl;
            
            try {
                requestMissingData();
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to get missing packets: " << e.what() << std::endl;
                // Restore initial packets if missing packet request failed completely
                if (packetList.empty()) {
                    packetList = initialPacketList;
                }
            }
        }
        
        // Always try to write what we have
        if (!packetList.empty()) {
            exportToJson();
        } else {
            throw std::runtime_error("No packets available to write");
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        throw; // Re-throw to ensure the program exits with an error
    }
}
