#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "ca_mame/compat/GameLibrary.hpp"
#include "ca_mame/compat/MameBridge.hpp"
#include "ca_mame/compat/AuditStore.hpp"
#include "ca_mame/compat/LibraryIndex.hpp"
#include "ca_mame/compat/LibraryPreferences.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <future>
#include <chrono>
#include <unordered_map>
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
 ca_mame::compat::LibraryPreferences prefs;
 const auto prefsFile=std::filesystem::path(argv[2])/".ca-mame-preferences.tsv";
 prefs.load(prefsFile);
 if(!scan.directoryAccessible){std::cerr<<"ROM directory unavailable\n";return 2;}
 if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_GAMEPAD)){std::cerr<<SDL_GetError()<<"\n";return 1;}
 SDL_Window* window=SDL_CreateWindow("CA-MAME Game Library",1200,760,SDL_WINDOW_RESIZABLE);
 if(!window){std::cerr<<SDL_GetError()<<"\n";SDL_Quit();return 1;}
 SDL_Renderer* renderer=SDL_CreateRenderer(window,nullptr);
 if(!renderer){std::cerr<<SDL_GetError()<<"\n";SDL_DestroyWindow(window);SDL_Quit();return 1;}
 SDL_StartTextInput(window);
 SDL_Gamepad* gamepad=nullptr;
 int gamepadCount=0;
 SDL_JoystickID* gamepads=SDL_GetGamepads(&gamepadCount);
 if(gamepads){for(int i=0;i<gamepadCount;++i){gamepad=SDL_OpenGamepad(gamepads[i]);if(gamepad)break;}SDL_free(gamepads);}
 // Optional BMP artwork: <rom-directory>/artwork/<driver>.bmp
 SDL_Texture* artwork=nullptr;
 std::string artworkDriver;
 int artworkCategory=0;
 const char* artworkCategories[]={"snap","titles","cabinets"};
 std::string search;
 bool localOnly=true,running=true,favoritesOnly=false,recentOnly=false;
 std::size_t selected=0,offset=0;
 std::string feedback="F5 verifies ROM/BIOS/CHD requirements";
 std::string feedbackGame;
 std::unordered_map<std::string,std::string> diagnostics;
 std::future<ca_mame::compat::ProcessResult> verification;
 std::string pendingGame;
 std::future<int> gameProcess;
 std::string runningGame;
 std::unordered_map<std::string,std::size_t> diagnosticScroll;
 bool detailsExpanded=false;
 bool pendingLaunch=false;
 auto startVerification=[&](const std::string& game,bool launch){
  if(verification.valid()||gameProcess.valid()){feedbackGame=game;feedback="Another operation is already running";return;}
  const auto plan=ca_mame::compat::MameBridge::verifyRomsPlan(argv[3],game,std::filesystem::path(argv[2]));
  feedbackGame=game;
  if(!plan){feedback="BLOCKED: invalid verification command";return;}
  pendingGame=game;pendingLaunch=launch;
  diagnosticScroll[game]=0;
  feedback="Verifying with MAME in background...";
  verification=std::async(std::launch::async,[plan=*plan](){return ca_mame::compat::MameBridge::capture(plan);});
 };
 ca_mame::compat::LibraryIndex index(scan);
 auto filtered=index.find(search,localOnly);
 auto refine=[&](){filtered.erase(std::remove_if(filtered.begin(),filtered.end(),[&](std::size_t i){const auto& id=scan.entries[i].game.name;return (favoritesOnly&&!prefs.favorite(id))||(recentOnly&&std::find(prefs.recent().begin(),prefs.recent().end(),id)==prefs.recent().end());}),filtered.end());};
 refine();
 auto refresh=[&](){
  filtered=index.find(search,localOnly);
  refine();selected=0;offset=0;
 };
 auto launchSelected=[&](){
  if(filtered.empty())return;
  const auto& game=scan.entries[filtered[selected]].game;
  if(!game.runnable){feedbackGame=game.name;feedback="BLOCKED: MAME marks driver non-runnable";return;}
  startVerification(game.name,true);
 };
 while(running){
  SDL_Event e;
  while(SDL_PollEvent(&e)){
   if(e.type==SDL_EVENT_QUIT)running=false;
   if(e.type==SDL_EVENT_GAMEPAD_ADDED&&!gamepad)gamepad=SDL_OpenGamepad(e.gdevice.which);
   if(e.type==SDL_EVENT_GAMEPAD_REMOVED&&gamepad&&SDL_GetGamepadID(gamepad)==e.gdevice.which){SDL_CloseGamepad(gamepad);gamepad=nullptr;}
   if(e.type==SDL_EVENT_GAMEPAD_BUTTON_DOWN){
    if(e.gbutton.button==SDL_GAMEPAD_BUTTON_DPAD_LEFT)artworkCategory=(artworkCategory+2)%3;
    if(e.gbutton.button==SDL_GAMEPAD_BUTTON_DPAD_RIGHT)artworkCategory=(artworkCategory+1)%3;
    if(e.gbutton.button==SDL_GAMEPAD_BUTTON_DPAD_DOWN&&selected+1<filtered.size())++selected;
    if(e.gbutton.button==SDL_GAMEPAD_BUTTON_DPAD_UP&&selected>0)--selected;
    if(e.gbutton.button==SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER&&!filtered.empty())diagnosticScroll[scan.entries[filtered[selected]].game.name]+=5;
    if(e.gbutton.button==SDL_GAMEPAD_BUTTON_LEFT_SHOULDER&&!filtered.empty()){auto& scroll=diagnosticScroll[scan.entries[filtered[selected]].game.name];scroll=scroll>5?scroll-5:0;}
    if(e.gbutton.button==SDL_GAMEPAD_BUTTON_SOUTH)launchSelected();
    if(e.gbutton.button==SDL_GAMEPAD_BUTTON_WEST&&!filtered.empty()){prefs.toggleFavorite(scan.entries[filtered[selected]].game.name);prefs.save(prefsFile);if(favoritesOnly)refresh();}
   }
   if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&e.button.button==SDL_BUTTON_LEFT){
    int width=0,height=0;SDL_GetWindowSize(window,&width,&height);
    const int row=static_cast<int>((e.button.y-100.0f)/20.0f);
    if(e.button.x<static_cast<float>(std::max(380,width*58/100))&&e.button.y>=100&&row>=0&&row<std::max(1,(height-120)/20)&&offset+static_cast<std::size_t>(row)<filtered.size())selected=offset+static_cast<std::size_t>(row);
   }
   if(e.type==SDL_EVENT_MOUSE_WHEEL){
    if(e.wheel.y>0&&selected>0)--selected;
    if(e.wheel.y<0&&selected+1<filtered.size())++selected;
   }
   if(e.type==SDL_EVENT_TEXT_INPUT){search+=e.text.text;refresh();}
   if(e.type==SDL_EVENT_KEY_DOWN){
    switch(e.key.key){
     case SDLK_ESCAPE:running=false;break;
     case SDLK_BACKSPACE:if(!search.empty()){search.pop_back();refresh();}break;
     case SDLK_TAB:localOnly=!localOnly;refresh();break;
     case SDLK_LEFT:artworkCategory=(artworkCategory+2)%3;break;
     case SDLK_RIGHT:artworkCategory=(artworkCategory+1)%3;break;
     case SDLK_F:favoritesOnly=!favoritesOnly;recentOnly=false;refresh();break;
     case SDLK_R:recentOnly=!recentOnly;favoritesOnly=false;refresh();break;
     case SDLK_SPACE:if(!filtered.empty()){prefs.toggleFavorite(scan.entries[filtered[selected]].game.name);prefs.save(prefsFile);if(favoritesOnly)refresh();}break;
     case SDLK_DOWN:if(selected+1<filtered.size())++selected;break;
     case SDLK_UP:if(selected>0)--selected;break;
     case SDLK_PAGEDOWN:if(detailsExpanded&&!filtered.empty())diagnosticScroll[scan.entries[filtered[selected]].game.name]+=5;else selected=std::min(selected+20,filtered.empty()?std::size_t(0):filtered.size()-1);break;
     case SDLK_PAGEUP:if(detailsExpanded&&!filtered.empty()){auto& scroll=diagnosticScroll[scan.entries[filtered[selected]].game.name];scroll=scroll>5?scroll-5:0;}else selected=selected>20?selected-20:0;break;
     case SDLK_F6:detailsExpanded=!detailsExpanded;break;
     case SDLK_HOME:if(!filtered.empty())diagnosticScroll[scan.entries[filtered[selected]].game.name]=0;break;
     case SDLK_END:if(!filtered.empty())diagnosticScroll[scan.entries[filtered[selected]].game.name]=1000000;break;
     case SDLK_F5:
      if(!filtered.empty())startVerification(scan.entries[filtered[selected]].game.name,false);
      break;
     case SDLK_RETURN:case SDLK_KP_ENTER:launchSelected();break;
     default:break;
    }
   }
  }
  if(verification.valid()&&verification.wait_for(std::chrono::seconds(0))==std::future_status::ready){
   const auto result=verification.get();
   const std::string game=pendingGame;
   diagnostics[game]=result.output+(result.truncated?"\n[Output truncated]":"");
   audits.record(game,result.exitCode);audits.save(auditFile);
   feedbackGame=game;
   feedback=result.exitCode==0?"PASS: verified by MAME":("FAIL: MAME audit exit "+std::to_string(result.exitCode));
   if(pendingLaunch&&result.exitCode==0){
    const auto plan=ca_mame::compat::MameBridge::plan(argv[3],game,std::filesystem::path(argv[2]));
    if(plan){prefs.recordLaunch(game);prefs.save(prefsFile);
     runningGame=game;feedback="MAME running (launcher remains responsive)";
     gameProcess=std::async(std::launch::async,[plan=*plan](){return ca_mame::compat::MameBridge::launch(plan);});
    }else feedback="BLOCKED: invalid launch command";
   }
   pendingLaunch=false;
  }
  if(gameProcess.valid()&&gameProcess.wait_for(std::chrono::seconds(0))==std::future_status::ready){
   const int code=gameProcess.get();feedbackGame=runningGame;
   feedback=code==0?"MAME exited normally":("LAUNCH FAILED: MAME exit "+std::to_string(code));
   runningGame.clear();
  }
  int w=1200,h=760;SDL_GetWindowSize(window,&w,&h);
  const int listWidth=std::max(380,w*58/100);
  const int visible=std::max(1,(h-120)/20);
  if(selected<offset)offset=selected;
  if(selected>=offset+static_cast<std::size_t>(visible))offset=selected-visible+1;
  SDL_SetRenderDrawColor(renderer,14,18,29,255);SDL_RenderClear(renderer);
  SDL_SetRenderDrawColor(renderer,32,42,62,255);
  SDL_FRect header{0,0,static_cast<float>(w),88};SDL_RenderFillRect(renderer,&header);
  SDL_SetRenderDrawColor(renderer,235,241,250,255);
  const std::string title="CA-MAME ARCADE LIBRARY  /  "+std::to_string(filtered.size())+" games";
  SDL_RenderDebugText(renderer,16,12,title.c_str());
  SDL_RenderDebugText(renderer,16,32,("Search: "+search+"_").c_str());
  SDL_RenderDebugText(renderer,16,52,"TAB local | F favorites | R recent | SPACE star | ENTER/A verify+launch | F5 audit");
  SDL_RenderDebugText(renderer,16,68,"D-pad/arrows navigate | X favorite | click select | ESC exit");
  SDL_SetRenderDrawColor(renderer,26,33,48,255);
  SDL_FRect panel{static_cast<float>(listWidth),90,static_cast<float>(std::max(0,w-listWidth)),static_cast<float>(std::max(0,h-90))};
  SDL_RenderFillRect(renderer,&panel);
  for(int i=0;i<visible&&offset+static_cast<std::size_t>(i)<filtered.size();++i){
   const auto idx=offset+static_cast<std::size_t>(i);
   const auto& item=scan.entries[filtered[idx]];
   const float y=100.0f+20.0f*i;
   if(idx==selected){SDL_SetRenderDrawColor(renderer,49,89,145,255);SDL_FRect r{8,y-2,static_cast<float>(listWidth-16),19};SDL_RenderFillRect(renderer,&r);}
   const bool local=item.localArchive||item.hasChdDirectory;
   if(local)SDL_SetRenderDrawColor(renderer,130,231,177,255);
   else SDL_SetRenderDrawColor(renderer,175,185,204,255);
   const auto audit=audits.status(item.game.name);
   const std::string status=audit==ca_mame::compat::AuditState::Passed?"OK":audit==ca_mame::compat::AuditState::Failed?"BAD":"???";
   std::string line=(prefs.favorite(item.game.name)?"* ":"  ")+item.game.name+" ["+status+"] "+item.game.description;
   const auto limit=static_cast<std::size_t>(std::max(10,(listWidth-30)/8));
   if(line.size()>limit)line.resize(limit);
   SDL_RenderDebugText(renderer,14,y,line.c_str());
  }
  if(!filtered.empty()){
   const auto& item=scan.entries[filtered[selected]];
   const std::string artworkKey=item.game.name+":"+std::to_string(artworkCategory);
   if(artworkDriver!=artworkKey){
    if(artwork){SDL_DestroyTexture(artwork);artwork=nullptr;}
    artworkDriver=artworkKey;
    const auto folder=std::filesystem::path(argv[2])/"artwork"/artworkCategories[artworkCategory];
    for(const char* ext:{".png",".jpg",".jpeg",".bmp"}){
     const auto path=folder/(item.game.name+ext);
     std::error_code ec;
     if(!std::filesystem::is_regular_file(path,ec))continue;
     artwork=IMG_LoadTexture(renderer,path.string().c_str());
     if(artwork)break;
    }
    // Backward compatibility with original artwork/<driver>.bmp layout.
    if(!artwork&&artworkCategory==0){const auto oldPath=std::filesystem::path(argv[2])/"artwork"/(item.game.name+".bmp");artwork=IMG_LoadTexture(renderer,oldPath.string().c_str());}
   }
   const float x=static_cast<float>(listWidth+18);
   SDL_SetRenderDrawColor(renderer,245,246,252,255);
   SDL_RenderDebugText(renderer,x,106,"SELECTED GAME");
   SDL_RenderDebugText(renderer,x,130,item.game.name.c_str());
   const float available=static_cast<float>(std::max(80,w-listWidth-36));
   const auto clip=[&](const std::string& value){
    const auto limit=static_cast<std::size_t>(std::max(8,static_cast<int>(available)/8));
    return value.substr(0,limit);
   };
   SDL_RenderDebugText(renderer,x,152,clip(item.game.description).c_str());
   SDL_RenderDebugText(renderer,x,174,clip(item.game.manufacturer+"  |  "+item.game.year).c_str());
   SDL_RenderDebugText(renderer,x,196,(std::string("ROM files: ")+(item.localArchive||item.hasChdDirectory?"found":"not found")).c_str());
   const auto audit=audits.status(item.game.name);
   SDL_RenderDebugText(renderer,x,218,(std::string("Last audit: ")+(audit==ca_mame::compat::AuditState::Passed?"PASS":audit==ca_mame::compat::AuditState::Failed?"FAIL":"UNKNOWN")).c_str());
   SDL_RenderDebugText(renderer,x,240,(std::string("Favorite: ")+(prefs.favorite(item.game.name)?"YES":"NO")).c_str());
   SDL_RenderDebugText(renderer,x,262,(std::string("MAME runnable flag: ")+(item.game.runnable?"YES":"NO")).c_str());
   SDL_RenderDebugText(renderer,x,282,clip("Hardware driver: "+(item.game.sourceFile.empty()?std::string("unknown"):item.game.sourceFile)).c_str());
   SDL_RenderDebugText(renderer,x,302,clip("Clone parent: "+(item.game.parent.empty()?std::string("none"):item.game.parent)).c_str());
   SDL_RenderDebugText(renderer,x,322,clip("ROM/BIOS parent: "+(item.game.bios.empty()?std::string("none"):item.game.bios)).c_str());
   SDL_RenderDebugText(renderer,x,342,(std::string("ZIP archive: ")+(item.localArchive?"FOUND":"NOT FOUND")).c_str());
   SDL_RenderDebugText(renderer,x,358,(std::string("CHD directory: ")+(item.hasChdDirectory?"FOUND":"NOT FOUND")).c_str());
   SDL_RenderDebugText(renderer,x,374,(std::string("Artwork: ")+artworkCategories[artworkCategory]+"  [LEFT/RIGHT]").c_str());
   SDL_SetRenderDrawColor(renderer,feedbackGame==item.game.name?255:175,feedbackGame==item.game.name?190:200,150,255);
   SDL_RenderDebugText(renderer,x,396,clip(feedbackGame==item.game.name?feedback:"F5: verify selected game before launch").c_str());
   const auto detail=diagnostics.find(item.game.name);
   const float diagTop=416.0f;
   const int diagRows=detailsExpanded?std::max(5,(h-460)/15):5;
   if(detail!=diagnostics.end()){
    std::istringstream lines(detail->second);std::string line;
    std::vector<std::string> rows;
    while(std::getline(lines,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();rows.push_back(std::move(line));}
    auto& scroll=diagnosticScroll[item.game.name];
    const std::size_t maxScroll=rows.size()>static_cast<std::size_t>(diagRows)?rows.size()-diagRows:0;
    scroll=std::min(scroll,maxScroll);
    SDL_SetRenderDrawColor(renderer,215,224,240,255);
    for(int i=0;i<diagRows&&scroll+static_cast<std::size_t>(i)<rows.size();++i)
     SDL_RenderDebugText(renderer,x,diagTop+i*15,clip(rows[scroll+i]).c_str());
    SDL_RenderDebugText(renderer,x,diagTop+diagRows*15,clip("Diagnostics "+std::to_string(scroll+1)+"/"+std::to_string(rows.size())+" | F6 expand | PgUp/PgDn scroll").c_str());
   }
   const float artTop=detailsExpanded?static_cast<float>(h-90):(detail==diagnostics.end()?424.0f:520.0f);
   SDL_FRect artRect{x,artTop,std::max(1.0f,available),static_cast<float>(std::max(0,h-static_cast<int>(artTop)-32))};
   SDL_SetRenderDrawColor(renderer,19,25,38,255);SDL_RenderFillRect(renderer,&artRect);
   if(artwork&&artRect.h>20){
    float tw=0,th=0;
    if(SDL_GetTextureSize(artwork,&tw,&th)&&tw>0&&th>0){
     const float scale=std::min(artRect.w/tw,artRect.h/th);
     SDL_FRect dest{artRect.x+(artRect.w-tw*scale)/2,artRect.y+(artRect.h-th*scale)/2,tw*scale,th*scale};
     SDL_RenderTexture(renderer,artwork,nullptr,&dest);
    }
   }else if(artRect.h>50){
    SDL_SetRenderDrawColor(renderer,155,167,187,255);
    SDL_RenderDebugText(renderer,x+12,artRect.y+20,"NO ARTWORK FOUND");
    SDL_RenderDebugText(renderer,x+12,artRect.y+40,"artwork/<category>/<driver>.png");
   }
  }else if(artwork){SDL_DestroyTexture(artwork);artwork=nullptr;artworkDriver.clear();}
  SDL_RenderPresent(renderer);
  SDL_Delay(16);
 }
 if(artwork)SDL_DestroyTexture(artwork);
 if(gamepad)SDL_CloseGamepad(gamepad);
 SDL_StopTextInput(window);
 SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();
 return 0;
}
