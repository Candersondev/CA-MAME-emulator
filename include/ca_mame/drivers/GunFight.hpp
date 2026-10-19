#pragma once
#include <array>
#include <cstdint>
#include <span>
#include "ca_mame/core/IMachine.hpp"
#include "ca_mame/bus/MemoryBus.hpp"
#include "ca_mame/bus/IoBus.hpp"
#include "ca_mame/cpu/Intel8080.hpp"

namespace ca_mame::drivers {
// Midway Gun Fight (1975): initial independent board model.
// Shares the Intel 8080 core but has its own input/shift-register I/O.
class GunFight final : public IMachine {
public:
    static constexpr std::size_t Width=224, Height=256;
    static constexpr std::uint32_t MasterHz=19968000, CpuHz=MasterHz/10;
    static constexpr std::uint32_t PixelHz=MasterHz/4, HTotal=0x140, VTotal=0x106;
    static constexpr std::uint32_t IntTrigger1=0x080, IntTrigger2=0x0da;
    GunFight();
    std::string_view driverId() const noexcept override {return "gunfight";}
    void reset() override;
    std::uint64_t runFrame() override;
    std::uint64_t runHalfFrame();
    std::uint64_t elapsedCycles() const noexcept {return cycles_;}
    std::uint64_t elapsedHalfFrames() const noexcept {return halfFrames_;}
    std::array<std::uint32_t,Width*Height> frameBuffer() const;
    bool loadRom(std::uint16_t address,std::span<const std::uint8_t> bytes);
    void setInput(std::uint8_t port,std::uint8_t value) noexcept;
    std::uint8_t readPort(std::uint8_t port) const {return io_.read(port);}
    void writePort(std::uint8_t port,std::uint8_t value) {io_.write(port,value);}
    std::uint8_t readMemory(std::uint16_t address) const {return memory_.read(address);}
    Intel8080& cpu() noexcept {return cpu_;}
    std::uint8_t soundPort() const noexcept {return soundPort_;}
    std::uint8_t takeSoundRisingEdges() noexcept {const auto result=soundEdges_;soundEdges_=0;return result;}
private:
    MemoryBus memory_;
    IoBus io_;
    Intel8080 cpu_;
    std::array<std::uint8_t,3> inputs_{};
    std::uint16_t shift_=0;
    std::uint8_t shiftCount_=0;
    std::uint8_t soundPort_=0,soundEdges_=0;
    std::uint64_t cycles_=0;
    std::uint64_t halfFrames_=0;
};
}
