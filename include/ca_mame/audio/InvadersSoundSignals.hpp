#pragma once
#include <cstdint>
#include "ca_mame/drivers/SpaceInvaders.hpp"

namespace ca_mame::audio {
// Logical board output lines, not synthesized waveforms. Mapping follows the
// Space Invaders port 3/5 control signals; actual analog synthesis is separate.
enum class InvadersSignal : std::uint8_t {
    UfoLoop, Shot, BaseExplosion, InvaderHit, BonusBase,
    Fleet1, Fleet2, Fleet3, Fleet4, UfoHit, FlipScreen, Unknown
};
struct InvadersSoundEvent {
    InvadersSignal signal;
    bool active;
    std::uint64_t cycle;
};
constexpr InvadersSignal decodeInvadersSignal(std::uint8_t port,std::uint8_t bit) noexcept {
    if(port==3) {
        switch(bit) {
            case 0:return InvadersSignal::UfoLoop;
            case 1:return InvadersSignal::Shot;
            case 2:return InvadersSignal::BaseExplosion;
            case 3:return InvadersSignal::InvaderHit;
            case 4:return InvadersSignal::BonusBase;
            default:return InvadersSignal::Unknown;
        }
    }
    if(port==5) {
        switch(bit) {
            case 0:return InvadersSignal::Fleet1;
            case 1:return InvadersSignal::Fleet2;
            case 2:return InvadersSignal::Fleet3;
            case 3:return InvadersSignal::Fleet4;
            case 4:return InvadersSignal::UfoHit;
            case 5:return InvadersSignal::FlipScreen;
            default:return InvadersSignal::Unknown;
        }
    }
    return InvadersSignal::Unknown;
}
constexpr InvadersSoundEvent decodeInvadersEvent(const drivers::SoundTransition& event) noexcept {
    return {decodeInvadersSignal(event.port,event.bit),event.active,event.cycle};
}
}
