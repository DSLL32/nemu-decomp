# Historical toolchains

## Nemu64 0.8 toolchain identification

Static inspection of the binaries in `master/` identifies three required
toolchain generations. None is currently installed under `toolchains/`;
the VC4.2 and VC5 layouts above are older than all four targets.

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

The workspace also contains `Borland Delphi 6.0 Enterprise Edition (ISO).7z`.
Its compiler and runtime files report version **6.0.6.163**; `DCC32.EXE`
contains the compiler banner **14.01**. The media's `vcl60.bpl` independently
matches the same Delphi 6 `TControl` rule (instance size `0x15c`, with the
Delphi 6 discriminator branch). This corroborates the major-version result,
but does not prove that the plugins used this exact update. Archive/member
hashes and reference values are saved in
[delphi6-reference.json](../analysis/delphi6-reference.json). Reference files
were extracted into scratch space; the toolchain has not been installed.

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

For matching work, the next toolchain additions are VC7.0 build 9466 (including
MASM and its original libraries), VC6 SP5 plus Processor Pack, and Delphi 6
with its VCL/RTL. Compiler identification is complete at this level; no
recompilation match or Delphi update-level identification has yet been made.
