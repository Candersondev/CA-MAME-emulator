#pragma once
#include <array>
#include <cstdint>
#include "ca_mame/cpu/ICpu.hpp"
#include "ca_mame/bus/WordBus.hpp"
namespace ca_mame {
class PowerPC602 final:public ICpu {
public:
    explicit PowerPC602(WordBus& bus):bus_(bus){reset();}
    void reset() override;
    std::uint32_t step() override;
    std::uint32_t pc() const noexcept{return pc_;}
    void setPC(std::uint32_t pc) noexcept{pc_=pc;}
    std::uint32_t gpr(unsigned i) const noexcept{return registers_[i&31];}
    void setGpr(unsigned i,std::uint32_t v) noexcept{registers_[i&31]=v;}
    bool faulted() const noexcept{return fault_;}
    void setExternalInterrupt(bool asserted) noexcept {externalInterrupt_=asserted;}
    bool externalInterrupt() const noexcept{return externalInterrupt_;}
    static constexpr std::uint32_t MSR_EE=0x00008000u;
    std::uint32_t msr() const noexcept{return msr_;}
    void setMSR(std::uint32_t value) noexcept{msr_=value;}
    std::uint32_t srr0() const noexcept{return srr0_;}
    std::uint32_t srr1() const noexcept{return srr1_;}
    std::uint32_t lr() const noexcept{return lr_;}
    std::uint32_t cr() const noexcept{return cr_;}
private:
    WordBus& bus_;
    std::array<std::uint32_t,32> registers_{};
    std::uint32_t pc_=0;
    bool fault_=false;
    std::uint32_t lr_=0,cr_=0;
    std::uint32_t srr0_=0,srr1_=0,msr_=0;
    bool externalInterrupt_=false;
};
}
