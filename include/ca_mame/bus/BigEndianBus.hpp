#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
namespace ca_mame {
class BigEndianBus {
public:
    explicit BigEndianBus(std::size_t size):memory_(size,0){}
    std::size_t size() const noexcept{return memory_.size();}
    bool read32(std::uint32_t address,std::uint32_t& value) const noexcept {
        if(address%4 || address>=memory_.size() || memory_.size()-address<4)return false;
        value=(std::uint32_t(memory_[address])<<24)|(std::uint32_t(memory_[address+1])<<16)|
              (std::uint32_t(memory_[address+2])<<8)|memory_[address+3];
        return true;
    }
    bool write32(std::uint32_t address,std::uint32_t value) noexcept {
        if(address%4 || address>=memory_.size() || memory_.size()-address<4)return false;
        memory_[address]=std::uint8_t(value>>24);memory_[address+1]=std::uint8_t(value>>16);
        memory_[address+2]=std::uint8_t(value>>8);memory_[address+3]=std::uint8_t(value);
        return true;
    }
private:
    std::vector<std::uint8_t> memory_;
};
}
