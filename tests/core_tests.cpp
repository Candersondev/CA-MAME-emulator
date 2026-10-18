#include <cassert>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include "ca_mame/core/Machine.hpp"
class TestCpu final : public ca_mame::ICpu {
public:
    void reset() override { steps = 0; }
    std::uint32_t step() override { ++steps; return 4; }
    unsigned steps = 0;
};
int main() {
    ca_mame::Machine machine;
    machine.bus().write(0x1234, 0xA5);
    assert(machine.bus().read(0x1234) == 0xA5);
    machine.reset();
    assert(machine.bus().read(0x1234) == 0);
    bool threw = false;
    try { (void)machine.runSteps(1); }
    catch(const std::logic_error&) { threw = true; }
    assert(threw);
    auto cpu = std::make_unique<TestCpu>();
    auto* raw = cpu.get();
    ca_mame::Machine configured(std::move(cpu));
    assert(configured.runSteps(3) == 12);
    assert(raw->steps == 3);
    configured.reset();
    assert(raw->steps == 0);
    return 0;
}
