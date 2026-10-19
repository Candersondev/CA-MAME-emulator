#include "ca_mame/compat/DependencyInspector.hpp"
#include "ca_mame/compat/ZipAudit.hpp"
#include <algorithm>
#include <cctype>
#include <system_error>
#include <unordered_set>
#include <unordered_map>
#include <sstream>
#include <iomanip>
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
 std::unordered_map<std::string,ZipAuditResult> archives;
 auto inspectArchive=[&](const std::string& set)->const ZipAuditResult*{
  if(!MameBridge::validDriverName(set))return nullptr;
  const auto path=root/(set+".zip");
  if(!std::filesystem::is_regular_file(path,ec))return nullptr;
  auto it=archives.find(set);
  if(it==archives.end())it=archives.emplace(set,auditZip(path)).first;
  return &it->second;
 };
 for(const auto& rom:game.roms){
  if(rom.optional||rom.name.empty())continue;
  const ZipEntryAudit* match=nullptr;
  for(const auto& set:{game.name,game.parent,game.bios}){
   if(const auto* archive=inspectArchive(set)){
    for(const auto& entry:archive->entries){
     if(entry.name==rom.name){match=&entry;break;}
    }
   }
   if(match)break;
  }
  if(!match){
   const auto unpacked=root/game.name/rom.name;
   if(std::filesystem::is_regular_file(unpacked,ec)){
    report.items.push_back({rom.name,"ROM file",DependencyState::Found,"Loose ROM present; run MAME verification to check checksums"});
    continue;
   }
   report.items.push_back({rom.name,"ROM file",DependencyState::Missing,"Missing from inspected ZIP sets; may exist in a 7z archive or alternate MAME ROM path"});
   report.hasMissing=true;
   continue;
  }
  std::ostringstream crc;crc<<std::hex<<std::setfill('0')<<std::setw(8)<<match->crc;
  std::string expected=rom.crc;
  std::transform(expected.begin(),expected.end(),expected.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
  const bool matches=expected.empty()||expected==crc.str();
  const bool valid=match->supported&&match->verified&&matches;
  report.items.push_back({rom.name,"ROM CRC32",valid?DependencyState::Found:DependencyState::Missing,
   !match->supported?"Unsupported ZIP entry compression or encryption":
   !match->verified?"Corrupted ZIP entry: calculated CRC32 does not match ZIP directory":
   !matches?"ROM CRC32 differs from MAME metadata: expected "+expected+", found "+crc.str():
   "CRC32 matches; MAME SHA-1 and final audit still required"});
  if(!valid)report.hasMissing=true;
 }
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

DependencyReport DependencyInspector::inspectDetailed(const Game& game,const std::filesystem::path& root,const std::vector<Game>& catalog){
 auto report=inspectDetailed(game,root);
 std::unordered_map<std::string,const Game*> lookup;
 for(const auto& entry:catalog)lookup.emplace(entry.name,&entry);
 std::unordered_set<std::string> seen{game.name};
 std::vector<std::string> pending;
 for(const auto& initial:{game.parent,game.bios})if(!initial.empty()&&seen.insert(initial).second)pending.push_back(initial);
 for(std::size_t i=0;i<pending.size()&&i<256;++i){
  const auto name=pending[i];
  const auto found=lookup.find(name);
  if(found==lookup.end())continue;
  const Game& ancestor=*found->second;
  for(const auto& parent:{ancestor.parent,ancestor.bios}){
   if(parent.empty()||!MameBridge::validDriverName(parent)||!seen.insert(parent).second)continue;
   pending.push_back(parent);
   const bool present=existsSet(root,parent);
   report.items.push_back({parent,"Transitive parent/BIOS",present?DependencyState::Found:DependencyState::Missing,
     present?"Dependency set present; verify contents with MAME":"Required ancestor set not found locally; check MAME ROM paths"});
   if(!present)report.hasMissing=true;
  }
 }
 return report;
}

}
