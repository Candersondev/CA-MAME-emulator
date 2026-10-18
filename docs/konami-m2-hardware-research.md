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


## BDA mapping verified against MAME 3dom2.h / 3dom2.cpp
- Shared 16 MiB RAM (two 8 MiB banks): `0x40000000–0x40ffffff`.
- PowerBus controller: `0x00010000` (64 KiB window). Interrupt enable set at +0x40, status at +0x50, enable clear at +0x440, status clear via error-status +0x460 (current MAME behavior).
- Memory controller: `0x00020000`; VDU: `0x00030000`; triangle engine: `0x00040000`; DSP: `0x00060000`; control ports: `0x00070000`; MPEG: `0x00080000`; CPU ID: `0x10000000`.
- Texture RAM: `0x000c0000–0x000c3fff`; CDE slot: `0x04000000–0x04ffffff`.
- Current CA-MAME implements only RAM, a CPU0 ID stub, and the first PowerBus interrupt registers. Other listed device windows are **not implemented**.
- Temporary compatibility aliases for legacy unit tests at low RAM and `0x11000000` prototype IRQ are **not hardware mappings** and must be removed after test migration.

Sources: https://github.com/mamedev/mame/blob/master/src/mame/konami/3dom2.h and https://github.com/mamedev/mame/blob/master/src/mame/konami/3dom2.cpp .

## Memory-controller implementation (initial)
MAME `m2_memctl_device` registers at `0x00020000`: MCONFIG (+0x0), MREF (+0x4), MCNTL (+0x8), MRESET (+0xC). Two 8 MiB banks encode as SS0=5 and SS1=5, use the expression `(5 << 13) | (5 << 10)` as authoritative. MCONFIG/MREF are read/write. MCNTL/MRESET are acknowledged no-op writes and zero reads, following the current MAME stubs, **not real reset behavior**. MREF GPIO external callbacks are not implemented. RAM backing remains fixed-size regardless of MCONFIG writes.

## Initial CDE register model
The CDE is mapped into BDA slot 4 at `0x04000000–0x04ffffff`. CA-MAME currently implements a **subset** of registers: device ID (+0x000), version (+0x004), serial debug control (+0x00c), serial write completion (+0x014), interrupt status (+0x018), interrupt enable (+0x01c), partial reset control (+0x020), boot-block status (+0x208), system configuration (+0x280), and other fixed-value reads. The +0x400 write alias clears bits where supported. Unknown CDE registers currently fault instead of silently pretending to work. DMA, CD command transport, serial device callbacks, CD-ready timing, and actual CPU reset effects are not implemented. Source: https://github.com/mamedev/mame/blob/master/src/mame/konami/3dom2.h and https://github.com/mamedev/mame/blob/master/src/mame/konami/3dom2.cpp .
