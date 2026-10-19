#pragma once
#include "ca_mame/compat/MameBridge.hpp"
#include <filesystem>
#include <string>
#include <vector>
namespace ca_mame::compat {
enum class DependencyState { Found, Missing, NotApplicable };
struct DependencyItem { std::string name; std::string role; DependencyState state; std::string advice; };
struct DependencyReport { std::vector<DependencyItem> items; bool hasMissing=false; };
class DependencyInspector {
public:
 static DependencyReport inspect(const Game& game,const std::filesystem::path& romDirectory);
};
} // namespace ca_mame::compat
