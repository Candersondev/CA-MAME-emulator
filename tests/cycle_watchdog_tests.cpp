#include <cstdint>
#include "ca_mame/devices/CycleWatchdog.hpp"
int main() {
    using ca_mame::devices::CycleWatchdog;
    CycleWatchdog watchdog(100);
    if(watchdog.check(99)) return 1;
    if(!watchdog.check(100)) return 2;
    if(watchdog.check(101)) return 3;
    if(!watchdog.tripped()) return 4;
    watchdog.kick(120);
    if(watchdog.tripped() || watchdog.check(219)) return 5;
    if(!watchdog.check(220)) return 6;
    watchdog.reset(500);
    if(watchdog.check(599) || !watchdog.check(600)) return 7;
    CycleWatchdog disabled(0);
    if(disabled.check(UINT64_MAX)) return 8;
    watchdog.reset(1000);
    if(watchdog.check(10)) return 9;
    return 0;
}
