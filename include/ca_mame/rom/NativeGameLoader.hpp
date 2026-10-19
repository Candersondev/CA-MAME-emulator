#pragma once
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <vector>
#include "ca_mame/rom/NativeGameCatalog.hpp"
namespace ca_mame::rom {
template<class Board>
bool loadNativeGame(Board& board,const NativeGame& game,const std::filesystem::path& directory) {
    // Read and validate every file before changing the emulated machine.
    std::array<std::vector<std::uint8_t>,4> roms;
    for(std::size_t i=0;i<game.files.size();++i) {
        std::ifstream file(directory/std::string(game.files[i]),std::ios::binary|std::ios::ate);
        if(!file || file.tellg()!=static_cast<std::streamoff>(game.romBytes))return false;
        roms[i].resize(game.romBytes);
        file.seekg(0);
        if(!file.read(reinterpret_cast<char*>(roms[i].data()),
                      static_cast<std::streamsize>(roms[i].size())))return false;
    }
    for(std::size_t i=0;i<roms.size();++i)
        if(!board.loadRom(static_cast<std::uint16_t>(i*game.romBytes),roms[i]))return false;
    return true;
}
}
