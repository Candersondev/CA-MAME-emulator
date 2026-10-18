#include "ca_mame/rom/RomManager.hpp"
#include <fstream>
#include <array>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <utility>
namespace ca_mame::rom {
namespace {
std::uint32_t rotl(std::uint32_t v,unsigned n) {return (v<<n)|(v>>(32-n));}
std::uint16_t le16(const std::vector<std::uint8_t>& b,std::size_t i) {
    return std::uint16_t(b[i]|(std::uint16_t(b[i+1])<<8));
}
std::uint32_t le32(const std::vector<std::uint8_t>& b,std::size_t i) {
    return std::uint32_t(b[i])|(std::uint32_t(b[i+1])<<8)|(std::uint32_t(b[i+2])<<16)|(std::uint32_t(b[i+3])<<24);
}
bool verifyBytes(ca_mame::rom::AuditResult& result,const ca_mame::rom::RomDefinition& def,
                 const std::vector<std::uint8_t>& bytes) {
    result.actualSize=bytes.size();
    result.actualCrc32=ca_mame::rom::RomManager::crc32(bytes);
    result.actualSha1=ca_mame::rom::RomManager::sha1(bytes);
    if(bytes.size()!=def.expectedSize)result.message="ROM size mismatch";
    else if(def.verifyCrc32 && result.actualCrc32!=def.expectedCrc32)result.message="ROM CRC32 mismatch";
    else if(def.verifySha1 && result.actualSha1!=def.expectedSha1)result.message="ROM SHA-1 mismatch";
    else {result.ok=true;result.message="OK";}
    return result.ok;
}
}
std::string RomManager::sha1(const std::vector<std::uint8_t>& bytes) {
    std::array<std::uint32_t,5> h={0x67452301u,0xefcdab89u,0x98badcfeu,0x10325476u,0xc3d2e1f0u};
    std::vector<std::uint8_t> data=bytes;
    const auto bits=std::uint64_t(data.size())*8;
    data.push_back(0x80);
    while(data.size()%64!=56)data.push_back(0);
    for(int i=7;i>=0;--i)data.push_back(std::uint8_t(bits>>(i*8)));
    for(std::size_t offset=0;offset<data.size();offset+=64) {
        std::array<std::uint32_t,80> w{};
        for(unsigned i=0;i<16;++i) {
            const auto j=offset+i*4;
            w[i]=(std::uint32_t(data[j])<<24)|(std::uint32_t(data[j+1])<<16)|
                 (std::uint32_t(data[j+2])<<8)|data[j+3];
        }
        for(unsigned i=16;i<80;++i)w[i]=rotl(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
        auto a=h[0],b=h[1],c=h[2],d=h[3],e=h[4];
        for(unsigned i=0;i<80;++i) {
            std::uint32_t f=0,k=0;
            if(i<20){f=(b&c)|(~b&d);k=0x5a827999u;}
            else if(i<40){f=b^c^d;k=0x6ed9eba1u;}
            else if(i<60){f=(b&c)|(b&d)|(c&d);k=0x8f1bbcdcu;}
            else {f=b^c^d;k=0xca62c1d6u;}
            const auto temp=rotl(a,5)+f+e+k+w[i];
            e=d;d=c;c=rotl(b,30);b=a;a=temp;
        }
        h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;
    }
    std::ostringstream out;
    out<<std::hex<<std::setfill('0');
    for(const auto word:h)out<<std::setw(8)<<word;
    return out.str();
}
bool RomManager::readZipEntry(const std::filesystem::path& archive,const std::string& entry,
                              std::vector<std::uint8_t>& bytes) {
    std::vector<std::uint8_t> zip;
    if(!readFile(archive,zip) || zip.size()<22)return false;
    // Locate the end of central directory (ZIP comments may be present).
    std::size_t eocd=zip.size();
    const auto begin=zip.size()>65557?zip.size()-65557:0;
    for(std::size_t pos=zip.size()-22;;--pos) {
        if(le32(zip,pos)==0x06054b50u && pos+22u+le16(zip,pos+20)==zip.size()) {eocd=pos;break;}
        if(pos==begin)break;
    }
    if(eocd==zip.size() || le16(zip,eocd+4)!=0 || le16(zip,eocd+6)!=0)return false;
    const auto count=le16(zip,eocd+10);
    std::size_t pos=le32(zip,eocd+16);
    for(unsigned i=0;i<count;++i) {
        if(pos>zip.size() || zip.size()-pos<46 || le32(zip,pos)!=0x02014b50u)return false;
        const auto method=le16(zip,pos+10), flags=le16(zip,pos+8);
        const auto size=le32(zip,pos+24), compressed=le32(zip,pos+20);
        const auto nameLen=le16(zip,pos+28),extraLen=le16(zip,pos+30),commentLen=le16(zip,pos+32);
        const auto total=std::size_t(46)+nameLen+extraLen+commentLen;
        if(total>zip.size()-pos)return false;
        const std::string name(reinterpret_cast<const char*>(zip.data()+pos+46),nameLen);
        if(name==entry) {
            if(method!=0 || (flags&1) || size!=compressed)return false;
            const auto local=std::size_t(le32(zip,pos+42));
            if(local>zip.size() || zip.size()-local<30 || le32(zip,local)!=0x04034b50u)return false;
            const auto data=local+30u+le16(zip,local+26)+le16(zip,local+28);
            if(data>zip.size() || compressed>zip.size()-data)return false;
            std::vector<std::uint8_t> loaded(zip.begin()+data,zip.begin()+data+compressed);
            if(crc32(loaded)!=le32(zip,pos+16))return false;
            bytes=std::move(loaded);
            return true;
        }
        pos+=total;
    }
    return false;
}
AuditResult RomManager::auditZip(const std::filesystem::path& archive,const RomDefinition& definition) {
    AuditResult result;
    result.filename=definition.filename;
    std::vector<std::uint8_t> bytes;
    if(!readZipEntry(archive,definition.filename,bytes)) {
        result.message="ZIP entry missing, compressed, or invalid";
        return result;
    }
    verifyBytes(result,definition,bytes);
    return result;
}

std::uint32_t RomManager::crc32(const std::vector<std::uint8_t>& bytes) noexcept {
    std::uint32_t crc=0xffffffffu;
    for(const auto byte:bytes) {
        crc^=byte;
        for(int bit=0;bit<8;++bit)
            crc=(crc>>1) ^ ((crc&1u)?0xedb88320u:0u);
    }
    return ~crc;
}
bool RomManager::readFile(const std::filesystem::path& path,std::vector<std::uint8_t>& bytes) {
    std::ifstream stream(path,std::ios::binary);
    if(!stream)return false;
    std::vector<std::uint8_t> loaded((std::istreambuf_iterator<char>(stream)),
                                      std::istreambuf_iterator<char>());
    if(stream.bad())return false;
    bytes=std::move(loaded);
    return true;
}
AuditResult RomManager::audit(const std::filesystem::path& directory,const RomDefinition& definition) {
    AuditResult result;
    result.filename=definition.filename;
    const auto relative=std::filesystem::path(definition.filename);
    if(relative.empty() || relative.is_absolute() || relative.has_root_name()) {
        result.message="Invalid ROM filename";
        return result;
    }
    for(const auto& component:relative)
        if(component=="..") {result.message="Parent directory traversal rejected";return result;}
    std::vector<std::uint8_t> bytes;
    if(!readFile(directory/relative,bytes)) {
        result.message="ROM missing or unreadable";
        return result;
    }
    verifyBytes(result,definition,bytes);
    return result;
}
}
