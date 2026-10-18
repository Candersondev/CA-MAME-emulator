#pragma once
#include "ca_mame/compat/MameBridge.hpp"
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
namespace ca_mame::compat {
struct LibraryEntry {
 Game game;
 bool localArchive=false;
 bool hasChdDirectory=false;
 std::filesystem::path archivePath;
 // Archive presence is NOT ROM integrity verification or playability.
};
struct LibraryScan {
 std::vector<LibraryEntry> entries;
 std::size_t unknownArchives=0;
 std::size_t archiveCount=0;
 bool directoryAccessible=false;
};
class GameLibrary {
public:
 static LibraryScan scan(const std::vector<Game>& catalog,const std::filesystem::path& romDirectory);
 static std::vector<LibraryEntry> filter(const LibraryScan& scan,std::string_view search,bool onlyLocal);
};
}
