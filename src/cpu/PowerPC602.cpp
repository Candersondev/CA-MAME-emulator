#include "ca_mame/cpu/PowerPC602.hpp"
namespace ca_mame {
void PowerPC602::reset(){registers_.fill(0);pc_=0;fault_=false;lr_=cr_=ctr_=0;stopReason_=StopReason::None;faultAddress_=faultInstruction_=0;srr0_=srr1_=msr_=0;externalInterrupt_=false;}
std::uint32_t PowerPC602::step(){
    if(fault_)return 0;
    if(externalInterrupt_ && (msr_&MSR_EE)){
        srr0_=pc_;srr1_=msr_;msr_&=~MSR_EE;pc_=0x500;
        return 1; // Level-sensitive interrupt; board must acknowledge the source.
    }
    std::uint32_t instruction=0;
    if(!bus_.read32(pc_,instruction)){fault_=true;stopReason_=StopReason::InstructionFetch;faultAddress_=pc_;return 0;}
    const auto currentPC=pc_;
    pc_+=4;
    const unsigned op=instruction>>26;
    const unsigned rt=(instruction>>21)&31;
    const unsigned ra=(instruction>>16)&31;
    const auto immediate=static_cast<std::int16_t>(instruction&0xffff);
    auto unsupported=[&](){fault_=true;stopReason_=StopReason::UnsupportedInstruction;faultAddress_=currentPC;faultInstruction_=instruction;};
    auto dataFault=[&](std::uint32_t address){fault_=true;stopReason_=StopReason::DataAccess;faultAddress_=address;faultInstruction_=instruction;};
    switch(op) {
        case 31: {
            const unsigned xo=(instruction>>1)&1023u;
            const unsigned spr=((instruction>>16)&31u)|(((instruction>>11)&31u)<<5);
            switch(xo){
                case 83: registers_[rt]=msr_;break; // mfmsr
                case 146: msr_=registers_[rt];break; // mtmsr
                case 339: // mfspr
                    switch(spr){
                        case 8:registers_[rt]=lr_;break;
                        case 9:registers_[rt]=ctr_;break;
                        case 26:registers_[rt]=srr0_;break;
                        case 27:registers_[rt]=srr1_;break;
                        default:unsupported();break;
                    }
                    break;
                case 467: // mtspr
                    switch(spr){
                        case 8:lr_=registers_[rt];break;
                        case 9:ctr_=registers_[rt];break;
                        case 26:srr0_=registers_[rt];break;
                        case 27:srr1_=registers_[rt];break;
                        default:unsupported();break;
                    }
                    break;
                case 266:registers_[rt]=registers_[ra]+registers_[(instruction>>11)&31];break; // add, no OE/record support
                default:unsupported();break;
            }
            break;
        }
        case 12: // addic RT,RA,SIMM (carry flag not yet modeled)
            registers_[rt]=registers_[ra]+static_cast<std::uint32_t>(static_cast<std::int32_t>(immediate));
            break;
        case 13: // addic. (CR0 updates not yet modeled)
            registers_[rt]=registers_[ra]+static_cast<std::uint32_t>(static_cast<std::int32_t>(immediate));
            break;
        case 10: { // cmpli (32-bit unsigned)
            if(instruction&0x00200000u){unsupported();break;}
            const unsigned field=(instruction>>23)&7;
            const auto lhs=registers_[ra],rhs=instruction&0xffffu;
            const std::uint32_t flags=lhs<rhs?8u:(lhs>rhs?4u:2u);
            const unsigned shift=28-4*field;
            cr_=(cr_&~(0xfu<<shift))|(flags<<shift);
            break;
        }
        case 28: // andi. RS,RA,UIMM (CR0 pending)
            registers_[ra]=registers_[rt]&(instruction&0xffffu);
            break;
        case 26: // xori
            registers_[ra]=registers_[rt]^(instruction&0xffffu);
            break;
        case 25: // oris
            registers_[ra]=registers_[rt]|((instruction&0xffffu)<<16);
            break;
        case 19: { // rfi (privilege and full MSR validation pending)
            if(instruction==0x4e800020u){pc_=lr_&~3u;break;} // blr
            if(instruction!=0x4c000064u){unsupported();break;}
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
            bool ctrOk=true;
            if(!(bo&4u)){--ctr_;ctrOk=((ctr_!=0)==((bo&2u)==0));}
            const bool bit=((cr_>>(31-bi))&1u)!=0;
            if(ctrOk && ((bo&16u) || bit==((bo&8u)!=0))) {
                std::int32_t displacement=static_cast<std::int16_t>(instruction&0xfffcu);
                pc_=(instruction&2u)?static_cast<std::uint32_t>(displacement):currentPC+static_cast<std::uint32_t>(displacement);
            }
            if(instruction&1u)lr_=currentPC+4;
            break;
        }
        case 11: { // cmpi: signed compare into selected CR field
            const unsigned field=(instruction>>23)&7;
            if(instruction&0x00200000u){unsupported();break;} // 64-bit comparison not supported
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
        case 34: { // lbz
            const auto address=(ra?registers_[ra]:0u)+static_cast<std::uint32_t>(static_cast<std::int32_t>(immediate));
            std::uint8_t value=0;
            if(!bus_.read8(address,value))dataFault(address);else registers_[rt]=value;
            break;
        }
        case 40: { // lhz
            const auto address=(ra?registers_[ra]:0u)+static_cast<std::uint32_t>(static_cast<std::int32_t>(immediate));
            std::uint16_t value=0;
            if(!bus_.read16(address,value))dataFault(address);else registers_[rt]=value;
            break;
        }
        case 42: { // lha
            const auto address=(ra?registers_[ra]:0u)+static_cast<std::uint32_t>(static_cast<std::int32_t>(immediate));
            std::uint16_t value=0;
            if(!bus_.read16(address,value))dataFault(address);else registers_[rt]=static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(value)));
            break;
        }
        case 38: { // stb
            const auto address=(ra?registers_[ra]:0u)+static_cast<std::uint32_t>(static_cast<std::int32_t>(immediate));
            if(!bus_.write8(address,static_cast<std::uint8_t>(registers_[rt])))dataFault(address);
            break;
        }
        case 44: { // sth
            const auto address=(ra?registers_[ra]:0u)+static_cast<std::uint32_t>(static_cast<std::int32_t>(immediate));
            if(!bus_.write16(address,static_cast<std::uint16_t>(registers_[rt])))dataFault(address);
            break;
        }
        case 33: { // lwzu: base register updated only on successful read
            if(ra==0 || ra==rt){unsupported();break;}
            const auto address=registers_[ra]+static_cast<std::uint32_t>(static_cast<std::int32_t>(immediate));
            std::uint32_t value=0;
            if(!bus_.read32(address,value)){dataFault(address);break;}
            registers_[rt]=value;registers_[ra]=address;
            break;
        }
        case 37: { // stwu
            if(ra==0){unsupported();break;}
            const auto address=registers_[ra]+static_cast<std::uint32_t>(static_cast<std::int32_t>(immediate));
            if(!bus_.write32(address,registers_[rt])){dataFault(address);break;}
            registers_[ra]=address;
            break;
        }
        case 32: { // lwz RT,D(RA)
            const auto address=(ra?registers_[ra]:0u)+static_cast<std::uint32_t>(static_cast<std::int32_t>(immediate));
            if(!bus_.read32(address,registers_[rt]))dataFault(address);
            break;
        }
        case 36: { // stw RS,D(RA)
            const auto address=(ra?registers_[ra]:0u)+static_cast<std::uint32_t>(static_cast<std::int32_t>(immediate));
            if(!bus_.write32(address,registers_[rt]))dataFault(address);
            break;
        }
        default:unsupported();break; // Explicit unsupported instruction, never silently NOP.
    }
    return fault_?0:1;
}
}
