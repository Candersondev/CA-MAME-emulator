#pragma once
#include <cstdint>
namespace ca_mame {
class WordBus {
public:
    virtual ~WordBus()=default;
    virtual bool read32(std::uint32_t address,std::uint32_t& value) const noexcept=0;
    virtual bool write32(std::uint32_t address,std::uint32_t value) noexcept=0;
};
}
