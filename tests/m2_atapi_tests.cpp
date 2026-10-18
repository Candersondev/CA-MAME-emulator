#include "ca_mame/drivers/M2MemoryMap.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
int main(){
 using namespace ca_mame::drivers;
 const std::string path="ca_mame_atapi_test.iso";
 {std::ofstream f(path,std::ios::binary|std::ios::trunc);if(!f)return 1;
  for(unsigned i=0;i<4096;++i)f.put(static_cast<char>(i&255u));}
 M2InterruptController irq;
 M2MemoryMap memory(irq);
 auto& cd=memory.cdRom();
 std::array<std::uint8_t,12> cmd{};
 std::vector<std::uint8_t> out;
 cmd[0]=0x00;
 if(cd.packet(cmd,out)||cd.senseKey()!=2)return 2;
 cmd={};cmd[0]=0x03;cmd[4]=18;
 if(!cd.packet(cmd,out)||out.size()!=18||out[12]!=0x3a)return 3;
 cmd={};cmd[0]=0x12;cmd[4]=36;
 if(!cd.packet(cmd,out)||out.size()!=36||out[0]!=5)return 4;
 if(!cd.mountIso(path)||cd.sectors()!=2)return 5;
 cmd={};cmd[0]=0x25;
 if(!cd.packet(cmd,out)||out.size()!=8||out[3]!=1||out[6]!=8)return 6;
 cmd={};cmd[0]=0x28;cmd[5]=1;cmd[8]=1;
 if(!cd.packet(cmd,out)||out.size()!=2048||out[0]!=0||out[1]!=1)return 7;
 std::size_t bytes=0;
 if(!memory.stageAtapiPacket(cmd,0x20000000u,bytes)||bytes!=2048)return 8;
 constexpr auto c=M2Cde::base;
 if(!memory.write32(c+0x240,0x800)||!memory.write32(c+0x1004,0x20000000u)||
    !memory.write32(c+0x1008,M2MemoryMap::ramBase)||!memory.write32(c+0x100c,2048)||
    !memory.write32(c+0x1000,0x80))return 9;
 std::uint32_t value=0;
 if(!memory.read32(M2MemoryMap::ramBase,value)||value!=0x00010203u)return 10;
 cmd={};cmd[0]=0x28;cmd[5]=2;cmd[8]=1;
 if(cd.packet(cmd,out)||cd.senseKey()!=5||cd.asc()!=0x21)return 11;
 cmd={};cmd[0]=0xff;
 if(cd.packet(cmd,out)||cd.asc()!=0x20)return 12;
 cd.eject();
 std::remove(path.c_str());
 return 0;
}
