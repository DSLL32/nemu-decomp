# Historical toolchains

## Installed toolchains

Each abbreviated directory contains one toolchain configuration for the Nemu
targets. Filenames are normalized to lowercase. Compiler binaries, headers,
libraries, and runtime sources are tracked directly in Git, alongside the
extraction scripts, verification results, and [version/hash manifest](manifest.json).
The toolchains use ordinary Git blobs without LFS. Git attributes preserve their
original bytes, including line endings, so checkouts retain the manifest hashes.
Installer media in `toolchains-staging/` remains untracked.

| Directory | Configuration | Main versions | Intended target |
|-----------|---------------|---------------|-----------------|
| `vc6` | VC6 RTM + SP5 + Processor Pack | CL 12.00.8804; C1 12.0.8867; C1XX 12.0.8964; C2 file version 13.0.9044; linker 6.0.8447 | `gs_dll.dll` |
| `vc7` | Visual Studio .NET 2002 / VC7.0 | Compiler 13.00.9466; linker and MASM 7.0.9466 | `nemu64.exe` |
| `d6u1` | Delphi 6 General Update 1 | Compiler / RTL / VCL build 6.0.6.190 | `dialogs.dll` |
| `d6u2` | Delphi 6 General Update 2 | Compiler / RTL / VCL build 6.0.6.240 | `debugger.dll` |

`vc6` includes static debug MFC/CRT libraries, MFC/ATL headers and sources,
resource tools, PDB support, and the Processor Pack headers and MASM. The
Processor Pack optimizer's file version starts with **13.0**, despite belonging
to this VC6 configuration; the compiled C++ Rich ID is **00312354** as required.

`vc7` includes CRT/ATL/MFC sources and libraries, MASM, resource tools, and the
Platform SDK. The SDK's **1,525 files** were restored from **49 cabinets** embedded
in `vs_setup.msi`, using the MSI's file and directory tables. `mspdb70.dll`,
`msobj10.dll`, and the VC7 runtime DLLs are colocated with the compiler;
`msobj10.dll` is required by LTCG.

Both Delphi directories include DCC32, the resource compiler and its `rw32core.dll` dependency,
linker support, command-line utilities, compiled units, RTL/VCL sources, and
runtime BPL/MAP files for reference. The plugins match different Delphi runtime
builds: Update 1 for dialogs and Update 2 for the debugger. Neither selected
configuration includes the later standalone RTL updates. The comparison below
records the evidence and the distinction between identifying library code and
proving the exact compiler executable used for application code.

`d6u1/bin/dcc32-personal.exe` is the unmodified Personal-edition compiler from
General Update 1, also build 6.0.6.190. The existing `dcc32.exe` is Enterprise.
The dialogs reconstruction uses Personal because its automatic 16-byte `DVCLAL`
resource matches the original. Both compilers generate matching executable code
with the tracked Update 1 libraries; the edition stamp distinguishes them.
The Personal compiler stays in the same version directory, and its installer
URL and hashes are recorded in [delphi-updates.json](delphi-updates.json).

`d6u2/bin/dcc32-personal.exe` similarly provides the unmodified General Update 2
Personal compiler (6.0.6.240) for the debugger's matching edition stamp. A full
VCL/theme probe emits identical code, data, imports and relocations with the
Update 2 Personal and Enterprise compilers; the Personal DVCLAL stamp matches
the original debugger.

### Running and checking the tools

[run_toolchain.py](../scripts/run_toolchain.py) sets the appropriate Wine
include/library paths. Use relative source paths from the working directory,
or Windows paths for absolute filenames. Create a reusable Wine prefix first:

