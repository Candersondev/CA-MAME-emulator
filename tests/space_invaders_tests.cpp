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
    expect(board.readMemory(0x4001)==0x76,"ROM mirrors at 0x4000");
    board.cpu().pc=0x2000;
    board.cpu().a=0xa5;
    // STA 0001: CPU must not overwrite the program ROM.
    const std::array<std::uint8_t,3> storeRom={0x32,0x01,0x00};
    expect(board.loadRom(0x100,storeRom),"load ROM store instruction");
    board.cpu().pc=0x100;
    board.cpu().halted=false;
    board.cpu().step();
    expect(board.readMemory(1)==0x76,"CPU cannot write ROM");
    // STA 6000 must mirror into RAM at 2000.
    const std::array<std::uint8_t,3> mirroredStore={0x32,0x00,0x60};
    expect(board.loadRom(0x110,mirroredStore),"load mirrored RAM store instruction");
    board.cpu().pc=0x110;
    board.cpu().a=0xa5;
    board.cpu().step();
    expect(board.readMemory(0x2000)==0xa5,"STA writes mapped RAM at 0x2000");
    expect(board.readMemory(0x6000)==0xa5,"RAM mirrors at 0x6000");
    if(board.readMemory(0x2000)!=0xa5 || board.readMemory(0x6000)!=0xa5)
        std::cerr<<"Diagnostic: PC="<<std::hex<<board.cpu().pc<<" A="<<unsigned(board.cpu().a)
                 <<" RAM2000="<<unsigned(board.readMemory(0x2000))
                 <<" RAM6000="<<unsigned(board.readMemory(0x6000))<<"\n";
    expect(board.runHalfFrame()>=16666,"half-frame cycle budget");
    board.reset();
    const auto pixels=board.frameBuffer();
    expect(pixels.size()==224*256,"video dimensions");
    expect(pixels[0]==0xff000000u,"blank video RAM renders black");
    return failures?1:0;
}
