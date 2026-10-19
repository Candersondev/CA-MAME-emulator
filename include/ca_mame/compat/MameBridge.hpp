#pragma once
#include <filesystem>
#include <istream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
namespace ca_mame::compat {
struct RomRequirement {std::string name,crc,sha1,region;bool optional=false;};
struct DiskRequirement {std::string name,sha1,region;bool optional=false;};
struct Game {std::string name,description,year,manufacturer;bool runnable=false;std::string sourceFile,parent,bios,deviceRef;bool mechanical=false;std::vector<RomRequirement> roms;std::vector<DiskRequirement> disks;};
struct ProcessResult {int exitCode=-1; std::string output; bool truncated=false;};
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
 // Capture stdout and stderr without invoking a shell; bounded to maxBytes.
 static ProcessResult capture(const LaunchPlan& plan,std::size_t maxBytes=65536);
 static std::optional<LaunchPlan> verifyRomsPlan(const std::filesystem::path& executable,
                 std::string_view game,const std::optional<std::filesystem::path>& romPath={});
};
}
