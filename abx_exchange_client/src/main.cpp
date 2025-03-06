#include "Client.hpp"
#include <iostream>

int main() {
    try {
        Client clientObj;
        clientObj.execute();
        std::cout << "Data collection complete. Check output.json for results." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
