#pragma once
#include <array>
#include <cstdint>
namespace ca_mame::drivers {
// Provisional interrupt model, not verified against Konami M2 hardware registers.
class M2InterruptController {
public:
    void reset() noexcept {pending_=0; masks_.fill(0);}
    void raise(unsigned line) noexcept {if(line<32)pending_|=(1u<<line);}
    void clear(unsigned line) noexcept {if(line<32)pending_&=~(1u<<line);}
    void setMask(unsigned cpu,std::uint32_t mask) noexcept {if(cpu<2)masks_[cpu]=mask;}
    std::uint32_t pending() const noexcept{return pending_;}
    std::uint32_t mask(unsigned cpu) const noexcept{return cpu<2?masks_[cpu]:0;}
    bool asserted(unsigned cpu) const noexcept{return cpu<2 && (pending_&masks_[cpu])!=0;}
private:
    std::uint32_t pending_=0;
    std::array<std::uint32_t,2> masks_{};
};
}
