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
    gun.writePort(1,0x05);
    if(gun.soundPort()!=0x05 || gun.takeSoundRisingEdges()!=0x05) return 12;
    if(gun.takeSoundRisingEdges()!=0) return 13;
    gun.writePort(7,0x01);
    if(gun.soundPort()!=0x01 || gun.takeSoundRisingEdges()!=0) return 14;
    gun.writePort(3,0x05);
    if(gun.takeSoundRisingEdges()!=0x04) return 15;
    if(gun.runFrame()==0) return 5;
    const auto registry=ca_mame::makeDefaultRegistry();
    auto machine=registry.create("gunfight");
    if(!machine || machine->driverId()!="gunfight") return 6;
    if(!registry.create("invaders")) return 7;
    // Video RAM is a 224 x 256 1-bit rotated bitmap.
    {
        ca_mame::drivers::GunFight video;
        const std::array<std::uint8_t,3> store={0x32,0x00,0x24};
        if(!video.loadRom(0,store)) return 8;
        video.cpu().a=0x81;
        video.cpu().step();
        const auto pixels=video.frameBuffer();
        if(pixels.size()!=224*256 || pixels[255*224]!=0xffffffffu ||
           pixels[248*224]!=0xffffffffu || pixels[254*224]!=0xff000000u) return 9;
    }
    // IRQ requests occur at scanlines 128 and 218 each 262-line frame.
    {
        using Board=ca_mame::drivers::GunFight;
        Board timing;
        for(std::uint64_t half=1;half<=240;++half) {
            const auto before=timing.elapsedCycles();
            const auto ran=timing.runHalfFrame();
            const auto frame=(half-1)/2;
            const auto line=(half&1)?Board::IntTrigger1:Board::IntTrigger2;
            const auto deadline=((frame*Board::VTotal+line)
                *std::uint64_t(Board::CpuHz)*Board::HTotal)/Board::PixelHz;
            if(timing.elapsedHalfFrames()!=half || ran!=timing.elapsedCycles()-before ||
               timing.elapsedCycles()<deadline || timing.elapsedCycles()>=deadline+4) return 10;
        }
    }
    // Interrupt vectors must be delivered through the shared Intel 8080.
    {
        ca_mame::drivers::GunFight irq;
        const std::array<std::uint8_t,1> halt={0x76};
        irq.loadRom(0,halt);
        irq.cpu().sp=0x2400;
        irq.cpu().setInterruptEnabled(true);
        irq.runHalfFrame();
        if(irq.cpu().step()!=11 || irq.cpu().pc!=0x08) return 11;
    }
    return 0;
}
