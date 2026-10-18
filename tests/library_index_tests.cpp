#include "ca_mame/compat/LibraryIndex.hpp"
#include <string>
int main(){
 using namespace ca_mame::compat;
 LibraryScan scan;
 for(int i=0;i<12000;++i){
  LibraryEntry e;
  e.game.name="game"+std::to_string(i);
  e.game.description=i==9999?"Evil Night":"Arcade";
  e.game.manufacturer=i==9999?"Konami":"Other";
  e.game.year="1998";
  e.localArchive=(i%2==0);
  scan.entries.push_back(std::move(e));
 }
 LibraryIndex index(scan);
 const auto all=index.find("",false);
 if(all.size()!=12000)return 1;
 const auto local=index.find("",true);
 if(local.size()!=6000)return 2;
 const auto target=index.find("EVIL NIGHT",false);
 if(target.size()!=1||target[0]!=9999)return 3;
 if(!index.find("konami",true).empty())return 4;
 if(index.find("1998",false).size()!=12000)return 5;
 return 0;
}
