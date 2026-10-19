#pragma once
#include <filesystem>
#include <optional>
#include "ca_mame/compat/MameBridge.hpp"
#include "ca_mame/rom/NativeGameCatalog.hpp"

namespace ca_mame::rom {
// A native launch is possible only for an explicitly supported driver with
// complete loose ROM files. Archive support is deliberately not assumed.
inline std::optional<compat::LaunchPlan> nativeLaunchPlan(
    std::string_view driver,
    const std::filesystem::path& romRoot,
    const std::filesystem::path& executable) {
    const auto* game=findNativeGame(driver);
    if(!game || executable.empty())return std::nullopt;
    const auto directory=romRoot/std::string(game->id);
    for(const auto name:game->files) {
        std::error_code ec;
        const auto path=directory/std::string(name);
        if(!std::filesystem::is_regular_file(path,ec) ||
           std::filesystem::file_size(path,ec)!=game->romBytes || ec)
            return std::nullopt;
    }
    compat::LaunchPlan plan;
    plan.executable=executable;
    plan.arguments={"--game",std::string(game->id),directory.string()};
    return plan;
}
}
