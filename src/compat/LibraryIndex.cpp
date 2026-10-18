#include "ca_mame/compat/LibraryIndex.hpp"
#include <algorithm>
#include <cctype>
namespace ca_mame::compat {
static std::string fold(std::string_view value){
 std::string result(value);
 std::transform(result.begin(),result.end(),result.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
 return result;
}
LibraryIndex::LibraryIndex(const LibraryScan& scan){
 records_.reserve(scan.entries.size());
 for(const auto& item:scan.entries)
  records_.push_back({fold(item.game.name+" "+item.game.description+" "+item.game.manufacturer+" "+item.game.year),
                      item.localArchive||item.hasChdDirectory});
}
std::vector<std::size_t> LibraryIndex::find(std::string_view query,bool localOnly) const{
 const auto term=fold(query);
 std::vector<std::size_t> indices;
 for(std::size_t i=0;i<records_.size();++i){
  const auto& item=records_[i];
  if(localOnly&&!item.local)continue;
  if(!term.empty()&&item.searchable.find(term)==std::string::npos)continue;
  indices.push_back(i);
 }
 return indices;
}
}
