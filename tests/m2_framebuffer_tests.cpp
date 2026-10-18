#include "ca_mame/drivers/KonamiM2.hpp"
#include <cstdint>
#include <vector>
int main(){
 ca_mame::drivers::KonamiM2 m;
 constexpr auto base=ca_mame::drivers::M2MemoryMap::ramBase;
 if(!m.loadWord(base,0x00112233u)||!m.loadWord(base+4,0x00445566u)||
    !m.loadWord(base+8,0x00778899u)||!m.loadWord(base+12,0x00aabbccu))return 1;
 std::vector<std::uint8_t> rgba;
 if(!m.captureFramebuffer(base,2,2,8,rgba)||rgba.size()!=16)return 2;
 if(rgba[0]!=0x11||rgba[1]!=0x22||rgba[2]!=0x33||rgba[3]!=255)return 3;
 if(rgba[12]!=0xaa||rgba[13]!=0xbb||rgba[14]!=0xcc||rgba[15]!=255)return 4;
 if(m.captureFramebuffer(base,0,2,8,rgba))return 5;
 if(m.captureFramebuffer(base,2,2,4,rgba))return 6;
 if(m.captureFramebuffer(0xfffffffcu,2,2,8,rgba))return 7;
 return 0;
}
