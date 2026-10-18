#pragma once
#include <array>
#include <cstdint>
#include "ca_mame/cpu/ICpu.hpp"
#include "ca_mame/bus/BigEndianBus.hpp"
namespace ca_mame {
class PowerPC602 final:public ICpu {
public:
    explicit PowerPC602(BigEndianBus& bus):bus_(bus){reset();}
    void reset() override;
    std::uint32_t step() override;
    std::uint32_t pc() const noexcept{return pc_;}
    void setPC(std::uint32_t pc) noexcept{pc_=pc;}
    std::uint32_t gpr(unsigned i) const noexcept{return registers_[i&31];}
    void setGpr(unsigned i,std::uint32_t v) noexcept{registers_[i&31]=v;}
    bool faulted() const noexcept{return fault_;}
    std::uint32_t lr() const noexcept{return lr_;}
    std::uint32_t cr() const noexcept{return cr_;}
private:
    BigEndianBus& bus_;
    std::array<std::uint32_t,32> registers_{};
    std::uint32_t pc_=0;
    bool fault_=false;
    std::uint32_t lr_=0,cr_=0;
};
}
