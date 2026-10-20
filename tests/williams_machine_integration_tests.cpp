#include <array>
#include <cstdint>
#include <iostream>
#include "ca_mame/drivers/Williams6809.hpp"
int main(){
 auto fail=[](int stage){std::cerr<<"Williams machine integration stage "<<stage<<" failed\n";return stage;};
 ca_mame::drivers::Williams6809 board;
 const std::array<std::uint8_t,3> bank0={0x12,0x34,0x56},bank1={0x9a,0xbc,0xde};
 if(!board.loadBank(0,bank0)||!board.loadBank(1,bank1))return fail(1);
 if(board.bankedRead(0)!=0x12)return fail(2);
 board.setRomBank(1);
 if(board.bankedRead(0)!=0x9a||board.bankedRead(2)!=0xde)return fail(3);
 board.video().setPalette(1,255,0,0);
 board.video().setPalette(2,0,255,0);
 const std::array<std::uint8_t,2> source={0x12,0x02};
 board.blitter().setSource(0x1000);
 board.blitter().setDestination(0);
 board.blitter().setDimensions(2,1);
 board.blitter().setControl(0);
 if(board.blitter().execute([&](std::uint16_t addr){return source[addr-0x1000];})!=2)return fail(4);
 if(board.video().read(0)!=0x12||board.video().read(1)!=0x02)return fail(5);
 board.blitter().setControl(1); // transparent zero nibbles
 board.blitter().setSolid(0x20);
 board.blitter().setControl(5);
 board.blitter().setDimensions(1,1);
 board.blitter().execute([&](std::uint16_t){return std::uint8_t(0);});
 if(board.video().read(0)!=0x22)return fail(6);
 board.pia(0).write(0,0xff);board.pia(0).write(1,4);board.pia(0).write(0,0xa5);
 if(board.pia(0).outputA()!=0xa5)return fail(7);
 board.reset();
 if(board.romBank()!=0||board.pia(0).outputA()!=0||board.video().read(0)!=0)return fail(8);
 if(board.bankedRead(0)!=0x12)return fail(9);
 return 0;
}
