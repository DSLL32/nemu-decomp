# audio.dll reconstruction

This reconstruction is incomplete. Native object checks match the instruction
bodies of 395 of 417 unique functions. Relocation identities and remaining
inferences are recorded separately; 22 functions and parts of the data layout
remain unresolved. All 417 retained functions now link in their original order,
and the complete resource section matches. Readonly layout and all bytes outside
recorded address fields also match; those address values remain incomplete.
The full 290,816-byte DLL does
**not** match the original. See
[`analysis/audio-integration-comparison.json`](../../analysis/audio-integration-comparison.json)
for the frozen build, exact counts, and limitations.

Build from the repository root:

```sh
python3 scripts/build_audio.py --report analysis/audio-build-comparison.json
```

The command returns failure until the complete output equals
`master/Plugin/audio.dll`. It creates a fresh source snapshot and build directory
under `~/.ai-tmp`, preserves compiler/linker commands and hashes, and uses a
private Wine prefix. `--work` selects an existing scratch root for prefix reuse.
`--unit NAME --objects-only` requests an explicitly partial object diagnostic.

The fourteen inferred translation units in `ahle_ds` use unmodified VC7
13.00.9466 and its static runtime. The DirectSound import library comes from the
tracked VC6 SDK; its import-directory provenance matches the original. The
thirteen C units and one C++ unit, runtime objects, historical `itoa` alias, and
SDK imports reproduce the original ordered Rich records. The `ahle_ds` project
directory is preserved in the original PDB path; private filenames and some
identifiers remain inferred.

The integer synthesis and envelope routines share one inferred source unit.
Buffer filtering/profiling form the next unit, followed by diagnostics and
decoders. These boundaries reproduce all thirteen repeated readonly constant
blocks and their surrounding literals without changing any of the 52 affected
function bodies. See
[`analysis/audio-source-grouping.json`](../../analysis/audio-source-grouping.json).

Historical link metadata is reproduced separately from source recovery. The
first DLL and generated PDB are preserved. The build restores the recorded GUID
and prior revision in three parsed fields of a copy of that generated PDB, then
runs the original linker at the recorded time and PDB path. LINK emits the final
RSDS record and revision itself. Native LIB generates the export object at its
recorded timestamp, one second before the final LINK clock. No DLL bytes are
patched. The initial PDB, metadata seed, export object, final PDB, audit, and
unstamped DLL remain in the frozen build directory. See
[`analysis/audio-link-identity.json`](../../analysis/audio-link-identity.json).

Resources are readable source in `Audio.rc`. Recovered AMP and UltraHLE/Azimer
ancestry, licenses, source hashes, inferred adaptations, and component checks are
documented in the `analysis/audio-*` reports and source comments. Finite behavior
checks supplement native comparison; they do not establish complete equality.
