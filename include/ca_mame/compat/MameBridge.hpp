#pragma once
#include <filesystem>
#include <istream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
namespace ca_mame::compat {
struct Game {std::string name,description,year,manufacturer,sourceFile,parent,bios,deviceRef;bool runnable=false,mechanical=false;};
struct LaunchPlan {std::filesystem::path executable;std::vector<std::string> arguments;};
class MameBridge {
public:
 static bool validDriverName(std::string_view name) noexcept;
 static std::optional<LaunchPlan> plan(const std::filesystem::path& executable,std::string_view game,
                                     const std::optional<std::filesystem::path>& romPath={});
 static std::vector<Game> parseListXml(std::string_view xml);
 static std::vector<Game> parseListXml(std::istream& input);
 // Launches a separately installed MAME executable without a command shell.
 // Returns process exit status, or -1 if process creation fails.
 static int launch(const LaunchPlan& plan);
 static std::optional<LaunchPlan> verifyRomsPlan(const std::filesystem::path& executable,
                 std::string_view game,const std::optional<std::filesystem::path>& romPath={});
};
}
