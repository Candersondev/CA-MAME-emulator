# Konami M2 hardware reference for CA-MAME

Source: MAME `src/mame/konami/konamim2.cpp` and `src/mame/konami/3dom2.cpp` (reviewed 2026-10-18).
https://github.com/mamedev/mame/blob/master/src/mame/konami/konamim2.cpp

## Verified address ranges in the MAME Konami driver

| Address | MAME device |
| --- | --- |
| 0x20000000-0x201fffff | 2 MiB boot ROM |
| 0xfff00000-0xffffffff | boot ROM high alias (first 1 MiB) |
| 0x37400000-0x37400007 | EEPROM write |
| 0x37600000-0x3760000f | ATAPI-related write |
| 0x37a00020-0x37a0003f | Konami gun/input I/O |
| 0x37c00010-0x37c0001f | serial I/O |
| 0x37e00000-0x37e0000f | Konami I/O |
| 0x3f000000-0x3fffffff | ATA interface |
| 0x36c00000-0x36c03fff | M48T58 timekeeper mapping (installed by game init) |
| 0x3e800000-0x3e80000f | YMZ280B sound mapping (installed by game init) |

Additional RAM, graphics, control, and interrupt mappings are implemented by MAME's 3DO M2 BDA/CDE devices, not fully described by the Konami driver's top-level address map. **Do not assume our prototype 1 MiB RAM at zero or IRQ registers at 0x10000000 are hardware accurate.**

## Evil Night-specific devices

- Dual PowerPC 602 CPUs, M2 clock 66,666,700 Hz.
- Boot ROM `636a01.8q`, 2 MiB.
- 93C46 serial EEPROM, M48T58 timekeeper SRAM, YMZ280B sound ROM/device.
- CR-589 ATAPI CD drive; game media represented by a CD image.
- Light guns expose X/Y via Konami input ports; an upward-aim switch is also modeled.
- MAME's BDA configuration uses two RAM banks set to 8 MiB each for the base Konami configuration.

## Implementation boundaries

CA-MAME currently models only a tiny PowerPC instruction subset. The M2 memory map has a real read-only boot-ROM window and high alias, but its RAM and interrupt-controller addresses remain test-only prototypes. It cannot load or execute the Evil Night firmware successfully. Next: verify BDA RAM mappings and PowerPC reset address; implement guest ROM loading with validated size and hashes, then add traced instruction/exception execution.
