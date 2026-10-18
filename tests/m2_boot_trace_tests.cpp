#include "ca_mame/drivers/KonamiM2.hpp"
#include <cstdint>
int main(){
 ca_mame::drivers::KonamiM2 m;
 m.powerOnReset();
 m.cpu(0).setPC(0);
 m.cpu(1).setPC(0);
 if(!m.loadWord(0,0x3860002au)||!m.loadWord(4,0x48000000u))return 1;
 auto t=m.traceBoot(12,2,3);
 if(t.executedPairs!=12||t.stoppedOnFault||t.entries.size()!=3)return 2;
 if(t.entries[0].pair!=0||t.entries[0].pc0!=0)return 3;
 if(t.entries[1].pair!=2||t.entries[1].pc0!=4)return 4;
 if(m.cpu(0).gpr(3)!=42||m.cpu(1).gpr(3)!=42)return 5;
 m.reset();
 m.cpu(0).setPC(0xdead0000u);
 m.cpu(1).setPC(0xdead0000u);
 auto f=m.traceBoot(10);
 if(!f.stoppedOnFault||f.executedPairs==0||f.executedPairs>10)return 6;
 if(f.entries.empty()||!f.entries.back().fault0||!f.entries.back().fault1)return 7;
 return 0;
}
