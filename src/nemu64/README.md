# Nemu64 executable reconstruction

This directory reconstructs `master/nemu64.exe`. The application now compiles
and links with the tracked historical tools. **The complete executable does
not yet match.** Instruction refinement and original image layout remain in
progress; successful compilation is not the completion criterion.

The original executable is 4,796,416 bytes, with SHA-256
`794fdc8830c5c87d770926c67291c8140cd3660576d4f0ae1cc51742f3c7cc4a`.

Build from the repository root:

```sh
python3 scripts/build_nemu.py --ordered-libraries --stage-storage \
  --function-order src/nemu64/application.order
```

The builder creates a source snapshot, generated CIC tables, output files,
logs, and a Wine prefix under `~/.ai-tmp`. It preserves raw linker output and
compares every byte with the original. Its exit status remains nonzero while
that comparison fails, even when compilation and linking succeed. Use
`--work ~/.ai-tmp/<directory>` to reuse a build directory and its Wine prefix.

The checked-in order file places the 648 application symbols in their observed
original order using the stock linker. The other two options use the recovered
CRT/import-member order and stage typed storage definitions at the inferred
original boundaries. These are the settings used by the latest complete-build
comparisons. The unmodified source snapshot and actual staged compiler inputs
are saved separately. Regenerate the order file with
`scripts/order_nemu_functions.py CANDIDATE.map --output application.order` when
a source change alters symbol decoration.

An optional library chronology recipe can be tested with:

```sh
python3 scripts/build_nemu.py --chronological-libraries --stage-storage \
  --function-order src/nemu64/application.order
```

This option implies `--ordered-libraries`. It selects the same unchanged
historical members through ordinary archives and explicit linker roots. The
two empty alias sections follow `setmode`, producing the original alignment
before `RtlUnwind`; the import hint/name pool also matches. The 187 identified
native functions now occupy their original absolute addresses with the current
tracked sources; all 54 mapped native-storage observations also have their
original addresses. These are layout observations, not a proof of complete
instruction or executable equality.

The compiled zlib and native `ByteSwap` members are also supplied through
ordinary archives. This grouping selects the original empty callback
implementation at `0x41FEF0`. Archive names and the native helper's historical
library membership are inferred; every compiled member body is preserved.

This remains an alternative under investigation: **115 of 152 IAT slots
match**, compared with 152 in the canonical recipe. Four imports have the
wrong insertion positions, displacing 37 slots. The other 148 retain their
original relative order. Neither recipe produces an exact executable. The
build report records native placement, observed native storage addresses,
import positions, Rich counts, resources, and every complete-file difference.
The inferred recipe is in `imports/link-chronology.json`; no scratch files or
modified object bodies are required.

The application uses Visual C++ 7.0, compiler 13.00.9466, with the static
single-thread runtime and whole-program optimization. `ByteSwap.c` is a native
C contribution; its separate compilation reproduces the original helper's
calling convention and accounts for the remaining native C compiler record.
The unchanged zlib 1.1.3/minizip sources use Visual C++ 6.0 RTM build 8168.
The build also reconstructs the GameShark and Kaillera import libraries with
their corresponding historical librarians.

The staged build contains 27 C++ translation units: 25 tracked source files
and two generated storage units. Seven inferred function-family splits preserve
every byte of all four program sections against the same-source 20-unit
control. The chronological build matches every original Rich compiler
ID/count pair and its checksum; the first two records remain in reversed order.
The splits and reproducible evidence are documented in
`analysis/nemu-translation-units.json` and `analysis/nemu-rich-order.json`.

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

The startup/UI fixture is available separately:

```sh
python3 scripts/check_nemu.py /path/to/rebuilt/nemu64.exe
```

It runs frozen copies in disposable Wine environments and compares the main
window, menus, About, Disclaimer, Settings, normal shutdown, and resulting
files. `analysis/nemu-runtime.json` identifies the exact tested executable;
this smoke check does not exercise game execution or certify binary equality.
