#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include "ca_mame/rom/NativeLaunchPlan.hpp"

int main() {
    namespace fs=std::filesystem;
    const auto root=fs::temp_directory_path()/"ca_mame_native_launch_plan_tests";
    std::error_code ec;
    fs::remove_all(root,ec);
    fs::create_directories(root/"invaders",ec);
    if(ec)return 1;
    if(ca_mame::rom::nativeLaunchPlan("invaders",root,"ca-mame-sdl"))return 2;
    const auto* game=ca_mame::rom::findNativeGame("invaders");
    if(!game)return 3;
    for(const auto filename:game->files) {
        std::ofstream out(root/"invaders"/std::string(filename),std::ios::binary);
        const std::string data(game->romBytes,'\0');
        out.write(data.data(),static_cast<std::streamsize>(data.size()));
    }
    const auto plan=ca_mame::rom::nativeLaunchPlan("invaders",root,"ca-mame-sdl");
    if(!plan || plan->arguments.size()!=3 || plan->arguments[0]!="--game" ||
       plan->arguments[1]!="invaders" ||
       fs::path(plan->arguments[2])!=root/"invaders")return 4;
    if(ca_mame::rom::nativeLaunchPlan("unknown",root,"ca-mame-sdl"))return 5;
    if(ca_mame::rom::nativeLaunchPlan("gunfight",root,"ca-mame-sdl"))return 6;
    fs::resize_file(root/"invaders"/"invaders.h",10,ec);
    if(ec || ca_mame::rom::nativeLaunchPlan("invaders",root,"ca-mame-sdl"))return 7;
    fs::remove_all(root,ec);
    return ec?8:0;
}
