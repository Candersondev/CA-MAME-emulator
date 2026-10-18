#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
namespace ca_mame::compat {
struct Game {std::string name,description,year,manufacturer;bool runnable=false;};
struct LaunchPlan {std::filesystem::path executable;std::vector<std::string> arguments;};
class MameBridge {
public:
 static bool validDriverName(std::string_view name) noexcept;
 static std::optional<LaunchPlan> plan(const std::filesystem::path& executable,std::string_view game,
                                     const std::optional<std::filesystem::path>& romPath={});
 static std::vector<Game> parseListXml(std::string_view xml);
};
}
