#pragma once
#include <cstdint>
#include "ca_mame/drivers/SpaceInvaders.hpp"

namespace ca_mame::audio {
// Exact rational conversion from Midway video frames to 44.1 kHz samples.
// The integer quotient difference distributes fractional samples across frames.
struct InvadersFrameClock {
    static constexpr std::uint64_t Numerator=
        std::uint64_t(InvadersAudioSampleRate())*drivers::SpaceInvaders::HTotal*
        drivers::SpaceInvaders::VTotal;
    static constexpr std::uint64_t Denominator=drivers::SpaceInvaders::PixelHz;
    static constexpr std::uint32_t InvadersAudioSampleRate() noexcept {return 44100;}
    static constexpr std::uint64_t sampleDeadline(std::uint64_t frames) noexcept {
        return frames*Numerator/Denominator;
    }
    static constexpr std::uint64_t nanosecondDeadline(std::uint64_t frames) noexcept {
        return frames*std::uint64_t(drivers::SpaceInvaders::HTotal)*
            drivers::SpaceInvaders::VTotal*1000000000ull/
            drivers::SpaceInvaders::PixelHz;
    }
};
}
