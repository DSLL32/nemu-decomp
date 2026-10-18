This is a buildable source reconstruction of `master/Plugin/LemD3D8.dll`, using
the preserved LemD3D source. **It does not yet produce a byte-matching DLL.**
The build and verification scripts fail the matching check until every file
byte agrees; no target code or data is inserted into the output.

The target is 294,912 bytes, SHA-256
`df1db2f653886dcf2082ddcbe78bbecc0e512d66e99b754b624eabbeb09f8cfb`.
Its PE timestamp is September 10, 2002, its linker is VC7.0, and its Rich header
records nine VC7 build-9466 LTCG C++ contributions. The current candidate uses
the tracked VC7 compiler and DirectX 8 headers/libraries under
`toolchains/vc7/platformsdk/*/prerelease`.

Run the matching build from the repository root:

```sh
python3 scripts/build_lemd3d.py --report analysis/lemd3d-comparison.json
```

Exit status 0 means complete byte equality, 1 means a successfully linked but
different DLL, and 2 means the build failed. A clean source snapshot, outputs,
MAP, command logs and a new Wine prefix are allocated under `~/.ai-tmp`.
`--work ~/.ai-tmp/NAME` reuses that scratch directory and prefix while refreshing
the source snapshot. Precompiled headers follow the archived project;
`--no-ltcg` and `--no-pch` select optional diagnostic builds. The script
records source, recipe, compiler and principal library hashes, preserves the
untouched linker output, and performs no metadata restoration.

An existing candidate can be checked independently:

```sh
python3 scripts/verify_lemd3d.py /path/to/LemD3D8.dll
```

The ordinary user build is `make lemd3d`, producing `bin/Plugin/LemD3D8.dll`.
It is also included in `make all`, replacing the previous master-file copy.
This build uses native objects and ordinary linker settings; it is separate
from the historical matching experiment. Both builds require 32-bit Wine and
the tracked VC7 toolchain. No DirectX download or Visual Studio project is
needed to build.

The starting point is the Google Code archive's
[`tags/Lemmy's source`](https://code.google.com/archive/p/lemd3d/source/default/source),
revision 11, last changed August 31, 2012. The
[GitHub tree](https://github.com/purplemarshmallow/Nemu64/tree/master/Source/lemd3d)
was first committed April 20, 2015 and contains further changes. Google Code's
trunk also contains a later revision of the plugin. The tag retains the original
INI helper, AMD CPU detection calls, configuration options, and version
resource that later sources changed or removed, making it the better initial
candidate. This identifies the oldest preserved baseline among these copies;
it does not date the actual source to 2002. Even the tag has already been
ported to Direct3D 9 and its VC project references a 2006 DirectX SDK.

Only C/C++ headers, sources, resource text and the unused historical CRC
assembly source are imported. The flat file organization and names follow the
tag. The original resource script is Windows-1252. Project files, IDE caches,
binaries and build logs are excluded. Upstream hashes, archive URL, source
history and project-option evidence are recorded in
[lemd3d-upstream.json](../../analysis/lemd3d-upstream.json).
[lemd3d-source-changes.patch](../../analysis/lemd3d-source-changes.patch) records
the changes from that baseline, decoded as Windows-1252 for readability.

The local changes restore Direct3D 8 types, integer pixel-shader handles,
texture-stage sampler settings, shader constants, vertex-buffer APIs, shader
assembly parameters and the original DirectX 8 messages. Shader cleanup checks
handles before deleting them and clears each handle, allowing repeated shutdown
before device creation. The Direct3D 9 depth-bias path is removed in favor of
the existing projection-based decal path. RDP scissor state is retained without
calling the Direct3D 9-only `SetScissorRect`. These are source restorations and
API-porting changes, not a claim that every recovered routine matches 2002.

The external AMD library was absent from both source repositories. The small
used interface and CPU detector are reconstructed under `vendor/amd3d`; see
its [provenance notes](vendor/amd3d/README.md). No library bytes are extracted
from the target to satisfy this dependency.

The [comparison report](../../analysis/lemd3d-comparison.json) currently confirms
all 13 export names and ordinals and **all three resource payloads byte for
byte**, including both dialogs and version information. Executable code,
initialized and zero storage, imports and relocations still differ. The source
contains later renderer/combiner behavior and storage layouts; the DirectX and
AMD library inputs and exact compiler/linker options also need further matching.
The retained DSP suggests `/MT /GX /O2 /Ob2`; `/GL` is supported by the target's
Rich header. These constraints do not establish the complete original recipe.

A differential runtime smoke test is available with Xvfb:

```sh
xvfb-run -a python3 scripts/check_lemd3d.py /path/to/LemD3D8.dll \
    --report analysis/lemd3d-runtime.json
```

It compares loading, all export names, plugin metadata, synthetic host
initialization, an accessible zeroed combiner header, resolution and window
state changes before device creation, idle screen updates, repeated cleanup,
and unloading against the master. It does not create a Direct3D device or
validate rendering, ROM compatibility, interactive dialogs, or the complete
combiner-debugger record layout. Passing it is separate from byte equality.
