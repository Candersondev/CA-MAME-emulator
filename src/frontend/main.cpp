#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include "ca_mame/drivers/KonamiM2.hpp"
#include <string_view>
#include "ca_mame/core/MachineRegistry.hpp"
int main(int argc,char** argv) {
    if(argc>=3 && std::string_view(argv[1])=="--m2-trace"){
        ca_mame::drivers::KonamiM2 board;
        if(!board.loadBootRom(argv[2])){
            std::cerr<<"M2 trace: boot ROM must be an accessible 2097152-byte file.\n";
            return 2;
        }
        unsigned limit=100;
        if(argc>=4){
            try {limit=static_cast<unsigned>(std::stoul(argv[3]));}
            catch(...){std::cerr<<"Invalid instruction count\n";return 2;}
            if(limit>100000){std::cerr<<"Maximum trace length is 100000\n";return 2;}
        }
        auto& cpu=board.cpu(0);
        cpu.setPC(ca_mame::drivers::M2MemoryMap::bootRomBase);
        std::cout<<"M2 diagnostic trace: experimental CPU, not a firmware boot guarantee\n";
        for(unsigned i=0;i<limit;++i){
            const auto address=cpu.pc();
            std::uint32_t opcode=0;
            if(!board.readWord(address,opcode)){std::cout<<"Unmapped fetch at 0x"<<std::hex<<address<<"\n";break;}
            std::cout<<std::dec<<i<<" PC=0x"<<std::hex<<std::setw(8)<<std::setfill('0')<<address
                     <<" opcode=0x"<<std::setw(8)<<opcode<<std::dec<<"\n";
            if(cpu.step()==0){std::cout<<"Stopped: unsupported instruction or bus fault\n";break;}
        }
        return 0;
    }
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
