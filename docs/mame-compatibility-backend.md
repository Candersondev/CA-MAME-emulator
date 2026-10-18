# MAME compatibility backend

CA-MAME's native cores remain separate from upstream MAME. The compatibility layer provides validated machine identifiers, an argument-vector launch plan, and an initial parser for MAME `-listxml` metadata. The parser is **not** a complete streaming XML implementation and is not suitable for untrusted or very large XML files. The launcher plan does not execute a process.

MAME is not bundled. ROM auditing, executable discovery, actual launching, and full-catalog indexing are future work. A machine marked `runnable=yes` is not necessarily playable. The optional upstream executable is a practical path to broad coverage without asserting CA-MAME natively supports every arcade system.

Embedding upstream MAME code requires a separate GPL and third-party licensing review. No copyrighted ROMs or BIOS are distributed.

## External execution (implemented)
Install MAME separately, then use `ca-mame --mame-launch /path/to/mame evilngt /path/to/roms` to run a game or `ca-mame --mame-verify /path/to/mame evilngt /path/to/roms` to run MAME's ROM audit. On Windows provide the path to `mame.exe`. Both commands start a real child process without a shell and return its exit status. These commands do not download MAME or ROMs. The existing `--mame-plan` remains a dry run.

**Limits:** This is a synchronous launcher without UI lifecycle controls, process timeouts, full-catalog indexing, automatic executable discovery, or ROM audit result parsing. MAME's compatibility and ROM completeness vary by game. Native Konami M2/Evil Night emulation is unaffected.

## Game Library (implemented)
Run `ca-mame --mame-library catalog.xml /path/to/roms` to enumerate catalog entries and whether a matching .zip or .7z archive or game-named directory exists. Append `--local` to show only locally detected entries. The scan is nonrecursive and uses case-insensitive names for portability. It does not open archives, validate CHDs, check parent/clone dependencies, or assert a game is playable. Use `--mame-verify` for ROM verification by an installed MAME executable. Catalog XML parsing currently loads the entire input and is not yet streaming; huge full MAME catalogs may use significant memory.

## SDL3 graphical browser
When built with `CA_MAME_ENABLE_SDL3=ON`, the `ca-mame-library` executable offers a keyboard-operated graphical browser: `ca-mame-library catalog.xml /path/to/roms /path/to/mame`. Type to search game ID, description, or manufacturer; Tab toggles local-only versus all; arrow/Page Up/Page Down keys navigate; Enter launches selected game using external MAME; Escape quits. This is an initial SDL3 text-based GUI using SDL's built-in debug font, not a full featured artwork browser. Launch is synchronous; the browser hides while MAME runs. Local indicators mean filename/directory presence only, not verified ROM integrity.

## Persistent ROM audit status
In the SDL3 game browser press F5 on the selected game to invoke the installed MAME executable with `-verifyroms`. CA-MAME records PASS for exit code zero and FAIL for a nonzero exit code, in `.ca-mame-audits.tsv` inside the ROM directory; unknown entries display `[????]`. This is the last recorded MAME process result, not a live check. It does not parse detailed audit messages, detect changed ROM files, or confirm actual gameplay. The ROM directory must be writable to save status.

## Large catalog improvements
The CLI and SDL3 browser now use an incremental 64-KiB input reader rather than reading the entire `-listxml` file into a string. Regression tests cover 5,000 machines and buffer boundaries. The result still holds all parsed game metadata in memory, and the lightweight parser does not fully implement XML entity decoding, XML namespaces, or complete malformed-XML validation. Large-catalog searching still uses a linear scan and can be optimized further.

## Large catalog indexing
The CLI and SDL3 browser use incremental XML machine-record extraction instead of loading the entire MAME `-listxml` into one string. The SDL3 browser builds a case-insensitive index once, then filters integer references to catalog entries without copying their metadata. The index searches driver name, description, manufacturer, and year. The incremental parser caps buffered machine data at approximately 1 MiB; unusually large machine elements may be skipped. This is a lightweight extractor rather than a general-purpose validating XML parser.

## Browser favorites, recent games and controller
The SDL3 browser persists favorites and a 30-entry recent-launch list in `.ca-mame-preferences.tsv` inside the ROM directory. Space toggles the selected favorite; F filters favorites; R filters recently launched games; Tab toggles local-only. Controller D-pad up/down selects games, South/A launches external MAME, West/X toggles favorites. Left mouse click selects a row and mouse wheel moves selection. The interface still uses SDL3's built-in debug text and has no artwork decoding or thumbnail support yet. Controller availability and interactive behavior require testing on actual hardware.

## Arcade-style game details and artwork
The SDL3 library now has a two-column layout: a searchable list on the left and a game-details panel on the right. The details panel shows driver ID, description, manufacturer, year, local ROM presence, last recorded ROM audit, favorite state and MAME's runnable flag. To display an artwork preview, place a BMP image at `<rom-directory>/artwork/<driver>.bmp`, e.g. `roms/artwork/evilngt.bmp`. The browser loads only the selected game's image and releases the previous texture when selection changes. Missing artwork displays a placeholder. PNG/JPEG, cabinet images, and artwork downloading are not implemented in this version. The MAME runnable flag and ROM-file presence do not establish native CA-MAME playability.

## PNG/JPEG artwork categories
The graphical library can load artwork using SDL3_image in PNG, JPEG or BMP format. Organize optional artwork under `<rom-directory>/artwork/snap/<driver>.png` for gameplay screenshots, `artwork/titles/<driver>.jpg` for title screens, and `artwork/cabinets/<driver>.png` for cabinet photos. Each category searches `.png`, `.jpg`, `.jpeg`, and `.bmp` in that order. Use Left/Right arrow keys or gamepad D-pad Left/Right to switch artwork categories. Legacy `artwork/<driver>.bmp` files remain supported in screenshot mode. Images are scaled to fit without stretching and loaded on selection/category change, not all at once. Artwork is supplied by the user; the emulator does not bundle or download copyrighted artwork.
