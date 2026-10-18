#include "ca_mame/compat/GameLibrary.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
int main(){
 namespace fs=std::filesystem;
 using namespace ca_mame::compat;
 const auto dir=fs::temp_directory_path()/("ca_mame_library_test_"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 struct Cleanup{fs::path dir;~Cleanup(){std::error_code ec;fs::remove_all(dir,ec);}} cleanup{dir};
 std::error_code ec;fs::create_directories(dir/"evilngt",ec);if(ec)return 1;
 {std::ofstream f(dir/"EVILNGT.ZIP");f<<"not a real ROM";}
 {std::ofstream f(dir/"unknown.7z");f<<"not a real ROM";}
 const std::vector<Game> catalog={{"evilngt","Evil Night","1998","Konami",true},{"invaders","Space Invaders","1978","Taito",true}};
 const auto scan=GameLibrary::scan(catalog,dir);
 if(!scan.directoryAccessible||scan.entries.size()!=2||scan.archiveCount!=2||scan.unknownArchives!=1)return 2;
 if(!scan.entries[0].localArchive||!scan.entries[0].hasChdDirectory||scan.entries[1].localArchive)return 3;
 const auto found=GameLibrary::filter(scan,"KONAMI",true);
 if(found.size()!=1||found[0].game.name!="evilngt")return 4;
 if(!GameLibrary::filter(scan,"nothing",false).empty())return 5;
 if(GameLibrary::scan(catalog,dir/"missing").directoryAccessible)return 6;
 return 0;
}