```sh
mkdir -p "$HOME/.ai-tmp"
export WINEPREFIX="$(mktemp -d "$HOME/.ai-tmp/nemu-wine.XXXXXX")/wine"
python3 scripts/run_toolchain.py vc6 cl /c example.cpp
python3 scripts/run_toolchain.py vc7 cl /c /O2 /GL example.cpp
python3 scripts/run_toolchain.py d6u1 dcc32 dialogs.dpr
python3 scripts/run_toolchain.py d6u1 dcc32-personal dialogs.dpr
python3 scripts/run_toolchain.py d6u2 dcc32 debugger.dpr
python3 scripts/run_toolchain.py d6u2 dcc32-personal debugger.dpr
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
PDB, MASM, and resources; and a Delphi VCL DLL with resources and a detailed map
using each of the two Delphi configurations.
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
| `delphi6-updates/D6_upd1_ent.exe`, `delphi6-updates/D6_upd2_ent.exe` | English Update 1 and Update 2 overlays for the two Delphi configurations |
| `delphi6-updates/D6_upd1_per.exe` | Update 1 Personal compiler, installed alongside Enterprise for the original dialogs edition stamp |
| `delphi6-updates/D6_upd2_per.exe` | Update 2 Personal compiler for the original debugger edition stamp |
| `en_vs6_ent_cd2.iso`, `en_vs6_ent_cd3.iso` | Other VS6 products/support content; no additional native compiler components needed |
| `VSPROD4.iso` | Additional product samples/support content; no needed compiler payload |
| `VSWCUD1.iso` | Windows component prerequisites such as IE, MDAC, and .NET Framework |
| `en_codeadv.exe` | Code Advisor for **Visual Basic 6**, version 1.1 |
| `en_kb3096896-x86.msi` | VB6 SP6 extended-runtime security update |
| `en_mdac28_sp1.exe` | MDAC 2.8 SP1 data-access runtime |

The last six rows are not applied to the native compiler layouts. None supplies
a missing Nemu compiler component.

[unpack_toolchains.py](../scripts/unpack_toolchains.py) reproduces the layouts
from `toolchains-staging/`, requiring `7z`, `unshield`, and `msiinfo` (`msitools` on Debian).
It extracts the SP5 MDF's 2,048-byte data payloads from 2,448-byte raw sectors,
overlays SP5 on VC6 RTM with consistent casing, then applies the Processor Pack's
INF mappings, including renamed intrinsic headers. For Delphi it verifies the
update archive hashes, extracts the nested InstallShield cabinets, and overlays
the English compiler, libraries, sources, and runtime packages. It does not run installer
executables. Scratch files are allocated under `~/.ai-tmp`.

```sh
python3 scripts/unpack_toolchains.py
```

The script refuses to overwrite existing `vc6`, `vc7`, `d6u1`, or `d6u2` directories.
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
| `master/Plugin/debugger.dll` | Delphi 6 General Update 2 RTL/VCL, build 6.0.6.240 | High; unique candidate match across tested runtime configurations |
| `master/Plugin/dialogs.dll` | Delphi 6 General Update 1 RTL/VCL, build 6.0.6.190 | High; unique candidate match across tested runtime configurations |

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
The update comparison below identifies the matching RTL/VCL builds.
`DVCLAL` alone is not sufficient to select an exact installation edition.

`toolchains-staging/` contains `Borland Delphi 6.0 Enterprise Edition (ISO).7z`.
Its compiler and runtime files report version **6.0.6.163**; `DCC32.EXE`
contains the compiler banner **14.01**. The media's `vcl60.bpl` independently
matches the same Delphi 6 `TControl` rule (instance size `0x15c`, with the
Delphi 6 discriminator branch). This corroborates the major-version result,
but does not prove that the plugins used this exact update. Archive/member
hashes and reference values are saved in
[delphi6-reference.json](../analysis/delphi6-reference.json). That media supplies
the base for both updated directories. Its executed DCC32 banner reports 14.0; 14.01 above is an
additional embedded compiler string.

The debugger retains 80 package unit names and 12 forms: bookmarks, commands,
opcode breaks, register-access breaks, memory dumping, image viewing, library
options, memory, registers, search, statistics, and symbols. The dialogs plugin
retains 67 unit names and the three forms described above. Both also contain
`ThemeMgr`, `ThemeSrv`, `JwaUxTheme`, and `JwaTmSchema`, so their third-party
theming components will need to be accounted for in a reconstruction.

### Delphi runtime comparison

The six-candidate [comparison results](../analysis/delphi-update-matches.json)
select **General Update 1 for `dialogs.dll`** and **General Update 2 for
`debugger.dll`**. No single supplied runtime configuration matches both plugins.
The installed directories are therefore `d6u1` and `d6u2`.

| Reference configuration | Eligible blocks | Unmatched in debugger | Unmatched in dialogs |
|-------------------------|----------------:|----------------------:|---------------------:|
| Original media (RTM) | 1,192 | 23 | 2 |
| General Update 1 | 1,194 | 21 | **0** |
| General Update 2 | 1,192 | **0** | 20 |
| General Update 2 + RTL Update 1 | 1,195 | 4 | 24 |
| General Update 2 + RTL Update 2 | 1,193 | 4 | 23 |
| General Update 2 + RTL Update 2 + RTL Update 3 | 1,192 | 5 | 24 |

For the winning comparisons, Update 1 has **1,186 unique** block matches in
`dialogs.dll` and 8 blocks with multiple possible addresses. Update 2 has
**1,185 unique** block matches in `debugger.dll` and 7 ambiguous blocks. These
counts describe the eligible reference blocks, not every function in a plugin.

The matcher also checks the destinations of direct calls/jumps when both
endpoint blocks were identified. **2,464 edges** agree for Update 1/dialogs and
**2,440 edges** agree for Update 2/debugger, with **zero conflicts**. Separately,
RTTI locates each original `TControl` VMT; its `Destroy`, `SetParent`, and
`WndProc` slots point to the matching bodies in both winning comparisons.
This anchors those identities independently of scanning for a similar body.

The discriminating evidence spans several units:

- Update 1 supplies the `@LStrCatN` instruction and `TReader.ReadPropValue`
  changes absent from RTM. The string helper's `mov edi, eax` becomes
  `mov edi, ecx` in both originals; the [initial RTM comparison](../analysis/delphi-rtl-comparison.json)
  records the exact byte and original addresses.
- Update 2 matches the debugger's changed integer conversion, thread-local
  counter, action, image-list, and control routines. The dialogs plugin instead
  matches their Update 1 forms. `TControl.WndProc` is one VMT-anchored example.
- RTL Updates 1 and 2 introduce different synchronization method bodies:
  `TMultiReadExclusiveWriteSynchronizer.BeginWrite`, `EndWrite`, `BeginRead`,
  and `EndRead` do not match the debugger. RTL Update 3 additionally changes
  `TReader.ReadRootComponent`. The separate RTL updates are therefore not
  applied to either installed configuration.

[check_delphi_updates.py](../scripts/check_delphi_updates.py) independently
extracts the original media and all updates, builds a static VCL probe with each
candidate's compiler and precompiled units, emits detailed maps, and compares
both original plugins. The variants use the documented cumulative update
chains; RTL Update 2 includes RTL Update 1. All work occurs in new scratch
space and no original plugin is executed.

```sh
python3 scripts/check_delphi_updates.py --output analysis/delphi-update-matches.json
```

This requires `7z`, `unshield`, Wine, and Debian's `python3-capstone`.
`WINEPREFIX` can reuse an existing scratch prefix. The full block inventories,
reference executables/maps, and compile logs remain in the printed scratch
folder. The JSON report retains hashes, compiler/runtime versions, matching
counts, examples, missing block names, edge checks, and VMT anchors.

[compare_delphi_rtl.py](../scripts/compare_delphi_rtl.py) is the underlying
matcher. It selects map-bounded blocks of 32–2,048 bytes with at least 24 fixed
bytes, requires each signature to be unique in its reference, and masks PE
`HIGHLOW` relocations and external `rel32` operands. Internal branch distances
and ordinary instruction operands remain fixed. `--validate-links` enables
edge and VMT checks; `--matches PATH` writes the complete block inventory.

These results identify matching **RTL/VCL builds** and justify using their
corresponding compiler distributions for reconstruction. They do not prove
the exact application `dcc32.exe` or every compiler switch independently of
the precompiled libraries. Unresolved callees/global data and possible local
patches remain limitations. A missing signature alone is not conclusive:
smart linking, alignment, and block boundaries can also affect matching.

### Recovered Delphi update media

All five reference updates were recovered on 2026-10-15 and saved under
`toolchains-staging/delphi6-updates/`. General Updates 1 and 2 supply the installed
configurations; the three standalone RTL updates remain comparison references. The
[download manifest](delphi-updates.json) records exact source URLs, sizes,
SHA-256 hashes, inspected component versions, and the extraction scratch path.

| Update | Surviving download | Verified payload version |
|--------|--------------------|--------------------------|
| General Update 1, Enterprise | [D6_upd1_ent.exe](http://download.borlandforum.com/delphi/Delphi6Update/D6_upd1_ent.exe) | DCC32 / RTL / VCL `6.0.6.190` |
| General Update 2, Enterprise | [D6_upd2_ent.exe](http://download.borlandforum.com/delphi/Delphi6Update/D6_upd2_ent.exe) | DCC32 / RTL / VCL `6.0.6.240` |
| RTL Update 1, Enterprise | [D6_RTL1_ent.exe](http://download.borlandforum.com/delphi/Delphi6Update/D6_RTL1_ent.exe) | RTL `6.0.6.241` |
| RTL Update 2, Enterprise | [del6_ent_rtl2.exe](https://t1.daumcdn.net/cfile/tistory/241A083357BD17FE14) | RTL `6.0.6.242` |
| RTL Update 3 | [d6_rtl3.zip](https://t1.daumcdn.net/cfile/tistory/2226563357BD17FB0D) | RTL `6.0.6.243` |

The BorlandForum links work over **HTTP**; HTTPS failed in this environment.
The last two files are attachments on [this preserved download page](https://tctt.tistory.com/57).
The successful CDN requests supplied that page as the HTTP `Referer` and used
`User-Agent: Mozilla/5.0`. Its General Update 2 attachment is split into EGG
volumes; the complete EXE above avoids needing that archive format.

General Update 2 was independently downloaded from
[DowngradeFiles](https://downgradefiles.pdp-11.net/download.php?file=323e15d516e4080c4f0406b9a0eea5f8)
and has the same SHA-256 as the BorlandForum copy. All five archives extracted
successfully with `7z`; all four EXEs' nested InstallShield `data1.cab` payloads
also extracted successfully with `unshield`. Included Borland release notes and
PE version resources agree with the update labels. These checks establish
readability and content consistency; no original publisher checksums were
available for authentication. None of the installers was executed.

The included RTL Update 2 release notes explicitly say that it includes the
previous RTL update. General Update 2 includes General Update 1. Retaining
the superseded updates is useful here because we need separate historical
reference builds. RTL Update 3 is a ZIP containing `Classes.pas`, release/debug
`Classes.dcu`, `rtl.dcp`, `rtl60.bpl`, `rtl60.map`, and `BaseRTL.Msm`.

For reference extraction, create a new directory under `~/.ai-tmp`, unpack an
EXE there with `7z x`, then unpack its `data1.cab` into a second directory with
`unshield -d DEST x PATH/TO/data1.cab`. This exposes compiler binaries, units,
sources, packages, and maps without running the installer. The automated update
comparison performs these extractions into isolated reference configurations.

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
