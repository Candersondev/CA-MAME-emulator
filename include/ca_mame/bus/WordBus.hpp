#pragma once
#include <cstdint>
namespace ca_mame {
class WordBus {
public:
    virtual ~WordBus()=default;
    virtual bool read8(std::uint32_t address,std::uint8_t& value) const noexcept {
        std::uint32_t word=0;
        if(!read32(address&~3u,word))return false;
        value=static_cast<std::uint8_t>(word>>(24-8*(address&3u)));
        return true;
    }
    virtual bool read16(std::uint32_t address,std::uint16_t& value) const noexcept {
        if(address&1u)return false;
        std::uint32_t word=0;
        if(!read32(address&~3u,word))return false;
        value=static_cast<std::uint16_t>(word>>(16-8*(address&2u)));
        return true;
    }
    virtual bool write8(std::uint32_t address,std::uint8_t value) noexcept {
        std::uint32_t word=0;
        const auto base=address&~3u;
        if(!read32(base,word))return false;
        const auto shift=24-8*(address&3u);
        return write32(base,(word&~(0xffu<<shift))|(std::uint32_t(value)<<shift));
    }
    virtual bool write16(std::uint32_t address,std::uint16_t value) noexcept {
        if(address&1u)return false;
        std::uint32_t word=0;
        const auto base=address&~3u;
        if(!read32(base,word))return false;
        const auto shift=16-8*(address&2u);
        return write32(base,(word&~(0xffffu<<shift))|(std::uint32_t(value)<<shift));
    }
    virtual bool read32(std::uint32_t address,std::uint32_t& value) const noexcept=0;
    virtual bool write32(std::uint32_t address,std::uint32_t value) noexcept=0;
};
}
