# Nemu64 executable reconstruction

This directory reconstructs `master/nemu64.exe`. The application now compiles
and links with the tracked historical tools. **The complete executable does
not yet match.** Instruction refinement and original image layout remain in
progress; successful compilation is not the completion criterion.

The original executable is 4,796,416 bytes, with SHA-256
`794fdc8830c5c87d770926c67291c8140cd3660576d4f0ae1cc51742f3c7cc4a`.

Build from the repository root:

```sh
python3 scripts/build_nemu.py
```

The builder creates a source snapshot, generated CIC tables, output files,
logs, and a Wine prefix under `~/.ai-tmp`. It preserves raw linker output and
compares every byte with the original. Its exit status remains nonzero while
that comparison fails, even when compilation and linking succeed. Use
`--work ~/.ai-tmp/<directory>` to reuse a build directory and its Wine prefix.

The application uses Visual C++ 7.0, compiler 13.00.9466, with the static
single-thread runtime and whole-program optimization. `ByteSwap.c` is a native
C contribution; its separate compilation reproduces the original helper's
calling convention and accounts for the remaining native C compiler record.
The unchanged zlib 1.1.3/minizip sources use Visual C++ 6.0 RTM build 8168.
The build also reconstructs the GameShark and Kaillera import libraries with
their corresponding historical librarians.

Private identifiers, filenames, and translation-unit boundaries are inferred.
Address inventories under `analysis/nemu-*.json` distinguish source recovery,
instruction comparisons, behavioral checks, and complete-image comparison.
Address-normalized function comparisons alone do not establish an exact build.

Host instructions are generated from C/C++ and reconstructed symbolic inline
assembly where the original uses assembly. The source does not embed original
x86 function bodies. Resources include ordinary UI assets, an algorithmically
regenerated CIC lookup table, guest boot instructions, and a formatted guest
controller-memory image. Their provenance is recorded separately; guest data
is not host executable code. See [res/README.md](res/README.md) and
[vendor/zlib-1.1.3/README.reconstruction.txt](vendor/zlib-1.1.3/README.reconstruction.txt).

`scripts/verify_nemu.py` provides the final unmasked whole-file check.
`scripts/compare_nemu_functions.py` and the mapping scripts provide diagnostic
evidence while reconstruction continues.
