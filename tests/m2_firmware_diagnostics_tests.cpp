#include "ca_mame/drivers/KonamiM2.hpp"
#include <cstdint>
int main(){
 ca_mame::drivers::KonamiM2 m;
 m.powerOnReset();
 if(m.snapshot(0).pc!=m.resetVector||m.snapshot(1).pc!=m.resetVector)return 1;
 m.cpu(0).setPC(0);
 m.cpu(1).setPC(0);
 // Two synthetic PowerPC instructions: addi r3,0,42; branch to self
 if(!m.loadWord(0,0x3860002au)||!m.loadWord(4,0x48000000u))return 2;
 if(m.runUntilFault(6)!=6)return 3;
 if(m.snapshot(0).faulted||m.snapshot(1).faulted)return 4;
 if(m.cpu(0).gpr(3)!=42||m.cpu(1).gpr(3)!=42)return 5;
 if(m.snapshot(0).cycles==0||m.snapshot(1).cycles==0)return 6;
 return 0;
}
