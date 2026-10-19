#include "ca_mame/compat/DependencyInspector.hpp"
#include <filesystem>
#include <fstream>
#include <chrono>
#include "ca_mame/compat/ZipAudit.hpp"
int main(){
 namespace fs=std::filesystem;
 using namespace ca_mame::compat;
 const auto root=fs::temp_directory_path()/("ca_mame_dep_"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 fs::create_directories(root);
 Game game;game.name="clone";game.parent="parent";game.bios="bios";
 {std::ofstream file(root/"clone.zip");file<<"dummy";}
 {std::ofstream file(root/"parent.7z");file<<"dummy";}
 auto r=DependencyInspector::inspect(game,root);
 if(r.items.size()!=3||!r.hasMissing||r.items[0].state!=DependencyState::Found||r.items[1].state!=DependencyState::Found||r.items[2].state!=DependencyState::Missing){fs::remove_all(root);return 1;}
 fs::create_directory(root/"bios");
 r=DependencyInspector::inspect(game,root);
 if(r.hasMissing||r.items.size()!=3){fs::remove_all(root);return 2;}
 game.bios="parent";r=DependencyInspector::inspect(game,root);
 if(r.items.size()!=2){fs::remove_all(root);return 3;}
 const auto invalid=auditZip(root/"clone.zip");
 if(invalid.readable||invalid.error.empty()){fs::remove_all(root);return 4;}
 game.roms.push_back({"missing.bin","12345678","","",false});
 const auto detailed=DependencyInspector::inspectDetailed(game,root);
 if(!detailed.hasMissing){fs::remove_all(root);return 5;}
 fs::remove_all(root);
 return 0;
}
