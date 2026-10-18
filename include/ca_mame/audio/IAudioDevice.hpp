#pragma once
#include <cstdint>
namespace ca_mame {
class IAudioDevice {
public:
    virtual ~IAudioDevice() = default;
    virtual void reset() = 0;
    virtual void advance(std::uint32_t cycles) = 0;
};
}
