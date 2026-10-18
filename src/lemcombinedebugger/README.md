# LemCombineDebugger reconstruction

These Pascal units reconstruct `master/Plugin/LemCombineDebugger.dll`.
A clean native build reproduces the complete 600,064-byte DLL, SHA-256
`0d1105d3638f7c3d0203de44a948003e7c77a67f4185b13e9458be220d9a8fbd`.
The [comparison report](../../analysis/lemcombinedebugger-comparison.json)
records source/tool hashes, compiler commands, and equality of every section
and resource payload.

```sh
python3 scripts/build_lemcombinedebugger.py
python3 scripts/verify_lemcombinedebugger.py /path/to/LemCombineDebugger.dll
```

The matching build uses the tracked Delphi 6 Personal Update 1 compiler and
DCUs. It creates a fresh source snapshot and Wine prefix under `~/.ai-tmp`;
`--work` reuses a scratch directory while discarding stale application DCUs.
Compiling `UnitCamera.pas` before linking the DPR reproduces the original
initialization traversal through the circular standard VCL dependencies.
The whole-file verifier does not mask differences or modify either input.

`make lemcombinedebugger` is the ordinary user build into
`bin/Plugin/LemCombineDebugger.dll`. It uses the shared Delphi Makefile rules
and produces a working copy without archival timestamp restoration or the
matching build's resource serialization step.

## Recovered units

| File | Retained implementation |
| --- | --- |
| `LemCombineDebugger.dpr` | Four `cdecl` plugin exports and original ordinals |
| `UnitCamera` | Camera movement, rotation, reset, and window notification |
| `UnitCombineEditor` | Combiner command parser/editor, script loading/saving, renumbering, clipboard |
| `UnitCombiners` | Live combiner list, formula display, highlights, custom-script deletion |
| `UnitDecs` | 244-byte Nemu host interface and shared graphics-debugger state |
| `RCP` | Four retained CPU/RSP register-name tables |
| `UnitMain` | Empty linked unit initialization/finalization wrappers |

Unit names, three form class names, published fields, 43 event names, and form
property streams survive in the DLL. The flat directory and DPR placement are
inferred. Private methods, locals, and ABI/global names are inferred from their
uses; lost spelling, comments, formatting, unused declarations, and unlinked
code are not uniquely recoverable. The host ABI declarations and register tables
also agree with this repository's existing debugger reconstruction. The shared
combiner-state layout agrees with the preserved LemD3D C++ declarations.

No Pascal source for this DLL was found in the preserved LemD3D Google Code
archive or the Nemu64 Git history. Those repositories include the DLL and the
related graphics plugin source. Search locations and hashes are recorded in
[lemcombinedebugger-upstream.json](../../analysis/lemcombinedebugger-upstream.json).
The application is reconstructed Pascal, with the original RTL/VCL supplied by
the tracked toolchain; it contains no extracted instruction blobs or inline
assembly.

## Resource representation and timestamps

The three DFM files are readable source recovered from their form streams.
Delphi's text converter serializes three empty `ComboBoxQueryDevice.Items`
entries as empty WideStrings; the original form writer serialized those same
empty entries as ANSI strings. The
[form serializer](../../scripts/lemcombinedebugger_forms.py) emits the original
ANSI representation after the known preceding list entries, checks each entry
occurs exactly once, and updates the form resource's size. This runs before
linking. Every generated form byte must still pass the full-file verifier.

The compiler emits all code, initialized data, imports, exports, relocations,
form streams, resource strings, and `PACKAGEINFO`. After linking,
[the metadata restorer](../../scripts/restore_lemcombinedebugger_metadata.py)
may change only 51 resource-directory timestamp DWORDs to the original local
DOS time, `2002-02-08T00:28:42`. It first proves that replacing only those fields
would make the complete file identical; otherwise it refuses and leaves the
output untouched. The untouched compiler output is preserved as
`LemCombineDebugger.unstamped.dll`.

The inferred global names also satisfy Delphi's imported-data name hashes.
Matching hashes constrain names without proving their original spelling; see
[lemcombinedebugger-imports.json](../../analysis/lemcombinedebugger-imports.json).
