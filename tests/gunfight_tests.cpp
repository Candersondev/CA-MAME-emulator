#include <array>
#include "ca_mame/drivers/GunFight.hpp"
#include "ca_mame/drivers/SpaceInvaders.hpp"
#include "ca_mame/core/MachineRegistry.hpp"
int main() {
    ca_mame::drivers::GunFight gun;
    const std::array<std::uint8_t,2> rom={0x00,0x76};
    if(!gun.loadRom(0,rom) || gun.loadRom(0x2000,rom)) return 1;
    gun.reset();
    if(gun.readMemory(1)!=0x76) return 2;
    gun.setInput(1,0x42);
    if(gun.readPort(1)!=0x42 || gun.readPort(5)!=0x42) return 3;
    gun.writePort(4,0xaa);
    gun.writePort(4,0x55);
    gun.writePort(2,2);
    if(gun.readPort(3)!=0x56) return 4;
    if(gun.runFrame()==0) return 5;
    const auto registry=ca_mame::makeDefaultRegistry();
    auto machine=registry.create("gunfight");
    if(!machine || machine->driverId()!="gunfight") return 6;
    if(!registry.create("invaders")) return 7;
    return 0;
}
