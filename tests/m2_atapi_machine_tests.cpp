#include "ca_mame/drivers/KonamiM2.hpp"
#include "ca_mame/drivers/M2AtapiTaskfile.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
int fail(int code){std::fprintf(stderr,"m2_atapi_machine_tests failed at checkpoint %d\\n",code);return code;}
int main(){
 using namespace ca_mame::drivers;
 const char* path="ca_mame_machine_cd_test.iso";
 {std::ofstream f(path,std::ios::binary|std::ios::trunc);if(!f)return fail(1);
  for(unsigned i=0;i<6144;++i)f.put(static_cast<char>((i/2048+1)&255));}
 KonamiM2 m;
 if(!m.mountCdIso(path)||!m.cdReady())return fail(2);
 constexpr auto b=M2AtapiTaskfile::base;
 if(!m.loadWord(b+0x10,0xa0))return fail(3);
 // READ(10), starting LBA 1, two sectors
 const std::array<std::uint16_t,6> cmd={0x2800,0,0x0001,0,0x0002,0};
 for(const auto w:cmd)if(!m.loadWord(b,w))return fail(4);
 std::uint32_t v=0;
 if(!m.readWord(b+0x10,v)||v!=0x48)return fail(5);
 for(unsigned i=0;i<4096/2;++i){
    if(!m.readWord(b,v))return fail(6);
    const auto expected=(i<1024)?0x0202u:0x0303u;
    if(v!=expected)return fail(7);
 }
 if(!m.readWord(b+0x10,v)||v!=0x40)return fail(8);
 if(m.readWord(b,v))return fail(9); // data port exhausted
 m.reset();
 if(!m.readWord(b+0x10,v)||v!=0x40)return fail(10);
 m.ejectCd();
 if(m.cdReady())return fail(11);
 std::remove(path);
 return 0;
}
