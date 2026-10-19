#pragma once
#include <array>
#include <cstdint>
#include "ca_mame/cpu/ICpu.hpp"
namespace ca_mame {
// Motorola MC6809E CPU foundation: big-endian 16-bit bus, reset vector,
// direct/extended addressing, condition codes, stack, branches and interrupts.
// Unsupported opcodes are explicitly counted, never silently treated as NOPs.
class Motorola6809 final : public ICpu {
public:
    explicit Motorola6809(std::array<std::uint8_t,65536>& memory):memory_(memory){reset();}
    void reset() override;
    std::uint32_t step() override;
    void requestIrq() noexcept {irq_=true;}
    void requestFirq() noexcept {firq_=true;}
    void requestNmi() noexcept {nmi_=true;}
    std::uint8_t a=0,b=0,dp=0,cc=0x50;
    std::uint16_t x=0,y=0,u=0,s=0,pc=0;
    std::uint64_t unsupportedOpcodes() const noexcept{return unsupported_;}
    bool halted() const noexcept{return sync_;}
    std::uint8_t read(std::uint16_t address) const noexcept{return memory_[address];}
    void write(std::uint16_t address,std::uint8_t value) noexcept{memory_[address]=value;}
private:
    std::array<std::uint8_t,65536>& memory_;
    bool irq_=false,firq_=false,nmi_=false,sync_=false;
    std::uint64_t unsupported_=0;
    std::uint8_t fetch(){return read(pc++);}
    std::uint16_t read16(std::uint16_t addr) const {
        return std::uint16_t(read(addr)<<8|read(std::uint16_t(addr+1)));
    }
    std::uint16_t fetch16(){auto hi=fetch();return std::uint16_t(hi<<8|fetch());}
    void write16(std::uint16_t addr,std::uint16_t value){
        write(addr,std::uint8_t(value>>8));write(std::uint16_t(addr+1),std::uint8_t(value));
    }
    void push8(std::uint8_t value){write(--s,value);}
    void push16(std::uint16_t value){push8(std::uint8_t(value));push8(std::uint8_t(value>>8));}
    std::uint8_t pop8(){return read(s++);}
    std::uint16_t pop16(){auto hi=pop8();return std::uint16_t(hi<<8|pop8());}
    void setNZ8(std::uint8_t v){cc=std::uint8_t((cc&~0x0c)|(v&0x80?8:0)|(v==0?4:0));}
    void setNZ16(std::uint16_t v){cc=std::uint8_t((cc&~0x0c)|(v&0x8000?8:0)|(v==0?4:0));}
    std::uint32_t interrupt(std::uint16_t vector,bool entire);
};
}
