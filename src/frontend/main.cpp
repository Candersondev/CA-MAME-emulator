#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include "ca_mame/drivers/KonamiM2.hpp"
#include <string_view>
#include "ca_mame/core/MachineRegistry.hpp"
#include "ca_mame/rom/MidwayGameDetector.hpp"
#include "ca_mame/compat/MameBridge.hpp"
#include "ca_mame/compat/GameLibrary.hpp"
#include <fstream>
#include <sstream>
int main(int argc,char** argv) {
    if(argc==3 && std::string_view(argv[1])=="--native-detect") {
        const auto driver=ca_mame::rom::detectMidway8080Game(argv[2]);
        if(driver.empty()){std::cerr<<"Unknown native arcade ROM; no hardware selected\\n";return 2;}
        std::cout<<"Native arcade hardware: "<<driver<<"\\n";
        return 0;
    }
    if(argc>=4 && std::string_view(argv[1])=="--mame-library"){
        std::ifstream input(argv[2],std::ios::binary);
        if(!input){std::cerr<<"Unable to open MAME XML catalog\n";return 2;}
        const auto games=ca_mame::compat::MameBridge::parseListXml(input);
        const auto scan=ca_mame::compat::GameLibrary::scan(games,argv[3]);
        if(!scan.directoryAccessible){std::cerr<<"ROM directory is not accessible\n";return 2;}
        const bool localOnly=argc>=5 && std::string_view(argv[4])=="--local";
        const auto matches=ca_mame::compat::GameLibrary::filter(scan,{},localOnly);
        for(const auto& item:matches)
            std::cout<<item.game.name<<" | "<<item.game.description
                     <<" | "<<(item.localArchive?"archive present":"no archive")
                     <<" | "<<(item.hasChdDirectory?"game directory present":"no game directory")<<"\n";
        std::cout<<"Catalog: "<<games.size()<<", archives: "<<scan.archiveCount
                 <<", unmatched archives: "<<scan.unknownArchives
                 <<", shown: "<<matches.size()<<" (presence only; NOT ROM verification)\n";
        return 0;
    }
    if(argc>=4 && (std::string_view(argv[1])=="--mame-launch" ||
                    std::string_view(argv[1])=="--mame-verify")){
        const bool verify=std::string_view(argv[1])=="--mame-verify";
        const auto rom=argc>=5?std::optional<std::filesystem::path>(argv[4]):std::nullopt;
        const auto p=verify?ca_mame::compat::MameBridge::verifyRomsPlan(argv[2],argv[3],rom):
                            ca_mame::compat::MameBridge::plan(argv[2],argv[3],rom);
        if(!p){std::cerr<<"Invalid MAME command arguments\n";return 2;}
        const int status=ca_mame::compat::MameBridge::launch(*p);
        if(status<0){std::cerr<<"Could not start external MAME executable\n";return 3;}
        return status;
    }

    if(argc>=3 && std::string_view(argv[1])=="--mame-catalog"){
        std::ifstream input(argv[2],std::ios::binary);
        if(!input){std::cerr<<"Unable to open MAME XML catalog\\n";return 2;}
        std::ostringstream buffer;buffer<<input.rdbuf();
        const auto games=ca_mame::compat::MameBridge::parseListXml(buffer.str());
        for(const auto& game:games)std::cout<<game.name<<" | "<<game.description<<" | "<<game.year<<" | "<<(game.runnable?"runnable flag":"not runnable")<<"\\n";
        std::cout<<"Catalog entries: "<<games.size()<<" (metadata only)\\n";return 0;
    }
    if(argc>=4 && std::string_view(argv[1])=="--mame-plan"){
        const auto plan=ca_mame::compat::MameBridge::plan(argv[2],argv[3],argc>=5?std::optional<std::filesystem::path>(argv[4]):std::nullopt);
        if(!plan){std::cerr<<"Invalid MAME launch plan\\n";return 2;}
        std::cout<<"Executable: "<<plan->executable.string()<<"\\n";
        for(const auto& arg:plan->arguments)std::cout<<"Argument: "<<arg<<"\\n";
        std::cout<<"Dry run only; no external process launched\\n";return 0;
    }

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
        board.powerOnReset();
        auto& cpu=board.cpu(0);
        std::cout<<"M2 diagnostic trace: experimental CPU, not a firmware boot guarantee\n";
        for(unsigned i=0;i<limit;++i){
            const auto address=cpu.pc();
            std::uint32_t opcode=0;
            if(!board.readWord(address,opcode)){std::cout<<"Unmapped fetch at 0x"<<std::hex<<address<<"\n";break;}
            std::cout<<std::dec<<i<<" PC=0x"<<std::hex<<std::setw(8)<<std::setfill('0')<<address
                     <<" opcode=0x"<<std::setw(8)<<opcode<<std::dec<<"\n";
            if(cpu.step()==0){
                const auto reason=cpu.stopReason();
                const char* label=reason==ca_mame::PowerPC602::StopReason::InstructionFetch?"instruction fetch":
                    reason==ca_mame::PowerPC602::StopReason::DataAccess?"data access":
                    reason==ca_mame::PowerPC602::StopReason::UnsupportedInstruction?"unsupported opcode":"unknown";
                std::cout<<"Stopped: "<<label<<" at 0x"<<std::hex<<cpu.faultAddress()
                         <<" opcode=0x"<<cpu.faultInstruction()<<std::dec<<"\n";break;
            }
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
