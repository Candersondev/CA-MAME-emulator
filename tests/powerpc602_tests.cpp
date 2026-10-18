#include "ca_mame/bus/BigEndianBus.hpp"
#include "ca_mame/cpu/PowerPC602.hpp"
#include <cassert>
int main(){
    ca_mame::BigEndianBus bus(256);
    std::uint32_t value=0;
    assert(bus.write32(0,0x3860002a)); // addi r3,r0,42
    assert(bus.write32(4,0x90600080)); // stw r3,128(r0)
    assert(bus.write32(8,0x80800080)); // lwz r4,128(r0)
    assert(bus.write32(12,0x60840001)); // ori r4,r4,1
    ca_mame::PowerPC602 cpu(bus);
    assert(cpu.step()==1 && cpu.gpr(3)==42 && cpu.pc()==4);
    assert(cpu.step()==1 && bus.read32(128,value) && value==42);
    assert(cpu.step()==1 && cpu.gpr(4)==42);
    assert(cpu.step()==1 && cpu.gpr(4)==43);
    assert(!bus.read32(1,value));
    assert(!bus.write32(255,1));
    assert(bus.write32(16,0xffffffff));
    cpu.setPC(16);
    assert(cpu.step()==0 && cpu.faulted());
    cpu.reset();
    assert(bus.write32(0x40,0x60000000));
    cpu.setPC(0x40);
    cpu.setExternalInterrupt(true);
    assert(cpu.step()==1 && cpu.pc()==0x44); // EE=0: interrupt masked
    cpu.setMSR(ca_mame::PowerPC602::MSR_EE);
    assert(cpu.step()==1 && cpu.pc()==0x500 && cpu.srr0()==0x44);
    assert(cpu.srr1()==ca_mame::PowerPC602::MSR_EE);
    cpu.reset();
    assert(!cpu.faulted() && cpu.pc()==0 && cpu.gpr(3)==0);
}
