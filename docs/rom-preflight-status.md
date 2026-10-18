# ROM preflight integration

CA-MAME uses MAME's `-verifyroms` command to validate ROM, BIOS and CHD dependencies. The frontend should run this before launching selected games and report failures. Verification requires an installed MAME executable and matching game files.
