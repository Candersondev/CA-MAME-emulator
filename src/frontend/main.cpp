#include <iostream>
#include <string_view>
#include "ca_mame/core/MachineRegistry.hpp"
int main(int argc,char** argv) {
    const auto registry=ca_mame::makeDefaultRegistry();
    if(argc==2 && std::string_view(argv[1])=="--list") {
        for(const auto& entry:registry.entries())
            std::cout<<entry.id<<" - "<<entry.description<<"\n";
        return 0;
    }
    std::cout<<"CA-MAME 0.1.0 - multi-system arcade framework\n"
             <<"Use --list to view registered arcade hardware drivers.\n";
    return 0;
}
