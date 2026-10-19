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
    board.writePort(3,0x05);
    expect(board.soundPort3()==0x05 && board.soundRisingEdges3()==0x05,"sound port 3 rising edges");
    expect(board.soundRisingEdges3()==0,"sound port 3 edges clear after consumption");
    board.writePort(3,0x05);
    expect(board.soundRisingEdges3()==0,"held sound bits do not retrigger");
    board.writePort(3,0x00);
    board.writePort(3,0x04);
    expect(board.soundRisingEdges3()==0x04,"sound bit retriggers after falling edge");
    board.writePort(5,0x81);
    expect(board.soundPort5()==0x81 && board.soundRisingEdges5()==0x81,"sound port 5 rising edges");
    {
        const auto transitions=board.takeSoundTransitions();
        expect(!transitions.empty(),"sound line transitions recorded");
        bool sawRise=false,sawFall=false;
        for(const auto& event:transitions) {
            if(event.port==3 && event.bit==0 && event.active) sawRise=true;
            if(event.port==3 && event.bit==0 && !event.active) sawFall=true;
        }
        expect(sawRise && sawFall,"sound line captures both rising and falling edges");
        expect(board.takeSoundTransitions().empty(),"sound event queue drains once");
    }
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
    expect(board.readMemory(0x110)==0x32 && board.readMemory(0x111)==0x00 && board.readMemory(0x112)==0x60,"STA instruction bytes loaded correctly");
    board.cpu().pc=0x110;
    board.cpu().halted=false;
    board.cpu().a=0xa5;
    board.cpu().step();
    expect(board.readMemory(0x2000)==0xa5,"STA writes mapped RAM at 0x2000");
    expect(board.readMemory(0x6000)==0xa5,"RAM mirrors at 0x6000");
    if(board.readMemory(0x2000)!=0xa5 || board.readMemory(0x6000)!=0xa5)
        std::cerr<<"Diagnostic: PC="<<std::hex<<board.cpu().pc<<" A="<<unsigned(board.cpu().a)
                 <<" RAM2000="<<unsigned(board.readMemory(0x2000))
                 <<" ROM110="<<unsigned(board.readMemory(0x110))<<" ROM111="<<unsigned(board.readMemory(0x111))
                 <<" RAM6000="<<unsigned(board.readMemory(0x6000))<<"\n";
    expect(board.runHalfFrame()>=16666,"half-frame cycle budget");
    board.reset();
    expect(board.soundPort3()==0 && board.soundPort5()==0 && board.soundRisingEdges3()==0 && board.soundRisingEdges5()==0,"reset clears sound latches");
    // With a NOP-filled ROM, every instruction costs four cycles. Absolute
    // 120 Hz deadlines must retain fractional-cycle phase across half-frames.
    for(unsigned frame=1;frame<=120;++frame) {
        const auto prior=board.elapsedCycles();
        const auto executed=board.runHalfFrame();
        const auto deadline=std::uint64_t(frame)*ca_mame::drivers::SpaceInvaders::CpuHz/120;
        expect(board.elapsedHalfFrames()==frame,"half-frame counter increments once");
        expect(executed==board.elapsedCycles()-prior,"half-frame cycles match elapsed cycles");
        expect(board.elapsedCycles()>=deadline && board.elapsedCycles()<deadline+4,"half-frame timing stays within one NOP");
    }
    expect(board.elapsedCycles()==ca_mame::drivers::SpaceInvaders::CpuHz,"120 half-frames consume exactly one second of NOP cycles");
    board.reset();
    static const auto pixels=board.frameBuffer();
    expect(pixels.size()==224*256,"video dimensions");
    expect(pixels[0]==0xff000000u,"blank video RAM renders black");
    const std::array<std::uint8_t,3> videoStore={0x32,0x00,0x24};
    expect(board.loadRom(0x100,videoStore),"load video RAM store instruction");
    board.cpu().pc=0x100;
    board.cpu().a=0x81;
    board.cpu().step();
    static const auto video=board.frameBuffer();
    expect(video[255*224]==0xffffffffu,"video bit 0 maps to lower-left pixel");
    expect(video[248*224]==0xffffffffu,"video bit 7 maps to eighth pixel");
    expect(video[254*224]==0xff000000u,"unset video bits remain black");
    // Board-level interrupt delivery: each half-frame requests alternating
    // RST 1 (0x08) and RST 2 (0x10), acknowledged at the next CPU step.
    {
        ca_mame::drivers::SpaceInvaders irqBoard;
        const std::array<std::uint8_t,1> haltRom={0x76};
        expect(irqBoard.loadRom(0,haltRom),"load interrupt test HALT");
        irqBoard.cpu().sp=0x2400;
        irqBoard.cpu().setInterruptEnabled(true);
        const auto firstCycles=irqBoard.runHalfFrame();
        expect(firstCycles>=16666 && irqBoard.elapsedHalfFrames()==1,"first interrupt boundary at 120 Hz");
        expect(irqBoard.cpu().halted,"CPU remains halted until interrupt acknowledgment");
        expect(irqBoard.cpu().step()==11 && irqBoard.cpu().pc==0x08,"first board interrupt is RST 1");
        expect(!irqBoard.cpu().halted && !irqBoard.cpu().interruptsEnabled(),"RST 1 wakes CPU and masks interrupts");
        expect(irqBoard.cpu().sp==0x23fe && irqBoard.readMemory(0x23fe)==1 &&
               irqBoard.readMemory(0x23ff)==0,"RST 1 pushes HALT return address");
    }
    {
        ca_mame::drivers::SpaceInvaders irqBoard;
        const std::array<std::uint8_t,1> haltRom={0x76};
        expect(irqBoard.loadRom(0,haltRom),"load masked interrupt test HALT");
        irqBoard.cpu().sp=0x2400;
        irqBoard.runHalfFrame(); // RST 1 must be discarded while INTE is clear.
        expect(irqBoard.cpu().step()==4 && irqBoard.cpu().halted,"masked RST 1 does not wake HALT");
        irqBoard.cpu().setInterruptEnabled(true);
        irqBoard.runHalfFrame(); // Second boundary must request RST 2.
        expect(irqBoard.elapsedHalfFrames()==2,"second half-frame boundary reached");
        expect(irqBoard.cpu().step()==11 && irqBoard.cpu().pc==0x10,"second board interrupt is RST 2");
        expect(irqBoard.cpu().sp==0x23fe && irqBoard.readMemory(0x23fe)==1,
               "RST 2 pushes HALT return address");
    }
    // Ten seconds of deterministic ROM execution, without distributing
    // copyrighted game ROMs. JMP 0000 loops forever and costs 10 cycles.
    {
        ca_mame::drivers::SpaceInvaders soak;
        const std::array<std::uint8_t,3> loop={0xc3,0x00,0x00};
        expect(soak.loadRom(0,loop),"load synthetic long-running ROM");
        for(unsigned frame=1;frame<=1200;++frame) {
            soak.runHalfFrame();
            if(frame%120==0) {
                const auto deadline=std::uint64_t(frame)*ca_mame::drivers::SpaceInvaders::CpuHz/120;
                expect(soak.elapsedCycles()>=deadline && soak.elapsedCycles()<deadline+10,
                       "ten-second ROM execution maintains bounded cycle overshoot");
                expect(soak.cpu().pc==0 && !soak.cpu().halted,
                       "synthetic ROM continues executing without HALT");
            }
        }
        expect(soak.elapsedHalfFrames()==1200 && soak.elapsedCycles()==20000000,
               "ten emulated seconds finish with exact cycle budget");
    }
    // Execute a miniature ROM that reads real board inputs, writes video
    // RAM, and triggers a sound port through 8080 IN/OUT instructions.
    {
        ca_mame::drivers::SpaceInvaders gameplay;
        const std::array<std::uint8_t,11> program={
            0xdb,0x01,       // IN 1: read coin/start/fire/left/right
            0x32,0x00,0x24,  // STA 2400: update video RAM
            0xd3,0x03,       // OUT 3: sound control
            0xc3,0x00,0x00,  // JMP 0000
            0x00
        };
        expect(gameplay.loadRom(0,program),"load input-video-sound integration ROM");
        gameplay.setInput(1,0x11); // coin and fire
        gameplay.runHalfFrame();
        expect(gameplay.readMemory(0x2400)==0x11,"ROM reads input and writes video RAM");
        expect(gameplay.soundPort3()==0x11,"ROM drives sound control via OUT");
        expect(gameplay.soundRisingEdges3()==0x11,"ROM-generated sound events captured");
        static const auto rendered=gameplay.frameBuffer();
        expect(rendered[255*224]==0xffffffffu && rendered[251*224]==0xffffffffu,
               "input-driven ROM updates expected video pixels");
        gameplay.setInput(1,0x02); // second player start
        gameplay.runHalfFrame();
        expect(gameplay.readMemory(0x2400)==0x02 && gameplay.soundPort3()==0x02,
               "ROM responds to changing input during execution");
    }
    return failures?1:0;
}
