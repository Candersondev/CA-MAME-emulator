# Evil Night (Konami M2) compatibility target

**Status: research / unsupported.** Reusable mouse/light-gun state implemented and unit-tested; not yet wired to a Konami M2 driver. No PowerPC 602 or Konami M2 driver exists in CA-MAME yet. Do not list this game as playable.

## Verified hardware reference

- MAME driver: https://github.com/mamedev/mame/blob/master/src/mame/konami/konamim2.cpp
- MAME short name: `evilngt`; regional clone `hellngt`.
- 1998 Konami light-gun game; dual PowerPC 602 CPUs and 3DO M2 graphics/audio chipset.
- Requires optical-disc media (MAME CHD) as well as ROMs.
- Audio includes Yamaha YMZ280B, DSP/DAC and CD audio.
- Gun coordinates and triggers must be mapped to host pointer/mouse/gamepad input.
- Current MAME driver flags imperfect timing.

## Proposed implementation order

1. **ROM/media audit**: manifest for `evilngt`, `hellngt`; parent/clone inheritance; BIOS/EEPROM/NVRAM metadata; read-only CHD discovery. No proprietary data in repository.
2. **Hardware platform**: reusable big-endian PowerPC 602 CPU core, instruction/exception/timer tests, dual-CPU scheduler and shared-memory synchronization.
3. **Konami M2 board**: address decoding, ROM/RAM, boot path, EEPROM/timekeeper, optical drive controller, interrupts, board I/O.
4. **Video**: M2 graphics command processor, texture formats, 3D rasterization, display timing.
5. **Audio**: YMZ280B, DSP/DAC, CD audio synchronization.
6. **Inputs**: two or three light-gun pointers/triggers, calibration, coin/start/service.
7. **Compatibility**: deterministic boot tests, attract mode, gameplay and audio validation on user-supplied legally obtained media.

## Definition of playable

- Boots from a complete audited ROM + optical media set.
- Displays attract mode without crashes.
- Starts a credit and plays through gameplay with functioning aiming, triggers, graphics and sound.
- Runs at practical speed and is covered by repeatable regression tests.

**Licensing:** CA-MAME remains an independent project. Do not copy MAME implementation code without a deliberate licensing review.

## Windows mouse control specification

- Player 1: move the mouse to aim; left mouse button fires; right mouse button reloads.
- Aim coordinates must be converted from SDL window coordinates to the game's logical render area, accounting for letterboxing and scaling.
- A shot outside the logical image should be reported as off-screen/reload, not clamped to an in-game hit.
- Shared input state lives in `include/ca_mame/input/LightGun.hpp` and is unit-tested.
- This is an input subsystem milestone only. Evil Night cannot boot until its M2 hardware driver and media support exist.
