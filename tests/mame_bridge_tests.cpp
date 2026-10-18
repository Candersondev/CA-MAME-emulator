#include "ca_mame/compat/MameBridge.hpp"
#include <string>
#include <sstream>
int main(){
 using ca_mame::compat::MameBridge;
 if(!MameBridge::validDriverName("evilngt")||MameBridge::validDriverName("../evilngt")||MameBridge::validDriverName("x;rm"))return 1;
 auto p=MameBridge::plan("mame","evilngt",std::filesystem::path("my roms"));
 if(!p||p->arguments.size()!=3||p->arguments[0]!="evilngt"||p->arguments[2]!="my roms")return 2;
 if(MameBridge::plan("mame","evil night"))return 3;
 auto verify=MameBridge::verifyRomsPlan("mame","evilngt",std::filesystem::path("my roms"));
 if(!verify||verify->arguments.size()!=4||verify->arguments[0]!="-verifyroms"||verify->arguments[1]!="evilngt")return 5;
 if(MameBridge::verifyRomsPlan("mame","../evilngt"))return 6;
 if(MameBridge::launch({{}, {}})!=-1)return 7;
 const std::string xml="<mame><machine name=\"evilngt\" runnable=\"yes\"><description>Evil Night</description><year>1998</year><manufacturer>Konami</manufacturer></machine><machine name=\"broken\" runnable=\"no\"><description>Test</description></machine></mame>";
 auto games=MameBridge::parseListXml(xml);
 if(games.size()!=2||games[0].name!="evilngt"||games[0].description!="Evil Night"||games[0].year!="1998"||!games[0].runnable||games[1].runnable)return 4;
 std::istringstream input(xml);
 const auto streamed=MameBridge::parseListXml(input);
 if(streamed.size()!=games.size()||streamed[0].name!="evilngt"||streamed[1].runnable)return 8;
 std::string large="<mame>";
 for(int i=0;i<5000;++i)large+="<machine name=\\\"game"+std::to_string(i)+"\\\"><description>Game</description></machine>";
 large+="</mame>";
 std::istringstream bulk(large);
 if(MameBridge::parseListXml(bulk).size()!=5000)return 9;
 return 0;
}
