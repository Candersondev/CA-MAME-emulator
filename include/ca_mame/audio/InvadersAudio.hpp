#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>
#include "ca_mame/audio/InvadersSoundSignals.hpp"

namespace ca_mame::audio {
// Initial deterministic digital approximation, not yet a transistor-level
// simulation of the original discrete analog circuitry.
class InvadersAudio {
public:
    static constexpr std::uint32_t SampleRate=44100;
    void reset() noexcept { active_.fill(false); envelope_.fill(0.0f); phase_.fill(0.0); }
    void apply(const InvadersSoundEvent& event) noexcept {
        const auto index=static_cast<unsigned>(event.signal);
        if(index>=10) return; // FlipScreen and Unknown are not sound.
        if(event.active && !active_[index]) envelope_[index]=1.0f;
        active_[index]=event.active;
    }
    // Render a CPU-cycle interval with transitions applied at their respective
    // sample positions. Events must be in nondecreasing cycle order.
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
        constexpr std::array<double,10> frequency={
            110.0, 800.0, 85.0, 520.0, 650.0,
            110.0, 125.0, 140.0, 155.0, 390.0
        };
        for(auto& sample:output) {
            float mixed=0.0f;
            for(unsigned i=0;i<active_.size();++i) {
                if(!active_[i] && envelope_[i]<0.0001f) continue;
                const bool loop=i==0;
                if(!loop) envelope_[i]*=0.9993f;
                const float level=loop?(active_[i]?1.0f:0.0f):envelope_[i];
                const float square=phase_[i]<0.5?1.0f:-1.0f;
                mixed+=square*level*0.08f;
                phase_[i]+=frequency[i]/SampleRate;
                if(phase_[i]>=1.0) phase_[i]-=1.0;
            }
            sample=std::clamp(mixed,-1.0f,1.0f);
        }
    }
private:
    std::array<bool,10> active_{};
    std::array<float,10> envelope_{};
    std::array<double,10> phase_{};
};
}
