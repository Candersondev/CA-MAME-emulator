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
 return 0;
}
