#pragma once
#include <array>
#include <cstdint>
namespace ca_mame::hardware {
// Experimental Williams-era nibble framebuffer and programmable RGB palette.
// This is a rendering primitive, not yet a verified Williams video bus map.
class WilliamsVideo {
public:
 static constexpr unsigned Width=304,Height=256;
 void reset() noexcept {vram_.fill(0);palette_.fill(0xff000000);}
 void setPalette(unsigned index,std::uint8_t r,std::uint8_t g,std::uint8_t b) noexcept {
    if(index<16)palette_[index]=0xff000000u|(std::uint32_t(r)<<16)|(std::uint32_t(g)<<8)|b;
 }
 void write(unsigned address,std::uint8_t value) noexcept {
    if(address<vram_.size())vram_[address]=value;
 }
 std::uint8_t read(unsigned address) const noexcept{return address<vram_.size()?vram_[address]:0xff;}
 std::uint32_t pixel(unsigned x,unsigned y) const noexcept {
    if(x>=Width||y>=Height)return 0xff000000;
    const auto offset=y*(Width/2)+x/2;
    const auto value=vram_[offset];
    return palette_[(x&1)?(value&15):(value>>4)];
 }
private:
 std::array<std::uint8_t,Width*Height/2> vram_{};
 std::array<std::uint32_t,16> palette_{};
};
}
