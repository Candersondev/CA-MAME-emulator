#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <chrono>
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
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
