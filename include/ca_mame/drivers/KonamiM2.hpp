#pragma once
#include <cstdint>
#include "ca_mame/drivers/M2MemoryMap.hpp"
#include "ca_mame/cpu/PowerPC602.hpp"
namespace ca_mame::drivers {
// Experimental shared-RAM dual-processor harness; NOT a Konami M2 board implementation.
class KonamiM2 {
public:
    KonamiM2():memory_(irq_),cpu0_(memory_),cpu1_(memory_){}
    void reset(){memory_.resetRegisters();irq_.reset();cpu0_.reset();cpu1_.reset();cycles0_=cycles1_=0;}
    static constexpr std::uint32_t resetVector=0xfff00100u;
    void powerOnReset(){reset();cpu0_.setPC(resetVector);cpu1_.setPC(resetVector);cpu0_.setMSR(0x40u);cpu1_.setMSR(0x40u);}
    bool loadBootRom(const std::string& path){return memory_.loadBootRom(path);}
    bool loadWord(std::uint32_t address,std::uint32_t value){return memory_.write32(address,value);}
    bool readWord(std::uint32_t address,std::uint32_t& value)const{return memory_.read32(address,value);}
    void stepPair(){cpu0_.setExternalInterrupt(irq_.asserted(0)||memory_.hardwareInterruptAsserted());cpu1_.setExternalInterrupt(irq_.asserted(1)||memory_.hardwareInterruptAsserted());cycles0_+=cpu0_.step();cycles1_+=cpu1_.step();}
    void raiseHardwareInterrupt(unsigned line) noexcept {memory_.raiseHardwareInterrupt(line);}
    M2InterruptController& interrupts() noexcept{return irq_;}
    PowerPC602& cpu(unsigned index){return index==0?cpu0_:cpu1_;}
    std::uint64_t cycles(unsigned index)const{return index==0?cycles0_:cycles1_;}
private:
    M2InterruptController irq_;
    M2MemoryMap memory_;
    PowerPC602 cpu0_,cpu1_;
    std::uint64_t cycles0_=0,cycles1_=0;
};
}
