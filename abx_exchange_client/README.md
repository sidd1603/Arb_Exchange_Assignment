# ABX Exchange Client

A C++ client application that connects to the ABX mock exchange server to receive and process stock ticker data.

## Project Structure

- `src/` - C++ client source files
  - `main.cpp` - Entry point
  - `Client.cpp` - Client implementation
  - `Client.hpp` - Client header file
- `server/` - Node.js server files
  - `main.js` - Server implementation
  - `package.json` - Node.js package configuration
- `CMakeLists.txt` - CMake build configuration

## Prerequisites

- MSYS2 with CLANG64 environment (for Windows)
- CMake (version 3.11 or higher)
- C++17 compatible compiler
- Node.js (for running the server)
- Internet connection (for downloading dependencies)

## Setup Instructions

1. Install MSYS2 from https://www.msys2.org/
2. Open MSYS2 CLANG64 terminal and install required packages:
   ```bash
   pacman -S mingw-w64-clang-x86_64-cmake mingw-w64-clang-x86_64-clang
   ```
3. Install Node.js from https://nodejs.org/ (LTS version recommended)

## Running the Server

1. Open a terminal in the server directory:
   ```bash
   cd server
   ```

2. Start the server:
   ```bash
   node main.js
   ```

The server will start listening on port 3000.

## Building and Running the Client

1. Open MSYS2 CLANG64 terminal in the project root

2. Create and enter build directory:
   ```bash
   mkdir build
   cd build
   ```

3. Configure and build:
   ```bash
   cmake ..
   cmake --build .
   ```

4. Run the client:
   ```bash
   ./client
   ```

The client will:
- Connect to the server
- Request and receive stock ticker data
- Handle any missing sequences
- Generate an output.json file with the results

## Output

The program generates an `output.json` file containing the received stock ticker data in JSON format. Each entry includes:
- Symbol
- Buy/Sell indicator
- Quantity
- Price
- Packet sequence number

## Error Handling

- The client automatically retries if it fails to receive any packets
- Missing sequences are detected and requested separately
- Connection errors are handled gracefully 