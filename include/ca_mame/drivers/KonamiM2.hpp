#pragma once
#include <cstdint>
#include "ca_mame/drivers/M2MemoryMap.hpp"
#include "ca_mame/cpu/PowerPC602.hpp"
namespace ca_mame::drivers {
// Experimental shared-RAM dual-processor harness; NOT a Konami M2 board implementation.
class KonamiM2 {
public:
    KonamiM2():memory_(irq_),cpu0_(memory_),cpu1_(memory_){}
    void reset(){irq_.reset();cpu0_.reset();cpu1_.reset();cycles0_=cycles1_=0;}
    bool loadWord(std::uint32_t address,std::uint32_t value){return memory_.write32(address,value);}
    bool readWord(std::uint32_t address,std::uint32_t& value)const{return memory_.read32(address,value);}
    void stepPair(){cpu0_.setExternalInterrupt(irq_.asserted(0));cpu1_.setExternalInterrupt(irq_.asserted(1));cycles0_+=cpu0_.step();cycles1_+=cpu1_.step();}
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
