#include "ca_mame/drivers/SpaceInvaders.hpp"
#include <algorithm>
namespace ca_mame::drivers {
SpaceInvaders::SpaceInvaders() : cpu_(memory_) {
    memory_.configureSpaceInvadersMap();
    cpu_.attachIo(io_);
    io_.onRead(0,[this]{return inputs_[0];});
    io_.onRead(1,[this]{return inputs_[1];});
    io_.onRead(2,[this]{return inputs_[2];});
    io_.onRead(3,[this]{return std::uint8_t((shift_>>(8-shiftOffset_))&0xff);});
    io_.onWrite(2,[this](std::uint8_t v){shiftOffset_=v&7;});
    io_.onWrite(4,[this](std::uint8_t v){shift_=std::uint16_t((std::uint16_t(v)<<8)|(shift_>>8));});
    reset();
}
void SpaceInvaders::reset() {
    memory_.clear();cpu_.reset();
    inputs_={0x0e,0x08,0x00,0x00};
    shift_=0;shiftOffset_=0;cycles_=0;halfFrames_=0;
}
void SpaceInvaders::setInput(std::uint8_t port,std::uint8_t value) {
    if(port<inputs_.size())inputs_[port]=value;
}
bool SpaceInvaders::loadRom(std::uint16_t address,std::span<const std::uint8_t> bytes) {
    // 0x0000-0x1fff is the original 8 KiB program ROM region.
    if(address>=0x2000 || bytes.size()>0x2000u-address)return false;
    for(std::size_t i=0;i<bytes.size();++i)memory_.loadRomByte(std::uint16_t(address+i),bytes[i]);
    return true;
}
std::uint64_t SpaceInvaders::runHalfFrame() {
    // Alternating RST 1 / RST 2 at nominal 120 Hz, with 2 MHz CPU.
    // CPU instructions can cross the boundary; overshoot is retained.
    // Use an absolute rational deadline to avoid accumulating fractional
    // 2 MHz / 120 Hz truncation error over long emulation sessions.
    const auto target=std::uint64_t(++halfFrames_)*CpuHz/120;
    const auto before=cycles_;
    while(cycles_<target)cycles_+=cpu_.step();
    cpu_.interrupt((halfFrames_&1)?1:2);
    return cycles_-before;
}
std::array<std::uint32_t,SpaceInvaders::Width*SpaceInvaders::Height>
SpaceInvaders::frameBuffer() const {
    std::array<std::uint32_t,Width*Height> pixels{};
    // Video RAM: 0x2400-0x3fff, 1 bit per pixel, rotated 90 degrees.
    for(std::size_t offset=0;offset<0x1c00;++offset) {
        auto bits=memory_.read(std::uint16_t(0x2400+offset));
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
