#include "ca_mame/compat/LibraryPreferences.hpp"
#include <chrono>
#include <filesystem>
int main(){
 namespace fs=std::filesystem;
 using namespace ca_mame::compat;
 const auto path=fs::temp_directory_path()/("ca_mame_prefs_"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".tsv");
 struct Cleanup{fs::path p;~Cleanup(){std::error_code ec;fs::remove(p,ec);fs::remove(p.string()+".tmp",ec);}} cleanup{path};
 LibraryPreferences p;
 p.toggleFavorite("evilngt");p.toggleFavorite("../invalid");
 p.recordLaunch("invaders");p.recordLaunch("evilngt");p.recordLaunch("invaders");
 if(!p.favorite("evilngt")||p.recent().size()!=2||p.recent()[0]!="invaders")return 1;
 if(!p.save(path))return 2;
 LibraryPreferences q;
 if(!q.load(path)||!q.favorite("evilngt")||q.recent().size()!=2)return 3;
 q.toggleFavorite("evilngt");if(q.favorite("evilngt"))return 4;
 return 0;
}
