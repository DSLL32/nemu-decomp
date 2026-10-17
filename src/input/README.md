# Input plugin reconstruction

This directory reconstructs `master/Plugin/Input.dll` as Delphi source. The
original file is 634,880 bytes, with SHA-256
`2c0fb513c6c34115ef070c9f33767f7cf15d28de4becbd74bd90fe515e496298`.

Build with the tracked Delphi 6 Personal Update 2 compiler:

```sh
python3 scripts/build_input.py
```

The build directory and its Wine prefix are created under `~/.ai-tmp`. The
script prints the resulting DLL, detailed MAP, and comparison report. Exit zero
requires equality of the complete original and rebuilt files. A previous build
can be checked independently with:

```sh
python3 scripts/verify_input.py /path/to/Input.dll --map /path/to/Input.map
```

The compiler produces all code, initialized data, imports, exports, relocation
records, and resource payloads. Only the 54 resource-directory clock fields are
restored after linking, to `2002-07-26T23:26:24`. The untouched compiler output is
retained as `input.unstamped.dll`.

| Source | Reconstructed responsibility |
| --- | --- |
| `Input.dpr` | Five C calling convention exports; controller polling, button/axis mapping and rumble |
| `UnitDIStuff.pas` | Device enumeration, DirectInput setup, registry configuration, error handling and keyboard definitions |
| `DInput.pas` | DirectInput API declarations, data formats, loader and error strings |
| `UnitSettings.pas` | Controller mappings and configuration form |
| `UnitSelectInputDevices.pas` | Enabled devices, force feedback and dead zones |
| `UnitWaitKey.pas` | Interactive button selection |
| `UnitSensitivity.pas` | Analog sensitivity form |
| `UnitAdaptoid.pas` | Adaptoid driver discovery and controller command packets |
| `UnitDecs.pas` | Eight-byte host controller records |
| `RCP.pas` | Shared register-name tables retained by Delphi smart linking |
| `components/MultiMon.pas` | Historical project variant of the Delphi multiple-monitor compatibility unit |

The four textual DFMs were recovered from the original resources and round-trip
to the original binary streams. Published component names, field offsets, event
names, unit names, and exports are preserved by the DLL. Private helper names,
local names, other type names, global identifiers, and the flat application
folder structure are inferred. The global identifiers also reproduce Delphi's
imported-data hash ordering; that constraint does not establish their original
spelling. Lost comments, unused declarations, and unlinked code are not uniquely
recoverable.

Observed behavior is preserved, including fixed-length conversion of all 40
button-name characters, the device-selection form's repeated registry-key
removal, and the digital-axis mapping's existing bit operations. The MultiMon
source retains the Borland/Microsoft attribution and documents the observed
predicate and declaration-order differences from the archived toolchain source.

Compiler evidence, original metadata, and code layout are recorded in
`analysis/input-fingerprint.json`, `analysis/input-inventory.json`, and
`analysis/input-unit-map.json`. The other `analysis/input-*.json` reports document
intermediate instruction and operand checks; their stated limitations apply.
Only the complete-file comparison is the final matching criterion.
