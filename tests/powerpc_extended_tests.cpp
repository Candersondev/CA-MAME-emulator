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
    cpu.reset();
    // Update-form stores/loads and CTR-dependent conditional branch.
    assert(bus.write32(0x100,0x3860002a)); // addi r3,r0,42
    assert(bus.write32(0x104,0x38800100)); // addi r4,r0,256
    assert(bus.write32(0x108,(37u<<26)|(3u<<21)|(4u<<16)|4u)); // stwu r3,4(r4)
    assert(bus.write32(0x10c,(33u<<26)|(5u<<21)|(4u<<16)|4u)); // lwzu r5,4(r4)
    assert(bus.write32(0x110,xform(6,9,467))); // mtspr CTR,r6
    assert(bus.write32(0x114,(16u<<26)|(16u<<21)|8u)); // bdnz +8
    assert(bus.write32(0x118,0x60000000)); // nop
    assert(bus.write32(0x11c,0x60000000)); // nop
    cpu.setPC(0x100);cpu.setGpr(6,2);
    for(int i=0;i<5;++i)assert(cpu.step()==1);
    assert(cpu.gpr(4)==0x108 && cpu.gpr(5)==42 && cpu.ctr()==2);
    assert(cpu.step()==1 && cpu.pc()==0x11c && cpu.ctr()==1);
}
