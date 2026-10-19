#include "ca_mame/compat/DependencyInspector.hpp"
#include <algorithm>
#include <cctype>
#include <system_error>
#include <unordered_set>
namespace ca_mame::compat {
static bool existsSet(const std::filesystem::path& root,const std::string& name){
 if(!MameBridge::validDriverName(name))return false;
 std::error_code ec;
 for(const auto& ext:{".zip",".7z"}){
  if(std::filesystem::is_regular_file(root/(name+ext),ec))return true;
  ec.clear();
 }
 return std::filesystem::is_directory(root/name,ec);
}
DependencyReport DependencyInspector::inspect(const Game& game,const std::filesystem::path& root){
 DependencyReport report;
 std::unordered_set<std::string> visited;
 auto add=[&](const std::string& name,const std::string& role){
  if(name.empty()||!MameBridge::validDriverName(name)||!visited.insert(name).second)return;
  const bool found=existsSet(root,name);
  std::string advice=found?"Set present (contents not yet verified)":
    "Place "+name+".zip or "+name+".7z in your ROM directory, or configure MAME's ROM search path";
  report.items.push_back({name,role,found?DependencyState::Found:DependencyState::Missing,advice});
  if(!found)report.hasMissing=true;
 };
 add(game.name,"Game set");
 add(game.parent,"Clone parent");
 if(game.bios!=game.name&&game.bios!=game.parent)add(game.bios,"ROM/BIOS parent");
 return report;
}
DependencyReport DependencyInspector::inspectDetailed(const Game& game,const std::filesystem::path& root){
 auto report=inspect(game,root);
 std::error_code ec;
 for(const auto& disk:game.disks){
  if(disk.optional)continue;
  const auto path=root/game.name/(disk.name+".chd");
  const bool found=std::filesystem::is_regular_file(path,ec);
  report.items.push_back({disk.name+".chd","CHD disk",found?DependencyState::Found:DependencyState::Missing,
   found?"CHD present; SHA-1 and internal integrity require MAME verification":
   "Expected "+path.string()+"; MAME may also locate it in a parent or configured ROM path"});
  if(!found)report.hasMissing=true;
  ec.clear();
 }
 return report;
}

}
