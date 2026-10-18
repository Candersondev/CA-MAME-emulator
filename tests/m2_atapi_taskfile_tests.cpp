#include "ca_mame/drivers/M2MemoryMap.hpp"
#include <array>
#include <cstdint>
int main(){
 using namespace ca_mame::drivers;
 M2InterruptController irq;
 M2MemoryMap memory(irq);
 constexpr auto b=M2AtapiTaskfile::base;
 std::uint32_t v=0;
 if(!memory.read32(b+0x10,v)||v!=0x40)return 1;
 if(!memory.write32(b+0x10,0xa0))return 2;
 if(!memory.read32(b+0x10,v)||v!=0x48)return 3;
 // INQUIRY, allocation length 36
 const std::array<std::uint16_t,6> words={0x1200,0x0000,0x2400,0,0,0};
 for(auto w:words)if(!memory.write32(b,w))return 4;
 if(!memory.hardwareInterruptAsserted())return 5;
 if(!memory.read32(b+0x10,v)||v!=0x48||memory.hardwareInterruptAsserted())return 6;
 if(!memory.read32(b,v)||v!=0x0580)return 7;
 for(unsigned i=1;i<18;++i)if(!memory.read32(b,v))return 8;
 if(!memory.read32(b+0x10,v)||v!=0x40)return 9;
 if(!memory.write32(b+0x10,0xa0))return 10;
 const std::array<std::uint16_t,6> tur={0,0,0,0,0,0};
 for(auto w:tur)if(!memory.write32(b,w))return 11;
 if(!memory.read32(b+0x10,v)||v!=0x41)return 12; // no disc
 if(!memory.read32(b+0x04,v)||v!=4)return 13;
 return 0;
}
