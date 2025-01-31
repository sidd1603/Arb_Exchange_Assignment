cmake_minimum_required(VERSION 3.11)

# Set compilers before project() call
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

project(abx_client)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

add_executable(client 
    src/main.cpp
    src/Client.cpp
)

target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(client PRIVATE nlohmann_json::nlohmann_json)

if(WIN32)
    target_link_libraries(client PRIVATE wsock32 ws2_32)
endif()