#pragma once
#include <cstdint>
#include "ca_mame/cpu/ICpu.hpp"
#include "ca_mame/bus/MemoryBus.hpp"
namespace ca_mame {
class Intel8080 final : public ICpu {
public:
    explicit Intel8080(MemoryBus& bus) : bus_(bus) { reset(); }
    void reset() override;
    std::uint32_t step() override;
    void interrupt(std::uint8_t vector) noexcept;
    void setInterruptEnabled(bool enabled) noexcept { inte_ = enabled; }
    std::uint8_t a=0,b=0,c=0,d=0,e=0,h=0,l=0;
    std::uint16_t pc=0,sp=0;
    bool z=false,s=false,p=false,cy=false,ac=false;
    bool halted=false;
    bool interruptsEnabled() const noexcept { return inte_; }
private:
    MemoryBus& bus_;
    bool inte_=false, pending_=false;
    std::uint8_t pendingVector_=0;
    std::uint8_t read(std::uint16_t addr) const { return bus_.read(addr); }
    void write(std::uint16_t addr,std::uint8_t value) { bus_.write(addr,value); }
    std::uint8_t fetch() { auto value=read(pc); ++pc; return value; }
    std::uint16_t fetch16() { auto lo=fetch(); return std::uint16_t(lo | (std::uint16_t(fetch())<<8)); }
    std::uint16_t hl() const { return std::uint16_t((h<<8)|l); }
    void setHL(std::uint16_t v) { h=std::uint8_t(v>>8); l=std::uint8_t(v); }
    std::uint16_t pair(int index) const;
    void setPair(int index,std::uint16_t v);
    std::uint8_t reg(int index) const;
    void setReg(int index,std::uint8_t v);
    void flagsZSP(std::uint8_t v);
    void add(std::uint8_t v,bool carry);
    void sub(std::uint8_t v,bool borrow,bool compare=false);
    void logic(std::uint8_t v,int operation);
    void push(std::uint16_t v);
    std::uint16_t pop();
    bool condition(int index) const;
    std::uint8_t flags() const;
    void setFlags(std::uint8_t value);
};
}
