# CA-MAME Emulator

An original, modular C++20 arcade-emulation project. This is **not** a fork of MAME and does not yet emulate commercial arcade hardware.

## Build

Requirements: CMake 3.21+, C++20 compiler.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Run `ca-mame` from the build output directory. The initial executable is a hardware-agnostic scaffold.

## Architecture

- `core`: machine orchestration and lifecycle
- `cpu`: pluggable processor interface
- `bus`: addressable memory bus
- `video`: framebuffer/video-device interface
- `audio`: audio-device interface
- `drivers`: future arcade board configurations
- `rom`: future ROM definitions and validation
- `frontend`: CLI now; SDL frontend later
- `tests`: deterministic unit tests

## Roadmap

1. Add a concrete CPU and test ROM (e.g. Intel 8080).
2. Implement a documented arcade board's memory and I/O map.
3. Add timing, interrupts, graphics, input and audio.
4. Add ROM auditing and legally obtained ROM loading.
5. Add SDL3 display/input/audio and regression tests.

No proprietary ROMs or copyrighted BIOS images are included.
