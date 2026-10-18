#include "ca_mame/drivers/KonamiM2.hpp"
#include <cassert>
int main(){
    ca_mame::drivers::KonamiM2 board;
    constexpr std::uint32_t irq=ca_mame::drivers::M2MemoryMap::prototypeIrqBase;
    // CPU 0 enables interrupt line 2 using a guest store instruction.
    assert(board.loadWord(0,0x3c601000)); // addis r3,r0,0x1000
    assert(board.loadWord(4,0x38800004)); // addi r4,r0,4
    assert(board.loadWord(8,0x90830004)); // stw r4,4(r3)
    board.cpu(1).setPC(0x100);
    assert(board.loadWord(0x100,0x60000000)); // nop
    board.stepPair();board.stepPair();board.stepPair();
    std::uint32_t mask=0;
    assert(board.readWord(irq+4,mask) && mask==4);
    board.interrupts().raise(2);
    board.cpu(0).setMSR(ca_mame::PowerPC602::MSR_EE);
    board.stepPair();
    assert(board.cpu(0).pc()==0x500 && board.cpu(0).srr0()==12);
    assert(board.cpu(0).srr1()==ca_mame::PowerPC602::MSR_EE);
    assert((board.cpu(0).msr()&ca_mame::PowerPC602::MSR_EE)==0);
    assert(board.cpu(1).pc()==0x110);
    assert(board.loadWord(0x500,0x4c000064)); // rfi
    board.interrupts().clear(2);
    assert(board.loadWord(0x110,0x60000000));
    board.stepPair();
    assert(board.cpu(0).pc()==12);
    assert(board.cpu(0).msr()==ca_mame::PowerPC602::MSR_EE);
    assert(board.cpu(1).pc()==0x114);
    board.reset();
    assert(!board.interrupts().asserted(0));
}
