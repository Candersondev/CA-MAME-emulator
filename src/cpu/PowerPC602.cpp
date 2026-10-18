#include "ca_mame/cpu/PowerPC602.hpp"
namespace ca_mame {
void PowerPC602::reset(){registers_.fill(0);pc_=0;fault_=false;}
std::uint32_t PowerPC602::step(){
    if(fault_)return 0;
    std::uint32_t instruction=0;
    if(!bus_.read32(pc_,instruction)){fault_=true;return 0;}
    pc_+=4;
    const unsigned op=instruction>>26;
    const unsigned rt=(instruction>>21)&31;
    const unsigned ra=(instruction>>16)&31;
    const auto immediate=static_cast<std::int16_t>(instruction&0xffff);
    switch(op) {
        case 14: // addi RT,RA,SIMM (RA=0 means literal zero)
            registers_[rt]=(ra?registers_[ra]:0u)+static_cast<std::uint32_t>(static_cast<std::int32_t>(immediate));
            break;
        case 15: // addis RT,RA,SIMM
            registers_[rt]=(ra?registers_[ra]:0u)+(static_cast<std::uint32_t>(static_cast<std::int32_t>(immediate))<<16);
            break;
        case 24: // ori RA,RS,UIMM (canonical PPC nop is ori r0,r0,0)
            registers_[ra]=registers_[rt]|(instruction&0xffff);
            break;
        case 32: { // lwz RT,D(RA)
            const auto address=(ra?registers_[ra]:0u)+static_cast<std::uint32_t>(static_cast<std::int32_t>(immediate));
            if(!bus_.read32(address,registers_[rt]))fault_=true;
            break;
        }
        case 36: { // stw RS,D(RA)
            const auto address=(ra?registers_[ra]:0u)+static_cast<std::uint32_t>(static_cast<std::int32_t>(immediate));
            if(!bus_.write32(address,registers_[rt]))fault_=true;
            break;
        }
        default:fault_=true;break; // Explicit unsupported instruction, never silently NOP.
    }
    return fault_?0:1;
}
}
