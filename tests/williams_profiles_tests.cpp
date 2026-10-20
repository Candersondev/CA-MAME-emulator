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
 // Bank selection is independent from the fixed high ROM and reset vector.
 const std::array<std::uint8_t,1> marker0={0x15},marker2={0x2a};
 if(!board.loadBank(0,marker0)||!board.loadBank(2,marker2))return fail(8);
 if(board.busRead(0)!=0x15)return fail(9);
 // Special Chip register latch: CA00 starts a command on Robotron boards.
 board.busWrite(0xca01,0xf0);
 board.busWrite(0xca02,0x40);board.busWrite(0xca03,0x00);
 board.busWrite(0xca04,0x40);board.busWrite(0xca05,0x10);
 board.busWrite(0xca06,2);board.busWrite(0xca07,3);
 if(board.specialChipStarts()!=0||board.specialChipRegister(1)!=0xf0)return fail(12);
 board.busWrite(0xca00,0x00);
 if(board.specialChipStarts()!=1||board.busRead(0xca00)!=0)return fail(13);
 board.busWrite(0x4000,0x9c);
 board.busWrite(0xca02,0x40);board.busWrite(0xca03,0x00);
 board.busWrite(0xca04,0x40);board.busWrite(0xca05,0x20);
 board.busWrite(0xca06,1);board.busWrite(0xca07,1);
 board.busWrite(0xca00,0);
 if(board.busRead(0x4020)!=0x9c||board.specialChipBytes()!=7)return fail(15);
 board.busWrite(0x4001,0x0f);board.busWrite(0x4021,0xa5);
 board.busWrite(0xca01,0x77);
 board.busWrite(0xca03,0x01);board.busWrite(0xca05,0x21);
 board.busWrite(0xca00,0x18);
 if(board.busRead(0x4021)!=0xa7)return fail(16);
 board.busWrite(0xca01,0x32);board.busWrite(0xca00,0x50);
 if(board.busRead(0x4021)!=0x37)return fail(17);
 board.busWrite(0x4100,0x56);
 board.busWrite(0xca02,0x40);board.busWrite(0xca03,0x00);
 board.busWrite(0xca04,0x40);board.busWrite(0xca05,0x22);
 board.busWrite(0xca06,2);board.busWrite(0xca07,1);
 board.busWrite(0xca00,1);
 if(board.busRead(0x4022)!=0x9c||board.busRead(0x4023)!=0x56)return fail(18);
 if(board.specialChipEstimatedCycles()<board.specialChipBytes())return fail(19);
 // Direct register writes defer bus contention until CPU scheduling.
 const auto pending=board.pendingDmaCycles();
 if(!pending||board.dmaStallCycles()!=0)return fail(21);
 const auto before=board.elapsedCycles();
 const auto diag=board.runBootDiagnostic(1);
 if(board.pendingDmaCycles()!=0||board.dmaStallCycles()!=pending)return fail(22);
 if(diag.cycles-before<1+pending)return fail(23);
 board.busWrite(0xc900,2);
 if(board.busRead(0)!=0x2a)return fail(10);
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
 if(board.busRead(0)!=0x15||board.cpu().pc!=0xe000)return fail(11);
 if(board.specialChipStarts()!=0||board.specialChipRegister(1)!=0)return fail(14);
 if(board.specialChipBytes()!=0||board.specialChipEstimatedCycles()!=0)return fail(20);
 if(board.pendingDmaCycles()!=0||board.dmaStallCycles()!=0)return fail(24);
 return 0;
}
