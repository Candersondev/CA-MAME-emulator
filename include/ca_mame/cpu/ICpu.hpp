#pragma once
#include <cstdint>
namespace ca_mame {
class ICpu {
public:
    virtual ~ICpu() = default;
    virtual void reset() = 0;
    virtual std::uint32_t step() = 0; // elapsed clock cycles
};
}
