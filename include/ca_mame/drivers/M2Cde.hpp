#pragma once
#include <cstdint>
#include <array>
namespace ca_mame::drivers {
// Initial 3DO M2 CDE register model. No CD transport, DMA or timers yet.
class M2Cde {
public:
    static constexpr std::uint32_t base=0x04000000u;
    static constexpr std::uint32_t size=0x01000000u;
    static constexpr std::uint32_t serialWriteDone=0x10000000u;
    void reset() noexcept {serialControl_=0;status_=0;enable_=0;bootBlockEnable_=1;visaDisable_=0;for(auto& device:devices_)device={};for(auto& channel:dma_)channel={};}
    // Explicit test-device data injection; no physical CD device is emulated.
    bool loadBioByte(std::uint32_t address,std::uint8_t value) noexcept {
        if(address<0x20000000u || address>=0x20010000u)return false;
        bioRam_[address-0x20000000u]=value;
        return true;
    }
    bool readBioByte(std::uint32_t address,std::uint8_t& value) const noexcept {
        if(address<0x20000000u || address>=0x20010000u)return false;
        value=bioRam_[address-0x20000000u];
        return true;
    }
    struct Transfer {std::uint32_t source=0,destination=0,count=0;bool fixedSource=false;};
    bool prepareTransfer(unsigned index,std::uint32_t control,Transfer& transfer) const noexcept {
        if(index>=2 || !(control&0x80u) || (control&0x400u))return false;
        const auto& ch=dma_[index];
        if(ch.count==0 || ch.count>0x10000u || (ch.count&1u) ||
           (ch.powerAddress&1u) || (ch.busAddress&1u))return false;
        const unsigned slot=((ch.busAddress>>26)&7u);
        const auto setup=devices_[slot].setup;
        if(!(setup&0x800u))return false; // 16-bit BioBus only
        transfer={ch.busAddress,ch.powerAddress,ch.count,(setup&0xe000u)!=0};
        if(transfer.source<0x20000000u || transfer.source>=0x20010000u)return false;
        const auto bytes=transfer.fixedSource?2u:transfer.count;
        return bytes<=0x10000u-(transfer.source-0x20000000u);
    }
    void completeTransfer(unsigned index,const Transfer& transfer) noexcept {
        auto& ch=dma_[index];
        ch.busAddress+=transfer.fixedSource?0u:transfer.count;
        ch.powerAddress+=transfer.count;
        ch.count=0;
        ch.control&=~0x80u;
        status_|=(index==0?0x80u:0x100u);
    }
    bool interruptAsserted() const noexcept {return (status_&enable_)!=0;}
    void raiseInterrupt(std::uint32_t mask) noexcept {status_|=mask;}
    bool read32(std::uint32_t offset,std::uint32_t& value) const noexcept {
        if(offset>=0x240u && offset<0x280u && (offset&3u)==0){
            const auto& device=devices_[(offset-0x240u)/8u];
            value=((offset-0x240u)&4u)?device.cycleTime:device.setup;
            return true;
        }
        if(offset>=0x1000u && offset<0x1040u && (offset&3u)==0){
            const auto& channel=dma_[(offset-0x1000u)/0x20u];
            switch((offset-0x1000u)&0x1fu){
                case 0x00:value=channel.control;return true;
                case 0x04:value=channel.busAddress;return true;
                case 0x08:value=channel.powerAddress;return true;
                case 0x0c:value=channel.count;return true;
                default:return false;
            }
        }
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
        const auto reg=offset&~0x400u;
        if(reg>=0x240u && reg<0x280u && (reg&3u)==0){
            auto& device=devices_[(reg-0x240u)/8u];
            auto& target=((reg-0x240u)&4u)?device.cycleTime:device.setup;
            target=clear?(target&~data):data;
            return true;
        }
        if(reg>=0x1000u && reg<0x1040u && (reg&3u)==0){
            auto& channel=dma_[(reg-0x1000u)/0x20u];
            switch((reg-0x1000u)&0x1fu){
                case 0x00:{
                    const auto next=clear?(channel.control&~data):data;
                    // DMA engine is not implemented. Reject START rather than
                    // pretending a transfer has completed.
                    if(next&0x80u)return false;
                    if(!(channel.control&0x200u) && (next&0x200u)){channel={};return true;}
                    channel.control=next;
                    return true;
                }
                case 0x04:channel.busAddress=clear?(channel.busAddress&~data):data;return true;
                case 0x08:channel.powerAddress=clear?(channel.powerAddress&~data):data;return true;
                case 0x0c:channel.count=clear?(channel.count&~data):data;return true;
                default:return false;
            }
        }
        switch(reg){
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
    struct BioDevice {std::uint32_t setup=0,cycleTime=0;};
    struct DmaChannel {std::uint32_t control=0,busAddress=0,powerAddress=0,count=0;};
    std::array<BioDevice,8> devices_{};
    std::array<std::uint8_t,0x10000> bioRam_{};
    std::array<DmaChannel,2> dma_{};
    std::uint32_t serialControl_=0,status_=0,enable_=0,bootBlockEnable_=1,visaDisable_=0;
};
}
