#include <cassert>
#include "ca_mame/cpu/Intel8080.hpp"
int main() {
    ca_mame::MemoryBus bus;
    ca_mame::Intel8080 cpu(bus);
    // LXI SP,4000; MVI A,0F; ADI 01; STA 2000; HLT
    const unsigned char program[]={0x31,0x00,0x40,0x3e,0x0f,0xc6,0x01,0x32,0x00,0x20,0x76};
    for(unsigned i=0;i<sizeof(program);++i)bus.write(static_cast<std::uint16_t>(i),program[i]);
    assert(cpu.step()==10);
    assert(cpu.sp==0x4000);
    assert(cpu.step()==7);
    assert(cpu.a==0x0f);
    assert(cpu.step()==7);
    assert(cpu.a==0x10 && cpu.ac && !cpu.cy);
    assert(cpu.step()==13);
    assert(bus.read(0x2000)==0x10);
    assert(cpu.step()==7 && cpu.halted);
    cpu.reset();
    assert(!cpu.halted && cpu.pc==0);
    // CALL 0010; return to 0003
    bus.write(0,0xcd);bus.write(1,0x10);bus.write(2,0);
    bus.write(0x10,0xc9);
    cpu.sp=0x4000;
    assert(cpu.step()==17 && cpu.pc==0x10);
    assert(cpu.step()==10 && cpu.pc==3);
    // MOV B,A and INR B
    bus.write(3,0x47);bus.write(4,0x04);
    cpu.a=0xff;
    assert(cpu.step()==5 && cpu.b==0xff);
    assert(cpu.step()==5 && cpu.b==0 && cpu.z);
    return 0;
}
