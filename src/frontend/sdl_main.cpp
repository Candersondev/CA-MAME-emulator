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
#include "ca_mame/drivers/GunFight.hpp"
#include "ca_mame/audio/GunFightAudio.hpp"
#include "ca_mame/rom/NativeGameCatalog.hpp"
#include "ca_mame/rom/NativeGameLoader.hpp"
#include "ca_mame/audio/InvadersAudio.hpp"
#include "ca_mame/audio/InvadersFrameClock.hpp"

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
int runInvaders(Board& board);
int runGunFight(ca_mame::drivers::GunFight& board);
int main(int argc,char** argv) {
    // Unified native launcher: --game <id> <rom-directory>, or a ROM
    // directory whose final component matches a supported game ID.
    if(argc==4 && std::string_view(argv[1])=="--game") {
        const auto* game=ca_mame::rom::findNativeGame(argv[2]);
        if(!game) {std::cerr<<"Unsupported native game: "<<argv[2]<<"\\n";return 2;}
        if(game->id=="gunfight") {
            ca_mame::drivers::GunFight gun;
            if(!ca_mame::rom::loadNativeGame(gun,*game,argv[3])) {
                std::cerr<<"Missing or invalid Gun Fight ROM files\\n";return 2;
            }
            return runGunFight(gun);
        }
        Board invaders;
        invaders.setAccurateTiming(true);
        if(!ca_mame::rom::loadNativeGame(invaders,*game,argv[3])) {
            std::cerr<<"Missing or invalid Space Invaders ROM files\\n";return 2;
        }
        return runInvaders(invaders);
    }
    if(argc!=5) {
        std::cerr<<"Usage: ca-mame-sdl --game invaders|gunfight ROM_DIRECTORY\\n"
                    "Legacy: ca-mame-sdl invaders.h invaders.g invaders.f invaders.e\\n";
        return 2;
    }
    Board board;
    board.setAccurateTiming(true);
    for(int i=1;i<5;++i)
        if(!loadFile(board,argv[i],static_cast<std::uint16_t>((i-1)*0x800)))return 1;
    return runInvaders(board);
}
int runInvaders(Board& board) {
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
    // PCM synthesis is driven by board sound-control transitions.
    SDL_AudioSpec audioSpec{};
    audioSpec.format=SDL_AUDIO_F32;
    audioSpec.channels=1;
    audioSpec.freq=44100;
    SDL_AudioStream* audio=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&audioSpec,nullptr,nullptr);
    if(audio) SDL_ResumeAudioStreamDevice(audio);
    else std::cerr<<"Audio unavailable: "<<SDL_GetError()<<"\n";
    ca_mame::audio::InvadersAudio soundGenerator;
    constexpr int sampleRate=static_cast<int>(ca_mame::audio::InvadersAudio::SampleRate);
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
        // Apply all board control-line transitions, including falling edges.
        // Audio is generated independently of the video frontend.
        const auto transitions=board.takeSoundTransitions();
        std::vector<ca_mame::audio::InvadersSoundEvent> audioEvents;
        audioEvents.reserve(transitions.size());
        for(const auto& transition:transitions)
            audioEvents.push_back(ca_mame::audio::decodeInvadersEvent(transition));
        // Preserve the legacy rising-edge API for other clients, but drain
        // it here so frontend consumers do not accumulate stale events.
        board.soundRisingEdges3();
        board.soundRisingEdges5();
        ++frameNumber;
        const auto frameSampleCount=FrameClock::sampleDeadline(frameNumber)
            -FrameClock::sampleDeadline(frameNumber-1);
        // Always advance the synthesizer for the emulated frame, even if
        // SDL's playback queue is full. Skipping renderTimed would silently
        // discard control transitions and freeze oscillator/envelope time.
        std::vector<float> samples(static_cast<std::size_t>(frameSampleCount));
        soundGenerator.renderTimed(samples,audioEvents,frameStart,frameEnd);
        if(audio) {
            constexpr int maxQueuedBytes=sampleRate*int(sizeof(float))/5;
            const auto queued=SDL_GetAudioStreamQueued(audio);
            if(queued>=0 && queued<maxQueuedBytes) {
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

int runGunFight(ca_mame::drivers::GunFight& board) {
    using Gun=ca_mame::drivers::GunFight;
    if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS))return 1;
    SDL_Window* window=SDL_CreateWindow("CA-MAME - Gun Fight",Gun::Width*3,Gun::Height*3,SDL_WINDOW_RESIZABLE);
    if(!window){SDL_Quit();return 1;}
    SDL_Renderer* renderer=SDL_CreateRenderer(window,nullptr);
    SDL_Texture* texture=renderer?SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,Gun::Width,Gun::Height):nullptr;
    if(!texture){if(renderer)SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();return 1;}
    SDL_SetTextureScaleMode(texture,SDL_SCALEMODE_NEAREST);
    SDL_SetRenderLogicalPresentation(renderer,Gun::Width,Gun::Height,SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_AudioSpec spec{};spec.format=SDL_AUDIO_F32;spec.channels=1;spec.freq=44100;
    SDL_AudioStream* audio=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,nullptr,nullptr);
    if(audio)SDL_ResumeAudioStreamDevice(audio);
    ca_mame::audio::GunFightAudio synth;
    using Clock=ca_mame::audio::InvadersFrameClock;
    std::uint64_t frames=0;
    auto next=std::chrono::steady_clock::now();
    bool running=true;
    while(running) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type==SDL_EVENT_QUIT ||
               (event.type==SDL_EVENT_KEY_DOWN && event.key.key==SDLK_ESCAPE))running=false;
        }
        if(!running)break;
        SDL_PumpEvents();
        const bool* keys=SDL_GetKeyboardState(nullptr);
        std::uint8_t p1=0,p2=0;
        setBit(p1,0x01,keys[SDL_SCANCODE_UP]);
        setBit(p1,0x02,keys[SDL_SCANCODE_DOWN]);
        setBit(p1,0x04,keys[SDL_SCANCODE_LEFT]);
        setBit(p1,0x08,keys[SDL_SCANCODE_RIGHT]);
        setBit(p1,0x80,keys[SDL_SCANCODE_RCTRL]);
        setBit(p2,0x01,keys[SDL_SCANCODE_W]);
        setBit(p2,0x02,keys[SDL_SCANCODE_S]);
        setBit(p2,0x04,keys[SDL_SCANCODE_A]);
        setBit(p2,0x08,keys[SDL_SCANCODE_D]);
        setBit(p2,0x80,keys[SDL_SCANCODE_LCTRL]);
        board.setInput(0,p1);board.setInput(1,p2);
        board.setInput(2,keys[SDL_SCANCODE_C]?0x40:0);
        board.runFrame();
        synth.write(board.soundPort());
        board.takeSoundRisingEdges();
        ++frames;
        const auto count=Clock::sampleDeadline(frames)-Clock::sampleDeadline(frames-1);
        std::vector<float> samples(static_cast<std::size_t>(count));
        synth.render(samples);
        if(audio && SDL_GetAudioStreamQueued(audio)<44100*int(sizeof(float))/5)
            SDL_PutAudioStreamData(audio,samples.data(),static_cast<int>(samples.size()*sizeof(float)));
        const auto pixels=board.frameBuffer();
        SDL_UpdateTexture(texture,nullptr,pixels.data(),Gun::Width*sizeof(std::uint32_t));
        SDL_RenderClear(renderer);SDL_RenderTexture(renderer,texture,nullptr,nullptr);SDL_RenderPresent(renderer);
        next+=std::chrono::nanoseconds(Clock::nanosecondDeadline(frames)-Clock::nanosecondDeadline(frames-1));
        const auto now=std::chrono::steady_clock::now();
        if(next>now)std::this_thread::sleep_until(next);
        else if(now-next>std::chrono::milliseconds(100))next=now;
    }
    if(audio)SDL_DestroyAudioStream(audio);
    SDL_DestroyTexture(texture);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();
    return 0;
}
