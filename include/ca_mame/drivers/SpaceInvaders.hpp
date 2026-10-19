#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include "ca_mame/core/IMachine.hpp"
#include "ca_mame/bus/MemoryBus.hpp"
#include "ca_mame/bus/IoBus.hpp"
#include "ca_mame/cpu/Intel8080.hpp"
namespace ca_mame::drivers {
// Space Invaders board model. Sound events are latched; audio synthesis and watchdog remain TODO.
class SpaceInvaders final : public IMachine {
public:
    static constexpr std::size_t Width=224, Height=256;
    static constexpr std::uint32_t MasterHz=19968000;
    static constexpr std::uint32_t CpuHz=2000000; // Legacy scheduler until raster timing migration
    static constexpr std::uint32_t HardwareCpuHz=MasterHz/10;
    static constexpr std::uint32_t PixelHz=MasterHz/4;
    static constexpr std::uint32_t HTotal=0x140;
    static constexpr std::uint32_t VTotal=0x106;
    static constexpr std::uint32_t IntTrigger1=0x080;
    static constexpr std::uint32_t IntTrigger2=0x0da;
    static constexpr std::uint64_t FrameCycleNumerator=std::uint64_t(HardwareCpuHz)*HTotal*VTotal;
    static constexpr std::uint32_t FrameCycleDenominator=PixelHz;
    SpaceInvaders();
    std::string_view driverId() const noexcept override { return "invaders"; }
    void reset() override;
    std::uint64_t runFrame() override { return runHalfFrame()+runHalfFrame(); }
    void setInput(std::uint8_t port,std::uint8_t value);
    bool loadRom(std::uint16_t address,std::span<const std::uint8_t> bytes);
    std::uint8_t readMemory(std::uint16_t address) const { return memory_.read(address); }
    std::uint8_t readPort(std::uint8_t port) const { return io_.read(port); }
    std::uint8_t soundPort3() const noexcept { return sound3_; }
    std::uint8_t soundPort5() const noexcept { return sound5_; }
    std::uint8_t soundRisingEdges3() noexcept { const auto edges=sound3Edges_; sound3Edges_=0; return edges; }
    std::uint8_t soundRisingEdges5() noexcept { const auto edges=sound5Edges_; sound5Edges_=0; return edges; }
    void writePort(std::uint8_t port,std::uint8_t value) { io_.write(port,value); }
    std::uint64_t runHalfFrame();
    std::uint64_t elapsedCycles() const noexcept { return cycles_; }
    unsigned elapsedHalfFrames() const noexcept { return halfFrames_; }
    std::array<std::uint32_t,Width*Height> frameBuffer() const;
    Intel8080& cpu() noexcept { return cpu_; }
private:
    MemoryBus memory_;
    IoBus io_;
    Intel8080 cpu_;
    std::array<std::uint8_t,4> inputs_{};
    std::uint16_t shift_=0;
    std::uint8_t shiftOffset_=0;
    std::uint8_t sound3_=0,sound5_=0,sound3Edges_=0,sound5Edges_=0;
    std::uint64_t cycles_=0;
    unsigned halfFrames_=0;
};
}
