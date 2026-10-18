#include <SDL3/SDL.h>
#include "ca_mame/compat/GameLibrary.hpp"
#include "ca_mame/compat/MameBridge.hpp"
#include "ca_mame/compat/AuditStore.hpp"
#include "ca_mame/compat/LibraryIndex.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main(int argc,char** argv){
 if(argc<4){std::cerr<<"Usage: ca-mame-library <catalog.xml> <rom-directory> <mame-executable>\n";return 2;}
 std::ifstream file(argv[1],std::ios::binary);
 if(!file){std::cerr<<"Cannot read catalog\n";return 2;}
 auto games=ca_mame::compat::MameBridge::parseListXml(file);
 auto scan=ca_mame::compat::GameLibrary::scan(games,argv[2]);
 ca_mame::compat::AuditStore audits;
 const auto auditFile=std::filesystem::path(argv[2])/".ca-mame-audits.tsv";
 audits.load(auditFile);
 if(!scan.directoryAccessible){std::cerr<<"ROM directory unavailable\n";return 2;}
 if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)){std::cerr<<SDL_GetError()<<"\n";return 1;}
 SDL_Window* window=SDL_CreateWindow("CA-MAME Game Library",1000,700,SDL_WINDOW_RESIZABLE);
 if(!window){std::cerr<<SDL_GetError()<<"\n";SDL_Quit();return 1;}
 SDL_Renderer* renderer=SDL_CreateRenderer(window,nullptr);
 if(!renderer){std::cerr<<SDL_GetError()<<"\n";SDL_DestroyWindow(window);SDL_Quit();return 1;}
 SDL_StartTextInput(window);
 std::string search;
 bool localOnly=true,running=true;
 std::size_t selected=0,offset=0;
 ca_mame::compat::LibraryIndex index(scan);
 auto filtered=index.find(search,localOnly);
 auto refresh=[&](){
  filtered=index.find(search,localOnly);
  selected=0;offset=0;
 };
 while(running){
  SDL_Event e;
  while(SDL_PollEvent(&e)){
   if(e.type==SDL_EVENT_QUIT)running=false;
   if(e.type==SDL_EVENT_TEXT_INPUT){search+=e.text.text;refresh();}
   if(e.type==SDL_EVENT_KEY_DOWN){
    switch(e.key.key){
     case SDLK_ESCAPE:running=false;break;
     case SDLK_BACKSPACE:if(!search.empty()){search.pop_back();refresh();}break;
     case SDLK_TAB:localOnly=!localOnly;refresh();break;
     case SDLK_DOWN:if(selected+1<filtered.size())++selected;break;
     case SDLK_UP:if(selected>0)--selected;break;
     case SDLK_PAGEDOWN:selected=std::min(selected+20,filtered.empty()?std::size_t(0):filtered.size()-1);break;
     case SDLK_PAGEUP:selected=selected>20?selected-20:0;break;
     case SDLK_F5:
      if(!filtered.empty()){
       const auto plan=ca_mame::compat::MameBridge::verifyRomsPlan(argv[3],scan.entries[filtered[selected]].game.name,std::filesystem::path(argv[2]));
       if(plan){const int code=ca_mame::compat::MameBridge::launch(*plan);
        audits.record(filtered[selected].game.name,code);
        if(!audits.save(auditFile))std::cerr<<"Failed to save audit status\\n";
        std::cerr<<"MAME ROM verification exit status: "<<code<<"\\n";}
      }break;
     case SDLK_RETURN:case SDLK_KP_ENTER:
      if(!filtered.empty()){
       const auto plan=ca_mame::compat::MameBridge::plan(argv[3],filtered[selected].game.name,std::filesystem::path(argv[2]));
       if(plan){SDL_HideWindow(window);const int result=ca_mame::compat::MameBridge::launch(*plan);
        std::cerr<<"External MAME exit status: "<<result<<"\n";SDL_ShowWindow(window);}
      }break;
     default:break;
    }
   }
  }
  int w=1000,h=700;SDL_GetWindowSize(window,&w,&h);
  const int visible=std::max(1,(h-90)/18);
  if(selected<offset)offset=selected;
  if(selected>=offset+static_cast<std::size_t>(visible))offset=selected-visible+1;
  SDL_SetRenderDrawColor(renderer,15,19,29,255);SDL_RenderClear(renderer);
  SDL_SetRenderDrawColor(renderer,230,235,244,255);
  const std::string title="CA-MAME LIBRARY  |  "+std::to_string(filtered.size())+" games  |  "+(localOnly?"LOCAL":"ALL");
  SDL_RenderDebugText(renderer,18,12,title.c_str());
  const std::string query="Search: "+search+"_";
  SDL_RenderDebugText(renderer,18,32,query.c_str());
  SDL_RenderDebugText(renderer,18,52,"Type to search | TAB filter | arrows select | ENTER launch | F5 verify | ESC exit");
  for(int i=0;i<visible&&offset+static_cast<std::size_t>(i)<filtered.size();++i){
   const auto idx=offset+static_cast<std::size_t>(i);
   const auto& item=scan.entries[filtered[idx]];
   const float y=80.0f+18.0f*i;
   if(idx==selected){SDL_SetRenderDrawColor(renderer,48,84,130,255);SDL_FRect r{12,y-2,static_cast<float>(w-24),17};SDL_RenderFillRect(renderer,&r);}
   const bool local=item.localArchive||item.hasChdDirectory;
   if(local)SDL_SetRenderDrawColor(renderer,120,230,160,255);
   else SDL_SetRenderDrawColor(renderer,175,180,190,255);
   const auto audit=audits.status(item.game.name);
   const std::string auditLabel=audit==ca_mame::compat::AuditState::Passed?"[PASS] ":
                                audit==ca_mame::compat::AuditState::Failed?"[FAIL] ":"[????] ";
   std::string line=auditLabel+(local?"[LOCAL] ":"[-----] ")+item.game.name+" | "+item.game.description+" | "+item.game.year+" | "+item.game.manufacturer;
   const std::size_t maxChars=static_cast<std::size_t>(std::max(10,(w-35)/8));
   if(line.size()>maxChars)line.resize(maxChars);
   SDL_RenderDebugText(renderer,18,y,line.c_str());
  }
  SDL_RenderPresent(renderer);
  SDL_Delay(16);
 }
 SDL_StopTextInput(window);
 SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();
 return 0;
}
