#include <array>
#include <cassert>
#include <cstdint>
#include "ca_mame/drivers/SpaceInvaders.hpp"
int main() {
    ca_mame::drivers::SpaceInvaders board;
    board.writePort(4,0xaa);
    board.writePort(4,0x55);
    board.writePort(2,2);
    assert(board.readPort(3)==0x56);
    board.setInput(1,0x91);
    assert(board.readPort(1)==0x91);
    std::array<std::uint8_t,2> rom={0x00,0x76};
    assert(board.loadRom(0,rom));
    assert(!board.loadRom(0x2000,rom));
    assert(board.readMemory(0)==0);
    assert(board.readMemory(1)==0x76);
    assert(board.runHalfFrame()>=16666);
    board.reset();
    auto pixels=board.frameBuffer();
    assert(pixels.size()==224*256);
    assert(pixels[0]==0xff000000u);
    return 0;
}
