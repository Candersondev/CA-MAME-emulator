#pragma once
#include <cstdint>
#include "ca_mame/bus/BigEndianBus.hpp"
#include "ca_mame/cpu/PowerPC602.hpp"
namespace ca_mame::drivers {
// Experimental shared-RAM dual-processor harness; NOT a Konami M2 board implementation.
class KonamiM2 {
public:
    KonamiM2():ram_(1024*1024),cpu0_(ram_),cpu1_(ram_){}
    void reset(){cpu0_.reset();cpu1_.reset();cycles0_=cycles1_=0;}
    bool loadWord(std::uint32_t address,std::uint32_t value){return ram_.write32(address,value);}
    bool readWord(std::uint32_t address,std::uint32_t& value)const{return ram_.read32(address,value);}
    void stepPair(){cycles0_+=cpu0_.step();cycles1_+=cpu1_.step();}
    PowerPC602& cpu(unsigned index){return index==0?cpu0_:cpu1_;}
    std::uint64_t cycles(unsigned index)const{return index==0?cycles0_:cycles1_;}
private:
    BigEndianBus ram_;
    PowerPC602 cpu0_,cpu1_;
    std::uint64_t cycles0_=0,cycles1_=0;
};
}
