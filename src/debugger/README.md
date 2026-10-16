This directory reconstructs `master/Plugin/debugger.dll`, the Nemu64 debugger
plugin, as Delphi source and editable form resources. **The complete rebuilt
DLL is byte-for-byte identical to the original.** The
[recorded clean build comparison](../../analysis/debugger-comparison.json)
has zero differing file bytes, with all 80 linked unit ranges, 144 published
method/export blocks, and twelve form resources matching exactly.

The target is 977,408 bytes, with SHA-256
`e10e8b27a53951dbaeba6d72bf61e3859d3e2654741f00d29798c993411637bd`.
The completion test compares every byte, including executable code, data,
resources, imports, exports, relocations, headers, layout, and padding.

Run from the repository root:

```sh
python3 scripts/build_debugger.py --report analysis/debugger-comparison.json
```

The build requires Wine, Python with Capstone, and the tracked `d6u2` and `vc7`
toolchains. It uses the unmodified Delphi 6 General Update 2 Personal compiler,
`toolchains/d6u2/bin/dcc32-personal.exe` (6.0.6.240), with the Update 2 RTL/VCL
units. The Personal compiler emits the original `DVCLAL` edition resource.
Toolchain versions, installer provenance, and hashes are recorded in
[toolchains/README.md](../../toolchains/README.md) and
[delphi-updates.json](../../toolchains/delphi-updates.json).

The script creates a scratch directory and Wine prefix under `~/.ai-tmp`, or
reuses `WINEPREFIX` if supplied. `--work PATH` selects an existing build directory
under `~/.ai-tmp`; `--incremental` reuses compiled units. A second compiler pass
links the cached units, reproducing the original initialization and package
ordering. The script prints the DLL, MAP, and comparison-report paths and exits
nonzero while any file byte differs. Delphi's streaming implementation converts the text DFM files into
resources, and VC7's resource compiler compiles the recovered manifest.

No target executable bytes are embedded in source or patched into the linked
DLL. The build preserves `debugger.unstamped.dll`, then sets only the PE
resource-directory timestamps to the original value, `2002-06-25 22:18:06`.
Resource payloads, executable code, and data are left as emitted by the tools.
The report records the untouched compiler output's hash and the final
comparison. An existing build can be checked independently:

```sh
python3 scripts/verify_debugger.py /path/to/debugger.dll --map /path/to/debugger.map
```

The differential runtime probe additionally requires Xvfb and runs both DLLs
through the same disposable Win32 host, using a fresh scratch Wine prefix:

```sh
python3 scripts/check_debugger.py /path/to/debugger.dll --report analysis/debugger-runtime.json
```

The [recorded runtime comparison](../../analysis/debugger-runtime.json) passed
for all four exports, the ten menu windows, CPU/RSP and bookmark notifications,
register-access and opcode breakpoint controls, window closing, and unloading.
The two runs produced identical stdout and generated-file inventories. This
host supplies synthetic memory and registers and compares native control state;
it does not exercise every interactive operation or compare rendered pixels.
The report records the original and rebuilt DLL hashes.

The twelve form units retain the names present in the original DLL:

| Source unit | Form class | Role |
| --- | --- | --- |
| `UnitDebugCommands` | `TFormDebug` | CPU/RSP commands, stepping, breakpoints, symbols, RDP history |
| `UnitDebugMemory` | `TFormMemory` | Memory display, editing, and watches |
| `UnitDebugRegisters` | `TFormRegisters` | CPU, RSP, FPU, and hardware registers |
| `UnitBookmarks` | `TFormBookmarks` | Address bookmarks and folders |
| `UnitDebugBreakOpcode` | `TFormDebugBreakOpcode` | Opcode break conditions |
| `UnitRegChange` | `TFormDebugRegisterChange` | Register-access break conditions |
| `UnitDump` | `TFormDump` | Binary, disassembly, and C-style memory dumps |
| `UnitSearch` | `TFormSearch` | Memory searches and results |
| `UnitStats` | `TFormStats` | Register, opcode, and block statistics |
| `UnitImageView` | `TFormImageView` | Memory-backed image display |
| `UnitLibOptions` | `TFormLibOptions` | Library and symbol-tool settings |
| `UnitSymbolViewer` | `TFormSymbols` | Symbol list |

`debugger.dpr` contains the four `cdecl` plugin exports, with the original
ordinals: `ToolFunc` (1), `ToolGetMenuText` (2), `ToolFree` (3), and `ToolInit`
(4). `UnitDecs` describes the 244-byte host interface and memory-access helpers.
`RCP` and `UnitOpcode` hold register and opcode tables. `UnitDisassembly`
implements CPU, RSP, and RDP decoding; `UnitMIPSToCCode` produces the original
C-style dump text. `UnitMain` and `r4300` have only their linked initialization
and finalization wrappers. The empty `TFormImageView.FormResize` event also
matches an empty original routine.

The binary preserves unit and form class names, published component fields and
their offsets, 140 published event names and entrypoints, all form streams, and
export identities. These are recorded in
[debugger-inventory.json](../../analysis/debugger-inventory.json).
[debugger-unit-map.json](../../analysis/debugger-unit-map.json) records the
recovered CODE spans, initialization/finalization entries, and their evidence.
Its `current_source_uses` field is a snapshot taken during the unit audit, not
proof of the original `uses` clauses. Code order, initialization order, and the
`PACKAGEINFO` unit sequence are distinct.

Private helper names, local names, ABI field names, and most global names are
inferred from their uses. [debugger-imports.json](../../analysis/debugger-imports.json)
records the reconstructed globals, imported-data addresses, and the name-hash
constraints; a matching hash does not establish an original spelling. Unused
ABI slots retain offset-based names. The flat
application directory, `components` subdirectory, and placement of exports in
the DPR are inferred source organization. The goal is the original linked
implementation; lost comments, formatting, unused declarations, and unlinked
code cannot be recovered uniquely from the DLL.

The Theme Manager and JEDI API units in `components` reconstruct the linked
behavior shared with the dialogs plugin. Their newer reference source is
preserved unchanged under
[dialogs/vendor/theme-manager-1.10.1](../dialogs/vendor/theme-manager-1.10.1),
with URLs, mirror revision, and file hashes in
[provenance.json](../dialogs/vendor/theme-manager-1.10.1/provenance.json).
Mike Lischke's Theme Manager authorship and the JEDI translation credits and
license notices remain in the source headers. Those headers describe the
reference release; they do not establish the release number used by Nemu.
[components/README.txt](components/README.txt) explains the reconstructed
behavior and limits of the reference material.

The small assembler blocks are symbolic source: a byte swap in the dump unit
and the Theme Manager window-procedure adapters. Original output strings and
observed quirks are retained, including the dump generator's `ToDo` comment and
the theme API-loader's mismatched entrypoint name. These are part of the
reconstructed behavior, not unfinished implementations.

`RemoveRSPBreakpoint` includes three explicitly marked self-assignments as
compiler guidance. Delphi 6 counts their references when choosing registers,
then removes the assignments. This reproduces the original EDI/EDX allocation
without adding instructions. The algorithm is fully reconstructed; these
statements are not claimed to be recovered original text. The original
optimized-away expressions cannot be identified uniquely. The evidence in
[debugger-allocator.json](../../analysis/debugger-allocator.json) separates the
15-byte register-allocation mismatch from the matching reconstruction.
