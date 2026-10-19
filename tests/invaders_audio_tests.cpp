#include <array>
#include <cmath>
#include <iostream>
#include "ca_mame/audio/InvadersAudio.hpp"

int main() {
    using namespace ca_mame::audio;
    InvadersAudio audio;
    std::array<float,1024> samples{};
    audio.render(samples);
    for(float v:samples) if(v!=0.0f) {std::cerr<<"silence expected\n";return 1;}
    audio.apply({InvadersSignal::UfoLoop,true,0});
    audio.render(samples);
    bool nonzero=false;
    for(float v:samples) {
        if(!std::isfinite(v)||std::abs(v)>1.0f) return 2;
        nonzero|=v!=0.0f;
    }
    if(!nonzero) return 3;
    audio.apply({InvadersSignal::UfoLoop,false,1024});
    audio.render(samples);
    for(float v:samples) if(v!=0.0f) return 4;
    audio.apply({InvadersSignal::Shot,true,2048});
    audio.render(samples);
    nonzero=false;
    for(float v:samples) nonzero|=v!=0.0f;
    if(!nonzero) return 5;
    audio.reset();
    audio.render(samples);
    for(float v:samples) if(v!=0.0f) return 6;
    audio.apply({InvadersSignal::FlipScreen,true,0});
    audio.render(samples);
    for(float v:samples) if(v!=0.0f) return 7;
    return 0;
}
