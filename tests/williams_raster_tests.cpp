#include <array>
#include <cstdint>
#include <iostream>
#include "ca_mame/drivers/Williams6809.hpp"
int main(){
 auto fail=[](int n){std::cerr<<"Williams raster stage "<<n<<" failed\n";return n;};
 ca_mame::drivers::Williams6809 board;
 board.setBoardProfile(ca_mame::drivers::Williams6809::BoardProfile::Defender);
 board.reset();
 if(board.scanline()!=0||board.rasterInterrupts()!=0)return fail(1);
 board.advanceRaster(1000);
 if(board.scanline()==0||board.rasterInterrupts()!=0)return fail(2);
 board.advanceRaster(15667);
 if(board.scanline()!=0||board.rasterInterrupts()!=1)return fail(3);
 board.advanceRaster(16667);
 if(board.scanline()!=0||board.rasterInterrupts()!=2)return fail(4);
 // A large advance must preserve the number of completed frames.
 board.reset();
 board.advanceRaster(1000000);
 if(board.rasterInterrupts()!=59)return fail(6);
 if(board.scanline()==0)return fail(7);
 board.reset();
 if(board.scanline()!=0||board.rasterInterrupts()!=0)return fail(5);
 // A second PIA can assert IRQ independently of the video PIA.
 board.pia(1).write(1,0x07);
 board.pia(1).edgeCA1(true);
 if(!board.pia(1).irq()||board.pia(0).irq())return fail(8);
 board.pia(1).read(0);
 if(board.pia(1).irq())return fail(9);
 return 0;
}
