#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <span>
namespace ca_mame::audio {
// MAME's Gun Fight netlist reference: inverted D4-D7 latch selects one of
// four effects. This is a procedural approximation, not netlist simulation.
class GunFightAudio {
public:
    static constexpr std::uint32_t SampleRate=44100;
    void reset() noexcept {voice_=0;remaining_=0;phase_=0;noise_=0x9134abcd;}
    void write(std::uint8_t data) noexcept {
        const auto code=static_cast<unsigned>((~data>>4)&15);
        if(code>=1 && code<=4) {
            if(code!=voice_){voice_=code;remaining_=SampleRate/6;phase_=0;}
        } else voice_=0;
    }
    unsigned selectedVoice() const noexcept {return voice_;}
    void render(std::span<float> out) noexcept {
        constexpr std::array<double,4> freq={780.,930.,170.,210.};
        for(auto& sample:out) {
            if(!remaining_) {sample=0;continue;}
            --remaining_;
            noise_^=noise_<<13;noise_^=noise_>>17;noise_^=noise_<<5;
            const float white=float(noise_&0xffffu)/32767.5f-1.f;
            const bool hit=voice_>=3;
            const float tone=hit?white:(phase_<0.5?1.f:-1.f);
            sample=std::clamp(tone*0.18f*float(remaining_)/(SampleRate/6),-1.f,1.f);
            if(voice_>=1 && voice_<=4) {
                phase_+=freq[voice_-1]/SampleRate;
                if(phase_>=1.)phase_-=1.;
            }
        }
    }
private:
    unsigned voice_=0,remaining_=0;
    double phase_=0;
    std::uint32_t noise_=0x9134abcd;
};
}
