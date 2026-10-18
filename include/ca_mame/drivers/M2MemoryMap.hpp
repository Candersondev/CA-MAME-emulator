#pragma once
#include <cstdint>
#include "ca_mame/bus/WordBus.hpp"
#include "ca_mame/bus/BigEndianBus.hpp"
#include "ca_mame/drivers/M2InterruptController.hpp"
namespace ca_mame::drivers {
// Prototype register layout for unit tests ONLY; real M2 addresses are not established.
class M2MemoryMap final : public WordBus {
public:
    static constexpr std::uint32_t ramBase=0x00000000;
    static constexpr std::uint32_t ramSize=1024*1024;
    static constexpr std::uint32_t prototypeIrqBase=0x10000000;
    static constexpr std::uint32_t bootRomBase=0x20000000;
    static constexpr std::uint32_t bootRomSize=0x200000;
    static constexpr std::uint32_t bootRomMirrorBase=0xfff00000;
    explicit M2MemoryMap(M2InterruptController& irq):ram_(ramSize),bootRom_(bootRomSize),irq_(irq){}
    // Loads a word into ROM storage, bypassing guest read-only protection.
    bool loadBootWord(std::uint32_t offset,std::uint32_t value) noexcept{return bootRom_.write32(offset,value);}
    bool read32(std::uint32_t address,std::uint32_t& value) const noexcept {
        if(address<ramSize)return ram_.read32(address,value);
        if(address>=bootRomBase && address<bootRomBase+bootRomSize)return bootRom_.read32(address-bootRomBase,value);
        if(address>=bootRomMirrorBase)return bootRom_.read32(address-bootRomMirrorBase,value);
        if(address==prototypeIrqBase){value=irq_.pending();return true;}
        if(address==prototypeIrqBase+4){value=irq_.mask(0);return true;}
        if(address==prototypeIrqBase+8){value=irq_.mask(1);return true;}
        return false;
    }
    bool write32(std::uint32_t address,std::uint32_t value) noexcept {
        if(address<ramSize)return ram_.write32(address,value);
        if(address==prototypeIrqBase+4){irq_.setMask(0,value);return true;}
        if(address==prototypeIrqBase+8){irq_.setMask(1,value);return true;}
        if(address==prototypeIrqBase+12){for(unsigned i=0;i<32;++i)if(value&(1u<<i))irq_.clear(i);return true;}
        return false;
    }
private:
    BigEndianBus ram_,bootRom_;
    M2InterruptController& irq_;
};
}
