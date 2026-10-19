#include "ca_mame/cpu/Motorola6809.hpp"
namespace ca_mame {
void Motorola6809::reset(){
 a=b=dp=0;x=y=u=s=0;cc=0x50;irq_=firq_=nmi_=sync_=false;
 pc=read16(0xfffe);unsupported_=0;
}
std::uint32_t Motorola6809::interrupt(std::uint16_t vector,bool entire){
 sync_=false;
 if(entire){
    cc|=0x80;
    push16(pc);push16(u);push16(y);push16(x);push8(dp);push8(b);push8(a);push8(cc);
 }else{
    cc&=std::uint8_t(~0x80);
    push16(pc);push8(cc);
 }
 cc|=0x10;
 if(vector!=0xfff6)cc|=0x40;
 pc=read16(vector);
 return entire?19:10;
}
std::uint32_t Motorola6809::step(){
 if(nmi_){nmi_=false;return interrupt(0xfffc,true);}
 if(firq_ && !(cc&0x40)){firq_=false;return interrupt(0xfff6,false);}
 if(irq_ && !(cc&0x10)){irq_=false;return interrupt(0xfff8,true);}
 if(sync_)return 1;
 const auto op=fetch();
 switch(op){
 case 0x12:return 2; // NOP
 case 0x13:sync_=true;return 4; // SYNC
 case 0x20:pc=std::uint16_t(pc+static_cast<std::int8_t>(fetch()));return 3; // BRA
 case 0x21:fetch();return 3; // BRN
 case 0x26:{auto d=static_cast<std::int8_t>(fetch());if(!(cc&4))pc=std::uint16_t(pc+d);return 3;}
 case 0x27:{auto d=static_cast<std::int8_t>(fetch());if(cc&4)pc=std::uint16_t(pc+d);return 3;}
 case 0x16:{auto d=static_cast<std::int16_t>(fetch16());pc=std::uint16_t(pc+d);return 5;} // LBRA
 case 0x17:{auto d=static_cast<std::int16_t>(fetch16());push16(pc);pc=std::uint16_t(pc+d);return 9;} // LBSR
 case 0x39:pc=pop16();return 5; // RTS
 case 0x3b:{ // RTI: CC then entire frame when E flag set
    cc=pop8();
    if(cc&0x80){a=pop8();b=pop8();dp=pop8();x=pop16();y=pop16();u=pop16();}
    pc=pop16();return (cc&0x80)?15:6;
 }
 case 0x7e:pc=fetch16();return 3; // JMP extended
 case 0xbd:{auto address=fetch16();push16(pc);pc=address;return 8;} // JSR extended
 case 0x8d:{auto d=static_cast<std::int8_t>(fetch());push16(pc);pc=std::uint16_t(pc+d);return 7;}
 case 0x86:a=fetch();setNZ8(a);cc&=std::uint8_t(~2);return 2;
 case 0xc6:b=fetch();setNZ8(b);cc&=std::uint8_t(~2);return 2;
 case 0x8e:x=fetch16();setNZ16(x);cc&=std::uint8_t(~2);return 3;
 case 0xce:u=fetch16();setNZ16(u);cc&=std::uint8_t(~2);return 3;
 case 0x96:a=read(std::uint16_t(dp<<8|fetch()));setNZ8(a);cc&=std::uint8_t(~2);return 4;
 case 0xd6:b=read(std::uint16_t(dp<<8|fetch()));setNZ8(b);cc&=std::uint8_t(~2);return 4;
 case 0xb6:a=read(fetch16());setNZ8(a);cc&=std::uint8_t(~2);return 5;
 case 0xf6:b=read(fetch16());setNZ8(b);cc&=std::uint8_t(~2);return 5;
 case 0x97:{auto addr=std::uint16_t(dp<<8|fetch());write(addr,a);setNZ8(a);cc&=std::uint8_t(~2);return 4;}
 case 0xd7:{auto addr=std::uint16_t(dp<<8|fetch());write(addr,b);setNZ8(b);cc&=std::uint8_t(~2);return 4;}
 case 0xb7:{auto addr=fetch16();write(addr,a);setNZ8(a);cc&=std::uint8_t(~2);return 5;}
 case 0xf7:{auto addr=fetch16();write(addr,b);setNZ8(b);cc&=std::uint8_t(~2);return 5;}
 case 0x1a:cc|=fetch();return 3; // ORCC
 case 0x1c:cc&=fetch();return 3; // ANDCC
 case 0x4f:a=0;cc=std::uint8_t((cc&~0x0f)|4);return 2; // CLRA
 case 0x5f:b=0;cc=std::uint8_t((cc&~0x0f)|4);return 2;
 default:++unsupported_;sync_=true;return 1;
 }
}
}
