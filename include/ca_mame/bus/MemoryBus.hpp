#pragma once
#include <array>
#include <cstdint>
namespace ca_mame {
class MemoryBus {
public:
    std::uint8_t read(std::uint16_t address) const noexcept;
    void write(std::uint16_t address, std::uint8_t value) noexcept;
    void clear() noexcept;
    void clearRam() noexcept;
    // Space Invaders decodes only 14 address lines and has 8 KiB ROM.
    void configureSpaceInvadersMap() noexcept { invadersMap_ = true; }
    void loadRomByte(std::uint16_t address, std::uint8_t value) noexcept { memory_[address & 0x3fff] = value; }
private:
    std::array<std::uint8_t, 65536> memory_{};
    bool invadersMap_ = false;
};
}
