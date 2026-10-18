#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include "ca_mame/drivers/AtapiCdRom.hpp"
namespace ca_mame::drivers {
// Experimental 32-bit register view of Konami M2's ATA CS0 aperture.
// Real M2 64-bit byte-lane swapping and timing remain unimplemented.
class M2AtapiTaskfile {
public:
    static constexpr std::uint32_t base=0x3f000000u;
    static constexpr std::uint32_t size=0x01000000u;
    explicit M2AtapiTaskfile(AtapiCdRom& cd) noexcept:cd_(cd){}
    void reset() noexcept {status_=0x40;error_=0;features_=0;byteCount_=0;packetBytes_=0;dataIndex_=0;response_.clear();awaitingPacket_=false;irq_=false;}
    bool interruptAsserted() const noexcept{return irq_;}
    bool read32(std::uint32_t offset,std::uint32_t& value) noexcept {
        switch(offset){
            case 0x00:
                if(!(status_&8u) || dataIndex_>=response_.size())return false;
                value=std::uint32_t(response_[dataIndex_])<<8;
                if(dataIndex_+1<response_.size())value|=response_[dataIndex_+1];
                dataIndex_+=2;
                if(dataIndex_>=response_.size()){status_=0x40;irq_=true;}
                return true;
            case 0x04:value=error_;return true;
            case 0x08:value=byteCount_&0xffu;return true;
            case 0x0c:value=(byteCount_>>8)&0xffu;return true;
            case 0x10:value=status_;irq_=false;return true;
            default:return false;
        }
    }
    bool write32(std::uint32_t offset,std::uint32_t value) noexcept {
        switch(offset){
            case 0x00:
                if(!awaitingPacket_)return false;
                if(packetBytes_>=12)return false;
                packet_[packetBytes_++]=std::uint8_t(value>>8);
                packet_[packetBytes_++]=std::uint8_t(value);
                if(packetBytes_==12){
                    awaitingPacket_=false;
                    if(cd_.packet(packet_,response_)){
                        dataIndex_=0;status_=response_.empty()?0x40:0x48;
                        byteCount_=static_cast<std::uint16_t>(response_.size()>65535?65535:response_.size());
                    }else{status_=0x41;error_=0x04;}
                    irq_=true;
                }
                return true;
            case 0x04:features_=std::uint8_t(value);return true;
            case 0x08:byteCount_=static_cast<std::uint16_t>((byteCount_&0xff00u)|(value&0xffu));return true;
            case 0x0c:byteCount_=static_cast<std::uint16_t>((byteCount_&0xffu)|((value&0xffu)<<8));return true;
            case 0x10:
                irq_=false;error_=0;
                if((value&0xffu)==0xa0u){
                    awaitingPacket_=true;packetBytes_=0;response_.clear();status_=0x48;return true;
                }
                if((value&0xffu)==0x08u){reset();return true;}
                status_=0x41;error_=0x04;irq_=true;return false;
            default:return false;
        }
    }
private:
    AtapiCdRom& cd_;
    std::array<std::uint8_t,12> packet_{};
    std::vector<std::uint8_t> response_;
    std::size_t packetBytes_=0,dataIndex_=0;
    std::uint16_t byteCount_=0;
    std::uint8_t status_=0x40,error_=0,features_=0;
    bool awaitingPacket_=false,irq_=false;
};
}
