#include <array>
#include <cstdint>
#include <iostream>
#include "ca_mame/drivers/SpaceInvaders.hpp"
namespace {
int failures=0;
void expect(bool ok,const char* label) {
    if(!ok){std::cerr<<"FAIL: "<<label<<"\n";++failures;}
}
}
int main() {
    ca_mame::drivers::SpaceInvaders board;
    board.writePort(4,0xaa);
    board.writePort(4,0x55);
    board.writePort(2,2);
    expect(board.readPort(3)==0x56,"shift register high/low byte order and offset");
    board.writePort(2,0xff);
    expect(board.readPort(3)==0xd5,"shift offset masks to 3 bits");
    board.setInput(1,0x91);
    expect(board.readPort(1)==0x91,"input port 1");
    std::array<std::uint8_t,2> rom={0x00,0x76};
    expect(board.loadRom(0,rom),"valid ROM load");
    expect(!board.loadRom(0x2000,rom),"reject ROM outside program region");
    expect(!board.loadRom(0x1fff,rom),"reject ROM crossing ROM boundary");
    expect(board.readMemory(1)==0x76,"ROM contents");
    expect(board.runHalfFrame()>=16666,"half-frame cycle budget");
    board.reset();
    const auto pixels=board.frameBuffer();
    expect(pixels.size()==224*256,"video dimensions");
    expect(pixels[0]==0xff000000u,"blank video RAM renders black");
    return failures?1:0;
}
