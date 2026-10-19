#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
namespace ca_mame::compat {
class Sha1 {
 std::array<std::uint32_t,5> h_{{0x67452301u,0xefcdab89u,0x98badcfeu,0x10325476u,0xc3d2e1f0u}};
 std::array<unsigned char,64> block_{};
 std::uint64_t bytes_=0;
 std::size_t buffered_=0;
 void process();
public:
 void update(const unsigned char* data,std::size_t size);
 std::string finish();
};
}
