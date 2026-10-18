#include "ca_mame/compat/MameBridge.hpp"
#include <algorithm>
#include <cctype>
namespace ca_mame::compat {
bool MameBridge::validDriverName(std::string_view name) noexcept {
 return !name.empty() && name.size()<=64 && std::all_of(name.begin(),name.end(),[](unsigned char c){
  return std::isalnum(c)||c=='_';
 });
}
std::optional<LaunchPlan> MameBridge::plan(const std::filesystem::path& executable,std::string_view game,
 const std::optional<std::filesystem::path>& romPath){
 if(executable.empty()||!validDriverName(game))return std::nullopt;
 LaunchPlan result{executable,{std::string(game)}};
 if(romPath && !romPath->empty()){result.arguments.push_back("-rompath");result.arguments.push_back(romPath->string());}
 return result;
}
static std::string attribute(std::string_view tag,std::string_view key){
 const std::string needle=std::string(key)+"=\"";
 auto p=tag.find(needle);
 if(p==std::string_view::npos)return {};
 p+=needle.size();const auto end=tag.find('"',p);
 return end==std::string_view::npos?std::string{}:std::string(tag.substr(p,end-p));
}
static std::string element(std::string_view body,std::string_view name){
 const auto open=body.find("<"+std::string(name)+">");
 if(open==std::string_view::npos)return {};
 const auto start=open+name.size()+2;
 const auto end=body.find("</"+std::string(name)+">",start);
 return end==std::string_view::npos?std::string{}:std::string(body.substr(start,end-start));
}
std::vector<Game> MameBridge::parseListXml(std::string_view xml){
 std::vector<Game> result;
 std::size_t pos=0;
 while((pos=xml.find("<machine ",pos))!=std::string_view::npos){
  const auto end=xml.find('>',pos);
  if(end==std::string_view::npos)break;
  const auto close=xml.find("</machine>",end+1);
  if(close==std::string_view::npos)break;
  const auto tag=xml.substr(pos,end-pos+1),body=xml.substr(end+1,close-end-1);
  auto name=attribute(tag,"name");
  if(validDriverName(name)){
   Game game;game.name=std::move(name);game.description=element(body,"description");
   game.year=element(body,"year");game.manufacturer=element(body,"manufacturer");
   game.runnable=attribute(tag,"runnable")!="no";
   result.push_back(std::move(game));
  }
  pos=close+10;
 }
 return result;
}
}
