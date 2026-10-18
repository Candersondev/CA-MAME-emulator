#include "ca_mame/input/LightGun.hpp"
#include <cassert>
int main() {
    ca_mame::input::LightGun gun;
    gun.update(320.0f,240.0f,640,480,true,false);
    assert(gun.state().valid && gun.state().x==320 && gun.state().y==240);
    assert(gun.state().trigger && !gun.state().reload);
    gun.update(-2.0f,50.0f,640,480,true,false);
    assert(!gun.state().valid && gun.state().x==0 && gun.state().reload);
    gun.update(639.0f,479.0f,640,480,false,true);
    assert(gun.state().valid && gun.state().reload && !gun.state().trigger);
    gun.update(1.0f,1.0f,0,480,true,false);
    assert(!gun.state().valid && !gun.state().trigger);
    gun.reset();
    assert(!gun.state().valid && !gun.state().trigger);
}
