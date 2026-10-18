#include "ca_mame/drivers/M2MemoryMap.hpp"
#include <cstdint>
#include <cstdio>
int main(){
    using namespace ca_mame::drivers;
    M2InterruptController irq;
    M2MemoryMap map(irq);
    const auto base=M2Cde::base;
    std::uint32_t value=0;
    auto check=[&](std::uint32_t address,std::uint32_t expected)->bool {
        return map.read32(base+address,value)&&value==expected;
    };
    if(!check(0x240,0)||!check(0x27c,0))return 1;
    if(!map.write32(base+0x240,0x12345678)||!check(0x240,0x12345678))return 2;
    if(!map.write32(base+0x640,0x00005600)||!check(0x240,0x12340078))return 3;
    if(!map.write32(base+0x27c,0xabcdef01)||!check(0x27c,0xabcdef01))return 4;
    if(!map.write32(base+0x1004,0x21000000)||!check(0x1004,0x21000000))return 5;
    if(!map.write32(base+0x1008,0x40001000)||!check(0x1008,0x40001000))return 6;
    if(!map.write32(base+0x100c,0x200)||!check(0x100c,0x200))return 7;
    if(!map.write32(base+0x1024,0x22000000)||!check(0x1024,0x22000000))return 8;
    if(!check(0x1024,0x22000000)||!check(0x1004,0x21000000))return 9;
    if(map.write32(base+0x1000,0x80))return 10; // no fake DMA completion
    if(!map.write32(base+0x1000,0x200)||!check(0x1004,0))return 11;
    if(!check(0x1024,0x22000000))return 12;
    map.resetRegisters();
    if(!check(0x240,0)||!check(0x27c,0)||!check(0x1024,0))return 13;
    return 0;
}
