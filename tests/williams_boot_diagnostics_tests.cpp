#include <array>
#include <cstdint>
#include <iostream>
#include "ca_mame/drivers/Williams6809.hpp"
int main(){
 auto fail=[](int n){std::cerr<<"Williams boot diagnostic stage "<<n<<" failed\n";return n;};
 ca_mame::drivers::Williams6809 board;
 const std::array<std::uint8_t,2> vector={0x80,0x00};
 const std::array<std::uint8_t,3> loop={0x12,0x20,0xfd};
 if(!board.loadProgram(0xfffe,vector)||!board.loadProgram(0x8000,loop))return fail(1);
 board.reset();
 const auto result=board.runBootDiagnostic(200);
 if(result.pc<0x8000||result.pc>0x8003||result.cycles<200||
    result.unsupported!=0||result.halted)return fail(2);
 board.write(0x8000,0x02);
 board.reset();
 const auto invalid=board.runBootDiagnostic(200);
 if(!invalid.halted||invalid.unsupported!=1||invalid.cycles>10)return fail(3);
 return 0;
}
