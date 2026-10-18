#pragma once
#include <cstdint>
#include <memory>
#include "ca_mame/bus/MemoryBus.hpp"
#include "ca_mame/cpu/ICpu.hpp"
namespace ca_mame {
class Machine {
public:
    explicit Machine(std::unique_ptr<ICpu> cpu = nullptr);
    void reset();
    std::uint64_t runSteps(std::uint32_t instructions);
    MemoryBus& bus() noexcept { return bus_; }
    const MemoryBus& bus() const noexcept { return bus_; }
private:
    MemoryBus bus_;
    std::unique_ptr<ICpu> cpu_;
};
}
