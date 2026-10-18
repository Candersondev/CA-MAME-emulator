#include <iostream>
#include "ca_mame/core/Machine.hpp"
int main() {
    ca_mame::Machine machine;
    machine.reset();
    std::cout << "CA-MAME 0.1.0 - emulator framework initialized (no arcade driver loaded)\n";
    return 0;
}
