#include "ca_mame/drivers/Williams6809.hpp"
#include <algorithm>
namespace ca_mame::drivers {
Williams6809::Williams6809():cpu_(memory_){reset();}
void Williams6809::reset(){
    // Preserve loaded program images across board resets.
    cpu_.reset();cycles_=frames_=0;
}
bool Williams6809::loadProgram(std::uint16_t address,std::span<const std::uint8_t> bytes){
    if(bytes.size()>memory_.size()-address)return false;
    std::copy(bytes.begin(),bytes.end(),memory_.begin()+address);
    return true;
}
std::uint64_t Williams6809::runFrame(){
    // Temporary bounded CPU scheduler, not an accurate Williams video clock.
    const auto target=++frames_*1000000ull/60;
    const auto before=cycles_;
    while(cycles_<target){
        cycles_+=cpu_.step();
        if(cpu_.halted())break; // unsupported opcode or SYNC: fail closed
    }
    return cycles_-before;
}
}
