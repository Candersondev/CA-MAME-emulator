# MAME compatibility backend

CA-MAME's native cores remain separate from upstream MAME. The compatibility layer provides validated machine identifiers, an argument-vector launch plan, and an initial parser for MAME `-listxml` metadata. The parser is **not** a complete streaming XML implementation and is not suitable for untrusted or very large XML files. The launcher plan does not execute a process.

MAME is not bundled. ROM auditing, executable discovery, actual launching, and full-catalog indexing are future work. A machine marked `runnable=yes` is not necessarily playable. The optional upstream executable is a practical path to broad coverage without asserting CA-MAME natively supports every arcade system.

Embedding upstream MAME code requires a separate GPL and third-party licensing review. No copyrighted ROMs or BIOS are distributed.
