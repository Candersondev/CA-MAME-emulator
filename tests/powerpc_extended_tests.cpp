#include "ca_mame/bus/BigEndianBus.hpp"
#include "ca_mame/cpu/PowerPC602.hpp"
#include <cassert>
#include <cstdint>
static std::uint32_t xform(unsigned rt,unsigned spr,unsigned xo){
    return (31u<<26)|(rt<<21)|((spr&31u)<<16)|((spr>>5)<<11)|(xo<<1);
}
int main(){
    ca_mame::BigEndianBus bus(4096);
    ca_mame::PowerPC602 cpu(bus);
    assert(bus.write32(0,xform(3,8,467))); // mtspr LR,r3
    assert(bus.write32(4,xform(4,8,339))); // mfspr r4,LR
    assert(bus.write32(8,xform(5,0,146))); // mtmsr r5
    assert(bus.write32(12,xform(6,0,83))); // mfmsr r6
    assert(bus.write32(16,0x4e800020)); // blr
    cpu.setGpr(3,0x80);
    cpu.setGpr(5,ca_mame::PowerPC602::MSR_EE);
    for(int i=0;i<4;++i)assert(cpu.step()==1);
    assert(cpu.lr()==0x80 && cpu.gpr(4)==0x80);
    assert(cpu.msr()==ca_mame::PowerPC602::MSR_EE);
    assert(cpu.gpr(6)==ca_mame::PowerPC602::MSR_EE);
    assert(cpu.step()==1 && cpu.pc()==0x80);
    assert(bus.write32(0x80,0xffffffff));
    assert(cpu.step()==0);
    assert(cpu.stopReason()==ca_mame::PowerPC602::StopReason::UnsupportedInstruction);
    assert(cpu.faultAddress()==0x80 && cpu.faultInstruction()==0xffffffff);
    cpu.reset();
    cpu.setPC(0xfff00000);
    assert(cpu.step()==0);
    assert(cpu.stopReason()==ca_mame::PowerPC602::StopReason::InstructionFetch);
    assert(cpu.faultAddress()==0xfff00000);
}
