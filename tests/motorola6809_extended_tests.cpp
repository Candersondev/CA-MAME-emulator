#include <array>
#include <cstdint>
#include <iostream>
#include "ca_mame/cpu/Motorola6809.hpp"
int main(){
 auto fail=[](int n){std::cerr<<"6809 extended stage "<<n<<" failed\n";return n;};
 std::array<std::uint8_t,65536> ram{};
 ram[0xfffe]=0x80;ram[0xffff]=0x00;
 // LDX #$1234; LDS via LEAS ,X; LDA #$56; PSHS A,X;
 // CLRA; PULS A,X; CMPA #$56; BNE skip; BRA -2.
 const std::array<std::uint8_t,18> program={
  0x8e,0x12,0x34,0x32,0x84,0x86,0x56,0x34,0x12,
  0x4f,0x35,0x12,0x81,0x56,0x26,0x02,0x20,0xfe
 };
 for(unsigned i=0;i<program.size();++i)ram[0x8000+i]=program[i];
 ca_mame::Motorola6809 cpu(ram);
 if(cpu.step()!=3||cpu.x!=0x1234)return fail(1);
 if(cpu.step()!=4||cpu.s!=0x1234)return fail(2);
 if(cpu.step()!=2||cpu.a!=0x56)return fail(3);
 if(cpu.step()!=8||cpu.s!=0x1231)return fail(4);
 if(cpu.step()!=2||cpu.a!=0)return fail(5);
 if(cpu.step()!=8||cpu.a!=0x56||cpu.x!=0x1234||cpu.s!=0x1234)return fail(6);
 if(cpu.step()!=2||(cpu.cc&4)==0)return fail(7);
 if(cpu.step()!=3||cpu.pc!=0x8010)return fail(8);
 if(cpu.step()!=3||cpu.pc!=0x8010)return fail(9);
 return 0;
}
