#include "ca_mame/drivers/GunFight.hpp"
namespace ca_mame::drivers {
GunFight::GunFight():cpu_(memory_) {
    memory_.configureSpaceInvadersMap(); // shared 14-bit decode and ROM protection
    cpu_.attachIo(io_);
    for(std::uint8_t port=0;port<8;++port) {
        io_.onRead(port,[this,port] {
            switch(port&3) {
                case 0:return inputs_[0];
                case 1:return inputs_[1];
                case 2:return inputs_[2];
                default:return std::uint8_t((shift_>>(8-shiftCount_))&0xff);
            }
        });
        // Gun Fight has partial output decoding: shift count on A1,
        // shift data on A2; A0 drives sound (not yet synthesized).
        io_.onWrite(port,[this,port](std::uint8_t value) {
            if(port&2) shiftCount_=value&7;
            if(port&4) shift_=std::uint16_t((std::uint16_t(value)<<8)|(shift_>>8));
        });
    }
    reset();
}
void GunFight::reset() {
    memory_.clearRam();
    cpu_.reset();
    inputs_.fill(0);
    shift_=0;
    shiftCount_=0;
    cycles_=frames_=0;
}
bool GunFight::loadRom(std::uint16_t address,std::span<const std::uint8_t> bytes) {
    if(address>=0x2000 || bytes.size()>0x2000u-address)return false;
    for(std::size_t i=0;i<bytes.size();++i)
        memory_.loadRomByte(std::uint16_t(address+i),bytes[i]);
    return true;
}
void GunFight::setInput(std::uint8_t port,std::uint8_t value) noexcept {
    if(port<inputs_.size())inputs_[port]=value;
}
std::uint64_t GunFight::runFrame() {
    // Initial 2 MHz / 60 Hz scheduler; board-specific raster/IRQ TODO.
    const auto target=++frames_*2000000ull/60;
    const auto before=cycles_;
    while(cycles_<target)cycles_+=cpu_.step();
    return cycles_-before;
}
}
