#include "ca_mame/core/MachineRegistry.hpp"
#include "ca_mame/drivers/SpaceInvaders.hpp"
#include "ca_mame/drivers/GunFight.hpp"
#include "ca_mame/drivers/Williams6809.hpp"
#include <algorithm>
#include <utility>
namespace ca_mame {
bool MachineRegistry::registerMachine(std::string id,std::string description,Factory factory) {
    if(id.empty() || !factory || std::any_of(entries_.begin(),entries_.end(),
        [&](const Entry& e){return e.id==id;})) return false;
    entries_.push_back({std::move(id),std::move(description),factory});
    return true;
}
std::unique_ptr<IMachine> MachineRegistry::create(std::string_view id) const {
    const auto it=std::find_if(entries_.begin(),entries_.end(),
        [&](const Entry& e){return e.id==id;});
    return it==entries_.end()?nullptr:it->factory();
}
MachineRegistry makeDefaultRegistry() {
    MachineRegistry registry;
    registry.registerMachine("invaders","Space Invaders (Intel 8080)",
        []()->std::unique_ptr<IMachine>{return std::make_unique<drivers::SpaceInvaders>();});
    registry.registerMachine("gunfight","Gun Fight (Midway 8080, experimental)",
        []()->std::unique_ptr<IMachine>{return std::make_unique<drivers::GunFight>();});
    registry.registerMachine("williams6809","Williams Motorola 6809 hardware (experimental CPU foundation)",
        []()->std::unique_ptr<IMachine>{return std::make_unique<drivers::Williams6809>();});
    return registry;
}
}
