#pragma once
#include <array>
#include <string_view>
namespace ca_mame::rom {
// Exact known original program-ROM basenames; unknown sets are never guessed.
inline std::string_view detectMidway8080Game(std::string_view name) noexcept {
    constexpr std::array<std::string_view,4> invaders={
        "invaders.h","invaders.g","invaders.f","invaders.e"};
    constexpr std::array<std::string_view,4> gunfight={
        "7609h.bin","7609g.bin","7609f.bin","7609e.bin"};
    const auto slash=name.find_last_of("/\\");
    if(slash!=std::string_view::npos) name.remove_prefix(slash+1);
    for(const auto file:invaders) if(name==file)return "invaders";
    for(const auto file:gunfight) if(name==file)return "gunfight";
    return {};
}
}
