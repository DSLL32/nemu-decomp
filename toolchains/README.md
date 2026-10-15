# Historical toolchains

## Installed toolchains

Each abbreviated directory contains one toolchain configuration for the Nemu
targets. Filenames are normalized to lowercase. Compiler binaries, headers,
libraries, and runtime sources remain local; Git tracks the extraction scripts,
verification results, and [version/hash manifest](manifest.json).

| Directory | Configuration | Main versions | Intended target |
|-----------|---------------|---------------|-----------------|
| `vc6` | VC6 RTM + SP5 + Processor Pack | CL 12.00.8804; C1 12.0.8867; C1XX 12.0.8964; C2 file version 13.0.9044; linker 6.0.8447 | `gs_dll.dll` |
| `vc7` | Visual Studio .NET 2002 / VC7.0 | Compiler 13.00.9466; linker and MASM 7.0.9466 | `nemu64.exe` |
| `d6` | Delphi 6 media build 6.0.6.163 | DCC32 executable banner 14.0; resource compiler 5.40 | `debugger.dll`, `dialogs.dll` |

`vc6` includes static debug MFC/CRT libraries, MFC/ATL headers and sources,
resource tools, PDB support, and the Processor Pack headers and MASM. The
Processor Pack optimizer's file version starts with **13.0**, despite belonging
to this VC6 configuration; the compiled C++ Rich ID is **00312354** as required.

`vc7` includes CRT/ATL/MFC sources and libraries, MASM, resource tools, and the
Platform SDK. The SDK's **1,525 files** were restored from **49 cabinets** embedded
in `vs_setup.msi`, using the MSI's file and directory tables. `mspdb70.dll`,
`msobj10.dll`, and the VC7 runtime DLLs are colocated with the compiler;
`msobj10.dll` is required by LTCG.

`d6` includes DCC32, the resource compiler and its `rw32core.dll` dependency,
linker support, command-line utilities, compiled units, RTL/VCL sources, and
runtime BPL/MAP files for reference. The exact Delphi update used by the original
plugins remains unresolved; this installation successfully reproduces their
observed `TControl` layout.

### Running and checking the tools

[run_toolchain.py](../scripts/run_toolchain.py) sets the appropriate Wine
include/library paths. Use relative source paths from the working directory,
or Windows paths for absolute filenames. Create a reusable Wine prefix first:

```sh
mkdir -p "$HOME/.ai-tmp"
export WINEPREFIX="$(mktemp -d "$HOME/.ai-tmp/nemu-wine.XXXXXX")/wine"
python3 scripts/run_toolchain.py vc6 cl /c example.cpp
python3 scripts/run_toolchain.py vc7 cl /c /O2 /GL example.cpp
python3 scripts/run_toolchain.py d6 dcc32 example.dpr
```

Without `WINEPREFIX`, the runner allocates a new prefix under `~/.ai-tmp` and
prints its location. The Delphi runner supplies the installed unit/include/
resource search paths. Use this runner in place of the original installer-
dependent `vcvars32.bat` files.

```sh
python3 scripts/check_toolchains.py --output analysis/toolchain-smoke.json
```

The [verification suite](../scripts/check_toolchains.py) compiles and links a
VC6 debug MFC DLL with PDB, MASM, and resources; a VC7 LTCG console program with
PDB, MASM, and resources; and a Delphi VCL DLL with resources and a detailed map.
It executes the generated console probe, verifies Microsoft Rich IDs and
checksums, and checks the Delphi layout. All checks passed; the
[recorded results](../analysis/toolchain-smoke.json) include the emitted IDs.
Generated files and logs stay under a new `~/.ai-tmp` directory.

### Installer inspection and reproducibility

| Staged media | Use |
|-------------|-----|
| `en_vs6_ent_cd1.iso` | VC6 RTM compiler, headers, libraries, sources, and shared tools |
| `2001-03_X08-02111_X06-21660_CD_ROM.7z` | SP5 English payloads plus **`enu/PP/vcpp.exe`**, the matching Processor Pack |
| `VSPROD1.iso`, `VSPROD2.iso`, `VSPROD3.iso` | VC7 files spread across the discs, shared compiler dependencies, and MSI-embedded SDK |
| `Borland Delphi 6.0 Enterprise Edition (ISO).7z` | Delphi 6 compiler, units, sources, and runtime references |
| `en_vs6_ent_cd2.iso`, `en_vs6_ent_cd3.iso` | Other VS6 products/support content; no additional native compiler components needed |
| `VSPROD4.iso` | Additional product samples/support content; no needed compiler payload |
| `VSWCUD1.iso` | Windows component prerequisites such as IE, MDAC, and .NET Framework |
| `en_codeadv.exe` | Code Advisor for **Visual Basic 6**, version 1.1 |
| `en_kb3096896-x86.msi` | VB6 SP6 extended-runtime security update |
| `en_mdac28_sp1.exe` | MDAC 2.8 SP1 data-access runtime |

