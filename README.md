# CA-MAME Emulator

An original, modular C++20 arcade-emulation project. This is **not** a fork of MAME and does not yet emulate commercial arcade hardware.

## Build

Requirements: CMake 3.21+, C++20 compiler.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Run `ca-mame` from the build output directory for the headless scaffold. The optional SDL3 frontend requires SDL3 development files and four legally obtained Space Invaders ROMs.\n\n```sh\ncmake -S . -B build-sdl -DCA_MAME_ENABLE_SDL3=ON\ncmake --build build-sdl --config Release\n# Windows: build-sdl/Release/ca-mame-sdl.exe invaders.h invaders.g invaders.f invaders.e\n# Linux/macOS: build-sdl/ca-mame-sdl invaders.h invaders.g invaders.f invaders.e\n```\n\nThe ROM order is important: invaders.h (0x0000), invaders.g (0x0800), invaders.f (0x1000), invaders.e (0x1800). Keyboard: C coin, 1/2 start, Left/Right/Space player 1, A/D/Right Ctrl player 2, Escape quit.\n\n**Limitations:** The Space Invaders driver and Intel 8080 CPU are preliminary; original ROM execution is not yet validated end-to-end. SDL3 frontend is not exercised by the default CI matrix. No audio emulation or ROM auditing is provided.

## Architecture

- `core`: machine orchestration and lifecycle
- `cpu`: pluggable processor interface
- `bus`: addressable memory bus
- `video`: framebuffer/video-device interface
- `audio`: audio-device interface
- `drivers`: preliminary Space Invaders board implementation
- `rom`: future ROM definitions and validation
- `frontend`: headless CLI and optional SDL3 graphics/input frontend
- `tests`: deterministic unit tests

## Roadmap

1. Add a concrete CPU and test ROM (e.g. Intel 8080).
2. Implement a documented arcade board's memory and I/O map.
3. Add timing, interrupts, graphics, input and audio.
4. Add ROM auditing and legally obtained ROM loading.
5. Add SDL3 display/input/audio and regression tests.

No proprietary ROMs or copyrighted BIOS images are included.


### Experimental Konami M2 boot trace

After building `ca-mame`, run `ca-mame --m2-trace path/to/your-legally-obtained-2MiB-boot-rom.bin 100` to log up to 100 instructions from the currently modeled ROM base. This is diagnostic only; Evil Night is not bootable. ROM size must be exactly 2,097,152 bytes. The RAM layout is provisional and does not yet match all M2 BDA memory decoding.

## Downloadable Windows arcade launcher
The SDL3 Windows GitHub Actions job produces `CA-MAME-Windows-SDL3` as a ZIP artifact. Extract it and run `Start-CA-MAME.cmd` to open the arcade library. The ZIP includes `ca-mame-library.exe`, `SDL3.dll`, `SDL3_image.dll`, `Start-CA-MAME.ps1`, the native experimental `ca-mame-sdl.exe`, and artwork folders. **External MAME is not bundled**; install MAME separately and copy its executable beside the launcher or invoke `Start-CA-MAME.ps1 -MameExecutable "C:\\path\\to\\mame.exe"`. On first run the launcher creates `catalog.xml` from `mame.exe -listxml`, then opens the browser. Put legally obtained ROM files in `roms/`. The browser launches compatible titles through the external MAME program; this is not native CA-MAME support for every MAME title. The executable artifact is retained for 30 days by GitHub Actions.
