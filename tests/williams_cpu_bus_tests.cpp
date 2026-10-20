#include <array>
#include <cstdint>
#include <iostream>
#include "ca_mame/drivers/Williams6809.hpp"
int main(){
 auto fail=[](int stage){std::cerr<<"Williams CPU bus stage "<<stage<<" failed\n";return stage;};
 ca_mame::drivers::Williams6809 board;
 // Program in fixed high ROM: LDA #1; STA $d020 (bank select);
 // LDA $0010 (banked ROM); STA $d004 (PIA1 DDR B);
 // LDA #$04; STA $d005 (PIA1 control B);
 // LDA #$a5; STA $d004 (PIA1 output B); BRA -2.
 const std::array<std::uint8_t,27> code={
  0x86,0x01,0xb7,0xd0,0x20,
  0xb6,0x00,0x10,0xb7,0xd0,0x06,
  0x86,0x04,0xb7,0xd0,0x07,
  0x86,0xa5,0xb7,0xd0,0x06,
  0x20,0xfe,0x12,0x12,0x12,0x12
 };
 const std::array<std::uint8_t,2> vector={0xe0,0x00};
 const std::array<std::uint8_t,17> bank0{};
 std::array<std::uint8_t,17> bank1{};bank1[16]=0xff;
 if(!board.loadProgram(0xe000,code)||!board.loadProgram(0xfffe,vector)||
    !board.loadBank(0,bank0)||!board.loadBank(1,bank1))return fail(1);
 board.enableHardwareBus();
 board.reset();
 if(board.cpu().pc!=0xe000)return fail(2);
 for(int i=0;i<8;++i)board.cpu().step();
 if(board.romBank()!=1||board.cpu().a!=0xa5)return fail(3);
 if(board.pia(1).outputB()!=0xa5)return fail(4);
 if(board.busRead(0x0010)!=0xff)return fail(5);
 board.busWrite(0x4000,0x12);
 if(board.video().read(0)!=0x12||board.busRead(0x4000)!=0x12)return fail(6);
 board.busWrite(0xd010,4); // solid fill
 board.busWrite(0xd011,0x34);
 board.busWrite(0xd012,0x00);board.busWrite(0xd013,0x00);
 board.busWrite(0xd014,0x00);board.busWrite(0xd015,0x00);
 board.busWrite(0xd016,1);board.busWrite(0xd017,1); // trigger
 if(board.video().read(0)!=0x34)return fail(7);
 board.busWrite(0x0000,0xaa);
 if(board.busRead(0x0000)!=bank1[0])return fail(8); // ROM is write-protected
 board.reset();
 if(board.romBank()!=0||board.video().read(0)!=0)return fail(9);
 return 0;
}
