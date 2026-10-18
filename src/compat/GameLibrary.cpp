#include "ca_mame/compat/GameLibrary.hpp"
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <unordered_set>
#include <system_error>
namespace ca_mame::compat {
static std::string lower(std::string value){
 std::transform(value.begin(),value.end(),value.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
 return value;
}
LibraryScan GameLibrary::scan(const std::vector<Game>& catalog,const std::filesystem::path& romDirectory){
 LibraryScan scan;
 scan.entries.reserve(catalog.size());
 std::unordered_map<std::string,std::size_t> lookup;
 for(const auto& game:catalog){
  if(!MameBridge::validDriverName(game.name))continue;
  const auto id=lower(game.name);
  if(lookup.contains(id))continue;
  lookup.emplace(id,scan.entries.size());
  scan.entries.push_back({game});
 }
 std::error_code ec;
 if(!std::filesystem::is_directory(romDirectory,ec)||ec)return scan;
 scan.directoryAccessible=true;
 std::unordered_set<std::string> foundArchives;
 std::filesystem::directory_iterator it(romDirectory,std::filesystem::directory_options::skip_permission_denied,ec),end;
 while(!ec && it!=end){
  const auto& entry=*it;
  const auto filename=entry.path().filename().string();
  const auto ext=lower(entry.path().extension().string());
  std::error_code typeError;
  if(entry.is_regular_file(typeError) && !typeError && (ext==".zip"||ext==".7z")){
   ++scan.archiveCount;
   const auto id=lower(entry.path().stem().string());
   const auto found=lookup.find(id);
   if(found==lookup.end()){++scan.unknownArchives;}
   else if(foundArchives.insert(id).second){
    auto& item=scan.entries[found->second];item.localArchive=true;item.archivePath=entry.path();
   }
  }else if(entry.is_directory(typeError) && !typeError){
   const auto found=lookup.find(lower(filename));
   if(found!=lookup.end())scan.entries[found->second].hasChdDirectory=true;
  }
  it.increment(ec);
 }
 return scan;
}
std::vector<LibraryEntry> GameLibrary::filter(const LibraryScan& scan,std::string_view search,bool onlyLocal){
 const auto term=lower(std::string(search));
 std::vector<LibraryEntry> matches;
 for(const auto& entry:scan.entries){
  if(onlyLocal&&!entry.localArchive&&!entry.hasChdDirectory)continue;
  if(!term.empty()&&lower(entry.game.name).find(term)==std::string::npos&&
     lower(entry.game.description).find(term)==std::string::npos&&
     lower(entry.game.manufacturer).find(term)==std::string::npos)continue;
  matches.push_back(entry);
 }
 return matches;
}
}
