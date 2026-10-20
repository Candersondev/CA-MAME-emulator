#pragma once
#include <array>
#include <cstdint>
#include <span>
#include "ca_mame/core/IMachine.hpp"
#include "ca_mame/cpu/Motorola6809.hpp"
#include "ca_mame/hardware/PIA6821.hpp"
#include "ca_mame/hardware/WilliamsVideo.hpp"
#include "ca_mame/hardware/WilliamsBlitter.hpp"
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
    hardware::PIA6821& pia(unsigned index) noexcept{return pias_[index&1];}
    hardware::WilliamsVideo& video() noexcept{return video_;}
    hardware::WilliamsBlitter& blitter() noexcept{return blitter_;}
    void setRomBank(std::uint8_t bank) noexcept {romBank_=bank&3;}
    std::uint8_t romBank() const noexcept{return romBank_;}
    bool loadBank(std::uint8_t bank,std::span<const std::uint8_t> bytes);
    std::uint8_t bankedRead(std::uint16_t address) const noexcept;
private:
    std::array<std::uint8_t,65536> memory_{};
    Motorola6809 cpu_;
    std::array<hardware::PIA6821,2> pias_{};
    hardware::WilliamsVideo video_{};
    hardware::WilliamsBlitter blitter_{video_};
    std::array<std::array<std::uint8_t,0x4000>,4> banks_{};
    std::uint8_t romBank_=0;
    std::uint64_t cycles_=0,frames_=0;
};
}
