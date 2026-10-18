#include <cstdint>
#include <iostream>
#include "ca_mame/cpu/Intel8080.hpp"
namespace {
int failures=0;
void check(bool value,const char* message) {
    if(!value){std::cerr<<"FAIL: "<<message<<"\n";++failures;}
}
}
int main() {
    ca_mame::MemoryBus bus;
    ca_mame::Intel8080 cpu(bus);
    // ADC must include carry in auxiliary carry calculation.
    bus.write(0,0xce);bus.write(1,0x00); // ACI 00
    cpu.a=0x0f;cpu.cy=true;
    check(cpu.step()==7,"ACI timing");
    check(cpu.a==0x10 && cpu.ac && !cpu.cy,"ACI auxiliary carry");
    cpu.reset();
    // SUB / SBB / CMP preserve correct accumulator and borrow flags.
    bus.write(0,0xd6);bus.write(1,0x01);
    cpu.a=0;cpu.step();
    check(cpu.a==0xff && cpu.cy && cpu.s && !cpu.z,"SUI borrow and flags");
    cpu.reset();
    bus.write(0,0xfe);bus.write(1,0x10);
    cpu.a=0x10;cpu.step();
    check(cpu.a==0x10 && cpu.z && !cpu.cy,"CPI compare without accumulator write");
    cpu.reset();
    // INR must not change carry.
    bus.write(0,0x04);cpu.b=0xff;cpu.cy=true;cpu.step();
    check(cpu.b==0 && cpu.z && cpu.cy,"INR preserves carry");
    cpu.reset();
    // EI enables interrupts only after the following instruction.
    bus.write(0,0xfb);bus.write(1,0x00);bus.write(2,0x00);
    cpu.step();check(!cpu.interruptsEnabled(),"EI delayed immediately");
    cpu.step();check(!cpu.interruptsEnabled(),"EI delayed through following instruction");
    cpu.step();check(cpu.interruptsEnabled(),"EI enabled after following instruction");
    return failures?1:0;
}
