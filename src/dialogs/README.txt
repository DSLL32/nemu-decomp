dialogs.dll reconstruction -- BYTE EXACT
=======================================

Target: master/Plugin/dialogs.dll (644,096 bytes)
SHA-256: 73df81668476f65dcc6587dbd79c2082d6b29d05c367ffc3cf0f6e7d36af7ab5

The reproducible build matches the entire original DLL: all code, data, forms,
other resources, imports, exports, relocations, headers, layout and padding.
No executable code is copied from the target or patched into the build.
After linking, only the resource-directory timestamps are normalized to the
original build's local time. The untouched compiler output is also preserved.

Build and check (from the repository root)
-----------------------------------------

  python3 scripts/build_dialogs.py --report analysis/dialogs-comparison.json

The script prints the DLL and MAP paths and exits nonzero on any mismatch.
Output goes under ~/.ai-tmp. It allocates a Wine prefix there, or reuses
WINEPREFIX if supplied. Requires Wine, python3-capstone, and the tracked d6u1
and vc7 toolchains. No network access or installer is needed to build.

D6u1's unmodified Personal compiler (dcc32-personal.exe, build 6.0.6.190) emits
the original DVCLAL edition resource. Enterprise emits identical code but a
different edition stamp. The tracked Update 1 libraries supply the RTL/VCL.
VC7's resource compiler compiles the recovered manifest; Delphi's streaming
implementation converts the editable DFM files into the original form bytes.

The build first compiles all source, then removes UnitHeader.dcu and rebuilds
with the other DCUs cached. This reproduces both Delphi's unit initialization
order and PACKAGEINFO traversal. A clean single-pass build or an entirely
cached link produces different tables. This build sequence is inferred from
the emitted bytes; the original author's IDE build history is unknown.

Delphi stores local DOS timestamps in 54 PE resource-directory headers.
stamp_pe_resources.py sets only those fields to 2002-04-02 21:52:38. It does
not read the reference DLL or change any resource payload, code, or data.
dialogs.unstamped.dll preserves the compiler output before normalization.
comparison.json records that file's hash and the final byte-exact comparison.

Independent checks using the printed DLL path:

  python3 scripts/verify_dialogs.py /path/to/dialogs.dll --report report.json
  python3 scripts/check_dialogs.py /path/to/dialogs.dll --report behavior.json

The verifier checks full-file equality and reports raw section, application
routine, theme unit, and resource equality. Its older address-masked routine
diagnostics remain available for investigation but are not the completion test.
The behavior probe additionally needs Xvfb/xvfb-run. It creates a private Wine
prefix and disposable INI fixtures under ~/.ai-tmp, then runs the same host
against the original and rebuilt DLLs. All five scenarios pass: ROM info,
stopped/running settings, plugin selection, folder/RDRAM/plugin writes, and
three ROM INI sections. Browse dialogs, non-windowed labels, pixels, and error
paths are not independently exercised by that probe; full-file equality covers
their compiled implementation.

Reconstruction evidence and inferred information
------------------------------------------------

The three form unit names, class names, published fields, event names, field
offsets, and streams survive in the DLL. All form resources and their image
data are reproduced exactly. The manifest is recovered from the resource.
The four RCP string arrays preserve all 128 entries, including repetitions.

All 34 measured application blocks now match without address masking. The
last two code-generation issues were UpdateInfo's early Exit and NemuSettings'
skip-to-next-plugin branch. The plugin's fixed character arrays have one-based
bounds: Delphi emits @LStrFromPCharLen, not the different @LStrFromArray call
produced by zero-based declarations. All linked Theme Manager code has also
been reconstructed; see components/README.txt.

The host structure is 244 bytes. Unused slots retain their observed offsets;
Unknown98 is a real unused slot, not ReadError. ReadError, ShowErrors,
Breakpoints, and BreakpointCount are at $9C/$A0/$A4/$A8. ReadWord calls the
host through stdcall. Helper, local, and ABI field names are inferred where
no spelling survives. Nemu64_API, NemuMainWinHandle, and NemuFileDir are
plausible inferred global names chosen to reproduce Delphi's imported-data
table order. That fingerprint does not prove their original spelling.

The flat application layout, components directory, and placement of exports
in dialogs.dpr are inferred. UnitMain and Verlauf retain only the empty
initialization/finalization emitted into this DLL. Unused original declarations,
comments, source formatting, and unlinked library code cannot be recovered
from this binary. Byte equality establishes the linked implementation, not
textual identity with the lost source tree.

Recorded evidence: analysis/dialogs-inventory.json, dialogs-comparison.json,
and dialogs-behavior.json, plus toolchains/delphi-updates.json.
