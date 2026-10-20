#include <array>
#include <cstdint>
#include <iostream>
#include "ca_mame/cpu/Motorola6809.hpp"
#include "ca_mame/hardware/PIA6821.hpp"
#include "ca_mame/hardware/WilliamsVideo.hpp"
int main(){
 auto fail=[](int n){std::cerr<<"Williams expansion stage "<<n<<" failed\n";return n;};
 std::array<std::uint8_t,65536> ram{};
 ram[0xfffe]=0x80;ram[0xffff]=0x00;
 // LDX #$2000; LDA #$5a; STA ,X+; LDB ,-X; BRA -2
 const std::array<std::uint8_t,12> code={0x8e,0x20,0x00,0x86,0x5a,0xa7,0x80,0xe6,0x82,0x20,0xfe,0x12};
 for(unsigned i=0;i<code.size();++i)ram[0x8000+i]=code[i];
 ca_mame::Motorola6809 cpu(ram);
 if(cpu.step()!=3||cpu.x!=0x2000)return fail(1);
 if(cpu.step()!=2||cpu.a!=0x5a)return fail(2);
 if(cpu.step()!=6||cpu.x!=0x2001||ram[0x2000]!=0x5a)return fail(3);
 if(cpu.step()!=6||cpu.x!=0x2000||cpu.b!=0x5a)return fail(4);
 if(cpu.step()!=3||cpu.pc!=0x8009)return fail(5);
 ca_mame::hardware::PIA6821 pia;
 pia.write(0,0xf0); // DDR A
 pia.write(1,0x07); // enable port and rising CA1 IRQ
 pia.write(0,0xa0);pia.setPortA(0x0b);
 if(pia.read(0)!=0xab)return fail(6);
 pia.edgeCA1(true);
 if(!pia.irq() || !(pia.read(1)&0x80))return fail(7);
 pia.read(0);
 if(pia.irq())return fail(8);
 ca_mame::hardware::WilliamsVideo video;
 video.reset();video.setPalette(1,255,0,0);video.setPalette(2,0,255,0);
 video.write(0,0x12);
 if(video.pixel(0,0)!=0xffff0000u||video.pixel(1,0)!=0xff00ff00u)return fail(9);
 return 0;
}
