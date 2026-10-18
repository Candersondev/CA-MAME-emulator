#include "ca_mame/drivers/M2MemoryMap.hpp"
#include <cassert>
int main(){
    ca_mame::drivers::M2InterruptController irq;
    ca_mame::drivers::M2MemoryMap memory(irq);
    std::uint32_t value=0;
    assert(memory.write32(0,0x12345678));
    assert(memory.read32(0,value)&&value==0x12345678);
    assert(memory.loadBootWord(0,0x3860002a));
    assert(memory.read32(memory.bootRomBase,value) && value==0x3860002a);
    assert(memory.read32(memory.bootRomMirrorBase,value) && value==0x3860002a);
    assert(!memory.write32(memory.bootRomBase,0));
    assert(!memory.write32(memory.bootRomMirrorBase,0));
    assert(!memory.read32(0x30000000,value));
    assert(!memory.write32(1,0));
    irq.raise(3);
    assert(memory.read32(ca_mame::drivers::M2MemoryMap::prototypeIrqBase,value)&&value==8);
    assert(!irq.asserted(0));
    assert(memory.write32(ca_mame::drivers::M2MemoryMap::prototypeIrqBase+4,8));
    assert(irq.asserted(0)&&!irq.asserted(1));
    assert(memory.write32(ca_mame::drivers::M2MemoryMap::prototypeIrqBase+8,8));
    assert(irq.asserted(1));
    assert(memory.write32(ca_mame::drivers::M2MemoryMap::prototypeIrqBase+12,8));
    assert(!irq.asserted(0)&&!irq.asserted(1));
    irq.raise(31);irq.reset();
    assert(irq.pending()==0&&irq.mask(0)==0&&irq.mask(1)==0);
}
