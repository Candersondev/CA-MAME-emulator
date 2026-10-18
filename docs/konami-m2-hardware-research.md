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

## CDE BioBus and DMA register staging
Eight BioBus devices expose setup and cycle-time registers at CDE offsets `0x240–0x27c`. Two DMA channels expose control, BioBus address, PowerBus address and count at `0x1000–0x100c` and `0x1020–0x102c`. Write aliases at offset +`0x400` clear register bits. Rising DMA_RESET clears that channel's staged state. DMA_CURR_VALID (0x80) is rejected because no transfer engine exists. Next-descriptor registers, CD DMA, DMA timers, bus transactions and ATAPI remain unimplemented. Tests exercise both banks and reset isolation.

## Experimental BioBus-to-PowerBus DMA
The first transfer path copies even byte counts (up to 64 KiB) in 16-bit units from a simulated 64 KiB BioBus buffer at `0x20000000` to M2 RAM at `0x40000000`. It supports channels 0/1, fixed source when the setup register requests I/O, DMA completion status bits 0x80/0x100 and interrupt enable gating. Transfers are immediate (no timing) and restricted to this synthetic BioBus device. Unsupported reverse transfers, 8-bit modes, source ranges and invalid destination ranges fail explicitly. No ATAPI drive or disc data is connected. The DMA control write is intercepted by the M2 memory map; the CDE register model alone does not execute transfers.

## Initial ATAPI packet backend (experimental)
`AtapiCdRom` implements SCSI packet operations TEST UNIT READY (00), REQUEST SENSE (03), INQUIRY (12), READ CAPACITY (25), and READ(10) (28) against a flat 2048-byte-sector ISO file. The `M2MemoryMap::stageAtapiPacket` method is an **explicit host/test-only bridge** to the synthetic 64 KiB BioBus buffer; the existing CDE DMA path can copy staged data to RAM. No guest-visible ATAPI command register, IDE taskfile, interrupt/timing behavior, CD sector subchannels, 2352-byte raw BIN/CUE, or real arcade disc layout is implemented. Mounting an ISO is not wired to the frontend. This does not boot Evil Night.

## Experimental guest ATA taskfile
The new `M2AtapiTaskfile` maps an experimental 32-bit register view of the ATA CS0 aperture `0x3f000000–0x3fffffff` and supports command 0xA0 (PACKET), six 16-bit packet-data writes, PIO response reads, status/error and a simple interrupt latch. It invokes `AtapiCdRom` directly, without the host-only staging bridge. **This is not yet accurate Konami M2 bus wiring.** Upstream MAME maps the ATA CS0 device with `cs0_swap_r/cs0_swap_w` over a 64-bit bus, and the real ATA taskfile's register offsets and lane behavior need validation. Current register offsets are a provisional software-facing interface. Real IRQ routing, DMA handshakes, device timing, optical media layout and Evil Night boot are not supported. Source: https://github.com/mamedev/mame/blob/master/src/mame/konami/konamim2.cpp

## Integrated M2 development batch (October 2026)
The KonamiM2 harness now offers ISO mounting/ejecting, a bounded dual-PowerPC stepping method (`runUntilFault`) and per-CPU PC/fault/cycle snapshots for firmware diagnostics. Tests cover multi-sector READ(10) over the experimental ATA data port and synthetic dual-CPU execution. The physical ATA bus offset/byte-lane mapping is **not validated**; the taskfile remains provisional and does not yet emulate the CR-589 drive or the CDE/PowerBus external interrupt wiring. The BIOS is not bundled, and Evil Night boot has not been demonstrated. These integrations are preparatory, not a complete M2 hardware emulation.

## Firmware diagnostics and ATA interrupt model follow-up
`KonamiM2::diagnoseBoot(maxPairs)` provides a bounded execution report for both experimental PowerPC 602 instances: instruction addresses, fault reason, faulting address/opcode, cycle totals and whether the step budget was exhausted. The synthetic firmware test verifies this report. The ATA taskfile exposes a masked/pending interrupt distinction and non-acknowledging alternate-status inspection to facilitate future bus integration. **These are software diagnostic interfaces, not verified physical M2 ATA register mappings.** In particular, ATA IRQ routing to PowerBus and video output are not implemented in this batch. Do not interpret green CI as Evil Night boot support.

## Boot-trace harness
`KonamiM2::traceBoot(maxPairs, sampleEvery, maxEntries)` captures bounded sampled instruction addresses from both PowerPC processors and the first fault state. Its regression test uses synthetic instructions and an invalid fetch address. This supports investigation of a user-supplied legal boot ROM but **does not yet load or boot Evil Night**. Full milestone work still needed: hardware-accurate 64-bit ATA CS0 lane decode, real PowerBus IRQ routing, PowerPC instruction coverage, and graphics. The trace has no disk or video state and is not a game compatibility claim.
