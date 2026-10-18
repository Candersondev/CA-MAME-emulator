#pragma once
#include <cstdint>
#include <string_view>
namespace ca_mame {
class IMachine {
public:
    virtual ~IMachine() = default;
    virtual std::string_view driverId() const noexcept = 0;
    virtual void reset() = 0;
    virtual std::uint64_t runFrame() = 0;
};
}
