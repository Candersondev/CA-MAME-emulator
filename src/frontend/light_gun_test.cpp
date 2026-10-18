#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdio>
#include "ca_mame/input/LightGun.hpp"

int main() {
    if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)) {
        std::fprintf(stderr,"SDL initialization failed: %s\n",SDL_GetError());
        return 1;
    }
    SDL_Window* window=SDL_CreateWindow("CA-MAME - Light Gun Test",960,720,SDL_WINDOW_RESIZABLE);
    if(!window) {std::fprintf(stderr,"%s\n",SDL_GetError());SDL_Quit();return 1;}
    SDL_Renderer* renderer=SDL_CreateRenderer(window,nullptr);
    if(!renderer) {std::fprintf(stderr,"%s\n",SDL_GetError());SDL_DestroyWindow(window);SDL_Quit();return 1;}
    constexpr int width=640,height=480;
    SDL_SetRenderLogicalPresentation(renderer,width,height,SDL_LOGICAL_PRESENTATION_LETTERBOX);
    ca_mame::input::LightGun gun;
    bool running=true;
    bool trigger=false,reload=false;
    while(running) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type==SDL_EVENT_QUIT ||
               (event.type==SDL_EVENT_KEY_DOWN && event.key.key==SDLK_ESCAPE))running=false;
            if(event.type==SDL_EVENT_MOUSE_BUTTON_DOWN || event.type==SDL_EVENT_MOUSE_BUTTON_UP) {
                const bool down=event.type==SDL_EVENT_MOUSE_BUTTON_DOWN;
                if(event.button.button==SDL_BUTTON_LEFT)trigger=down;
                if(event.button.button==SDL_BUTTON_RIGHT)reload=down;
            }
        }
        float x=0,y=0;
        const auto buttons=SDL_GetMouseState(&x,&y);
        trigger=(buttons&SDL_BUTTON_LMASK)!=0;
        reload=(buttons&SDL_BUTTON_RMASK)!=0;
        SDL_RenderCoordinatesFromWindow(renderer,x,y,&x,&y);
        gun.update(x,y,width,height,trigger,reload);
        const auto& state=gun.state();
        char title[200];
        std::snprintf(title,sizeof(title),
            "CA-MAME Gun Test | X:%u Y:%u | %s | %s | %s | Esc quits",
            unsigned(state.x),unsigned(state.y),
            state.valid?"ON SCREEN":"OFF SCREEN",
            state.trigger?"FIRE":"READY",state.reload?"RELOAD":"");
        SDL_SetWindowTitle(window,title);
        SDL_SetRenderDrawColor(renderer,12,17,27,255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer,50,65,85,255);
        for(int i=0;i<5;++i) {
            SDL_FRect target={float(80+i*105),float(100+(i%2)*180),48,48};
            SDL_RenderRect(renderer,&target);
        }
        if(state.valid) {
            if(state.reload)SDL_SetRenderDrawColor(renderer,255,195,50,255);
            else if(state.trigger)SDL_SetRenderDrawColor(renderer,255,70,70,255);
            else SDL_SetRenderDrawColor(renderer,65,240,135,255);
            const float cx=float(state.x),cy=float(state.y);
            SDL_RenderLine(renderer,cx-18,cy,cx+18,cy);
            SDL_RenderLine(renderer,cx,cy-18,cx,cy+18);
            SDL_FRect center={cx-3,cy-3,6,6};
            SDL_RenderRect(renderer,&center);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(8);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
