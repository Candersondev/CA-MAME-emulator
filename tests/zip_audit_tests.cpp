#include "ca_mame/compat/ZipAudit.hpp"
#include <zlib.h>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
static void w16(std::ofstream& f,std::uint16_t n){f.put(static_cast<char>(n));f.put(static_cast<char>(n>>8));}
static void w32(std::ofstream& f,std::uint32_t n){w16(f,static_cast<std::uint16_t>(n));w16(f,static_cast<std::uint16_t>(n>>16));}
static void writeZip(const std::filesystem::path& path,bool corrupt){
 const std::string name="test.bin",data="abc";
 const auto crc=static_cast<std::uint32_t>(crc32(0,reinterpret_cast<const Bytef*>(data.data()),static_cast<uInt>(data.size())));
 std::ofstream f(path,std::ios::binary);
 w32(f,0x04034b50);w16(f,20);w16(f,0);w16(f,0);w16(f,0);w16(f,0);w32(f,crc);w32(f,3);w32(f,3);w16(f,8);w16(f,0);
 f.write(name.data(),8);f.write(corrupt?"xbc":data.data(),3);
 const auto central=static_cast<std::uint32_t>(f.tellp());
 w32(f,0x02014b50);w16(f,20);w16(f,20);w16(f,0);w16(f,0);w16(f,0);w16(f,0);w32(f,crc);w32(f,3);w32(f,3);
 w16(f,8);w16(f,0);w16(f,0);w16(f,0);w16(f,0);w32(f,0);w32(f,0);f.write(name.data(),8);
 const auto end=static_cast<std::uint32_t>(f.tellp());
 w32(f,0x06054b50);w16(f,0);w16(f,0);w16(f,1);w16(f,1);w32(f,end-central);w32(f,central);w16(f,0);
}
int main(){
 namespace fs=std::filesystem;
 const auto root=fs::temp_directory_path()/("ca_mame_zip_"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 fs::create_directories(root);
 writeZip(root/"good.zip",false);writeZip(root/"bad.zip",true);
 const auto good=ca_mame::compat::auditZip(root/"good.zip");
 const auto bad=ca_mame::compat::auditZip(root/"bad.zip");
 fs::remove_all(root);
 if(!good.readable||good.entries.size()!=1||!good.entries[0].verified)return 1;
 if(good.entries[0].sha1!="a9993e364706816aba3e25717850c26c9cd0d89d")return 2;
 if(!bad.readable||bad.entries.size()!=1||bad.entries[0].verified)return 3;
 return 0;
}
