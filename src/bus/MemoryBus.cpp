#include "ca_mame/bus/MemoryBus.hpp"
namespace ca_mame {
std::uint8_t MemoryBus::read(std::uint16_t address) const noexcept { return memory_[invadersMap_ ? (address & 0x3fff) : address]; }
void MemoryBus::write(std::uint16_t address, std::uint8_t value) noexcept { if (invadersMap_) {
    address &= 0x3fff;
    if (address < 0x2000) return;
}
memory_[address] = value; }
void MemoryBus::clear() noexcept { memory_.fill(0); }
}
