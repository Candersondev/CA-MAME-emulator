#include "ca_mame/drivers/KonamiM2.hpp"
#include <cassert>
int main(){
    ca_mame::drivers::KonamiM2 board;
    assert(board.loadWord(0,0x3860002a)); // addi r3,r0,42
    assert(board.loadWord(4,0x90600080)); // stw r3,128(r0)
    assert(board.loadWord(8,0x80800080)); // lwz r4,128(r0)
    board.cpu(1).setPC(8);
    board.stepPair();
    assert(board.cpu(0).gpr(3)==42);
    board.stepPair();
    board.stepPair();
    assert(board.cpu(1).gpr(4)==42);
    assert(board.cycles(0)==3 && board.cycles(1)==3);
    board.reset();
    assert(board.cycles(0)==0 && board.cpu(1).gpr(4)==0);
}
