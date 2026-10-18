#include "ca_mame/core/Machine.hpp"
#include <stdexcept>
#include <utility>
namespace ca_mame {
Machine::Machine(std::unique_ptr<ICpu> cpu) : cpu_(std::move(cpu)) {}
void Machine::reset() {
    bus_.clear();
    if(cpu_) cpu_->reset();
}
std::uint64_t Machine::runSteps(std::uint32_t instructions) {
    if(!cpu_) throw std::logic_error("No CPU installed");
    std::uint64_t cycles = 0;
    for(std::uint32_t i = 0; i < instructions; ++i) cycles += cpu_->step();
    return cycles;
}
}
