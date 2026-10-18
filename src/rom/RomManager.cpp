#include "ca_mame/rom/RomManager.hpp"
#include <fstream>
#include <iterator>
#include <utility>
namespace ca_mame::rom {
std::uint32_t RomManager::crc32(const std::vector<std::uint8_t>& bytes) noexcept {
    std::uint32_t crc=0xffffffffu;
    for(const auto byte:bytes) {
        crc^=byte;
        for(int bit=0;bit<8;++bit)
            crc=(crc>>1) ^ ((crc&1u)?0xedb88320u:0u);
    }
    return ~crc;
}
bool RomManager::readFile(const std::filesystem::path& path,std::vector<std::uint8_t>& bytes) {
    std::ifstream stream(path,std::ios::binary);
    if(!stream)return false;
    std::vector<std::uint8_t> loaded((std::istreambuf_iterator<char>(stream)),
                                      std::istreambuf_iterator<char>());
    if(stream.bad())return false;
    bytes=std::move(loaded);
    return true;
}
AuditResult RomManager::audit(const std::filesystem::path& directory,const RomDefinition& definition) {
    AuditResult result;
    result.filename=definition.filename;
    const auto relative=std::filesystem::path(definition.filename);
    if(relative.empty() || relative.is_absolute() || relative.has_root_name()) {
        result.message="Invalid ROM filename";
        return result;
    }
    for(const auto& component:relative)
        if(component=="..") {result.message="Parent directory traversal rejected";return result;}
    std::vector<std::uint8_t> bytes;
    if(!readFile(directory/relative,bytes)) {
        result.message="ROM missing or unreadable";
        return result;
    }
    result.actualSize=bytes.size();
    result.actualCrc32=crc32(bytes);
    if(bytes.size()!=definition.expectedSize) {
        result.message="ROM size mismatch";
        return result;
    }
    if(definition.verifyCrc32 && result.actualCrc32!=definition.expectedCrc32) {
        result.message="ROM CRC32 mismatch";
        return result;
    }
    result.ok=true;
    result.message="OK";
    return result;
}
}
