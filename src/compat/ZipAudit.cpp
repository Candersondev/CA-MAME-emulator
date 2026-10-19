#include "ca_mame/compat/ZipAudit.hpp"
#include "ca_mame/compat/Sha1.hpp"
#include <zlib.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <vector>
namespace ca_mame::compat {
static std::uint16_t u16(const unsigned char* p){return std::uint16_t(p[0])|(std::uint16_t(p[1])<<8);}
static std::uint32_t u32(const unsigned char* p){return std::uint32_t(u16(p))|(std::uint32_t(u16(p+2))<<16);}
static bool readAt(std::ifstream& f,std::uint64_t offset,unsigned char* dst,std::size_t n){
 f.clear();f.seekg(static_cast<std::streamoff>(offset));return bool(f.read(reinterpret_cast<char*>(dst),static_cast<std::streamsize>(n)));
}
ZipAuditResult auditZip(const std::filesystem::path& path){
 ZipAuditResult out;
 std::ifstream f(path,std::ios::binary|std::ios::ate);
 if(!f){out.error="Cannot open ZIP";return out;}
 const auto end=f.tellg();
 if(end<22){out.error="ZIP too short";return out;}
 const auto size=static_cast<std::uint64_t>(end);
 const auto tailSize=static_cast<std::size_t>(std::min<std::uint64_t>(size,65557));
 std::vector<unsigned char> tail(tailSize);
 if(!readAt(f,size-tailSize,tail.data(),tail.size())){out.error="Cannot read ZIP footer";return out;}
 std::size_t eocd=tailSize;
 for(std::size_t i=tailSize-22;;--i){
  if(u32(tail.data()+i)==0x06054b50&&i+22+u16(tail.data()+i+20)==tailSize){eocd=i;break;}
  if(i==0)break;
 }
 if(eocd==tailSize){out.error="ZIP directory not found";return out;}
 const auto* footer=tail.data()+eocd;
 if(u16(footer+4)!=0||u16(footer+6)!=0||u16(footer+8)!=u16(footer+10)||
    u16(footer+10)==0xffff||u32(footer+12)==0xffffffff||u32(footer+16)==0xffffffff){
  out.error="Multi-disk or ZIP64 archive unsupported";return out;
 }
 const auto count=u16(footer+10);
 std::uint64_t offset=u32(footer+16),dirSize=u32(footer+12);
 if(offset>size||dirSize>size-offset){out.error="Invalid ZIP directory range";return out;}
 struct Item{std::string name;std::uint32_t crc,packed,unpacked,local;std::uint16_t method,flags;};
 std::vector<Item> items;items.reserve(count);
 const auto directoryEnd=offset+dirSize;
 for(unsigned i=0;i<count;++i){
  std::array<unsigned char,46> h{};
  if(offset+46>directoryEnd||!readAt(f,offset,h.data(),h.size())||u32(h.data())!=0x02014b50){out.error="Invalid ZIP directory entry";return out;}
  const auto nameLen=u16(h.data()+28),extra=u16(h.data()+30),comment=u16(h.data()+32);
  if(std::uint64_t(nameLen)+extra+comment>directoryEnd-offset-46){out.error="Invalid ZIP entry lengths";return out;}
  std::string name(nameLen,'\0');
  if(nameLen&&!readAt(f,offset+46,reinterpret_cast<unsigned char*>(name.data()),nameLen)){out.error="Invalid ZIP filename";return out;}
  items.push_back({name,u32(h.data()+16),u32(h.data()+20),u32(h.data()+24),u32(h.data()+42),u16(h.data()+10),u16(h.data()+8)});
  offset+=46+nameLen+extra+comment;
 }
 out.readable=true;
 for(const auto& item:items){
  ZipEntryAudit entry;entry.name=item.name;entry.crc=item.crc;
  if(item.name.empty()||item.name.back()=='/'){out.entries.push_back(std::move(entry));continue;}
  if(item.method>8||(item.method!=0&&item.method!=8)||(item.flags&1)||item.packed==0xffffffff||item.unpacked==0xffffffff){out.entries.push_back(std::move(entry));continue;}
  std::array<unsigned char,30> local{};
  if(!readAt(f,item.local,local.data(),local.size())||u32(local.data())!=0x04034b50){out.entries.push_back(std::move(entry));continue;}
  const std::uint64_t start=std::uint64_t(item.local)+30+u16(local.data()+26)+u16(local.data()+28);
  if(start>size||item.packed>size-start){out.entries.push_back(std::move(entry));continue;}
  entry.supported=true;
  z_stream stream{};
  if(item.method==8&&inflateInit2(&stream,-MAX_WBITS)!=Z_OK){entry.supported=false;out.entries.push_back(std::move(entry));continue;}
  std::array<unsigned char,32768> input{},output{};
  std::uint64_t remaining=item.packed,total=0;
  uLong crc=crc32(0L,Z_NULL,0);
  Sha1 sha;
  bool ok=true;
  f.clear();f.seekg(static_cast<std::streamoff>(start));
  while(remaining&&ok){
   const auto n=static_cast<std::size_t>(std::min<std::uint64_t>(remaining,input.size()));
   if(!f.read(reinterpret_cast<char*>(input.data()),static_cast<std::streamsize>(n))){ok=false;break;}
   remaining-=n;
   if(item.method==0){
    crc=crc32(crc,input.data(),static_cast<uInt>(n));sha.update(input.data(),n);total+=n;
   }else{
    stream.next_in=input.data();stream.avail_in=static_cast<uInt>(n);
    while(stream.avail_in&&ok){
     stream.next_out=output.data();stream.avail_out=static_cast<uInt>(output.size());
     const int code=inflate(&stream,Z_NO_FLUSH);
     const auto produced=output.size()-stream.avail_out;
     crc=crc32(crc,output.data(),static_cast<uInt>(produced));sha.update(output.data(),produced);total+=produced;
     if(total>item.unpacked|| (code!=Z_OK&&code!=Z_STREAM_END)){ok=false;break;}
     if(code==Z_STREAM_END){if(remaining||stream.avail_in)ok=false;break;}
     if(produced==0&&stream.avail_in==0)break;
    }
   }
   if(total>item.unpacked)ok=false;
  }
  if(item.method==8){ok=ok&&stream.total_out==item.unpacked&&stream.total_in==item.packed;inflateEnd(&stream);}
  entry.verified=ok&&total==item.unpacked&&crc==item.crc;
  if(entry.verified)entry.sha1=sha.finish();
  out.entries.push_back(std::move(entry));
 }
 return out;
}
}
