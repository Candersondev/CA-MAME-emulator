#include <array>
#include <cmath>
#include "ca_mame/audio/GunFightAudio.hpp"
#include "ca_mame/rom/MidwayGameDetector.hpp"
int main() {
    using ca_mame::rom::detectMidway8080Game;
    if(detectMidway8080Game("7609h.bin")!="gunfight" ||
       detectMidway8080Game("C:\\roms\\7609g.bin")!="gunfight" ||
       detectMidway8080Game("/roms/invaders.h")!="invaders" ||
       !detectMidway8080Game("unknown.bin").empty())return 1;
    ca_mame::audio::GunFightAudio audio;
    std::array<float,1024> samples{};
    audio.render(samples);
    for(auto x:samples)if(x!=0)return 2;
    for(unsigned effect=1;effect<=4;++effect) {
        audio.reset();
        audio.write(static_cast<std::uint8_t>((~effect&15)<<4));
        if(audio.selectedVoice()!=effect)return 3;
        audio.render(samples);
        bool audible=false;
        for(auto x:samples) {
            if(!std::isfinite(x)||std::abs(x)>1)return 4;
            audible|=x!=0;
        }
        if(!audible)return 5;
    }
    audio.write(0xff);
    if(audio.selectedVoice()!=0)return 6;
    return 0;
}
