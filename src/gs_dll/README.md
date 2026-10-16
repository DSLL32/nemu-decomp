This directory reconstructs `master/gs_dll.dll`, Nemu64's GameShark interpreter
and MFC cheat editor, as C++ source and editable Windows resources. All compiled
code and initialized program data match the original. The build produces a
**complete byte-exact DLL** after explicitly restoring historical noncode linker
residue and build metadata. The untouched linker output is preserved separately.

The target is 2,170,928 bytes, with SHA-256
`35a69ae0172add4355a7f4c4fa30b84adfffbd4bade67cce88631502627bc45b`.
The [clean build comparison](../../analysis/gs-comparison.json) checks every
file byte, including headers, padding, resources, imports, exports, and
relocations. It also records the raw linker output's hash and every metadata
restoration region.

Run from the repository root:

```sh
python3 scripts/build_gs.py --report analysis/gs-comparison.json
```

The build requires Python 3.9 or later, 32-bit Wine support, and the tracked
`toolchains/vc6` toolchain. It creates its project, output files, and disposable
Wine prefix under `~/.ai-tmp`. `--work PATH` selects a directory under that root;
omit it for a clean build. `gs_dll.linked.dll` is the untouched linker output;
`gs_dll.dll` is the final archival reproduction. A MAP file is generated unless
`--no-map` is supplied. The script refuses restoration if any nonmetadata byte
differs and exits nonzero if the final file differs from the original.

The confirmed toolchain is Visual C++ 6.0 SP5 with the Processor Pack: compiler
driver 12.00.8804, C++ front end 12.0.8964, code generator 13.0.9044, and linker
6.0.8447. The project uses static debug MFC and CRT, with
`/MTd /W3 /Gm /GX /GZ /ZI /Od`, precompiled headers, and incremental linking.
The original `G:\nemu\gs_dll` source path and
`E:\Microsoft Visual Studio\VC98` toolchain path are reproduced through Wine
drive mappings. Their spellings affect debug strings and library ordering.
Toolchain provenance and hashes are in
[toolchains/README.md](../../toolchains/README.md) and
[manifest.json](../../toolchains/manifest.json).

The linker and resource converter run with Wine's Windows 98 application
setting and the original VC6 `msvcrt.dll`, extracted from the tracked
redistributable. This reproduces the original incremental-linker heap base and
allocator markers. Compilation uses Wine's default runtime. The runtime
override is confined to the link invocation and its child process.

There are two archival residue ranges: 496,640 bytes inside the linker's
uninitialized `.textbss` reserve, and four alignment bytes after the PE debug
directory. The original linker leaked its memory into these ranges, including
ILK structures, obsolete input timestamps, process addresses, allocator residue,
and symbol names. A clean link emits zeroes there. The rebuilt ILK independently
reproduces all 185,756 surviving name-pool bytes, but historical unused memory
cannot be determined uniquely from source. The classification and experiments
are recorded in [gs-linker-state.json](../../analysis/gs-linker-state.json).

[restore_gs_metadata.py](../../scripts/restore_gs_metadata.py) preserves these
496,644 bytes from [gs-linker-residue.bin](../../analysis/gs-linker-residue.bin).
It also sets three PE/debug/export timestamps to `2002-01-12 16:58:06 UTC`, and
the NB10 PDB signature and age to their original values. The exact offsets,
values, and hashes are in
[gs-build-metadata.json](../../analysis/gs-build-metadata.json). Before writing,
the script hashes every byte outside those declared regions and requires an
exact match. It validates the archival asset, then requires the complete target
SHA-256. Executable instructions, initialized program data, resources, import
and export entries, and relocation entries are never restored from the target.
Full-file identity therefore includes archival preservation of incidental
linker state; the native compiler does not regenerate that state independently.

An existing output can be checked without building or restoring metadata:

```sh
python3 scripts/verify_gs.py /path/to/gs_dll.dll
```

The differential runtime probe additionally requires Xvfb:

```sh
python3 scripts/check_gs.py /path/to/gs_dll.dll --report analysis/gs-runtime.json
```

The [runtime comparison](../../analysis/gs-runtime.json) exercises the three
GameShark exports, code activation, deactivation and reactivation, New Code,
Add and Properties dialogs, INI saving and reopening, ROM reinitialization, and
unloading. It checks all eight supported interpreter types against expected RAM,
including both outcomes of every condition, byte truncation, chained skips,
unsupported types, and repeated updates. Native control snapshots, stdout, and
generated files must match. This uses synthetic RAM and deterministic valid
input; it does not cover every interactive operation, malformed files, or
rendered pixels. Whole-file equality is a separate check.

The eight translation-unit names survive in the original linker records:

| Source | Role |
| --- | --- |
| `StdAfx.cpp` | Precompiled MFC headers |
| `gs_dll.cpp` | Application object and three standard MFC COM exports |
| `gs_exeDlg.cpp` | Main cheat editor and About dialog |
| `GS_interpret.cpp` | GameShark exports, code execution, and active-list helpers |
| `Addsubcode.cpp` | Add address/value dialog |
| `NewCodeDlg.cpp` | New cheat dialog |
| `PropertiesDlg.cpp` | Address/value properties dialog |
| `RW_INI_files.cpp` | INI wrappers and profile-path storage |

`GSInitialize(char *, char *, char *)`, `GSConfigure()`, and `GSUpdate()` use
the original C++ export names and cdecl convention. The initializer receives
the byte-swapped ROM header, host RAM, and a complete INI filename; its third
parameter's reconstructed name `IniDirectory` does not imply a directory-only
argument. ROM titles select `<title>.Cheats` sections. The code editor retains
active names across dialog reopening within the process; it does not persist
activation to the INI file.

Types `80`/`A0` write bytes, `81`/`A1` write halfwords, `D0`/`D1` condition on
equality, and `D2`/`D3` condition on inequality. A failed condition skips one
following code. Byte and halfword addresses use XOR 3 and XOR 2 respectively
for the emulator's RAM layout. Other code types have no effect in this DLL.

The resource script reproduces all 38 original resource payloads and the full
resource section. Application dialogs are editable RC text; icons and the
bitmap are recovered assets, while standard MFC resources come from the
tracked toolchain. The original resource identifiers and visible strings are
retained.

[gs-symbols.json](../../analysis/gs-symbols.json) inventories the surviving
linker names, source paths, imports, exports, and command-line evidence.
[gs-addresses.json](../../analysis/gs-addresses.json) distinguishes recovered
function boundaries from inferred operand addresses. The
[library address map](../../analysis/gs-library-addresses.json) independently
matches library bodies and import identities; it requires Capstone to regenerate
with `scripts/map_gs_libraries.py`. The
[object comparison](../../analysis/gs-object-comparison.json) diagnoses all eight
application objects, including relocation targets. Its separate direct linked
comparison covers all 79 unique application routines, totaling 13,277 bytes,
without masking. Object diagnostics alone are not the completion criterion.

Class, method, and many global names survive as decorated symbols. Local names,
header organization, comments, and some private data names are inferred; the
flat project and `res` directory follow the retained paths and MFC conventions.
Unused source and original formatting cannot be recovered uniquely. Empty
callbacks and observed quirks are retained, including the INI integer helper's
32-byte buffer with a 256-byte API limit. These reproduce the original linked
implementation and are not unfinished routines.
