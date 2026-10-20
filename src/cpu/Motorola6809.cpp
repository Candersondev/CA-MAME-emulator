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
std::uint16_t Motorola6809::indexedAddress(bool& valid,unsigned& extraCycles){
 const auto post=fetch();
 auto* reg=&x;
 switch((post>>5)&3){case 1:reg=&y;break;case 2:reg=&u;break;case 3:reg=&s;break;}
 if(!(post&0x80)){
    const auto offset=static_cast<std::int8_t>((post&0x0f)|((post&0x10)?0xf0:0));
    extraCycles=1;return std::uint16_t(*reg+offset);
 }
 const bool indirect=(post&0x10)!=0;
 const auto mode=post&0x0f;
 std::uint16_t addr=0;
 switch(mode){
 case 0:addr=*reg;*reg=std::uint16_t(*reg+1);extraCycles=2;break;
 case 1:addr=*reg;*reg=std::uint16_t(*reg+2);extraCycles=3;break;
 case 2:*reg=std::uint16_t(*reg-1);addr=*reg;extraCycles=2;break;
 case 3:*reg=std::uint16_t(*reg-2);addr=*reg;extraCycles=3;break;
 case 4:addr=*reg;extraCycles=0;break;
 case 5:addr=std::uint16_t(*reg+static_cast<std::int8_t>(b));extraCycles=1;break;
 case 6:addr=std::uint16_t(*reg+static_cast<std::int8_t>(a));extraCycles=1;break;
 case 8:addr=std::uint16_t(*reg+static_cast<std::int8_t>(fetch()));extraCycles=1;break;
 case 9:addr=std::uint16_t(*reg+static_cast<std::int16_t>(fetch16()));extraCycles=4;break;
 case 11:addr=std::uint16_t(*reg+static_cast<std::int16_t>((a<<8)|b));extraCycles=4;break;
 case 12:{const auto displacement=static_cast<std::int8_t>(fetch());addr=std::uint16_t(pc+displacement);extraCycles=1;break;}
 case 13:{auto displacement=static_cast<std::int16_t>(fetch16());addr=std::uint16_t(pc+displacement);extraCycles=5;break;}
 case 15:addr=fetch16();extraCycles=5;break;
 default:valid=false;return 0;
 }
 if(indirect){if(mode==0 || mode==2){valid=false;return 0;}addr=read16(addr);extraCycles+=3;}
 return addr;
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
 case 0x30:case 0x31:case 0x32:case 0x33:{
    bool valid=true;unsigned extra=0;
    const auto addr=indexedAddress(valid,extra);
    if(!valid){++unsupported_;sync_=true;return 1;}
    if(op==0x30){x=addr;cc=std::uint8_t((cc&~4)|(x==0?4:0));}
    else if(op==0x31){y=addr;cc=std::uint8_t((cc&~4)|(y==0?4:0));}
    else if(op==0x32)s=addr;
    else u=addr;
    return 4+extra;
 }
 case 0x34:case 0x36:{ // PSHS / PSHU, high bits first
    const auto mask=fetch();auto& sp=(op==0x34)?s:u;
    auto push=[this,&sp](std::uint8_t value){write(--sp,value);};
    auto pushWord=[&](std::uint16_t value){push(std::uint8_t(value));push(std::uint8_t(value>>8));};
    if(mask&0x80)pushWord(pc);
    if(mask&0x40)pushWord(op==0x34?u:s);
    if(mask&0x20)pushWord(y);
    if(mask&0x10)pushWord(x);
    if(mask&0x08)push(dp);
    if(mask&0x04)push(b);
    if(mask&0x02)push(a);
    if(mask&0x01)push(cc);
    unsigned bytes=0;for(unsigned bit=0;bit<8;++bit)if(mask&(1u<<bit))bytes+=(bit>=4)?2:1;
    return 5+bytes;
 }
 case 0x35:case 0x37:{ // PULS / PULU, low bits first
    const auto mask=fetch();auto& sp=(op==0x35)?s:u;
    auto pop=[this,&sp](){return read(sp++);};
    auto popWord=[&](){const auto hi=pop();return std::uint16_t(hi<<8|pop());};
    if(mask&0x01)cc=pop();
    if(mask&0x02)a=pop();
    if(mask&0x04)b=pop();
    if(mask&0x08)dp=pop();
    if(mask&0x10)x=popWord();
    if(mask&0x20)y=popWord();
    if(mask&0x40){const auto value=popWord();if(op==0x35)u=value;else s=value;}
    if(mask&0x80)pc=popWord();
    unsigned bytes=0;for(unsigned bit=0;bit<8;++bit)if(mask&(1u<<bit))bytes+=(bit>=4)?2:1;
    return 5+bytes;
 }
 case 0x20:{const auto displacement=static_cast<std::int8_t>(fetch());pc=std::uint16_t(pc+displacement);return 3;} // BRA
 case 0x21:fetch();return 3; // BRN
 case 0x22:case 0x23:case 0x24:case 0x25:case 0x28:case 0x29:
 case 0x2a:case 0x2b:case 0x2c:case 0x2d:case 0x2e:case 0x2f:{
    const auto d=static_cast<std::int8_t>(fetch());
    const bool c=(cc&1)!=0,z=(cc&4)!=0,n=(cc&8)!=0,v=(cc&2)!=0;
    bool take=false;
    switch(op){
      case 0x22:take=!c&&!z;break;case 0x23:take=c||z;break;
      case 0x24:take=!c;break;case 0x25:take=c;break;
      case 0x28:take=!v;break;case 0x29:take=v;break;
      case 0x2a:take=!n;break;case 0x2b:take=n;break;
      case 0x2c:take=n==v;break;case 0x2d:take=n!=v;break;
      case 0x2e:take=!z&&(n==v);break;case 0x2f:take=z||(n!=v);break;
    }
    if(take)pc=std::uint16_t(pc+d);
    return 3;
 }
 case 0x26:{auto d=static_cast<std::int8_t>(fetch());if(!(cc&4))pc=std::uint16_t(pc+d);return 3;}
 case 0x27:{auto d=static_cast<std::int8_t>(fetch());if(cc&4)pc=std::uint16_t(pc+d);return 3;}
 case 0x16:{const auto displacement=static_cast<std::int16_t>(fetch16());pc=std::uint16_t(pc+displacement);return 5;} // LBRA
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
 case 0xa6:case 0xe6:case 0xa7:case 0xe7:
 case 0xae:case 0xee:case 0xaf:case 0xef:{
    bool valid=true;unsigned extra=0;
    const auto addr=indexedAddress(valid,extra);
    if(!valid){++unsupported_;sync_=true;return 1;}
    switch(op){
      case 0xa6:a=read(addr);setNZ8(a);cc&=std::uint8_t(~2);return 4+extra;
      case 0xe6:b=read(addr);setNZ8(b);cc&=std::uint8_t(~2);return 4+extra;
      case 0xa7:write(addr,a);setNZ8(a);cc&=std::uint8_t(~2);return 4+extra;
      case 0xe7:write(addr,b);setNZ8(b);cc&=std::uint8_t(~2);return 4+extra;
      case 0xae:x=read16(addr);setNZ16(x);cc&=std::uint8_t(~2);return 5+extra;
      case 0xee:u=read16(addr);setNZ16(u);cc&=std::uint8_t(~2);return 5+extra;
      case 0xaf:write16(addr,x);setNZ16(x);cc&=std::uint8_t(~2);return 5+extra;
      default:write16(addr,u);setNZ16(u);cc&=std::uint8_t(~2);return 5+extra;
    }
 }
 case 0x1f:{ // TFR register transfer
    const auto post=fetch();
    auto get=[this](unsigned reg)->std::uint16_t {
        switch(reg){case 0:return std::uint16_t(a<<8|b);case 1:return x;
        case 2:return y;case 3:return u;case 4:return s;case 5:return pc;
        case 8:return a;case 9:return b;case 10:return cc;case 11:return dp;
        default:return 0xffff;}
    };
    const auto src=unsigned(post>>4),dst=unsigned(post&15);
    if(src>11||dst>11||src==6||src==7||dst==6||dst==7||
       (src<8)!=(dst<8)){++unsupported_;sync_=true;return 1;}
    const auto value=get(src);
    switch(dst){case 0:a=std::uint8_t(value>>8);b=std::uint8_t(value);break;
    case 1:x=value;break;case 2:y=value;break;case 3:u=value;break;
    case 4:s=value;break;case 5:pc=value;break;
    case 8:a=std::uint8_t(value);break;case 9:b=std::uint8_t(value);break;
    case 10:cc=std::uint8_t(value);break;case 11:dp=std::uint8_t(value);break;}
    return 6;
 }
 case 0x8b:{const auto rhs=fetch();const auto result=unsigned(a)+rhs;
    cc=std::uint8_t((cc&~0x2f)|(result&0x100?1:0)|
        (((~(a^rhs))&(a^result)&0x80)?2:0)|
        ((result&0xff)==0?4:0)|(result&0x80?8:0)|
        (((a&15)+(rhs&15))>15?0x20:0));
    a=std::uint8_t(result);return 2;
 }
 case 0xcb:{const auto rhs=fetch();const auto result=unsigned(b)+rhs;
    cc=std::uint8_t((cc&~0x2f)|(result&0x100?1:0)|
        (((~(b^rhs))&(b^result)&0x80)?2:0)|
        ((result&0xff)==0?4:0)|(result&0x80?8:0)|
        (((b&15)+(rhs&15))>15?0x20:0));
    b=std::uint8_t(result);return 2;
 }
 case 0x81:case 0xc1:{ // CMPA / CMPB immediate
    const auto rhs=fetch();const auto lhs=(op==0x81)?a:b;
    const auto result=unsigned(lhs)-rhs;
    cc=std::uint8_t((cc&~0x0f)|(result&0x100?1:0)|
        (((lhs^rhs)&(lhs^result)&0x80)?2:0)|
        ((result&0xff)==0?4:0)|(result&0x80?8:0));
    return 2;
 }
 case 0x84:case 0x8a:case 0x88:case 0x85:
 case 0xc4:case 0xca:case 0xc8:case 0xc5:{
    const auto rhs=fetch();
    auto& reg=(op&0x40)?b:a;
    const auto kind=op&0x0f;
    std::uint8_t result=0;
    if(kind==4||kind==5)result=std::uint8_t(reg&rhs);
    else if(kind==8)result=std::uint8_t(reg^rhs);
    else result=std::uint8_t(reg|rhs);
    setNZ8(result);cc=std::uint8_t(cc&~2);
    if(kind!=5)reg=result;
    return 2;
 }
 case 0x80:case 0xc0:case 0x82:case 0xc2:{
    const auto rhs=fetch();
    auto& reg=(op&0x40)?b:a;
    const auto borrow=(op&2)?(cc&1):0;
    const auto lhs=unsigned(reg);
    const auto result=lhs-unsigned(rhs)-borrow;
    cc=std::uint8_t((cc&~0x0f)|(result&0x100?1:0)|
      (((lhs^rhs)&(lhs^result)&0x80)?2:0)|
      ((result&0xff)==0?4:0)|(result&0x80?8:0));
    reg=std::uint8_t(result);
    return 2;
 }
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
