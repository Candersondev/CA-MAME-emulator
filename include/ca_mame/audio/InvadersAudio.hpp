#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include "ca_mame/audio/InvadersSoundSignals.hpp"

namespace ca_mame::audio {
// Deterministic procedural approximation of the Space Invaders discrete sound
// board. This is NOT an electrical SN76477/discrete-circuit simulation.
// No samples or third-party sound ROMs are required.
class InvadersAudio {
public:
    static constexpr std::uint32_t SampleRate=44100;
    static constexpr unsigned VoiceCount=10;

    void reset() noexcept {
        active_.fill(false);
        envelope_.fill(0.0f);
        phase_.fill(0.0);
        age_.fill(0);
        noise_=0x12345678u;
        lowpass_=0.0f;
    }

    void apply(const InvadersSoundEvent& event) noexcept {
        const auto index=static_cast<unsigned>(event.signal);
        if(index>=VoiceCount) return; // FlipScreen/unknown: not audio
        if(event.active && !active_[index]) {
            envelope_[index]=1.0f;
            age_[index]=0;
            phase_[index]=0.0;
        }
        active_[index]=event.active;
    }

    // Cycle timestamps must be sorted. Events exactly at endCycle are applied
    // after the last sample, and affect the next render.
    void renderTimed(std::span<float> output,
                     std::span<const InvadersSoundEvent> events,
                     std::uint64_t startCycle,
                     std::uint64_t endCycle) noexcept {
        if(output.empty()) {
            for(const auto& event:events) apply(event);
            return;
        }
        std::size_t cursor=0;
        for(const auto& event:events) {
            const auto position=event.cycle<=startCycle?std::size_t{0}:
                event.cycle>=endCycle?output.size():
                static_cast<std::size_t>((static_cast<long double>(event.cycle-startCycle)
                    *output.size())/(endCycle-startCycle));
            const auto boundary=std::max(cursor,position);
            render(output.subspan(cursor,boundary-cursor));
            cursor=boundary;
            apply(event);
        }
        render(output.subspan(cursor));
    }

    void render(std::span<float> output) noexcept {
        // Sound types: UFO oscillator, laser sweep, noise bursts, short
        // invader steps and a UFO hit. Frequencies/envelopes are approximations.
        constexpr std::array<double,VoiceCount> baseFrequency={
            110.0, 950.0, 105.0, 420.0, 660.0,
            110.0, 125.0, 140.0, 155.0, 390.0
        };
        constexpr std::array<float,VoiceCount> decay={
            1.0f, 0.99965f, 0.99982f, 0.9996f, 0.99965f,
            0.9987f, 0.9987f, 0.9987f, 0.9987f, 0.99965f
        };
        for(auto& sample:output) {
            // Independent deterministic xorshift source for impact/noise voices.
            noise_^=noise_<<13;
            noise_^=noise_>>17;
            noise_^=noise_<<5;
            const float white=static_cast<float>(noise_&0xffffu)/32767.5f-1.0f;
            lowpass_+=0.12f*(white-lowpass_);
            float mixed=0.0f;
            for(unsigned i=0;i<VoiceCount;++i) {
                if(!active_[i] && envelope_[i]<0.0001f) continue;
                const bool ufo=i==0;
                if(!ufo) envelope_[i]*=decay[i];
                const float level=ufo?(active_[i]?1.0f:0.0f):envelope_[i];
                if(level<0.0001f) continue;
                const double seconds=static_cast<double>(age_[i])/SampleRate;
                double frequency=baseFrequency[i];
                if(i==1) frequency=950.0/(1.0+seconds*5.0); // falling laser
                if(i==9) frequency=390.0/(1.0+seconds*3.0); // UFO hit
                const float square=phase_[i]<0.5?1.0f:-1.0f;
                const float triangle=static_cast<float>(1.0-4.0*std::abs(phase_[i]-0.5));
                const float tone=(i==2 || i==3)?lowpass_:
                    (i==1 || i==9)?triangle:square;
                mixed+=tone*level*(i==2?0.12f:0.075f);
                phase_[i]+=frequency/SampleRate;
                if(phase_[i]>=1.0) phase_[i]-=1.0;
                ++age_[i];
            }
            sample=std::clamp(mixed,-1.0f,1.0f);
        }
    }
private:
    std::array<bool,VoiceCount> active_{};
    std::array<float,VoiceCount> envelope_{};
    std::array<double,VoiceCount> phase_{};
    std::array<std::uint64_t,VoiceCount> age_{};
    std::uint32_t noise_=0x12345678u;
    float lowpass_=0.0f;
};
}
