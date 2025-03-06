#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <direct.h>
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netinet/in.h>
#endif

struct Packet {
    char symbol[5];
    char buySellindicator;
    uint32_t quantity;
    uint32_t price;
    uint32_t packetSequence;
};

class Client {
public:
    Client();
    ~Client();
    void execute();

private:
    void setupNetwork();
    void establishConnection();
    void requestAllData();
    void receiveData();
    void identifyMissingSequences();
    void requestMissingData();
    void exportToJson(); 
    
    std::vector<Packet> packetList;
    std::vector<int> missingSequenceList;

    #ifdef _WIN32
        SOCKET sock = INVALID_SOCKET;
        WSADATA wsaData;
        struct sockaddr_in serv_addr;
    #else
        int networkSocket = -1;  // Previously undeclared
        struct sockaddr_in serverAddress; // Previously undeclared
    #endif
};
