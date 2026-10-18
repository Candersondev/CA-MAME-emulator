#include <cassert>
#include <cstdint>
#include "ca_mame/cpu/Intel8080.hpp"
int main() {
    ca_mame::MemoryBus memory;
    ca_mame::IoBus io;
    ca_mame::Intel8080 cpu(memory);
    std::uint8_t written=0;
    io.onWrite(3,[&](std::uint8_t v){written=v;});
    io.onRead(2,[]{return std::uint8_t{0xa5};});
    cpu.attachIo(io);
    // MVI A,42; OUT 03; IN 02; EI; NOP; HLT
    const std::uint8_t code[]={0x3e,0x42,0xd3,0x03,0xdb,0x02,0xfb,0x00,0x76};
    for(unsigned i=0;i<sizeof(code);++i)memory.write(static_cast<std::uint16_t>(i),code[i]);
    assert(cpu.step()==7 && cpu.a==0x42);
    assert(cpu.step()==10 && written==0x42);
    assert(cpu.step()==10 && cpu.a==0xa5);
    assert(cpu.step()==4 && !cpu.interruptsEnabled());
    assert(cpu.step()==4 && !cpu.interruptsEnabled());
    assert(cpu.step()==7 && cpu.interruptsEnabled() && cpu.halted);
    cpu.sp=0x4000;
    cpu.interrupt(1);
    assert(cpu.step()==11 && cpu.pc==8 && !cpu.halted);
    assert(!cpu.interruptsEnabled());
    return 0;
}
