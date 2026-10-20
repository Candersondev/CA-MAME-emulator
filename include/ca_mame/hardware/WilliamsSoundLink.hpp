#pragma once
#include <cstdint>
namespace ca_mame::hardware {
// Williams sound-board handshake foundation. The separate 6808/6802 CPU,
// its ROM map, DAC and PIA timing are not emulated yet.
class WilliamsSoundLink {
public:
 void reset() noexcept {command_=0;pending_=false;delivered_=0;acknowledged_=0;}
 void send(std::uint8_t command) noexcept {command_=command;pending_=true;++delivered_;}
 std::uint8_t command() const noexcept{return command_;}
 bool pending() const noexcept{return pending_;}
 std::uint8_t acknowledge() noexcept {pending_=false;++acknowledged_;return command_;}
 std::uint64_t delivered() const noexcept{return delivered_;}
 std::uint64_t acknowledged() const noexcept{return acknowledged_;}
private:
 std::uint8_t command_=0;
 bool pending_=false;
 std::uint64_t delivered_=0,acknowledged_=0;
};
}
