#include "ca_mame/core/MachineRegistry.hpp"
#include <iostream>
namespace {
int failures=0;
void check(bool ok,const char* name) {if(!ok){std::cerr<<"FAIL: "<<name<<"\n";++failures;}}
}
int main() {
    auto registry=ca_mame::makeDefaultRegistry();
    check(registry.entries().size()==1,"initial driver count");
    check(registry.entries()[0].id=="invaders","driver metadata");
    check(!registry.registerMachine("invaders","duplicate",[]()->std::unique_ptr<ca_mame::IMachine>{return nullptr;}),"duplicate rejected");
    check(!registry.registerMachine("","invalid",[]()->std::unique_ptr<ca_mame::IMachine>{return nullptr;}),"empty ID rejected");
    check(!registry.create("unknown"),"unknown driver rejected");
    auto machine=registry.create("invaders");
    check(machine && machine->driverId()=="invaders","registered board instantiation");
    if(machine) {
        machine->reset();
        check(machine->runFrame()>=33333,"board frame scheduler");
    }
    return failures?1:0;
}
