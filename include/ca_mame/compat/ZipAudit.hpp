#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
namespace ca_mame::compat {
struct ZipEntryAudit {std::string name;std::uint32_t crc=0;bool verified=false;bool supported=false;};
struct ZipAuditResult {bool readable=false;std::vector<ZipEntryAudit> entries;std::string error;};
ZipAuditResult auditZip(const std::filesystem::path& path);
}
