#include <array>
#include <cmath>
#include <iostream>
#include "ca_mame/audio/InvadersAudio.hpp"
#include "ca_mame/audio/InvadersFrameClock.hpp"

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
    audio.reset();
    std::array<float,100> timed{};
    const std::array<InvadersSoundEvent,1> startHalfway={{{InvadersSignal::UfoLoop,true,50}}};
    audio.renderTimed(timed,startHalfway,0,100);
    for(std::size_t i=0;i<50;++i) if(timed[i]!=0.0f) return 8;
    bool heard=false;
    for(std::size_t i=50;i<100;++i) heard|=timed[i]!=0.0f;
    if(!heard) return 9;
    const std::array<InvadersSoundEvent,1> stopHalfway={{{InvadersSignal::UfoLoop,false,150}}};
    audio.renderTimed(timed,stopHalfway,100,200);
    for(std::size_t i=50;i<100;++i) if(timed[i]!=0.0f) return 10;
    using Clock=InvadersFrameClock;
    constexpr std::uint64_t frames=600;
    std::uint64_t total=0;
    for(std::uint64_t frame=1;frame<=frames;++frame) {
        const auto count=Clock::sampleDeadline(frame)-Clock::sampleDeadline(frame-1);
        if(count<740 || count>741) return 11;
        total+=count;
    }
    if(total!=Clock::sampleDeadline(frames)) return 12;
    if(Clock::nanosecondDeadline(frames)<10000000000ull ||
       Clock::nanosecondDeadline(frames)>10100000000ull) return 13;
    // Every emulated sound output must produce finite, bounded PCM.
    for(unsigned voice=0;voice<InvadersAudio::VoiceCount;++voice) {
        InvadersAudio effect;
        effect.apply({static_cast<InvadersSignal>(voice),true,0});
        std::array<float,2048> block{};
        effect.render(block);
        bool audible=false;
        for(const auto value:block) {
            if(!std::isfinite(value) || std::abs(value)>1.0f) return 14;
            audible|=value!=0.0f;
        }
        if(!audible) return 15;
        // Rising edge triggers are deterministic after reset.
        effect.reset();
        effect.apply({static_cast<InvadersSignal>(voice),true,0});
        std::array<float,2048> replay{};
        effect.render(replay);
        if(block!=replay) return 16;
    }
    // Rapid rising/falling pulses still create a one-shot effect.
    {
        InvadersAudio pulse;
        const std::array<InvadersSoundEvent,2> events={{{InvadersSignal::Shot,true,10},
                                                       {InvadersSignal::Shot,false,11}}};
        std::array<float,100> block{};
        pulse.renderTimed(block,events,0,100);
        bool tail=false;
        for(std::size_t i=11;i<block.size();++i) tail|=block[i]!=0.0f;
        if(!tail) return 17;
    }
    return 0;
}