The last six rows are not applied to the native compiler layouts. None supplies
a missing Nemu compiler component.

[unpack_toolchains.py](../scripts/unpack_toolchains.py) reproduces the layouts
from `toolchains-staging/`, requiring `7z` and `msiinfo` (`msitools` on Debian).
It extracts the SP5 MDF's 2,048-byte data payloads from 2,448-byte raw sectors,
overlays SP5 on VC6 RTM with consistent casing, then applies the Processor Pack's
INF mappings, including renamed intrinsic headers. It does not run installer
executables. Scratch files are allocated under `~/.ai-tmp`.

```sh
python3 scripts/unpack_toolchains.py
```

The script refuses to overwrite existing `vc6`, `vc7`, or `d6` directories.
Use `--dest` for a separate extraction. A fresh extraction was verified to
reproduce every installed file byte-for-byte. The manifest records input-media
SHA-256 hashes, component versions/hashes, file counts, sizes, and a whole-tree
digest for each layout. The tree digest is SHA-256 over UTF-8 records of
`file_sha256`, two spaces, relative POSIX filename, and newline, in the Python
`Path` ordering used by the extraction script.

## Nemu64 0.8 toolchain identification

Static inspection of the binaries in `master/` identifies the three toolchain
generations installed above.

| Binary | Toolchain identification | Confidence |
|--------|--------------------------|------------|
| `master/nemu64.exe` | Visual Studio .NET 2002 / Visual C++ 7.0, compiler 13.00.9466, linker 7.00.9466, with MASM 7.0 contributions | High; explicit Rich build records |
| `master/gs_dll.dll` | Visual C++ 6.0 SP5 **with Processor Pack**, compiler build 9044; static debug MFC and CRT | High; Rich records and retained build metadata |
| `master/Plugin/debugger.dll` | Borland Delphi 6, Win32 VCL | High for Delphi 6; exact update level unresolved |
| `master/Plugin/dialogs.dll` | Borland Delphi 6, Win32 VCL | High for Delphi 6; exact update level unresolved |

The filename is **`dialogs.dll`**, plural. It belongs in the emulator UI scope:
its three embedded forms are `TFormHeader` (Rom info), `TFormNemuSettings`, and
`TFormRomSettings`. It exports `NemuSettings` alongside `ToolInit`, `ToolFree`,
`ToolFunc`, and `ToolGetMenuText`. The executable contains the corresponding
plugin lookup names. The GameShark API is provided separately by `gs_dll.dll`:
the executable imports `GSInitialize`, `GSConfigure`, and `GSUpdate` using
Microsoft C++ decorated names.

### Microsoft compiler evidence

`nemu64.exe` reports PE linker version 7.0 and a timestamp of
2003-03-23 04:51:00 UTC. Its Rich records include:

| Comp ID | Count | Meaning |
|---------|------:|---------|
| `001c24fa` | 93 | VC7 C, build 9466 |
| `001d24fa` | 2 | VC7 C++, build 9466 |
| `002c24fa` | 27 | VC7 C++ LTCG contributions, build 9466 |
| `004024fa` | 20 | MASM, build 9466 |
| `004524fa` | 1 | Resource converter, build 9466 |
| `003d24fa` | 1 | Linker, build 9466 |

The image also contains older VC6 and import-library records. Those describe
linked inputs; they do not change the final VC7 identification. The LTCG records
matter for later matching experiments, but do not establish every source file's
optimization settings. The image has no PE debug directory.

`gs_dll.dll` reports PE linker version 6.0 and a timestamp of
2002-01-12 16:58:06 UTC. Rich comp ID **`00312354`**, count **8**, identifies
VC6 SP5 Processor Pack C++ build **9044**. This count agrees with the eight
application objects in its retained link command:
`Addsubcode`, `gs_dll`, `gs_exeDlg`, `GS_interpret`, `NewCodeDlg`,
`PropertiesDlg`, `RW_INI_files`, and `StdAfx`.

