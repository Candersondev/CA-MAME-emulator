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
    cycles_=halfFrames_=0;
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
std::uint64_t GunFight::runHalfFrame() {
    const auto half=++halfFrames_;
    const auto frame=(half-1)/2;
    const auto scanline=(half&1)?IntTrigger1:IntTrigger2;
    const auto deadline=((frame*VTotal+scanline)*std::uint64_t(CpuHz)*HTotal)/PixelHz;
    const auto before=cycles_;
    while(cycles_<deadline) cycles_+=cpu_.step();
    cpu_.interrupt((half&1)?1:2);
    return cycles_-before;
}
std::uint64_t GunFight::runFrame() {
    return runHalfFrame()+runHalfFrame();
}
std::array<std::uint32_t,GunFight::Width*GunFight::Height>
GunFight::frameBuffer() const {
    std::array<std::uint32_t,Width*Height> pixels{};
    for(std::size_t offset=0;offset<0x1c00;++offset) {
        const auto bits=memory_.read(std::uint16_t(0x2400+offset));
        const auto x=offset/32;
        const auto yByte=offset%32;
        for(unsigned bit=0;bit<8;++bit) {
            const auto y=yByte*8+bit;
            pixels[(Height-1-y)*Width+x]=(bits&(1u<<bit))?0xffffffffu:0xff000000u;
        }
    }
    return pixels;
}
}
