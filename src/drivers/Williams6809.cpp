#include "ca_mame/drivers/Williams6809.hpp"
#include <algorithm>
namespace ca_mame::drivers {
Williams6809::Williams6809():cpu_(memory_){
    cpu_.attachBus([this](std::uint16_t address){return busRead(address);},
                   [this](std::uint16_t address,std::uint8_t value){busWrite(address,value);});
    reset();
}
void Williams6809::reset(){
    // Preserve loaded program images across board resets.
    cpu_.reset();cycles_=frames_=0;romBank_=0;rejectedDmaWrites_=0;specialChipRegisters_.fill(0);specialChipStarts_=0;specialChipBytes_=specialChipEstimatedCycles_=0;
    for(auto& pia:pias_)pia.reset();
    video_.reset();blitter_.resetStatistics();blitterRegisters_.fill(0);soundLink_.reset();rasterInterrupts_=0;scanline_=0;scanlineCycleRemainder_=0;
}
bool Williams6809::loadProgram(std::uint16_t address,std::span<const std::uint8_t> bytes){
    if(bytes.size()>memory_.size()-address)return false;
    std::copy(bytes.begin(),bytes.end(),memory_.begin()+address);
    return true;
}
bool Williams6809::loadBank(std::uint8_t bank,std::span<const std::uint8_t> bytes){
    if(bank>=banks_.size()||bytes.size()>banks_[bank].size())return false;
    std::copy(bytes.begin(),bytes.end(),banks_[bank].begin());
    return true;
}
std::uint8_t Williams6809::bankedRead(std::uint16_t address) const noexcept{
    // Diagnostic bank read; CPU bus interception uses the same bank window.
    if(address<0x4000)return banks_[romBank_][address];
    return memory_[address];
}
std::uint8_t Williams6809::busRead(std::uint16_t address){
    if(!hardwareBusEnabled_)return memory_[address];
    // Board profiles remain preliminary; actual MAME memory-map validation pending.
    if(profile_!=BoardProfile::Experimental){
        if(address>=0xc804 && address<=0xc807)return pias_[0].read(std::uint8_t(address&3));
        if(address>=0xc80c && address<=0xc80f)return pias_[1].read(std::uint8_t(address&3));
        if(profile_==BoardProfile::Robotron && address>=0xca00 && address<=0xca07)return specialChipRegisters_[address-0xca00];
        if(address==0xc900)return romBank_;
        if(address==0xc980)return soundLink_.command();
    }
    if(address<0x4000)return banks_[romBank_][address];
    if(address>=0x4000 && address<0xd000){
        const auto offset=unsigned(address-0x4000);
        if(offset<Width*Height/2)return video_.read(offset);
        return memory_[address];
    }
    if(address>=0xd000 && address<=0xd003)return pias_[0].read(std::uint8_t(address&3));
    if(address>=0xd004 && address<=0xd007)return pias_[1].read(std::uint8_t(address&3));
    if(address>=0xd010 && address<=0xd017)return blitterRegisters_[address-0xd010];
    if(address==0xd020)return romBank_;
    return memory_[address];
}
void Williams6809::busWrite(std::uint16_t address,std::uint8_t value){
    if(!hardwareBusEnabled_){memory_[address]=value;return;}
    if(profile_!=BoardProfile::Experimental){
        if(address>=0xc804 && address<=0xc807){pias_[0].write(std::uint8_t(address&3),value);return;}
        if(address>=0xc80c && address<=0xc80f){pias_[1].write(std::uint8_t(address&3),value);return;}
        if(profile_==BoardProfile::Robotron && address>=0xca00 && address<=0xca07){
            const auto index=unsigned(address-0xca00);
            specialChipRegisters_[index]=value;
            if(index==0){
                ++specialChipStarts_;
                const auto control=specialChipRegisters_[0];
                const auto solid=specialChipRegisters_[1];
                const auto source=std::uint16_t((unsigned(specialChipRegisters_[2])<<8)|specialChipRegisters_[3]);
                const auto destination=std::uint16_t((unsigned(specialChipRegisters_[4])<<8)|specialChipRegisters_[5]);
                const auto width=unsigned(specialChipRegisters_[6]);
                const auto height=unsigned(specialChipRegisters_[7]);
                // Serial step 1; screen format step 256. Size decoding is provisional.
                const auto srcStep=(control&1)?256u:1u;
                const auto dstStep=(control&2)?256u:1u;
                for(unsigned row=0;row<height;++row){
                    auto src=std::uint16_t(source+row*width*srcStep);
                    auto dst=std::uint16_t(destination+row*width*dstStep);
                    std::uint8_t shiftCarry=0;
                    for(unsigned col=0;col<width;++col){
                        const auto raw=busRead(src);
                        auto value=std::uint8_t((control&0x10)?solid:raw);
                        if(control&0x20){
                            const auto shifted=std::uint8_t((value>>4)|shiftCarry);
                            shiftCarry=std::uint8_t(value<<4);
                            value=shifted;
                        }
                        const auto previous=busRead(dst);
                        std::uint8_t mask=0xff;
                        if(control&0x08){
                            const auto foreground=std::uint8_t((control&0x10)?raw:value);
                            if(!(foreground&0xf0))mask&=0x0f;
                            if(!(foreground&0x0f))mask&=0xf0;
                        }
                        if(control&0x40)mask&=0xf0;
                        if(control&0x80)mask&=0x0f;
                        const auto result=std::uint8_t((previous&~mask)|(value&mask));
                        if(dst>=0x4000 && dst<0xd000)busWrite(dst,result);
                        else ++rejectedDmaWrites_;
                        ++specialChipBytes_;
                        specialChipEstimatedCycles_+=(control&4)?2u:1u;
                        src=std::uint16_t(src+srcStep);
                        dst=std::uint16_t(dst+dstStep);
                    }
                }
            }
            return;
        }
        if(address==0xc900){setRomBank(value);return;}
        if(address==0xc980){soundLink_.send(value);pias_[1].setPortB(value);return;}
    }
    if(address<0x4000)return; // banked ROM window is read-only
    if(address>=0x4000 && address<0xd000){
        const auto offset=unsigned(address-0x4000);
        if(offset<Width*Height/2)video_.write(offset,value);
        else memory_[address]=value;
        return;
    }
    if(address>=0xd000 && address<=0xd003){pias_[0].write(std::uint8_t(address&3),value);return;}
    if(address>=0xd004 && address<=0xd007){pias_[1].write(std::uint8_t(address&3),value);return;}
    if(address==0xd020){setRomBank(value);return;}
    if(address>=0xd010 && address<=0xd017){
        const auto index=unsigned(address-0xd010);
        blitterRegisters_[index]=value;
        if(index==7){
            blitter_.setControl(blitterRegisters_[0]);
            blitter_.setSolid(blitterRegisters_[1]);
            blitter_.setSource(std::uint16_t(blitterRegisters_[2]<<8|blitterRegisters_[3]));
            blitter_.setDestination(std::uint16_t(blitterRegisters_[4]<<8|blitterRegisters_[5]));
            blitter_.setDimensions(blitterRegisters_[6],blitterRegisters_[7]);
            // Register-triggered DMA uses CPU-visible addresses for both ends.
            // Avoid recursive hardware-register writes during DMA.
            blitter_.executeBus(
                [this](std::uint16_t address){return busRead(address);},
                [this](std::uint16_t address,std::uint8_t data){
                    if(address>=0x4000 && address<0xd000)
                        busWrite(address,data);
                    else ++rejectedDmaWrites_;
                });
        }
        return;
    }
    memory_[address]=value;
}
void Williams6809::advanceRaster(std::uint32_t cpuCycles){
    if(profile_==BoardProfile::Experimental)return;
    // Approximate 256 lines/frame at a nominal 1 MHz / 60 Hz CPU clock.
    // Exact Williams beam clocks and scanline interrupt positions are pending.
    // Use 64-bit arithmetic so large diagnostic advances cannot overflow.
    std::uint64_t accumulated=std::uint64_t(scanlineCycleRemainder_)+std::uint64_t(cpuCycles)*256u;
    while(accumulated>=16667u){
        accumulated-=16667u;
        scanline_=std::uint16_t((scanline_+1)%256);
        if(scanline_==0){
            ++rasterInterrupts_;
            pias_[0].edgeCA1(true);
            if(pias_[0].irq())cpu_.requestIrq();
        }
    }
    scanlineCycleRemainder_=static_cast<std::uint32_t>(accumulated);
}
Williams6809::BootDiagnostic Williams6809::runBootDiagnostic(std::uint64_t cycleBudget){
    const auto remaining=~std::uint64_t{0}-cycles_;
    const auto target=cycles_+(cycleBudget<remaining?cycleBudget:remaining);
    while(cycles_<target && !cpu_.halted()){
        if(hardwareBusEnabled_ && (pias_[0].irq()||pias_[1].irq()))cpu_.requestIrq();
        const auto spent=cpu_.step();
        cycles_+=spent?spent:1;
        advanceRaster(spent?spent:1);
    }
    return {cpu_.pc,cycles_,cpu_.unsupportedOpcodes(),cpu_.halted()};
}
std::uint64_t Williams6809::runFrame(){
    // Temporary bounded CPU scheduler, not an accurate Williams video clock.
    const auto target=++frames_*1000000ull/60;
    const auto before=cycles_;
    while(cycles_<target){
        if(hardwareBusEnabled_ && (pias_[0].irq()||pias_[1].irq()))cpu_.requestIrq();
        const auto spent=cpu_.step();
        // A zero-cycle step must not stall the frame scheduler forever.
        const auto advanced=spent?spent:1;
        cycles_+=advanced;
        advanceRaster(advanced);
        if(cpu_.halted())break; // unsupported opcode or SYNC: fail closed
    }
    return cycles_-before;
}
}
