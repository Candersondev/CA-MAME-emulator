#pragma once
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <vector>
namespace ca_mame::compat {
class LibraryPreferences {
public:
 bool load(const std::filesystem::path& file);
 bool save(const std::filesystem::path& file) const;
 bool favorite(std::string_view name) const;
 void toggleFavorite(std::string_view name);
 void recordLaunch(std::string_view name);
 const std::vector<std::string>& recent() const noexcept{return recent_;}
private:
 std::set<std::string> favorites_;
 std::vector<std::string> recent_;
};
}
