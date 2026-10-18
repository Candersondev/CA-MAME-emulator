#include "ca_mame/rom/RomManager.hpp"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
namespace {
int failures=0;
void check(bool ok,const char* message) {if(!ok){std::cerr<<"FAIL: "<<message<<"\n";++failures;}}
}
int main() {
    using namespace ca_mame::rom;
    const std::vector<std::uint8_t> standard={'1','2','3','4','5','6','7','8','9'};
    check(RomManager::crc32(standard)==0xcbf43926u,"CRC32 standard vector");
    check(RomManager::crc32({})==0u,"CRC32 empty vector");
    const auto dir=std::filesystem::temp_directory_path()/"ca_mame_rom_audit_test";
    std::filesystem::create_directories(dir);
    {
        std::ofstream file(dir/"sample.bin",std::ios::binary);
        file.write(reinterpret_cast<const char*>(standard.data()),static_cast<std::streamsize>(standard.size()));
    }
    const RomDefinition good={"sample.bin",9,0xcbf43926u,true};
    const auto result=RomManager::audit(dir,good);
    check(result.ok && result.actualSize==9 && result.actualCrc32==0xcbf43926u,"valid ROM audit");
    check(!RomManager::audit(dir,{"sample.bin",8,0,false}).ok,"reject wrong size");
    check(!RomManager::audit(dir,{"sample.bin",9,0,true}).ok,"reject wrong CRC");
    check(!RomManager::audit(dir,{"missing.bin",9,0,false}).ok,"reject missing ROM");
    check(!RomManager::audit(dir,{"../sample.bin",9,0,false}).ok,"reject path traversal");
    std::vector<std::uint8_t> loaded;
    check(RomManager::readFile(dir/"sample.bin",loaded) && loaded==standard,"read raw ROM");
    std::filesystem::remove_all(dir);
    return failures?1:0;
}
