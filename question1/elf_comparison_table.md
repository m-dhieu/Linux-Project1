## ELF COMPARISON TABLE

**Evidence-based comparison of `data_sync` and `data_sync_v2`**

| Identification | `data_sync` (Version 1) | `data_sync_v2` (Version 2) | Interpretation |
| :--- | :--- | :--- | :--- |
| **File format** | ELF 64-bit LSB | ELF 64-bit LSB | Both are 64-bit ELF files. |
| **Architecture** | x86-64 (AMD x86-64) | x86-64 (AMD x86-64) | Same target architecture. |
| **ELF type** | `ET_DYN` (DYN) | `ET_DYN` (DYN) | Dynamic section `FLAGS_1` reports `NOW PIE` for both; these are PIE executables. |
| **Entry point** | `0x1220` | `0x1300` | Startup entry addresses differ; neither is `main`. |
| **`main` symbol** | `0x173c` | `0x1d79` | Located from `nm`/`objdump` symbol and disassembly evidence. |
| **Program headers** | 13 | 13 | Same number of program-header entries. |
| **Section headers** | 37 | 37 | Same number of section-header entries. |
| **`.text` size** | `0x624` (1,572 bytes) | `0xb74` (2,932 bytes) | Version 2 has 1,360 more bytes of executable code. |
| **`.rodata` size** | `0x1ce` (462 bytes) | `0x3ee` (1,006 bytes) | Version 2 has 544 more bytes of read-only data. |
| **`.data` size** | `0x10` (16 bytes) | `0x10` (16 bytes) | Same initialized data size. |
| **`.bss` size** | `0x10` (16 bytes) | `0x10` (16 bytes) | Same uninitialized data size. |
| **Full symbol table** | 90 entries | 99 entries | Derived from `.symtab` size divided by 24-byte symbol entry size. |
| **Dynamic symbol table** | 22 entries | 29 entries | Derived from `.dynsym` size divided by 24-byte symbol entry size. |
| **Custom functions** | `join_paths`; `scan_and_compare`; `main` | `join_paths`; `log_event`; `calculate_checksum`; `deep_scan`; `main` | Version 2 adds logging, checksum and deeper comparison routines. |
| **Shared-library dependency** | `libc.so.6` | `libc.so.6` | Only direct `NEEDED` entry shown for each. |
| **Program interpreter** | `/lib64/ld-linux-x86-64.so.2` | `/lib64/ld-linux-x86-64.so.2` | Same dynamic ELF interpreter. |
| **Debug information** | Present; not stripped | Present; not stripped | Both include `.debug_*` sections and retain symbols. |
| **Common imported functions** | `opendir`, `readdir`, `closedir`, `malloc`, `free`, `snprintf`, `strcmp`, `printf`, `fprintf`, `perror`, `fwrite`, `strlen`, `__errno_location`, `__xstat`, `__libc_start_main`, `__stack_chk_fail` | Same common set | Directory traversal, memory, string, output and runtime support. |
| **Additional imports** | — | `fopen`, `fread`, `ferror`, `fclose`, `time`, `localtime_r`, `strftime` | Consistent with file reading/checksum and timestamped logging. |
| **`stat`-related import** | `__xstat` | `__xstat` | glibc ABI symbol shown in the imported-symbol list; do not label it as an imported symbol named `stat`. |
| **Notable disassembly** | `main` calls `scan_and_compare` at `0x13e7` | `main` calls `deep_scan` at `0x179f`; `calculate_checksum` at `0x1620` calls `fopen`/`fread`/`ferror`/`fclose` | Confirms the different internal call paths. |
| **Checksum evidence** | No checksum routine identified | FNV-1a 64-bit constants: `0xcbf29ce484222325` and `0x100000001b3` | FNV-1a is non-cryptographic. |
| **Dynamic linking flags** | `NOW PIE` | `NOW PIE` | Both use immediate binding and PIE, as reported in `FLAGS`/`FLAGS_1`. |
| **Build output size** | 22K (`ls -lh`) | 24K (`ls -lh`) | Rounded human-readable sizes from compilation evidence. |

### Summary
* `ET_DYN` alone can describe a shared object; the `PIE` flag in the dynamic section is the additional evidence that these particular `ET_DYN` files are position-independent executables.
* The ELF entry point is the process startup address (`_start`), while `main` is a C-level function reached later through runtime startup code.
* Sections organize file contents for linking and analysis; program headers describe the memory segments and interpreter information used by the loader.
* An undefined symbol in `nm` output is a symbol the executable expects to resolve externally; an imported function is not the same thing as a separate `NEEDED` library. Both binaries list `libc.so.6` as their direct shared-library dependency.
* No executable was run during this assessment, and `ldd` was not used.

### Evidence Files
The table is based on supplied terminal captures and evidence files in `evidence/`. Source files are `data_sync.c` and `data_sync_v2.c`.

