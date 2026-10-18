#pragma once
#include <cstdint>
#include "ca_mame/bus/WordBus.hpp"
#include "ca_mame/drivers/M2MemoryMap.hpp"

namespace ca_mame::drivers {
// Per-processor address-space view. All ordinary accesses share the same
// backing memory and devices; the BDA CPU ID register is processor-local.
class M2CpuBus final : public WordBus {
public:
    M2CpuBus(M2MemoryMap& memory, unsigned cpuIndex) noexcept
        : memory_(memory), cpuIndex_(cpuIndex) {}
    bool read32(std::uint32_t address, std::uint32_t& value) const noexcept override {
        if(address==M2MemoryMap::cpuIdBase){
            value=cpuIndex_==1 ? 0x80000000u : 0u;
            return true;
        }
        return memory_.read32(address,value);
    }
    bool write32(std::uint32_t address, std::uint32_t value) noexcept override {
        // MAME currently logs CPU ID writes without modifying identification.
        if(address==M2MemoryMap::cpuIdBase)return true;
        return memory_.write32(address,value);
    }
private:
    M2MemoryMap& memory_;
    unsigned cpuIndex_;
};
}
