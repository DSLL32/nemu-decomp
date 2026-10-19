# Nemu64 source reconstruction

This project reconstructs the original Nemu64 sources using gpt-6-astra and
gpt-6.1-sol. It combines recovered upstream sources with decompilation; private
names and source organization are inferred where the originals are unavailable.

| Binary | Matching status |
| --- | --- |
| `nemu64.exe` | [Not yet byte exact](src/nemu64/README.md) |
| `gs_dll.dll` | [Byte exact](analysis/gs-comparison.json) |
| `kailleraclient.dll` | [Byte exact](analysis/kaillera-byte-exact.json) |
| `Plugin/audio.dll` | [Not yet byte exact](src/audio/README.md) |
| `Plugin/Input.dll` | [Byte exact](analysis/input-comparison.json) |
| `Plugin/debugger.dll` | [Byte exact](analysis/debugger-comparison.json) |
| `Plugin/dialogs.dll` | [Byte exact](analysis/dialogs-comparison.json) |
| `Plugin/profile.dll` | [Byte exact](analysis/profile-comparison.json) |
| `Plugin/LemD3D8.dll` | [Byte exact](analysis/lemd3d-comparison.json) |
| `Plugin/LemCombineDebugger.dll` | [Byte exact](analysis/lemcombinedebugger-comparison.json) |

“Byte exact” means the complete rebuilt file matches the reference in `master/`.
The component documentation and build reports describe the historical tools,
inferred build settings, and any archival metadata restoration used.

Run `make` to build a working copy in `bin/` using the tracked toolchains and
Wine. All DLLs and `nemu64.exe` are built from source. Only `config.ini`,
`nemu64.ini`, `LemD3DCombine.dat`, and `LemD3DCombine.new` are copied from `master/`.
The ordinary Makefile build does not guarantee byte matching; use the separate
`scripts/build_*.py` reconstruction scripts described in each component's README
for matching builds and verification.

`make static` builds the experimental `bin/nemu64-static.exe` with all nine
bundled components linked from object files and static libraries. It runs without the loose DLLs or a
`Plugin/` directory, and still scans `Plugin/` for additional dynamic plugins.
Bundled filenames are skipped during that scan to prevent duplicate registration.
The four configuration/data files are still needed. See
[the static build notes](src/static/README.md) for the linking approach,
validation command, and current limitations.
