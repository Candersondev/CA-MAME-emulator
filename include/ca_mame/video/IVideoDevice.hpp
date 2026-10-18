#pragma once
#include <cstdint>
#include <span>
namespace ca_mame {
class IVideoDevice {
public:
    virtual ~IVideoDevice() = default;
    virtual void reset() = 0;
    virtual void advance(std::uint32_t cycles) = 0;
    virtual std::span<const std::uint32_t> pixels() const = 0;
};
}
