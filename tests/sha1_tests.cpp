#include "ca_mame/compat/Sha1.hpp"
#include <string>
int main(){
 using ca_mame::compat::Sha1;
 Sha1 empty;
 if(empty.finish()!="da39a3ee5e6b4b0d3255bfef95601890afd80709")return 1;
 Sha1 abc;const std::string value="abc";
 abc.update(reinterpret_cast<const unsigned char*>(value.data()),1);
 abc.update(reinterpret_cast<const unsigned char*>(value.data()+1),2);
 if(abc.finish()!="a9993e364706816aba3e25717850c26c9cd0d89d")return 2;
 return 0;
}
