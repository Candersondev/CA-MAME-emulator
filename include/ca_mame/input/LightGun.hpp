#pragma once
#include <algorithm>
#include <cstdint>
namespace ca_mame::input {
struct GunState {
    std::uint16_t x=0;
    std::uint16_t y=0;
    bool trigger=false;
    bool reload=false;
    bool valid=false;
};
class LightGun {
public:
    // Coordinates are in the game image's logical pixel space, not desktop pixels.
    void update(float x,float y,int width,int height,bool trigger,bool reload) noexcept {
        state_={};
        if(width<=0 || height<=0)return;
        state_.valid=x>=0 && y>=0 && x<width && y<height;
        state_.x=static_cast<std::uint16_t>(std::clamp(x,0.0f,float(width-1)));
        state_.y=static_cast<std::uint16_t>(std::clamp(y,0.0f,float(height-1)));
        state_.trigger=trigger;
        state_.reload=reload || (trigger && !state_.valid);
    }
    const GunState& state() const noexcept {return state_;}
    void reset() noexcept {state_={};}
private:
    GunState state_{};
};
}
