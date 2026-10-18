#include "ca_mame/drivers/M2MemoryMap.hpp"
#include <cassert>
#include <cstdint>
int main(){
    ca_mame::drivers::M2InterruptController irq;
    ca_mame::drivers::M2MemoryMap bus(irq);
    std::uint32_t value=0;
    using Map=ca_mame::drivers::M2MemoryMap;
    assert(bus.write32(Map::ramBase,0x12345678u));
    assert(bus.read32(Map::ramBase,value)&&value==0x12345678u);
    assert(bus.read32(Map::cpuIdBase,value)&&value==0u);
    assert(bus.read32(Map::memoryControlBase,value)&&value==((5u<<13)|(5u<<10)));
    assert(bus.write32(Map::memoryControlBase,0x12345000u));
    assert(bus.read32(Map::memoryControlBase,value)&&value==0x12345000u);
    assert(bus.write32(Map::memoryControlBase+4,0x00001234u));
    assert(bus.read32(Map::memoryControlBase+4,value)&&value==0x00001234u);
    assert(bus.write32(Map::memoryControlBase+8,0xffffffffu));
    assert(bus.read32(Map::memoryControlBase+8,value)&&value==0u);
    assert(bus.write32(Map::memoryControlBase+12,0xffffffffu));
    assert(bus.read32(Map::memoryControlBase+12,value)&&value==0u);
    assert(bus.read32(Map::powerBusBase+0x40,value)&&value==0);
    assert(bus.write32(Map::powerBusBase+0x40,1u<<24));
    bus.raiseHardwareInterrupt(24);
    assert(bus.hardwareInterruptAsserted());
    assert(bus.read32(Map::powerBusBase+0x50,value)&&value==(1u<<24));
    assert(bus.write32(Map::powerBusBase+0x440,1u<<24));
    assert(!bus.hardwareInterruptAsserted());
    assert(bus.write32(Map::powerBusBase+0x40,1u<<24));
    assert(bus.write32(Map::powerBusBase+0x460,1u<<24));
    assert(!bus.hardwareInterruptAsserted());
    bus.resetRegisters();
    assert(bus.read32(Map::powerBusBase+0x40,value)&&value==0);
}
