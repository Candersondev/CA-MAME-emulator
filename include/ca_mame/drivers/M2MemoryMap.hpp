#pragma once
#include <cstdint>
#include <array>
#include <fstream>
#include <string>
#include "ca_mame/bus/WordBus.hpp"
#include "ca_mame/bus/BigEndianBus.hpp"
#include "ca_mame/drivers/M2InterruptController.hpp"
#include "ca_mame/drivers/M2Cde.hpp"
namespace ca_mame::drivers {
// Prototype register layout for unit tests ONLY; real M2 addresses are not established.
class M2MemoryMap final : public WordBus {
public:
    static constexpr std::uint32_t ramBase=0x40000000;
    static constexpr std::uint32_t powerBusBase=0x00010000;
    static constexpr std::uint32_t memoryControlBase=0x00020000;
    static constexpr std::uint32_t cpuIdBase=0x10000000;
    static constexpr std::uint32_t ramSize=16*1024*1024; // Provisional contiguous 2x8 MiB backing; physical decode unverified.
    static constexpr std::uint32_t prototypeIrqBase=0x11000000; // compatibility-only
    static constexpr std::uint32_t bootRomBase=0x20000000;
    static constexpr std::uint32_t bootRomSize=0x200000;
    static constexpr std::uint32_t bootRomMirrorBase=0xfff00000;
    explicit M2MemoryMap(M2InterruptController& irq):ram_(ramSize),bootRom_(bootRomSize),irq_(irq){}
    bool loadBootRom(const std::string& path) {
        std::ifstream file(path,std::ios::binary|std::ios::ate);
        if(!file || file.tellg()!=static_cast<std::streamoff>(bootRomSize))return false;
        file.seekg(0);
        std::array<unsigned char,4> bytes{};
        for(std::uint32_t offset=0;offset<bootRomSize;offset+=4){
            if(!file.read(reinterpret_cast<char*>(bytes.data()),4))return false;
            const auto value=(std::uint32_t(bytes[0])<<24)|(std::uint32_t(bytes[1])<<16)|
                             (std::uint32_t(bytes[2])<<8)|bytes[3];
            if(!bootRom_.write32(offset,value))return false;
        }
        return true;
    }
    void resetRegisters() noexcept {powerBusEnable_=0;powerBusStatus_=0;memoryRefresh_=0;cde_.reset();}
    bool loadBioByte(std::uint32_t address,std::uint8_t value) noexcept {return cde_.loadBioByte(address,value);}
    bool readBioByte(std::uint32_t address,std::uint8_t& value) const noexcept {return cde_.readBioByte(address,value);}
    void raiseHardwareInterrupt(unsigned line) noexcept {if(line<32)powerBusStatus_|=(1u<<line);}
    bool hardwareInterruptAsserted() const noexcept{return (powerBusStatus_&powerBusEnable_)!=0 || cde_.interruptAsserted();}
    // Loads a word into ROM storage, bypassing guest read-only protection.
    bool loadBootWord(std::uint32_t offset,std::uint32_t value) noexcept{return bootRom_.write32(offset,value);}
    bool read32(std::uint32_t address,std::uint32_t& value) const noexcept {
        if(address>=M2Cde::base && address<M2Cde::base+M2Cde::size)return cde_.read32(address-M2Cde::base,value);
        if(address>=ramBase && address<ramBase+ramSize)return ram_.read32(address-ramBase,value);
        if(address<ramSize && (address<0x10000u || address>=0x100000u))return ram_.read32(address,value); // legacy test harness alias
        if(address==memoryControlBase){value=memoryConfig_;return true;}
        if(address==memoryControlBase+4){value=memoryRefresh_;return true;}
        if(address==memoryControlBase+8 || address==memoryControlBase+12){value=0;return true;} // MAME currently stubs reads
        if(address==cpuIdBase){value=0;return true;} // CPU0; CPU1 requires per-CPU view
        if(address==powerBusBase+0x40){value=powerBusEnable_;return true;}
        if(address==powerBusBase+0x50){value=powerBusStatus_;return true;}
        if(address>=bootRomBase && address<bootRomBase+bootRomSize)return bootRom_.read32(address-bootRomBase,value);
        if(address>=bootRomMirrorBase)return bootRom_.read32(address-bootRomMirrorBase,value);
        if(address==prototypeIrqBase){value=irq_.pending();return true;}
        if(address==prototypeIrqBase+4){value=irq_.mask(0);return true;}
        if(address==prototypeIrqBase+8){value=irq_.mask(1);return true;}
        return false;
    }
    bool write32(std::uint32_t address,std::uint32_t value) noexcept {
        if(address>=M2Cde::base && address<M2Cde::base+M2Cde::size){
            const auto offset=address-M2Cde::base;
            if(offset==0x1000u || offset==0x1020u){
                if(value&0x80u){
                    M2Cde::Transfer transfer{};
                    const unsigned index=(offset==0x1020u)?1u:0u;
                    if(!cde_.prepareTransfer(index,value,transfer))return false;
                    if(transfer.destination<ramBase || transfer.destination>=ramBase+ramSize ||
                       transfer.count>ramSize-(transfer.destination-ramBase))return false;
                    // Prevalidate the entire destination before mutating RAM.
                    for(std::uint32_t i=0;i<transfer.count;i+=2){
                        std::uint8_t hi=0,lo=0;
                        const auto src=transfer.source+(transfer.fixedSource?0u:i);
                        if(!cde_.readBioByte(src,hi)||!cde_.readBioByte(src+1,lo))return false;
                    }
                    for(std::uint32_t i=0;i<transfer.count;i+=2){
                        std::uint8_t hi=0,lo=0;
                        const auto src=transfer.source+(transfer.fixedSource?0u:i);
                        cde_.readBioByte(src,hi);
                        cde_.readBioByte(src+1,lo);
                        if(!ram_.write16(transfer.destination-ramBase+i,
                                        static_cast<std::uint16_t>((std::uint16_t(hi)<<8)|lo)))return false;
                    }
                    cde_.completeTransfer(index,transfer);
                    return true;
                }
            }
            return cde_.write32(offset,value);
        }
        if(address>=ramBase && address<ramBase+ramSize)return ram_.write32(address-ramBase,value);
        if(address<ramSize && (address<0x10000u || address>=0x100000u))return ram_.write32(address,value); // legacy test harness alias
        if(address==memoryControlBase){memoryConfig_=value;return true;}
        if(address==memoryControlBase+4){memoryRefresh_=value;return true;}
        if(address==memoryControlBase+8 || address==memoryControlBase+12)return true; // MAME currently ignores writes
        if(address==powerBusBase+0x40){powerBusEnable_|=value;return true;}
        if(address==powerBusBase+0x440){powerBusEnable_&=~value;return true;}
        if(address==powerBusBase+0x60){if(value==1)powerBusStatus_|=1u;return true;}
        if(address==powerBusBase+0x460){powerBusStatus_&=~value;return true;}
        if(address==prototypeIrqBase+4){irq_.setMask(0,value);return true;}
        if(address==prototypeIrqBase+8){irq_.setMask(1,value);return true;}
        if(address==prototypeIrqBase+12){for(unsigned i=0;i<32;++i)if(value&(1u<<i))irq_.clear(i);return true;}
        return false;
    }
private:
    BigEndianBus ram_,bootRom_;
    M2Cde cde_;
    M2InterruptController& irq_;
    std::uint32_t powerBusEnable_=0,powerBusStatus_=0;
    // Two 8 MiB banks: SS1=5 at bit 13, SS0=5 at bit 10.
    std::uint32_t memoryConfig_=(5u<<13)|(5u<<10),memoryRefresh_=0;
};
}
