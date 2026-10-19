#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "ca_mame/drivers/GunFight.hpp"
#include "ca_mame/audio/GunFightAudio.hpp"
#include "ca_mame/audio/InvadersFrameClock.hpp"

namespace {
using Board=ca_mame::drivers::GunFight;
bool loadFile(Board& board,const std::string& path,std::uint16_t address) {
    std::ifstream in(path,std::ios::binary|std::ios::ate);
    if(!in) { std::cerr<<"Cannot open ROM: "<<path<<"\n";return false; }
    const auto length=in.tellg();
    if(length!=1024) {std::cerr<<"Expected 1024-byte Gun Fight ROM: "<<path<<"\n";return false;}
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
    in.seekg(0);
    if(!in.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()))) {
        std::cerr<<"Failed to read ROM: "<<path<<"\n";return false;
    }
    if(!board.loadRom(address,bytes)) {std::cerr<<"ROM outside 0x0000-0x1fff: "<<path<<"\n";return false;}
    return true;
}
void setBit(std::uint8_t& value,std::uint8_t mask,bool pressed) {
    if(pressed)value|=mask;else value&=std::uint8_t(~mask);
}
void updateInputs(Board& board) {
    const bool* keys=SDL_GetKeyboardState(nullptr);
    std::uint8_t port1=0;
    std::uint8_t port2=0;
    setBit(port1,0x01,keys[SDL_SCANCODE_W]); // player 2 up
    setBit(port1,0x02,keys[SDL_SCANCODE_S]); // player 2 down
    setBit(port1,0x04,keys[SDL_SCANCODE_A]); // player 2 left
    setBit(port1,0x08,keys[SDL_SCANCODE_D]); // player 2 right
    setBit(port1,0x80,keys[SDL_SCANCODE_LCTRL]); // player 2 fire
    setBit(port2,0x01,keys[SDL_SCANCODE_UP]); // player 1 up
    setBit(port2,0x02,keys[SDL_SCANCODE_DOWN]);
    setBit(port2,0x04,keys[SDL_SCANCODE_LEFT]);
    setBit(port2,0x08,keys[SDL_SCANCODE_RIGHT]);
    setBit(port2,0x80,keys[SDL_SCANCODE_RCTRL]);
    board.setInput(0,port2);board.setInput(1,port1);
    board.setInput(2,keys[SDL_SCANCODE_C]?0x40:0);
}
}
int main(int argc,char** argv) {
    if(argc!=5) {
        std::cerr<<"Usage: ca-mame-gunfight-sdl 7609h.bin 7609g.bin 7609f.bin 7609e.bin\n"
                    "Each ROM must be a legally obtained 1024-byte Gun Fight ROM.\n";
        return 2;
    }
    Board board;
    for(int i=1;i<5;++i) {
        if(!loadFile(board,argv[i],static_cast<std::uint16_t>((i-1)*0x400)))return 1;
    }
    if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)) {
        std::cerr<<"SDL_Init: "<<SDL_GetError()<<"\n";return 1;
    }
    SDL_Window* window=SDL_CreateWindow("CA-MAME - Gun Fight",static_cast<int>(Board::Width*3),
                                       static_cast<int>(Board::Height*3),SDL_WINDOW_RESIZABLE);
    if(!window) {std::cerr<<SDL_GetError()<<"\n";SDL_Quit();return 1;}
    SDL_Renderer* renderer=SDL_CreateRenderer(window,nullptr);
    if(!renderer) {std::cerr<<SDL_GetError()<<"\n";SDL_DestroyWindow(window);SDL_Quit();return 1;}
    SDL_Texture* texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,
                                          static_cast<int>(Board::Width),static_cast<int>(Board::Height));
    if(!texture) {std::cerr<<SDL_GetError()<<"\n";SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 1;}
    SDL_SetTextureScaleMode(texture,SDL_SCALEMODE_NEAREST);
    SDL_SetRenderLogicalPresentation(renderer,static_cast<int>(Board::Width),static_cast<int>(Board::Height),
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);
    // PCM synthesis is driven by board sound-control transitions.
    SDL_AudioSpec audioSpec{};
    audioSpec.format=SDL_AUDIO_F32;
    audioSpec.channels=1;
    audioSpec.freq=44100;
    SDL_AudioStream* audio=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&audioSpec,nullptr,nullptr);
    if(audio) SDL_ResumeAudioStreamDevice(audio);
    else std::cerr<<"Audio unavailable: "<<SDL_GetError()<<"\n";
    ca_mame::audio::GunFightAudio soundGenerator;
    constexpr int sampleRate=static_cast<int>(ca_mame::audio::GunFightAudio::SampleRate);
    using FrameClock=ca_mame::audio::InvadersFrameClock;
    std::uint64_t frameNumber=0;
    bool running=true;
    auto next=std::chrono::steady_clock::now();
    while(running) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type==SDL_EVENT_QUIT)running=false;
            if(event.type==SDL_EVENT_KEY_DOWN && event.key.key==SDLK_ESCAPE)running=false;
        }
        if(!running)break;
        SDL_PumpEvents();
        updateInputs(board);
        const auto frameStart=board.elapsedCycles();
        board.runHalfFrame();
        board.runHalfFrame();
        const auto frameEnd=board.elapsedCycles();
        soundGenerator.write(board.soundPort());
        board.takeSoundRisingEdges();
        ++frameNumber;
        const auto frameSampleCount=FrameClock::sampleDeadline(frameNumber)
            -FrameClock::sampleDeadline(frameNumber-1);
        std::vector<float> samples(static_cast<std::size_t>(frameSampleCount));
        soundGenerator.render(samples);
        if(audio && SDL_GetAudioStreamQueued(audio)<sampleRate*int(sizeof(float))/5)
            SDL_PutAudioStreamData(audio,samples.data(),static_cast<int>(samples.size()*sizeof(float)));
        const auto pixels=board.frameBuffer();
        if(!SDL_UpdateTexture(texture,nullptr,pixels.data(),static_cast<int>(Board::Width*sizeof(std::uint32_t)))) {
            std::cerr<<"SDL_UpdateTexture: "<<SDL_GetError()<<"\n";break;
        }
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer,texture,nullptr,nullptr);
        SDL_RenderPresent(renderer);
        next=next+std::chrono::nanoseconds(
            FrameClock::nanosecondDeadline(frameNumber)
            -FrameClock::nanosecondDeadline(frameNumber-1));
        const auto now=std::chrono::steady_clock::now();
        if(next>now)std::this_thread::sleep_until(next);
        else if(now-next>std::chrono::milliseconds(100))next=now;
    }
    if(audio) SDL_DestroyAudioStream(audio);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
