#include <array>
#include <iostream>
#include "ca_mame/cpu/Motorola6809.hpp"
#include "ca_mame/drivers/Williams6809.hpp"
#include "ca_mame/core/MachineRegistry.hpp"
int main(){
    auto fail=[](int code){std::cerr<<"6809 test failure at stage "<<code<<"\n";return code;};
    std::array<std::uint8_t,65536> memory{};
    memory[0xfffe]=0x10;memory[0xffff]=0x00;
    // LDA #$42; STA <$20; LDB <$20; BRA -2
    const std::array<std::uint8_t,8> program={0x86,0x42,0x97,0x20,0xd6,0x20,0x20,0xfe};
    for(unsigned i=0;i<program.size();++i)memory[0x1000+i]=program[i];
    ca_mame::Motorola6809 cpu(memory);
    if(cpu.pc!=0x1000 || cpu.cc!=0x50)return fail(1);
    if(cpu.step()!=2 || cpu.a!=0x42)return fail(2);
    if(cpu.step()!=4 || memory[0x20]!=0x42)return fail(3);
    if(cpu.step()!=4 || cpu.b!=0x42)return fail(4);
    if(cpu.step()!=3 || cpu.pc!=0x1006)return fail(5);
    if(cpu.unsupportedOpcodes()!=0)return fail(6);
    memory[0x1006]=0x02;cpu.step();
    if(cpu.unsupportedOpcodes()!=1 || !cpu.halted())return fail(7);
    ca_mame::drivers::Williams6809 board;
    const std::array<std::uint8_t,2> resetVector={0x80,0x00};
    const std::array<std::uint8_t,3> loop={0x12,0x20,0xfd};
    if(!board.loadProgram(0xfffe,resetVector)||!board.loadProgram(0x8000,loop))return fail(8);
    board.reset();
    if(board.cpu().pc!=0x8000 || board.runFrame()==0)return fail(9);
    board.reset();
    if(board.read(0x8000)!=0x12 || board.cpu().pc!=0x8000)return fail(10);
    auto registry=ca_mame::makeDefaultRegistry();
    auto machine=registry.create("williams6809");
    if(!machine || machine->driverId()!="williams6809")return fail(11);
    return 0;
}
