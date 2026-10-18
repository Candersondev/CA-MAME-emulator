#pragma once
#include <cstdint>
#include "ca_mame/drivers/M2MemoryMap.hpp"
#include "ca_mame/drivers/M2CpuBus.hpp"
#include "ca_mame/cpu/PowerPC602.hpp"
namespace ca_mame::drivers {
// Experimental shared-RAM dual-processor harness; NOT a Konami M2 board implementation.
class KonamiM2 {
public:
    KonamiM2():memory_(irq_),bus0_(memory_,0),bus1_(memory_,1),cpu0_(bus0_),cpu1_(bus1_){}
    void reset(){memory_.resetRegisters();irq_.reset();cpu0_.reset();cpu1_.reset();cycles0_=cycles1_=0;}
    static constexpr std::uint32_t resetVector=0xfff00100u;
    void powerOnReset(){reset();cpu0_.setPC(resetVector);cpu1_.setPC(resetVector);cpu0_.setMSR(0x40u);cpu1_.setMSR(0x40u);}
    bool loadBootRom(const std::string& path){return memory_.loadBootRom(path);}
    bool mountCdIso(const std::string& path){return memory_.cdRom().mountIso(path);}
    void ejectCd() noexcept {memory_.cdRom().eject();}
    bool cdReady() const noexcept {return memory_.cdRom().ready();}
    bool loadWord(std::uint32_t address,std::uint32_t value){return memory_.write32(address,value);}
    bool readWord(std::uint32_t address,std::uint32_t& value)const{return memory_.read32(address,value);}
    void stepPair(){cpu0_.setExternalInterrupt(irq_.asserted(0)||memory_.hardwareInterruptAsserted());cpu1_.setExternalInterrupt(irq_.asserted(1)||memory_.hardwareInterruptAsserted());cycles0_+=cpu0_.step();cycles1_+=cpu1_.step();}
    struct CpuSnapshot {
        std::uint32_t pc=0,faultAddress=0,faultInstruction=0;
        PowerPC602::StopReason reason=PowerPC602::StopReason::None;
        std::uint64_t cycles=0;
        bool faulted=false,externalInterrupt=false;
    };
    CpuSnapshot snapshot(unsigned index) const noexcept {
        const auto& c=index==0?cpu0_:cpu1_;
        return {c.pc(),c.faultAddress(),c.faultInstruction(),c.stopReason(),
                index==0?cycles0_:cycles1_,c.faulted(),c.externalInterrupt()};
    }
    // Stop as soon as either CPU reports an unsupported instruction or bus fault.
    std::uint64_t runUntilFault(std::uint64_t maxPairs) {
        std::uint64_t completed=0;
        while(completed<maxPairs && !cpu0_.faulted() && !cpu1_.faulted()){
            stepPair();++completed;
        }
        return completed;
    }
    void raiseHardwareInterrupt(unsigned line) noexcept {memory_.raiseHardwareInterrupt(line);}
    M2InterruptController& interrupts() noexcept{return irq_;}
    PowerPC602& cpu(unsigned index){return index==0?cpu0_:cpu1_;}
    std::uint64_t cycles(unsigned index)const{return index==0?cycles0_:cycles1_;}
private:
    M2InterruptController irq_;
    M2MemoryMap memory_;
    M2CpuBus bus0_,bus1_;
    PowerPC602 cpu0_,cpu1_;
    std::uint64_t cycles0_=0,cycles1_=0;
};
}
