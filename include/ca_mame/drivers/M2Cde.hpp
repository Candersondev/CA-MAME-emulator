#pragma once
#include <cstdint>
namespace ca_mame::drivers {
// Initial 3DO M2 CDE register model. No CD transport, DMA or timers yet.
class M2Cde {
public:
    static constexpr std::uint32_t base=0x04000000u;
    static constexpr std::uint32_t size=0x01000000u;
    static constexpr std::uint32_t serialWriteDone=0x10000000u;
    void reset() noexcept {serialControl_=0;status_=0;enable_=0;bootBlockEnable_=1;visaDisable_=0;}
    bool interruptAsserted() const noexcept {return (status_&enable_)!=0;}
    void raiseInterrupt(std::uint32_t mask) noexcept {status_|=mask;}
    bool read32(std::uint32_t offset,std::uint32_t& value) const noexcept {
        switch(offset){
            case 0x000:value=0x00010000u;return true;
            case 0x004:value=0;return true;
            case 0x00c:value=serialControl_;return true;
            case 0x010:value=0;return true;
            case 0x018:value=status_;return true;
            case 0x01c:value=enable_;return true;
            case 0x02c:value=0;return true;
            case 0x200:value=0;return true;
            case 0x204:value=0x80;return true;
            case 0x208:value=bootBlockEnable_;return true;
            case 0x224:value=0xffffffffu;return true;
            case 0x280:value=0x63659000u;return true; // SYSCONFIG_ARCADE from upstream MAME
            case 0x284:value=visaDisable_;return true;
            case 0x2a0:value=0x20;return true;
            default:return false;
        }
    }
    bool write32(std::uint32_t offset,std::uint32_t data) noexcept {
        const bool clear=(offset&0x400u)!=0;
        switch(offset&~0x400u){
            case 0x00c:serialControl_=clear?(serialControl_&~data):data;return true;
            case 0x014:if(clear)return false;status_|=serialWriteDone;return true;
            case 0x018:status_=clear?(status_&~data):data;return true;
            case 0x01c:enable_=clear?(enable_&~data):(enable_|data);return true;
            case 0x020:if(clear)return false;if(data&1u)bootBlockEnable_=0;return true; // CPU reset not modeled
            case 0x284:visaDisable_=clear?(visaDisable_&~data):data;return true;
            default:return false;
        }
    }
private:
    std::uint32_t serialControl_=0,status_=0,enable_=0,bootBlockEnable_=1,visaDisable_=0;
};
}
