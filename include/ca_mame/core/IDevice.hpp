#pragma once
#include <string_view>
namespace ca_mame {
class IDevice {
public:
    virtual ~IDevice() = default;
    virtual std::string_view deviceId() const noexcept = 0;
    virtual void reset() = 0;
};
}
