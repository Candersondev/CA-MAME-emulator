#include "ca_mame/drivers/M2CpuBus.hpp"
#include "ca_mame/drivers/KonamiM2.hpp"
#include <cassert>
#include <cstdint>

int main(){
    using namespace ca_mame::drivers;
    M2InterruptController irq;
    M2MemoryMap memory(irq);
    M2CpuBus bus0(memory,0),bus1(memory,1);
    std::uint32_t value=0;
    assert(bus0.read32(M2MemoryMap::cpuIdBase,value)&&value==0u);
    assert(bus1.read32(M2MemoryMap::cpuIdBase,value)&&value==0x80000000u);
    assert(bus1.write32(M2MemoryMap::cpuIdBase,0xffffffffu));
    assert(bus1.read32(M2MemoryMap::cpuIdBase,value)&&value==0x80000000u);
    assert(bus0.write32(M2MemoryMap::ramBase,0x12345678u));
    assert(bus1.read32(M2MemoryMap::ramBase,value)&&value==0x12345678u);
    assert(bus1.write8(M2MemoryMap::ramBase+1,0xabu));
    assert(bus0.read32(M2MemoryMap::ramBase,value)&&value==0x12ab5678u);
    assert(bus0.write32(M2MemoryMap::powerBusBase+0x40,0x100u));
    assert(bus1.read32(M2MemoryMap::powerBusBase+0x40,value)&&value==0x100u);

    // Exercise the CPU interpreters through their separate bus views.
    KonamiM2 board;
    assert(board.loadWord(0,0x3c601000u)); // addis r3,r0,0x1000
    assert(board.loadWord(4,0x80630000u)); // lwz r3,0(r3) (CPU ID)
    board.cpu(0).setPC(0);
    board.cpu(1).setPC(0);
    board.stepPair();
    board.stepPair();
    assert(board.cpu(0).gpr(3)==0u);
    assert(board.cpu(1).gpr(3)==0x80000000u);
}
