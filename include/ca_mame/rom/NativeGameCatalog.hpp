#pragma once
#include <array>
#include <filesystem>
#include <string_view>
#include "ca_mame/rom/MidwayGameDetector.hpp"

namespace ca_mame::rom {
struct NativeGame {
    std::string_view id;
    std::array<std::string_view,4> files;
    std::size_t romBytes;
};
inline constexpr std::array<NativeGame,2> NativeGames{{
    {"invaders",{"invaders.h","invaders.g","invaders.f","invaders.e"},2048},
    {"gunfight",{"7609h.bin","7609g.bin","7609f.bin","7609e.bin"},1024}
}};
inline const NativeGame* findNativeGame(std::string_view id) noexcept {
    for(const auto& game:NativeGames) if(game.id==id)return &game;
    return nullptr;
}
inline const NativeGame* detectNativeGame(const std::filesystem::path& path) {
    const auto filename=path.filename().string();
    if(const auto* game=findNativeGame(filename))return game;
    if(filename=="invaders" || filename=="gunfight")return findNativeGame(filename);
    const auto id=detectMidway8080Game(filename);
    return findNativeGame(id);
}
}
