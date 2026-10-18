#pragma once
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
namespace ca_mame::compat {
enum class AuditState {Unknown,Passed,Failed};
class AuditStore {
public:
 bool load(const std::filesystem::path& file);
 bool save(const std::filesystem::path& file) const;
 void record(std::string_view driver,int exitCode);
 AuditState status(std::string_view driver) const;
 std::size_t size() const noexcept{return results_.size();}
private:
 std::map<std::string,AuditState> results_;
};
}
