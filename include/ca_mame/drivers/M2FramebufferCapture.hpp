#pragma once
#include <cstdint>
#include <vector>
#include "ca_mame/bus/WordBus.hpp"
namespace ca_mame::drivers {
// Diagnostic RAM scanout, NOT an emulation of the Konami M2 VDU.
// Converts a caller-selected big-endian XRGB8888 memory region to RGBA8888.
class M2FramebufferCapture {
public:
    static bool capture(const ca_mame::WordBus& bus,std::uint32_t address,
                        std::uint32_t width,std::uint32_t height,
                        std::uint32_t strideBytes,std::vector<std::uint8_t>& rgba) {
        rgba.clear();
        if(!width||!height||width>2048||height>2048||
           strideBytes<width*4u||strideBytes%4u||
           std::uint64_t(strideBytes)*(height-1u)+std::uint64_t(width)*4u>0x1000000u)
            return false;
        const auto last=std::uint64_t(address)+std::uint64_t(strideBytes)*(height-1u)+std::uint64_t(width)*4u;
        if(last>0x100000000ull||address%4u)return false;
        std::vector<std::uint8_t> out(std::size_t(width)*height*4u);
        for(std::uint32_t y=0;y<height;++y)
            for(std::uint32_t x=0;x<width;++x){
                std::uint32_t pixel=0;
                if(!bus.read32(address+y*strideBytes+x*4u,pixel))return false;
                const auto index=(std::size_t(y)*width+x)*4u;
                out[index]=std::uint8_t(pixel>>16);
                out[index+1]=std::uint8_t(pixel>>8);
                out[index+2]=std::uint8_t(pixel);
                out[index+3]=255;
            }
        rgba.swap(out);
        return true;
    }
};
}
