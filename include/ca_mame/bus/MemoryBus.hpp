#pragma once
#include <array>
#include <cstdint>
namespace ca_mame {
class MemoryBus {
public:
    std::uint8_t read(std::uint16_t address) const noexcept;
    void write(std::uint16_t address, std::uint8_t value) noexcept;
    void clear() noexcept;
private:
    std::array<std::uint8_t, 65536> memory_{};
};
}
