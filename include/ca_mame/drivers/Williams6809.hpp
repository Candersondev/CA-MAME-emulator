#pragma once
#include <array>
#include <cstdint>
#include <span>
#include "ca_mame/core/IMachine.hpp"
#include "ca_mame/cpu/Motorola6809.hpp"
namespace ca_mame::drivers {
// Williams 6809 platform foundation (Defender/Robotron lineage).
// Preliminary flat address space: bank switching, PIA, blitter, sound CPU,
// palette and real raster timing are not yet modeled.
class Williams6809 final:public IMachine {
public:
    static constexpr std::size_t Width=304,Height=256;
    Williams6809();
    std::string_view driverId() const noexcept override{return "williams6809";}
    void reset() override;
    std::uint64_t runFrame() override;
    bool loadProgram(std::uint16_t address,std::span<const std::uint8_t> bytes);
    std::uint8_t read(std::uint16_t address) const noexcept{return memory_[address];}
    void write(std::uint16_t address,std::uint8_t value) noexcept{memory_[address]=value;}
    Motorola6809& cpu() noexcept{return cpu_;}
    std::uint64_t elapsedCycles() const noexcept{return cycles_;}
private:
    std::array<std::uint8_t,65536> memory_{};
    Motorola6809 cpu_;
    std::uint64_t cycles_=0,frames_=0;
};
}
