#pragma once
#include <cstdint>

namespace ca_mame::devices {
// Reusable cycle-based watchdog for arcade boards that have one.
// A driver must explicitly wire its kick signal and reset callback;
// this class intentionally makes no assumptions about Space Invaders I/O.
class CycleWatchdog {
public:
    explicit constexpr CycleWatchdog(std::uint64_t timeoutCycles) noexcept
        : timeout_(timeoutCycles) {}
    constexpr void reset(std::uint64_t now=0) noexcept {
        lastKick_=now;
        tripped_=false;
    }
    constexpr void kick(std::uint64_t now) noexcept {
        lastKick_=now;
        tripped_=false;
    }
    // Returns true exactly once for each timeout until the next kick.
    constexpr bool check(std::uint64_t now) noexcept {
        if(tripped_ || timeout_==0 || now<lastKick_ ||
           now-lastKick_<timeout_) return false;
        tripped_=true;
        return true;
    }
    constexpr bool tripped() const noexcept { return tripped_; }
    constexpr std::uint64_t timeoutCycles() const noexcept { return timeout_; }
private:
    std::uint64_t timeout_;
    std::uint64_t lastKick_=0;
    bool tripped_=false;
};
}
