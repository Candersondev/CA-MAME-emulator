#include "ca_mame/compat/Sha1.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
namespace ca_mame::compat {
static std::uint32_t rot(std::uint32_t x,unsigned n){return (x<<n)|(x>>(32-n));}
void Sha1::process(){
 std::uint32_t w[80]{};
 for(unsigned i=0;i<16;++i)w[i]=(std::uint32_t(block_[4*i])<<24)|(std::uint32_t(block_[4*i+1])<<16)|(std::uint32_t(block_[4*i+2])<<8)|block_[4*i+3];
 for(unsigned i=16;i<80;++i)w[i]=rot(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
 auto a=h_[0],b=h_[1],c=h_[2],d=h_[3],e=h_[4];
 for(unsigned i=0;i<80;++i){
  std::uint32_t f,k;
  if(i<20){f=(b&c)|(~b&d);k=0x5a827999u;}
  else if(i<40){f=b^c^d;k=0x6ed9eba1u;}
  else if(i<60){f=(b&c)|(b&d)|(c&d);k=0x8f1bbcdcu;}
  else{f=b^c^d;k=0xca62c1d6u;}
  const auto t=rot(a,5)+f+e+k+w[i];e=d;d=c;c=rot(b,30);b=a;a=t;
 }
 h_[0]+=a;h_[1]+=b;h_[2]+=c;h_[3]+=d;h_[4]+=e;
}
void Sha1::update(const unsigned char* data,std::size_t size){
 bytes_+=size;
 while(size){
  const auto n=std::min(size,64-buffered_);
  std::copy_n(data,n,block_.data()+buffered_);
  data+=n;size-=n;buffered_+=n;
  if(buffered_==64){process();buffered_=0;}
 }
}
std::string Sha1::finish(){
 const auto bits=bytes_*8;
 const unsigned char marker=0x80;
 update(&marker,1);
 const unsigned char zero=0;
 while(buffered_!=56)update(&zero,1);
 unsigned char length[8];
 for(unsigned i=0;i<8;++i)length[i]=static_cast<unsigned char>(bits>>(56-8*i));
 update(length,8);
 std::ostringstream out;out<<std::hex<<std::setfill('0');
 for(auto value:h_)out<<std::setw(8)<<value;
 return out.str();
}
}
