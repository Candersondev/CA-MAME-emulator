#pragma once
#include <cstdint>
#include "ca_mame/hardware/WilliamsVideo.hpp"
namespace ca_mame::hardware {
// Experimental nibble-aware blitter operating on WilliamsVideo VRAM.
// Control bits: 0x01 transparent zero nibbles, 0x02 XOR, 0x04 solid source.
// This is a testable primitive, not a cycle-accurate Williams DMA controller.
class WilliamsBlitter {
public:
    explicit WilliamsBlitter(WilliamsVideo& video):video_(video){}
    void setSource(std::uint16_t address) noexcept {source_=address;}
    void setDestination(std::uint16_t address) noexcept {destination_=address;}
    void setDimensions(std::uint8_t width,std::uint8_t height) noexcept {
        width_=width;height_=height;
    }
    void setControl(std::uint8_t value) noexcept {control_=value;}
    void setSolid(std::uint8_t value) noexcept {solid_=value;}
    template<class Read>
    std::uint32_t execute(Read read) noexcept {
        std::uint32_t count=0;
        for(unsigned row=0;row<height_;++row)
            for(unsigned col=0;col<width_;++col){
                const auto src=std::uint16_t(source_+row*width_+col);
                const auto dst=std::uint16_t(destination_+row*width_+col);
                const std::uint8_t value=(control_&4)?solid_:read(src);
                const auto previous=video_.read(dst);
                std::uint8_t result=previous;
                for(unsigned shift:{0u,4u}){
                    const auto nibble=std::uint8_t((value>>shift)&15);
                    if((control_&1)&&nibble==0)continue;
                    const auto prior=std::uint8_t((previous>>shift)&15);
                    const auto updated=std::uint8_t((control_&2)?(prior^nibble):nibble);
                    result=std::uint8_t((result&~(15u<<shift))|(updated<<shift));
                }
                video_.write(dst,result);
                ++count;
            }
        return count;
    }
private:
    WilliamsVideo& video_;
    std::uint16_t source_=0,destination_=0;
    std::uint8_t width_=0,height_=0,control_=0,solid_=0;
};
}
