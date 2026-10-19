This source reconstruction builds a byte-exact copy of `master/Plugin/LemD3D8.dll`:
294,912 bytes, SHA-256
`df1db2f653886dcf2082ddcbe78bbecc0e512d66e99b754b624eabbeb09f8cfb`.

Run the matching build from the repository root:

```sh
python3 scripts/build_lemd3d.py --report analysis/lemd3d-comparison.json
```

Exit status 0 requires complete file equality; 1 means a linked DLL differs;
2 means compilation or linking failed. Sources, native linker inputs, objects,
MAP, logs and an isolated Wine prefix live under `~/.ai-tmp`.
`--work ~/.ai-tmp/NAME` reuses a scratch directory and Wine prefix while
refreshing its source snapshot. `--no-ltcg`, `--no-pch` and
`--no-metadata-restoration` are diagnostic options.

The compiler is VC7 build 9466, using `/O2 /Ob2 /MT /GL /GX` and the archived
precompiled-header arrangement. Nine C++ translation units match the target's
LTCG contribution count. The AMD C helpers use VC6; their instructions also
match. All compiler commands and principal tool/library/source hashes are
recorded in the [comparison report](../../analysis/lemd3d-comparison.json).

The available DirectX 8 SDK and CRT contain the correct native instructions.
[build_lemd3d_link_inputs.py](../../scripts/build_lemd3d_link_inputs.py) makes
the target's two equivalent-function folding choices explicit through standard
COFF weak aliases, after checking that both section bodies and their relocation
references are identical. It also lowers the alignment of six duplicate CRT
empty wide-string COMDAT definitions so the application's zero float remains
separate. These are inferred linker-input metadata choices, not a claim that
the original library archives were recovered. Every native section's contents
remain unchanged. The AMD assembler's COFF alignment is set to 32 bytes to
reproduce its original contribution alignment.

`LemD3D8.linked.dll` preserves the untouched linker output. Every instruction,
program datum, resource, import, export entry and relocation entry must already
match before [restore_lemd3d_metadata.py](../../scripts/restore_lemd3d_metadata.py)
can run. It restores the original PE/export timestamps, Rich provenance records,
PE-header position, unused relocation-section virtual allocation, and unreachable
linker-heap residue in header padding. The provenance records and heap residue
are archival metadata from the master, not regenerated original compiler state.
Their specification and hashes are in
[lemd3d-build-metadata.json](../../analysis/lemd3d-build-metadata.json).
The strict guard rejects any changed program byte or other PE layout field.
The final verifier then compares every byte without masking.

```sh
python3 scripts/verify_lemd3d.py /path/to/LemD3D8.dll
```

The ordinary user build is `make lemd3d`, producing `bin/Plugin/LemD3D8.dll`;
it is included in `make all`. It uses native objects and ordinary linker
settings, with no historical linker-input or metadata restoration. This is
separate from the matching build. Both need 32-bit Wine and the tracked tools;
no Visual Studio project or external DirectX download is needed.

The starting point is the Google Code archive's
[`tags/Lemmy's source`](https://code.google.com/archive/p/lemd3d/source/default/source),
revision 11, last changed August 31, 2012. The
[GitHub tree](https://github.com/purplemarshmallow/Nemu64/tree/master/Source/lemd3d)
was first committed April 20, 2015 and contains further changes. The tag is the
oldest preserved baseline among these copies, but already includes a Direct3D 9
port and later renderer changes; it is not the original September 2002 source.
Project files, IDE caches and upstream binaries are excluded. Original hashes,
URLs and compiler-option evidence are recorded in
[lemd3d-upstream.json](../../analysis/lemd3d-upstream.json).

The reconstruction restores Direct3D 8 APIs and the older fixed-function
combiner; the later pixel-shader tree files are omitted. Other recovered
changes include integer tile coordinates, original cache keys and cache-hit
checks, viewport invalidation, matrix multiplication, generated texture
coordinates, color-only VRML export, and the original state/combiner layouts.
Historical mistakes in lighting calculations and resource handling are retained
where the target demonstrates them. Identical command handlers are grouped at
their observed folding positions. The remaining diagnostic `#line` directives
reproduce original line numbers and the header path embedded in `DXTrace` calls
where they differ from the reconstructed source locations. Redundant directives
and directives attached to commented-out calls have been removed. The complete changes
from normalized upstream text are in
[lemd3d-source-changes.patch](../../analysis/lemd3d-source-changes.patch).
Source encodings are Windows-1252 where required.

The absent AMD3D dependency is reconstructed under `vendor/amd3d`; see its
[provenance notes](vendor/amd3d/README.md). This includes readable assembly for
original handwritten matrix kernels, ordinary scalar C, CPU detection, and
typed constant initializers. No executable instruction bytes are inserted from
the master DLL.

A differential runtime smoke test is available with Xvfb:

```sh
xvfb-run -a python3 scripts/check_lemd3d.py /path/to/LemD3D8.dll \
    --report analysis/lemd3d-runtime.json
```

It compares all 13 exports, plugin metadata, synthetic host initialization,
combiner-header access, resolution/window changes before device creation,
idle updates, repeated cleanup and unloading. Both the matching and ordinary
user builds pass. It does not create a Direct3D device or test rendering,
ROM compatibility, interactive dialogs, or the full combiner-debugger ABI.
