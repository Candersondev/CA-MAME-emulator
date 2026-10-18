#include "ca_mame/drivers/M2MemoryMap.hpp"
#include <cassert>
#include <cstdint>
int main(){
    ca_mame::drivers::M2InterruptController irq;
    ca_mame::drivers::M2MemoryMap memory(irq);
    constexpr auto base=ca_mame::drivers::M2Cde::base;
    std::uint32_t value=0;
    assert(memory.read32(base,value)&&value==0x00010000u);
    assert(memory.read32(base+0x280,value)&&value==0x63659000u);
    assert(memory.read32(base+0x208,value)&&value==1u);
    assert(memory.write32(base+0x01c,0x10000000u));
    assert(memory.read32(base+0x01c,value)&&value==0x10000000u);
    assert(memory.write32(base+0x014,0x55u));
    assert(memory.read32(base+0x018,value)&&value==0x10000000u);
    assert(memory.hardwareInterruptAsserted());
    assert(memory.write32(base+0x418,0x10000000u));
    assert(!memory.hardwareInterruptAsserted());
    assert(memory.write32(base+0x41c,0x10000000u));
    assert(memory.read32(base+0x01c,value)&&value==0);
    assert(memory.write32(base+0x020,1u));
    assert(memory.read32(base+0x208,value)&&value==0);
    memory.resetRegisters();
    assert(memory.read32(base+0x208,value)&&value==1u);
    assert(!memory.read32(base+0x9990,value));
}
