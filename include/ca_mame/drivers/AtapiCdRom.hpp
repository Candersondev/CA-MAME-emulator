#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
namespace ca_mame::drivers {
// Standalone ATAPI packet-command backend. Transport registers are not wired yet.
// Accepts a flat, 2048-byte/sector ISO image (not raw 2352-byte BIN/CUE).
class AtapiCdRom {
public:
    static constexpr std::uint32_t sectorSize=2048;
    bool mountIso(const std::string& path) {
        eject();
        std::ifstream file(path,std::ios::binary|std::ios::ate);
        if(!file)return false;
        const auto length=file.tellg();
        if(length<=0 || length%sectorSize!=0 || length>static_cast<std::streamoff>(0x7fffffff))return false;
        file.seekg(0);
        image_.resize(static_cast<std::size_t>(length));
        if(!file.read(reinterpret_cast<char*>(image_.data()),length)){eject();return false;}
        return true;
    }
    void eject() noexcept {image_.clear();senseKey_=0;asc_=0;}
    bool ready() const noexcept {return !image_.empty();}
    std::uint32_t sectors() const noexcept {return static_cast<std::uint32_t>(image_.size()/sectorSize);}
    std::uint8_t senseKey() const noexcept {return senseKey_;}
    std::uint8_t asc() const noexcept {return asc_;}
    // Returns command success. Unsupported and invalid requests return CHECK CONDITION
    // with sense available through REQUEST SENSE.
    bool packet(const std::array<std::uint8_t,12>& cdb,std::vector<std::uint8_t>& out) {
        out.clear();
        const auto be32=[&](unsigned n)->std::uint32_t {
            return (std::uint32_t(cdb[n])<<24)|(std::uint32_t(cdb[n+1])<<16)|
                   (std::uint32_t(cdb[n+2])<<8)|cdb[n+3];
        };
        if(cdb[0]==0x03) {
            out.assign(18,0);out[0]=0x70;out[2]=senseKey_;out[7]=10;out[12]=asc_;
            if(out.size()>cdb[4])out.resize(cdb[4]);
            senseKey_=asc_=0;return true;
        }
        if(cdb[0]==0x12) {
            if(cdb[1]&1u)return fail(5,0x24);
            out.assign(36,0);out[0]=5;out[1]=0x80;out[2]=0;out[3]=0x21;out[4]=31;
            const char vendor[8]={'C','A','-','M','A','M','E',' '};
            const char product[16]={'V','I','R','T','U','A','L',' ','C','D','-','R','O','M',' ',' '};
            for(unsigned i=0;i<8;++i)out[8+i]=static_cast<std::uint8_t>(vendor[i]);
            for(unsigned i=0;i<16;++i)out[16+i]=static_cast<std::uint8_t>(product[i]);
            out[32]='0';out[33]='0';out[34]='0';out[35]='1';
            if(out.size()>cdb[4])out.resize(cdb[4]);
            return success();
        }
        if(cdb[0]==0x00) return ready()?success():fail(2,0x3a);
        if(!ready())return fail(2,0x3a);
        if(cdb[0]==0x25){
            out.assign(8,0);
            const auto last=sectors()-1;
            for(unsigned i=0;i<4;++i){out[i]=std::uint8_t(last>>(24-8*i));out[i+4]=std::uint8_t(sectorSize>>(24-8*i));}
            return success();
        }
        if(cdb[0]==0x28){
            const auto lba=be32(2);
            const auto count=(std::uint32_t(cdb[7])<<8)|cdb[8];
            if(lba>sectors() || count>sectors()-lba)return fail(5,0x21);
            const auto first=std::size_t(lba)*sectorSize;
            const auto bytes=std::size_t(count)*sectorSize;
            out.assign(image_.begin()+first,image_.begin()+first+bytes);
            return success();
        }
        return fail(5,0x20);
    }
private:
    bool success() noexcept {senseKey_=asc_=0;return true;}
    bool fail(std::uint8_t key,std::uint8_t asc) noexcept {senseKey_=key;asc_=asc;return false;}
    std::vector<std::uint8_t> image_;
    std::uint8_t senseKey_=0,asc_=0;
};
}
