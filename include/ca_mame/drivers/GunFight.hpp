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
    GunFight();
    std::string_view driverId() const noexcept override {return "gunfight";}
    void reset() override;
    std::uint64_t runFrame() override;
    bool loadRom(std::uint16_t address,std::span<const std::uint8_t> bytes);
    void setInput(std::uint8_t port,std::uint8_t value) noexcept;
    std::uint8_t readPort(std::uint8_t port) const {return io_.read(port);}
    void writePort(std::uint8_t port,std::uint8_t value) {io_.write(port,value);}
    std::uint8_t readMemory(std::uint16_t address) const {return memory_.read(address);}
    Intel8080& cpu() noexcept {return cpu_;}
private:
    MemoryBus memory_;
    IoBus io_;
    Intel8080 cpu_;
    std::array<std::uint8_t,3> inputs_{};
    std::uint16_t shift_=0;
    std::uint8_t shiftCount_=0;
    std::uint64_t cycles_=0;
    std::uint64_t frames_=0;
};
}
