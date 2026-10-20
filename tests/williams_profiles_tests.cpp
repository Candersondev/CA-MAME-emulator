#include <array>
#include <cstdint>
#include <iostream>
#include "ca_mame/drivers/Williams6809.hpp"
#include "ca_mame/cpu/Motorola6809.hpp"
int main(){
 auto fail=[](int n){std::cerr<<"Williams profile stage "<<n<<" failed\n";return n;};
 std::array<std::uint8_t,65536> ram{};
 ram[0xfffe]=0x80;ram[0xffff]=0;
 const std::array<std::uint8_t,12> code={0x86,0xf0,0x84,0x0f,0x8a,0x03,0x88,0x01,0x80,0x01,0x20,0xfe};
 for(unsigned i=0;i<code.size();++i)ram[0x8000+i]=code[i];
 ca_mame::Motorola6809 cpu(ram);
 for(int i=0;i<5;++i)cpu.step();
 if(cpu.a!=1||cpu.unsupportedOpcodes()!=0)return fail(1);
 ca_mame::drivers::Williams6809 board;
 const std::array<std::uint8_t,2> vector={0xe0,0x00};
 const std::array<std::uint8_t,3> loop={0x12,0x20,0xfd};
 if(!board.loadProgram(0xfffe,vector)||!board.loadProgram(0xe000,loop))return fail(2);
 board.setBoardProfile(ca_mame::drivers::Williams6809::BoardProfile::Robotron);
 board.reset();
 board.busWrite(0xc900,2);
 if(board.romBank()!=2||board.busRead(0xc900)!=2)return fail(3);
 board.busWrite(0xc980,0x5a);
 if(board.soundCommand()!=0x5a||board.busRead(0xc980)!=0x5a)return fail(4);
 board.busWrite(0xc804,0xff);
 board.busWrite(0xc805,0x07);
 board.busWrite(0xc804,0xa5);
 if(board.pia(0).outputA()!=0xa5)return fail(5);
 board.runFrame();
 if(board.rasterInterrupts()!=1||!board.pia(0).irq())return fail(6);
 board.reset();
 if(board.soundCommand()!=0||board.rasterInterrupts()!=0||board.romBank()!=0)return fail(7);
 return 0;
}