The retained command at file offset `0x18a6d4` uses `/debug`,
`/incremental:yes`, and `/pdbtype:sept`. Embedded directives name
`nafxcwd.lib` and `libcmtd.lib`; `libcpmtd.lib` also appears in the library paths.
These identify static debug MFC, C, and C++ runtimes. Older compiler records in
the same image therefore must not be mistaken for the application compiler.
The `000420ff` record identifies build 8447 import/export generation, consistent
with the SP5 toolchain; it is not a separate explicit final-linker build record.

The CodeView NB10 record points to
`G:\nemu\gs_dll\Debug\gs_dll.pdb`, signature `3b4816e0`, age 60.
The PDB itself is not included in `master/`. Numerous C++ symbol names,
original source paths, and link directives survive in the DLL and will help
with subsequent decompilation.

Build interpretations use the primary
[richprint compiler ID database](https://github.com/dishather/richprint/blob/master/comp_id.txt),
including its product-ID definitions for VC7 LTCG and VC6 Processor Pack.

### Delphi evidence

Both DLLs have Borland-style `CODE`, `DATA`, `BSS`, `.idata`, `.edata`, `.reloc`,
`.rsrc` sections, Delphi RTTI, `PACKAGEINFO`/`DVCLAL` resources, and binary
`TPF0` forms. Neither imports a Delphi runtime package, consistent with static
RTL/VCL linkage. Their PE linker field is 2.25 and their common timestamp is
`0x2a425e19` (1992-06-19); these do **not** identify the Delphi release or
the actual build date.

The more specific discriminator is the embedded `TControl` VMT/RTTI:

| DLL | RTTI file offset | VMT virtual address | Instance size at VMT minus 40 | DWORD at VMT minus 84 |
|-----|------------------|---------------------|-------------------------------|----------------------|
| `debugger.dll` | `0x65d78` | `0x466700` | `0x15c` | `0x6f6f4207` |
| `dialogs.dll` | `0x509d0` | `0x451358` | `0x15c` | `0x6f6f4207` |

Both match Delphi **6** under
[Detect It Easy's Delphi detection rule](https://github.com/horsicq/Detect-It-Easy/blob/master/db/PE/compiler_Delphi.4.sg),
which explicitly distinguishes Delphi 6 from 7 for this VCL layout.
Delphi 6 corresponds to compiler version **14.0**, conditional **VER140**, and
package version **60** in
[Embarcadero's compiler version table](https://docwiki.embarcadero.com/RADStudio/Florence/en/Compiler_Versions).
The exact Delphi update level still requires comparison with known compiler/RTL
builds. `DVCLAL` alone is not sufficient to select an exact installation edition.

`toolchains-staging/` contains `Borland Delphi 6.0 Enterprise Edition (ISO).7z`.
Its compiler and runtime files report version **6.0.6.163**; `DCC32.EXE`
contains the compiler banner **14.01**. The media's `vcl60.bpl` independently
matches the same Delphi 6 `TControl` rule (instance size `0x15c`, with the
Delphi 6 discriminator branch). This corroborates the major-version result,
but does not prove that the plugins used this exact update. Archive/member
hashes and reference values are saved in
[delphi6-reference.json](../analysis/delphi6-reference.json). That media now
supplies `d6/`. The executed DCC32 banner reports 14.0; 14.01 above is an
additional embedded compiler string.

The debugger retains 80 package unit names and 12 forms: bookmarks, commands,
opcode breaks, register-access breaks, memory dumping, image viewing, library
options, memory, registers, search, statistics, and symbols. The dialogs plugin
retains 67 unit names and the three forms described above. Both also contain
`ThemeMgr`, `ThemeSrv`, `JwaUxTheme`, and `JwaTmSchema`, so their third-party
theming components will need to be accounted for in a reconstruction.

### Reproducing the inspection

Run from the repository root:

```sh
python3 scripts/pe_fingerprints.py master/nemu64.exe master/gs_dll.dll master/Plugin/debugger.dll master/Plugin/dialogs.dll > analysis/pe-fingerprints.json
```

The standard-library-only script does not execute the targets. The checked-in
[evidence file](../analysis/pe-fingerprints.json) records SHA-256 hashes, PE
headers, Rich IDs and checksum validation, imports, exports, CodeView metadata,
selected build strings, Delphi package units, form identities, and the exact
TControl discriminator values. Offsets and addresses in JSON are decimal.
Header/import/export results were cross-checked against GNU `objdump`.

The installed toolchains reproduce the relevant build fingerprints in fresh
probes. Matching individual original functions and identifying the exact Delphi
update level remain subsequent decompilation work.
