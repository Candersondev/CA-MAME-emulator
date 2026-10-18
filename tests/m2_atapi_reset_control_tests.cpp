#include "ca_mame/drivers/M2MemoryMap.hpp"
#include "ca_mame/drivers/M2InterruptController.hpp"
#include <cstdint>
int main(){
 ca_mame::drivers::M2InterruptController irq;
 ca_mame::drivers::M2MemoryMap map(irq);
 constexpr auto ata=ca_mame::drivers::M2AtapiTaskfile::base;
 constexpr auto ctrl=ca_mame::drivers::M2MemoryMap::konamiAtapiControlBase;
 std::uint32_t status=0;
 if(!map.write32(ata+0x10u,0xa0u))return 1;
 if(!map.read32(ata+0x10u,status)||status!=0x48u)return 2;
 if(!map.write32(ctrl,0x40000000u))return 3;
 if(map.atapiControl()!=0x4000u)return 4;
 if(!map.read32(ata+0x10u,status)||status!=0x40u)return 5;
 if(map.write32(ata,0x1200u))return 6;
 if(!map.write32(ctrl,0xc0000000u))return 7;
 if(!map.write32(ata+0x10u,0xa0u))return 8;
 if(!map.write32(ctrl,0x80000000u))return 9;
 if(!map.read32(ata+0x10u,status)||status!=0x48u)return 10;
 map.resetRegisters();
 if(map.atapiControl()!=0x8000u)return 11;
 return 0;
}
