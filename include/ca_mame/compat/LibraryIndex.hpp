#pragma once
#include "ca_mame/compat/GameLibrary.hpp"
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
namespace ca_mame::compat {
// Search result indices reference the original LibraryScan: no per-query entry copies.
class LibraryIndex {
public:
 explicit LibraryIndex(const LibraryScan& scan);
 std::vector<std::size_t> find(std::string_view query,bool localOnly) const;
private:
 struct Record {std::string searchable;bool local;};
 std::vector<Record> records_;
};
}
