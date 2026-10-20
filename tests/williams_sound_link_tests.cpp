#include <array>
#include <cstdint>
#include <iostream>
#include "ca_mame/drivers/Williams6809.hpp"
int main(){
 auto fail=[](int n){std::cerr<<"Williams sound link stage "<<n<<" failed\n";return n;};
 ca_mame::drivers::Williams6809 board;
 board.setBoardProfile(ca_mame::drivers::Williams6809::BoardProfile::Robotron);
 board.reset();
 if(board.soundLink().pending())return fail(1);
 board.busWrite(0xc980,0x35);
 if(!board.soundLink().pending()||board.soundCommand()!=0x35||board.soundLink().delivered()!=1)return fail(2);
 if(board.soundLink().acknowledge()!=0x35||board.soundLink().pending()||board.soundLink().acknowledged()!=1)return fail(3);
 board.busWrite(0xc980,0x7e);
 if(!board.soundLink().pending()||board.soundLink().delivered()!=2)return fail(4);
 board.reset();
 if(board.soundLink().pending()||board.soundCommand()!=0||board.soundLink().delivered()!=0)return fail(5);
 return 0;
}
