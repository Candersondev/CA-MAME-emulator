#include "ca_mame/drivers/KonamiM2.hpp"
#include <cassert>
#include <cstdint>
int main(){
    ca_mame::drivers::KonamiM2 board;
    board.powerOnReset();
    assert(board.cpu(0).pc()==0xfff00100u);
    assert(board.cpu(1).pc()==0xfff00100u);
    assert(board.cpu(0).msr()==0x40u);
    // Synthetic ROM at high reset alias, no copyrighted firmware.
    auto& cpu=board.cpu(0);
    // addi r3,r0,0x1234
    std::uint32_t resetWord=0;
    assert(board.readWord(0xfff00100u,resetWord)); // alias mapped
    cpu.setPC(0);
    assert(board.loadWord(0,0x38601234u));
    assert(board.loadWord(4,0x38800080u)); // addi r4,r0,128
    assert(board.loadWord(8,(44u<<26)|(3u<<21)|(4u<<16))); // sth r3,0(r4)
    assert(board.loadWord(12,(40u<<26)|(5u<<21)|(4u<<16))); // lhz r5,0(r4)
    assert(board.loadWord(16,(38u<<26)|(3u<<21)|(4u<<16)|2u)); // stb r3,2(r4)
    assert(board.loadWord(20,(34u<<26)|(6u<<21)|(4u<<16)|2u)); // lbz r6,2(r4)
    for(int i=0;i<6;++i)assert(cpu.step()==1);
    assert(cpu.gpr(5)==0x1234u && cpu.gpr(6)==0x34u);
}
