#include "ca_mame/drivers/M2MemoryMap.hpp"
#include <cstdint>
int main(){
    using namespace ca_mame::drivers;
    M2InterruptController irq;
    M2MemoryMap map(irq);
    const auto c=M2Cde::base;
    const auto ram=M2MemoryMap::ramBase;
    std::uint32_t v=0;
    if(!map.loadBioByte(0x20000000u,0x12)||!map.loadBioByte(0x20000001u,0x34)||
       !map.loadBioByte(0x20000002u,0xab)||!map.loadBioByte(0x20000003u,0xcd))return 1;
    if(!map.write32(c+0x240,0x800)||!map.write32(c+0x1004,0x20000000)||
       !map.write32(c+0x1008,ram)||!map.write32(c+0x100c,4))return 2;
    if(!map.write32(c+0x01c,0x80)||!map.write32(c+0x1000,0x80))return 3;
    if(!map.read32(ram,v)||v!=0x1234abcdu)return 4;
    if(!map.read32(c+0x018,v)||!(v&0x80)||!map.hardwareInterruptAsserted())return 5;
    if(!map.read32(c+0x100c,v)||v!=0)return 6;
    if(!map.write32(c+0x418,0x80)||map.hardwareInterruptAsserted())return 7;
    if(!map.write32(c+0x1024,0x20000000)||!map.write32(c+0x1028,ram+4)||
       !map.write32(c+0x102c,2)||!map.write32(c+0x1020,0x80))return 8;
    if(!map.read32(ram+4,v)||v!=0x12340000u)return 9;
    if(!map.read32(c+0x018,v)||!(v&0x100))return 10;
    if(!map.write32(c+0x1004,0x20000000)||!map.write32(c+0x1008,ram+8)||
       !map.write32(c+0x100c,3))return 11;
    if(map.write32(c+0x1000,0x80))return 12; // odd byte count rejected
    if(!map.read32(c+0x100c,v)||v!=3)return 13;
    if(!map.write32(c+0x100c,2)||!map.write32(c+0x1008,ram+M2MemoryMap::ramSize))return 14;
    if(map.write32(c+0x1000,0x80))return 15; // out-of-range destination
    if(!map.write32(c+0x1008,ram+8)||!map.write32(c+0x1004,0x2000fffe)||
       !map.write32(c+0x100c,4))return 16;
    if(map.write32(c+0x1000,0x80))return 17; // source crosses simulated device boundary
    return 0;
}
