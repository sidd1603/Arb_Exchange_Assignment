# ABX Exchange Client

This is a C++ client application designed to connect with the ABX mock exchange server to receive and process stock ticker information.

## Project Layout

- `src/` - Contains the C++ client source files
  - `main.cpp` - The entry point of the application
  - `Client.cpp` - Implementation of the client
  - `Client.hpp` - Header file for the client
- `server/` - Contains Node.js server files
  - `main.js` - Server implementation
  - `package.json` - Node.js package configuration
- `CMakeLists.txt` - Configuration file for CMake build

## Requirements

- MSYS2 with CLANG64 environment (for Windows users)
- CMake (version 3.11 or newer)
- A compiler compatible with C++17
- Node.js (required for server execution)
- Internet access (for downloading dependencies)

## Setup Guide

1. Download and install MSYS2 from https://www.msys2.org/
2. Launch the MSYS2 CLANG64 terminal and install the necessary packages:
   ```bash
   pacman -S mingw-w64-clang-x86_64-cmake mingw-w64-clang-x86_64-clang
   ```
3. Download and install Node.js from https://nodejs.org/ (LTS version is recommended)

## Server Execution

1. Navigate to the server directory in a terminal:
   ```bash
   cd server
   ```

2. Launch the server:
   ```bash
   node main.js
   ```

The server will begin listening on port 3000.

## Client Build and Execution

1. Open the MSYS2 CLANG64 terminal in the project's root directory

2. Create and navigate to the build directory:
   ```bash
   mkdir build
   cd build
   ```

3. Configure and compile the project:
   ```bash
   cmake ..
   cmake --build .
   ```

4. Execute the client:
   ```bash
   ./client
   ```

The client will:
- Establish a connection with the server
- Request and receive stock ticker data
- Manage any missing sequences
- Produce an `output.json` file with the results

## Output Details

The program generates an `output.json` file that contains the received stock ticker data in JSON format. Each entry includes:
- Symbol
- Buy/Sell indicator
- Quantity
- Price
- Packet sequence number

## Error Management

- The client will automatically retry if packet reception fails
- Missing sequences are identified and requested separately
- Connection errors are managed gracefully
