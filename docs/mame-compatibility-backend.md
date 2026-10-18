# MAME compatibility backend

CA-MAME's native cores remain separate from upstream MAME. The compatibility layer provides validated machine identifiers, an argument-vector launch plan, and an initial parser for MAME `-listxml` metadata. The parser is **not** a complete streaming XML implementation and is not suitable for untrusted or very large XML files. The launcher plan does not execute a process.

MAME is not bundled. ROM auditing, executable discovery, actual launching, and full-catalog indexing are future work. A machine marked `runnable=yes` is not necessarily playable. The optional upstream executable is a practical path to broad coverage without asserting CA-MAME natively supports every arcade system.

Embedding upstream MAME code requires a separate GPL and third-party licensing review. No copyrighted ROMs or BIOS are distributed.

## External execution (implemented)
Install MAME separately, then use `ca-mame --mame-launch /path/to/mame evilngt /path/to/roms` to run a game or `ca-mame --mame-verify /path/to/mame evilngt /path/to/roms` to run MAME's ROM audit. On Windows provide the path to `mame.exe`. Both commands start a real child process without a shell and return its exit status. These commands do not download MAME or ROMs. The existing `--mame-plan` remains a dry run.

**Limits:** This is a synchronous launcher without UI lifecycle controls, process timeouts, full-catalog indexing, automatic executable discovery, or ROM audit result parsing. MAME's compatibility and ROM completeness vary by game. Native Konami M2/Evil Night emulation is unaffected.

## Game Library (implemented)
Run `ca-mame --mame-library catalog.xml /path/to/roms` to enumerate catalog entries and whether a matching .zip or .7z archive or game-named directory exists. Append `--local` to show only locally detected entries. The scan is nonrecursive and uses case-insensitive names for portability. It does not open archives, validate CHDs, check parent/clone dependencies, or assert a game is playable. Use `--mame-verify` for ROM verification by an installed MAME executable. Catalog XML parsing currently loads the entire input and is not yet streaming; huge full MAME catalogs may use significant memory.
