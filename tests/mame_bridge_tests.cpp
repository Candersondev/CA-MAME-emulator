#include "ca_mame/compat/MameBridge.hpp"
#include <string>
int main(){
 using ca_mame::compat::MameBridge;
 if(!MameBridge::validDriverName("evilngt")||MameBridge::validDriverName("../evilngt")||MameBridge::validDriverName("x;rm"))return 1;
 auto p=MameBridge::plan("mame","evilngt",std::filesystem::path("my roms"));
 if(!p||p->arguments.size()!=3||p->arguments[0]!="evilngt"||p->arguments[2]!="my roms")return 2;
 if(MameBridge::plan("mame","evil night"))return 3;
 const std::string xml="<mame><machine name=\"evilngt\" runnable=\"yes\"><description>Evil Night</description><year>1998</year><manufacturer>Konami</manufacturer></machine><machine name=\"broken\" runnable=\"no\"><description>Test</description></machine></mame>";
 auto games=MameBridge::parseListXml(xml);
 if(games.size()!=2||games[0].name!="evilngt"||games[0].description!="Evil Night"||games[0].year!="1998"||!games[0].runnable||games[1].runnable)return 4;
 return 0;
}
