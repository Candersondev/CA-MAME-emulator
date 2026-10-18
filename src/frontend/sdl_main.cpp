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
#include "ca_mame/drivers/SpaceInvaders.hpp"

namespace {
using Board=ca_mame::drivers::SpaceInvaders;
bool loadFile(Board& board,const std::string& path,std::uint16_t address) {
    std::ifstream in(path,std::ios::binary|std::ios::ate);
    if(!in) { std::cerr<<"Cannot open ROM: "<<path<<"\n";return false; }
    const auto length=in.tellg();
    if(length!=2048) {std::cerr<<"Expected 2048-byte ROM: "<<path<<"\n";return false;}
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
    std::uint8_t port1=0x08;
    std::uint8_t port2=0;
    setBit(port1,0x01,keys[SDL_SCANCODE_C]); // coin
    setBit(port1,0x04,keys[SDL_SCANCODE_1]); // start 1
    setBit(port1,0x02,keys[SDL_SCANCODE_2]); // start 2
    setBit(port1,0x10,keys[SDL_SCANCODE_SPACE]); // player 1 fire
    setBit(port1,0x20,keys[SDL_SCANCODE_LEFT]);
    setBit(port1,0x40,keys[SDL_SCANCODE_RIGHT]);
    setBit(port2,0x10,keys[SDL_SCANCODE_RCTRL]); // player 2 fire
    setBit(port2,0x20,keys[SDL_SCANCODE_A]);
    setBit(port2,0x40,keys[SDL_SCANCODE_D]);
    board.setInput(1,port1);board.setInput(2,port2);
}
}
int main(int argc,char** argv) {
    if(argc!=5) {
        std::cerr<<"Usage: ca-mame-sdl invaders.h invaders.g invaders.f invaders.e\n"
                    "Each ROM must be a legally obtained 2048-byte original Space Invaders ROM.\n";
        return 2;
    }
    Board board;
    for(int i=1;i<5;++i) {
        if(!loadFile(board,argv[i],static_cast<std::uint16_t>((i-1)*0x800)))return 1;
    }
    if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)) {
        std::cerr<<"SDL_Init: "<<SDL_GetError()<<"\n";return 1;
    }
    SDL_Window* window=SDL_CreateWindow("CA-MAME - Space Invaders",static_cast<int>(Board::Width*3),
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
    // Initial synthesized UFO hum for sound port 3 bit 0. Original sampled
    // arcade effects are not included; other sound edges remain available.
    SDL_AudioSpec audioSpec{};
    audioSpec.format=SDL_AUDIO_F32;
    audioSpec.channels=1;
    audioSpec.freq=44100;
    SDL_AudioStream* audio=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&audioSpec,nullptr,nullptr);
    if(audio) SDL_ResumeAudioStreamDevice(audio);
    else std::cerr<<"Audio unavailable: "<<SDL_GetError()<<"\n";
    double audioPhase=0.0;
    std::array<double,8> effectPhase{};
    std::array<int,8> effectSamples{};
    std::uint32_t noiseState=0x12345678u;
    constexpr double tau=6.283185307179586;
    constexpr int sampleRate=44100;
    constexpr int frameSamples=735;
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
        board.runHalfFrame();
        board.runHalfFrame();
        // Ports 3/5: one-shot sound triggers, with port 3 bit 0 as
        // the continuously running UFO oscillator. These are intentionally
        // synthesized approximations, not samples of the original hardware.
        const auto edges3=board.soundRisingEdges3();
        const auto edges5=board.soundRisingEdges5();
        const std::array<std::uint8_t,8> triggers3={0,0x02,0x04,0x08,0x10,0,0,0};
        const std::array<std::uint8_t,8> triggers5={0x01,0x02,0x04,0x08,0x10,0,0,0};
        for(int i=0;i<4;++i) {
            if(edges3&triggers3[i+1]) {effectSamples[i]=sampleRate*(i==0?1:2)/10;effectPhase[i]=0;}
        }
        for(int i=0;i<4;++i) {
            if(edges5&triggers5[i]) {effectSamples[i+4]=sampleRate/9;effectPhase[i+4]=0;}
        }
        if(audio) {
            // Never let the queued audio grow without bound when rendering
            // falls behind or the playback device stalls.
            constexpr int maxQueuedBytes=sampleRate*int(sizeof(float))/5;
            if(SDL_GetAudioStreamQueued(audio)<maxQueuedBytes) {
                std::array<float,frameSamples> samples{};
                const bool ufo=(board.soundPort3()&1)!=0;
                for(auto& sample:samples) {
                    double mixed=ufo?0.10*std::sin(audioPhase):0.0;
                    audioPhase+=tau*110.0/sampleRate;
                    if(audioPhase>=tau)audioPhase-=tau;
                    for(int i=0;i<8;++i) {
                        if(effectSamples[i]<=0)continue;
                        const double duration=sampleRate*(i<4?(i==0?0.1:0.2):(1.0/9.0));
                        const double envelope=std::min(1.0,double(effectSamples[i])/duration);
                        const double freq=i<4?(i==0?880.0:(i==1?160.0:95.0)):(140.0+60.0*(i-4));
                        noiseState^=noiseState<<13;
                        noiseState^=noiseState>>17;
                        noiseState^=noiseState<<5;
                        const double noise=(noiseState&65535u)/32767.5-1.0;
                        mixed+=0.10*envelope*(i==1||i==2?noise:std::sin(effectPhase[i]));
                        effectPhase[i]+=tau*freq/sampleRate;
                        if(effectPhase[i]>=tau)effectPhase[i]-=tau;
                        --effectSamples[i];
                    }
                    sample=static_cast<float>(std::clamp(mixed,-1.0,1.0));
                }
                if(!SDL_PutAudioStreamData(audio,samples.data(),static_cast<int>(samples.size()*sizeof(float))))
                    std::cerr<<"SDL audio queue: "<<SDL_GetError()<<"\\n";
            }
        }
        const auto pixels=board.frameBuffer();
        if(!SDL_UpdateTexture(texture,nullptr,pixels.data(),static_cast<int>(Board::Width*sizeof(std::uint32_t)))) {
            std::cerr<<"SDL_UpdateTexture: "<<SDL_GetError()<<"\n";break;
        }
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer,texture,nullptr,nullptr);
        SDL_RenderPresent(renderer);
        next+=std::chrono::microseconds(16667);
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
