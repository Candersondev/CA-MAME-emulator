#include "ca_mame/compat/LibraryPreferences.hpp"
#include "ca_mame/compat/MameBridge.hpp"
#include <algorithm>
#include <fstream>
#include <utility>
namespace ca_mame::compat {
bool LibraryPreferences::favorite(std::string_view name) const{return favorites_.contains(std::string(name));}
void LibraryPreferences::toggleFavorite(std::string_view name){
 if(!MameBridge::validDriverName(name))return;
 const std::string id(name);
 if(!favorites_.erase(id))favorites_.insert(id);
}
void LibraryPreferences::recordLaunch(std::string_view name){
 if(!MameBridge::validDriverName(name))return;
 const std::string id(name);
 recent_.erase(std::remove(recent_.begin(),recent_.end(),id),recent_.end());
 recent_.insert(recent_.begin(),id);
 if(recent_.size()>30)recent_.resize(30);
}
bool LibraryPreferences::load(const std::filesystem::path& file){
 std::ifstream in(file,std::ios::binary);if(!in)return false;
 std::string line;if(!std::getline(in,line)||line!="CA-MAME-PREFS-V1")return false;
 std::set<std::string> favorites;std::vector<std::string> recent;
 while(std::getline(in,line)){
  if(line.size()<3||line[1]!='\t'||!MameBridge::validDriverName(std::string_view(line).substr(2)))return false;
  auto id=line.substr(2);
  if(line[0]=='F')favorites.insert(std::move(id));
  else if(line[0]=='R'){
   if(std::find(recent.begin(),recent.end(),id)==recent.end()&&recent.size()<30)recent.push_back(std::move(id));
  }else return false;
 }
 if(in.bad())return false;
 favorites_=std::move(favorites);recent_=std::move(recent);return true;
}
bool LibraryPreferences::save(const std::filesystem::path& file) const{
 const auto temp=std::filesystem::path(file.string()+".tmp");
 {
  std::ofstream out(temp,std::ios::binary|std::ios::trunc);if(!out)return false;
  out<<"CA-MAME-PREFS-V1\n";
  for(const auto& id:favorites_)out<<"F\t"<<id<<'\n';
  for(const auto& id:recent_)out<<"R\t"<<id<<'\n';
  out.flush();if(!out)return false;
 }
 std::error_code ec;std::filesystem::rename(temp,file,ec);
 if(!ec)return true;
 std::filesystem::remove(file,ec);ec.clear();std::filesystem::rename(temp,file,ec);
 return !ec;
}
}
