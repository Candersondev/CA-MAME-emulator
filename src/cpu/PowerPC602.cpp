#include "ca_mame/cpu/PowerPC602.hpp"
namespace ca_mame {
void PowerPC602::reset(){registers_.fill(0);pc_=0;fault_=false;lr_=cr_=srr0_=srr1_=msr_=0;externalInterrupt_=false;}
std::uint32_t PowerPC602::step(){
    if(fault_)return 0;
    if(externalInterrupt_ && (msr_&MSR_EE)){
        srr0_=pc_;srr1_=msr_;msr_&=~MSR_EE;pc_=0x500;
        return 1; // Level-sensitive interrupt; board must acknowledge the source.
    }
    std::uint32_t instruction=0;
    if(!bus_.read32(pc_,instruction)){fault_=true;return 0;}
    const auto currentPC=pc_;
    pc_+=4;
    const unsigned op=instruction>>26;
    const unsigned rt=(instruction>>21)&31;
    const unsigned ra=(instruction>>16)&31;
    const auto immediate=static_cast<std::int16_t>(instruction&0xffff);
    switch(op) {
        case 19: { // rfi (privilege and full MSR validation pending)
            if(instruction!=0x4c000064u){fault_=true;break;}
            pc_=srr0_&~3u;
            msr_=srr1_;
            break;
        }
        case 18: { // b / bl: 24-bit signed displacement shifted left two
            std::int32_t displacement=static_cast<std::int32_t>(instruction&0x03fffffcu);
            if(displacement&0x02000000)displacement|=static_cast<std::int32_t>(0xfc000000u);
            if(instruction&1u)lr_=currentPC+4;
            pc_=(instruction&2u)?static_cast<std::uint32_t>(displacement):currentPC+static_cast<std::uint32_t>(displacement);
            break;
        }
        case 16: { // bc: BO and CR bit selection (CTR-dependent modes not implemented)
            const unsigned bo=(instruction>>21)&31;
            const unsigned bi=(instruction>>16)&31;
            if(!(bo&4u)){fault_=true;break;} // requires CTR handling
            const bool bit=((cr_>>(31-bi))&1u)!=0;
            if((bo&16u) || bit==((bo&8u)!=0)) {
                std::int32_t displacement=static_cast<std::int16_t>(instruction&0xfffcu);
                pc_=(instruction&2u)?static_cast<std::uint32_t>(displacement):currentPC+static_cast<std::uint32_t>(displacement);
            }
            if(instruction&1u)lr_=currentPC+4;
            break;
        }
        case 11: { // cmpi: signed compare into selected CR field
            const unsigned field=(instruction>>23)&7;
            if(instruction&0x00200000u){fault_=true;break;} // 64-bit comparison not supported
            const auto lhs=static_cast<std::int32_t>(registers_[ra]);
            const auto rhs=static_cast<std::int32_t>(immediate);
            const std::uint32_t flags=lhs<rhs?8u:(lhs>rhs?4u:2u);
            const unsigned shift=28-4*field;
            cr_=(cr_&~(0xfu<<shift))|(flags<<shift);
            break;
        }
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
