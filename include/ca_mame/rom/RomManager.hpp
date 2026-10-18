#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
namespace ca_mame::rom {
struct RomDefinition {
    std::string filename;
    std::size_t expectedSize=0;
    std::uint32_t expectedCrc32=0;
    bool verifyCrc32=false;
};
struct AuditResult {
    std::string filename;
    bool ok=false;
    std::string message;
    std::size_t actualSize=0;
    std::uint32_t actualCrc32=0;
};
class RomManager {
public:
    static std::uint32_t crc32(const std::vector<std::uint8_t>& bytes) noexcept;
    static AuditResult audit(const std::filesystem::path& directory,const RomDefinition& definition);
    static bool readFile(const std::filesystem::path& path,std::vector<std::uint8_t>& bytes);
};
}
