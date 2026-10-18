#include "ca_mame/rom/RomManager.hpp"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <zlib.h>
namespace {
int failures=0;
void check(bool ok,const char* message) {if(!ok){std::cerr<<"FAIL: "<<message<<"\n";++failures;}}
}
int main() {
    using namespace ca_mame::rom;
    const std::vector<std::uint8_t> standard={'1','2','3','4','5','6','7','8','9'};
    check(RomManager::crc32(standard)==0xcbf43926u,"CRC32 standard vector");
    check(RomManager::sha1(standard)=="f7c3bc1d808e04732adf679965ccc34ca7ae3441","SHA-1 standard vector");
    check(RomManager::sha1({})=="da39a3ee5e6b4b0d3255bfef95601890afd80709","SHA-1 empty vector");
    check(RomManager::crc32({})==0u,"CRC32 empty vector");
    const auto dir=std::filesystem::temp_directory_path()/"ca_mame_rom_audit_test";
    std::filesystem::create_directories(dir);
    {
        std::ofstream file(dir/"sample.bin",std::ios::binary);
        file.write(reinterpret_cast<const char*>(standard.data()),static_cast<std::streamsize>(standard.size()));
    }
    const RomDefinition good={"sample.bin",9,0xcbf43926u,true};
    const auto result=RomManager::audit(dir,good);
    check(result.ok && result.actualSize==9 && result.actualCrc32==0xcbf43926u,"valid ROM audit");
    check(!RomManager::audit(dir,{"sample.bin",8,0,false}).ok,"reject wrong size");
    check(!RomManager::audit(dir,{"sample.bin",9,0,true}).ok,"reject wrong CRC");
    check(RomManager::audit(dir,{"sample.bin",9,0,false,"f7c3bc1d808e04732adf679965ccc34ca7ae3441",true}).ok,"valid SHA-1 audit");
    check(!RomManager::audit(dir,{"sample.bin",9,0,false,"0000000000000000000000000000000000000000",true}).ok,"reject wrong SHA-1");
    check(!RomManager::audit(dir,{"missing.bin",9,0,false}).ok,"reject missing ROM");
    check(!RomManager::audit(dir,{"../sample.bin",9,0,false}).ok,"reject path traversal");
    std::vector<std::uint8_t> loaded;
    check(RomManager::readFile(dir/"sample.bin",loaded) && loaded==standard,"read raw ROM");
    // Construct a minimal ZIP containing a raw-DEFLATE member.
    {
        z_stream zs{};
        check(deflateInit2(&zs,Z_DEFAULT_COMPRESSION,Z_DEFLATED,-MAX_WBITS,8,Z_DEFAULT_STRATEGY)==Z_OK,"deflate init");
        std::vector<std::uint8_t> compressed(128);
        zs.next_in=const_cast<Bytef*>(standard.data());
        zs.avail_in=static_cast<uInt>(standard.size());
        zs.next_out=compressed.data();
        zs.avail_out=static_cast<uInt>(compressed.size());
        check(deflate(&zs,Z_FINISH)==Z_STREAM_END,"deflate sample");
        compressed.resize(zs.total_out);
        deflateEnd(&zs);
        std::vector<std::uint8_t> zip;
        auto put16=[&](unsigned v){zip.push_back(std::uint8_t(v));zip.push_back(std::uint8_t(v>>8));};
        auto put32=[&](std::uint32_t v){put16(v&0xffff);put16(v>>16);};
        const std::string name="sample.bin";
        put32(0x04034b50);put16(20);put16(0);put16(8);put16(0);put16(0);
        put32(0xcbf43926);put32(static_cast<unsigned>(compressed.size()));put32(9);
        put16(static_cast<unsigned>(name.size()));put16(0);
        zip.insert(zip.end(),name.begin(),name.end());
        zip.insert(zip.end(),compressed.begin(),compressed.end());
        const auto central=zip.size();
        put32(0x02014b50);put16(20);put16(20);put16(0);put16(8);put16(0);put16(0);
        put32(0xcbf43926);put32(static_cast<unsigned>(compressed.size()));put32(9);
        put16(static_cast<unsigned>(name.size()));put16(0);put16(0);put16(0);put16(0);put32(0);put32(0);
        zip.insert(zip.end(),name.begin(),name.end());
        const auto centralSize=zip.size()-central;
        put32(0x06054b50);put16(0);put16(0);put16(1);put16(1);
        put32(static_cast<unsigned>(centralSize));put32(static_cast<unsigned>(central));put16(0);
        const auto archive=dir/"sample.zip";
        std::ofstream output(archive,std::ios::binary);
        output.write(reinterpret_cast<const char*>(zip.data()),static_cast<std::streamsize>(zip.size()));
        output.close();
        std::vector<std::uint8_t> unzipped;
        check(RomManager::readZipEntry(archive,name,unzipped) && unzipped==standard,"DEFLATE ZIP extraction");
        check(RomManager::auditZip(archive,good).ok,"DEFLATE ZIP ROM audit");
        check(!RomManager::auditZip(archive,{"missing.bin",9,0,false}).ok,"missing ZIP entry");
    }
    std::filesystem::remove_all(dir);
    return failures?1:0;
}
