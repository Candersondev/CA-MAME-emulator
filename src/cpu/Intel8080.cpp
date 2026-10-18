#include "ca_mame/cpu/Intel8080.hpp"
#include <stdexcept>
namespace ca_mame {
void Intel8080::reset() {
    a=b=c=d=e=h=l=0; pc=sp=0;
    z=s=p=cy=ac=false; halted=false; inte_=pending_=false; pendingVector_=0;
}
void Intel8080::interrupt(std::uint8_t vector) noexcept {
    if(inte_) { pending_=true; pendingVector_=vector & 7; }
}
std::uint16_t Intel8080::pair(int i) const {
    switch(i) { case 0:return (b<<8)|c; case 1:return (d<<8)|e; case 2:return hl(); default:return sp; }
}
void Intel8080::setPair(int i,std::uint16_t v) {
    switch(i) { case 0:b=v>>8;c=v;break; case 1:d=v>>8;e=v;break; case 2:setHL(v);break; default:sp=v;break; }
}
std::uint8_t Intel8080::reg(int i) const {
    switch(i) { case 0:return b; case 1:return c; case 2:return d; case 3:return e; case 4:return h; case 5:return l; case 6:return read(hl()); default:return a; }
}
void Intel8080::setReg(int i,std::uint8_t v) {
    switch(i) { case 0:b=v;break; case 1:c=v;break; case 2:d=v;break; case 3:e=v;break; case 4:h=v;break; case 5:l=v;break; case 6:write(hl(),v);break; default:a=v;break; }
}
void Intel8080::flagsZSP(std::uint8_t v) {
    z=v==0; s=(v&0x80)!=0;
    unsigned n=v; n^=n>>4; n^=n>>2; n^=n>>1; p=(n&1)==0;
}
void Intel8080::add(std::uint8_t v,bool carry) {
    unsigned ci=(carry&&cy)?1:0;
    unsigned result=unsigned(a)+v+ci;
    ac=((a&15)+(v&15)+ci)>15; cy=result>255; a=std::uint8_t(result); flagsZSP(a);
}
void Intel8080::sub(std::uint8_t v,bool borrow,bool compare) {
    unsigned bi=(borrow&&cy)?1:0;
    unsigned result=unsigned(a)-v-bi;
    ac=(a&15)<((v&15)+bi); cy=unsigned(a)<unsigned(v)+bi;
    auto out=std::uint8_t(result); flagsZSP(out); if(!compare) a=out;
}
void Intel8080::logic(std::uint8_t v,int op) {
    if(op==0) { a &= v; ac=true; }
    else if(op==1) { a ^= v; ac=false; }
    else { a |= v; ac=false; }
    cy=false; flagsZSP(a);
}
void Intel8080::push(std::uint16_t v) {
    write(--sp,std::uint8_t(v>>8)); write(--sp,std::uint8_t(v));
}
std::uint16_t Intel8080::pop() {
    auto lo=read(sp++); auto hi=read(sp++); return std::uint16_t(lo|(hi<<8));
}
bool Intel8080::condition(int i) const {
    switch(i) { case 0:return !z; case 1:return z; case 2:return !cy; case 3:return cy;
    case 4:return !p; case 5:return p; case 6:return !s; default:return s; }
}
std::uint8_t Intel8080::flags() const {
    return std::uint8_t(2 | (s?0x80:0) | (z?0x40:0) | (ac?0x10:0) | (p?4:0) | (cy?1:0));
}
void Intel8080::setFlags(std::uint8_t f) {
    s=f&0x80;z=f&0x40;ac=f&0x10;p=f&4;cy=f&1;
}
std::uint32_t Intel8080::step() {
    if(pending_ && inte_) {
        pending_=false; inte_=false; halted=false; push(pc); pc=std::uint16_t(pendingVector_*8); return 11;
    }
    if(halted) return 4;
    auto op=fetch();
    if(op>=0x40 && op<=0x7f) {
        if(op==0x76) { halted=true; return 7; }
        int dst=(op>>3)&7, src=op&7; setReg(dst,reg(src));
        return (dst==6||src==6)?7:5;
    }
    if(op>=0x80 && op<=0xbf) {
        auto v=reg(op&7); int operation=(op>>3)&7;
        switch(operation) {
            case 0:add(v,false);break; case 1:add(v,true);break;
            case 2:sub(v,false);break; case 3:sub(v,true);break;
            case 4:logic(v,0);break; case 5:logic(v,1);break;
            case 6:logic(v,2);break; default:sub(v,false,true);break;
        }
        return (op&7)==6?7:4;
    }
    if((op&0xcf)==0x01) { setPair((op>>4)&3,fetch16()); return 10; }
    if((op&0xcf)==0x03) { int i=(op>>4)&3;setPair(i,std::uint16_t(pair(i)+1));return 5; }
    if((op&0xcf)==0x0b) { int i=(op>>4)&3;setPair(i,std::uint16_t(pair(i)-1));return 5; }
    if((op&0xcf)==0x09) { unsigned sum=unsigned(hl())+pair((op>>4)&3);cy=sum>65535;setHL(std::uint16_t(sum));return 10; }
    if((op&0xc7)==0x04) { int i=(op>>3)&7;auto old=reg(i);auto v=std::uint8_t(old+1);
        ac=((old&15)+1)>15;setReg(i,v);flagsZSP(v);return i==6?10:5; }
    if((op&0xc7)==0x05) { int i=(op>>3)&7;auto old=reg(i);auto v=std::uint8_t(old-1);
        ac=(old&15)==0;setReg(i,v);flagsZSP(v);return i==6?10:5; }
    if((op&0xc7)==0x06) { int i=(op>>3)&7;setReg(i,fetch());return i==6?10:7; }
    if((op&0xcf)==0x02) { write(pair((op>>4)&1),a);return 7; }
    if((op&0xcf)==0x0a) { a=read(pair((op>>4)&1));return 7; }
    if((op&0xc7)==0xc0) { if(condition((op>>3)&7)) {pc=pop();return 11;}return 5; }
    if((op&0xc7)==0xc2) { auto addr=fetch16();if(condition((op>>3)&7))pc=addr;return 10; }
    if((op&0xc7)==0xc4) { auto addr=fetch16();if(condition((op>>3)&7)) {push(pc);pc=addr;return 17;}return 11; }
    if((op&0xcf)==0xc1) { int i=(op>>4)&3;auto v=pop();if(i==3){a=v>>8;setFlags(std::uint8_t(v));}else setPair(i,v);return 10; }
    if((op&0xcf)==0xc5) { int i=(op>>4)&3;push(i==3?std::uint16_t((a<<8)|flags()):pair(i));return 11; }
    if((op&0xc7)==0xc7) {push(pc);pc=std::uint16_t(op&0x38);return 11;}
    if((op&0xc7)==0xc6) {auto v=fetch();switch((op>>3)&7) {
        case 0:add(v,false);break;case 1:add(v,true);break;
        case 2:sub(v,false);break;case 3:sub(v,true);break;
        case 4:logic(v,0);break;case 5:logic(v,1);break;
        case 6:logic(v,2);break;default:sub(v,false,true);break;
    }return 7;}
    switch(op) {
        case 0x00:case 0x08:case 0x10:case 0x18:case 0x20:case 0x28:case 0x30:case 0x38:return 4;
        case 0x07:cy=(a&0x80)!=0;a=std::uint8_t((a<<1)|(cy?1:0));return 4;
        case 0x0f:cy=(a&1)!=0;a=std::uint8_t((a>>1)|(cy?0x80:0));return 4;
        case 0x17:{bool old=cy;cy=a&0x80;a=std::uint8_t((a<<1)|(old?1:0));return 4;}
        case 0x1f:{bool old=cy;cy=a&1;a=std::uint8_t((a>>1)|(old?0x80:0));return 4;}
        case 0x22:{auto addr=fetch16();write(addr,l);write(std::uint16_t(addr+1),h);return 16;}
        case 0x2a:{auto addr=fetch16();auto lo=read(addr);auto hi=read(std::uint16_t(addr+1));l=lo;h=hi;return 16;}
        case 0x27:{unsigned correction=0;bool newCy=cy;
            if((a&15)>9||ac) correction|=6;
            if(a>0x99||cy) {correction|=0x60;newCy=true;}
            unsigned result=unsigned(a)+correction;
            ac=((a&15)+(correction&15))>15;a=std::uint8_t(result);cy=newCy;flagsZSP(a);return 4;}
        case 0x2f:a=std::uint8_t(~a);return 4;
        case 0x32:{auto addr=fetch16();write(addr,a);return 13;}
        case 0x3a:{auto addr=fetch16();a=read(addr);return 13;}
        case 0x37:cy=true;return 4;
        case 0x3f:cy=!cy;return 4;
        case 0xc3:case 0xcb:pc=fetch16();return 10;
        case 0xc9:case 0xd9:pc=pop();return 10;
        case 0xcd:case 0xdd:case 0xed:case 0xfd:{auto addr=fetch16();push(pc);pc=addr;return 17;}
        case 0xd3:fetch();return 10; // OUT port placeholder: add I/O bus before game drivers
        case 0xdb:fetch();a=0xff;return 10; // IN port placeholder: unconnected bus
        case 0xe3:{auto lo=read(sp),hi=read(std::uint16_t(sp+1));write(sp,l);write(std::uint16_t(sp+1),h);l=lo;h=hi;return 18;}
        case 0xe9:pc=hl();return 5;
        case 0xeb:{auto oldD=d,oldE=e;d=h;e=l;h=oldD;l=oldE;return 5;}
        case 0xf3:inte_=false;return 4;
        case 0xf9:sp=hl();return 5;
        case 0xfb:inte_=true;return 4; // EI deferred enable to be implemented
        default:throw std::runtime_error("Unimplemented Intel 8080 opcode");
    }
}
}
