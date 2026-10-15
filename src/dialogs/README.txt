dialogs.dll reconstruction -- INCOMPLETE
======================================

Target: master/Plugin/dialogs.dll
SHA-256: 73df81668476f65dcc6587dbd79c2082d6b29d05c367ffc3cf0f6e7d36af7ab5

This is a buildable Pascal reconstruction in progress. The complete DLL is
NOT byte exact, so it does not yet meet AGENTS.md's decompilation criterion.
No executable code is copied from the target or patched into the build.

Recovered evidence
------------------

The three form unit names, class names, published fields, event names, field
offsets, and form streams come from the DLL. The editable DFM files compile
back to the exact original form resource bytes, including their image data.
The manifest is recovered from the original resource. The four RCP string
arrays preserve all 128 entries, including repeated and unused names.

Application routines are reconstructed from their x86 instructions and
checked with Delphi 6 Update 1. Helper/local/ABI field names are inferred
where no name survives. Unused host-interface slots are represented by
padding; the structure is 244 bytes. Unknown98 is a real unused slot, not
ReadError: the observed ReadError/ShowErrors/Breakpoints/BreakpointCount
offsets are $9C/$A0/$A4/$A8. ReadWord uses a stdcall host callback.

The flat source layout and placement of exports in dialogs.dpr are inferred.
UnitMain is present in PACKAGEINFO, but its declarations and uses clauses
have not been recovered. Its empty implementation is provisional.
Only empty initialization/finalization remains linked from Verlauf.

Build and check (from the repository root)
-----------------------------------------

  python3 scripts/build_dialogs.py --report analysis/dialogs-comparison.json

The script prints the DLL and MAP paths. All output goes under ~/.ai-tmp.
It allocates a Wine prefix there, or reuses WINEPREFIX if supplied. Requires
Wine, python3-capstone, and the tracked d6u1 and vc7 toolchains. VC7's resource
compiler compiles the recovered manifest; D6u1 compiles the Pascal source.
DFM text is converted with the original Delphi streaming implementation.

Use the printed DLL path for these checks:

  python3 scripts/verify_dialogs.py /path/to/dialogs.dll --report report.json
  python3 scripts/check_dialogs.py /path/to/dialogs.dll --report behavior.json

verify_dialogs.py exits 1 until the COMPLETE DLL is byte exact. A successful
build or behavioral probe does not override that result. The behavioral
probe additionally needs Xvfb/xvfb-run. It creates a private Wine prefix and
disposable INI fixtures under ~/.ai-tmp, then runs the same host against the
original and rebuilt DLLs. It checks ROM info, stopped/running settings,
plugin selection, folder/RDRAM/plugin writes, and three ROM INI sections.
Browse dialogs, non-windowed labels, pixels, and error paths remain untested.

Outstanding work
----------------

1. The linked Theme Manager version has not been recovered. The unchanged
   source in vendor/theme-manager-1.10.1 is a PROVISIONAL reference dependency,
   newer than the original. Its source headers preserve author/license
   notices; provenance.json pins the source mirror and file hashes. The
   original uses JwaUxTheme/JwaTmSchema, while this version uses UxTheme/
   TmSchema and pulls in additional VCL units. It cannot produce the same DLL.
2. TFormHeader.UpdateInfo and the NemuSettings export still differ in generated
   code. Their observed behavior agrees in the current host probe, but this
   does not establish equivalence on every path or recover the original code.
3. Resolve UnitMain and original uses/link order, then match all remaining
   runtime/component code, data, resources, relocations, imports, and PE layout.

Current diagnostics match 32/34 application symbol blocks after masking
HIGHLOW relocation operands and external rel32 call/jump operands. This
includes all 24 published event handlers. These are bounded MAP blocks and
can include literals/alignment/finalization. Masking does NOT validate the
referenced functions or variables. ReadWord's absolute host-field references
therefore also receive an independent offset check. Full-file equality is
the completion test. See analysis/dialogs-inventory.json, dialogs-comparison.json,
and dialogs-behavior.json for the recorded evidence and limitations.
