#pragma once
#include <array>
#include <cstdint>
#include <span>
#include "ca_mame/core/IMachine.hpp"
#include "ca_mame/cpu/Motorola6809.hpp"
#include "ca_mame/hardware/PIA6821.hpp"
#include "ca_mame/hardware/WilliamsVideo.hpp"
#include "ca_mame/hardware/WilliamsBlitter.hpp"
#include "ca_mame/hardware/WilliamsSoundLink.hpp"
namespace ca_mame::drivers {
// Williams 6809 platform foundation (Defender/Robotron lineage).
// Experimental hardware bus with ROM banks, PIA, sound command, framebuffer
// and DMA. Board-specific decoding and cycle-exact timing remain incomplete.
class Williams6809 final:public IMachine {
public:
    static constexpr std::size_t Width=304,Height=256;
    enum class BoardProfile {Experimental,Defender,Robotron};
    Williams6809();
    void setBoardProfile(BoardProfile profile) noexcept {profile_=profile;hardwareBusEnabled_=true;}
    BoardProfile boardProfile() const noexcept{return profile_;}
    std::uint8_t soundCommand() const noexcept{return soundLink_.command();}
    hardware::WilliamsSoundLink& soundLink() noexcept{return soundLink_;}
    std::uint64_t rasterInterrupts() const noexcept{return rasterInterrupts_;}
    std::uint64_t blitterBytes() const noexcept{return blitter_.totalBytes();}
    std::uint32_t lastBlitBytes() const noexcept{return blitter_.lastBytes();}
    std::uint64_t rejectedDmaWrites() const noexcept{return rejectedDmaWrites_;}
    std::uint8_t blitterRegister(unsigned index) const noexcept{return blitterRegisters_[index&7];}
    std::uint8_t specialChipRegister(unsigned index) const noexcept{return specialChipRegisters_[index&7];}
    std::uint64_t specialChipStarts() const noexcept{return specialChipStarts_;}
    std::uint16_t scanline() const noexcept{return scanline_;}
    void advanceRaster(std::uint32_t cpuCycles);
    std::string_view driverId() const noexcept override{return "williams6809";}
    void reset() override;
    std::uint64_t runFrame() override;
    bool loadProgram(std::uint16_t address,std::span<const std::uint8_t> bytes);
    std::uint8_t read(std::uint16_t address) const noexcept{return memory_[address];}
    void write(std::uint16_t address,std::uint8_t value) noexcept{memory_[address]=value;}
    Motorola6809& cpu() noexcept{return cpu_;}
    std::uint64_t elapsedCycles() const noexcept{return cycles_;}
    struct BootDiagnostic {std::uint16_t pc;std::uint64_t cycles;std::uint64_t unsupported;bool halted;};
    BootDiagnostic runBootDiagnostic(std::uint64_t cycleBudget);
    hardware::PIA6821& pia(unsigned index) noexcept{return pias_[index&1];}
    hardware::WilliamsVideo& video() noexcept{return video_;}
    hardware::WilliamsBlitter& blitter() noexcept{return blitter_;}
    void setRomBank(std::uint8_t bank) noexcept {romBank_=bank&3;}
    std::uint8_t romBank() const noexcept{return romBank_;}
    bool loadBank(std::uint8_t bank,std::span<const std::uint8_t> bytes);
    std::uint8_t bankedRead(std::uint16_t address) const noexcept;
    void enableHardwareBus(bool enabled=true) noexcept {hardwareBusEnabled_=enabled;}
    bool hardwareBusEnabled() const noexcept{return hardwareBusEnabled_;}
    std::uint8_t busRead(std::uint16_t address);
    void busWrite(std::uint16_t address,std::uint8_t value);
private:
    std::array<std::uint8_t,65536> memory_{};
    Motorola6809 cpu_;
    std::array<hardware::PIA6821,2> pias_{};
    hardware::WilliamsVideo video_{};
    hardware::WilliamsBlitter blitter_{video_};
    std::array<std::array<std::uint8_t,0x4000>,4> banks_{};
    std::uint8_t romBank_=0;
    BoardProfile profile_=BoardProfile::Experimental;
    hardware::WilliamsSoundLink soundLink_{};
    std::uint64_t rasterInterrupts_=0;
    std::uint16_t scanline_=0;
    std::uint32_t scanlineCycleRemainder_=0;
    bool hardwareBusEnabled_=false;
    std::array<std::uint8_t,8> blitterRegisters_{};
    std::uint64_t cycles_=0,frames_=0;
    std::uint64_t rejectedDmaWrites_=0;
    std::array<std::uint8_t,8> specialChipRegisters_{};
    std::uint64_t specialChipStarts_=0;
};
}
