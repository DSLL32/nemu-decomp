# Kaillera client 0.84 reconstruction — not yet byte exact

The reference is the 28,160-byte UPX 1.20-packed Kaillera **0.84** client,
SHA-256 `410503fdfd1e931adf6024df7b8d88c725260b568688c5a5a1ab9897c83ff72d`.
Its PE timestamp is June 2, 2001. It is an earlier version than the currently
published 0.9 SDK. The target contains 15 VC6 RTM C++ compiler contributions.

The full application now builds from source. The ordinary working-copy build is
`make kaillera`, which writes `bin/kailleraclient.dll`. The historical build
command below also packs a candidate with UPX 1.20 and compares every packed
byte, alongside diagnostic instruction signatures and resource payloads:

```sh
python3 scripts/build_kaillera.py --report analysis/kaillera-components.json
```

The command exits nonzero until the packed candidate is fully byte exact.
It allocates scratch space and its own Wine prefix under `~/.ai-tmp`;
`--work ~/.ai-tmp/<directory>` reuses a build directory and prefix.
The DLL candidate, component library, resources, copied sources, objects and logs remain
there. No executable code is copied from the reference into the build.

The separate full-file verifier accepts only all-byte equality with the packed
reference, checks its pinned hash, and does not normalize headers or packing:

```sh
python3 scripts/verify_kaillera.py /path/to/kailleraclient.dll
```

The published original JNetLib **0.3**, dated March 29, 2001, supplies the used
DNS, TCP, HTTP and socket utility functions. Its needed four implementation
files and supporting headers are vendored under `jnetlib/`. Compiling them with
unchanged VC6 RTM CL/C1XX/C2 and `/O2 /MD /GX /Gy` reproduces every linked
method's instruction bytes outside COFF relocation operands. Unused DNS reverse
lookup, TCP listener/server and HTTP accessor methods are not linked.
The retained `jnetlib.h` carries the upstream license and original include list;
the build includes only the required individual headers.

The publisher's surviving SDK 0.9 still carries its API v0.84 header, imported
unchanged as `kailleraclient.h`. Its DLL and import library are not vendored.
Source URLs, archive hashes and imported-file hashes are in
`analysis/kaillera-provenance.json`. No original private client implementation
was found in the publisher downloads or public source searches. The flat
application directory and most private C++ names are inferred. The publisher's
Linux server 0.86 retains RTTI names `UDPCom` and `KLRCom`, which identify the
shared transport classes; those original names are retained here.

`config.cpp` reconstructs cached INI reads/writes. `message.cpp` reconstructs
message serialization. `udpconnection.cpp` restores socket multiplexing, peer
lookup and transport. `reliableconnection.cpp` and `packetcache.h` restore the
two embedded message caches and resend/receive algorithms. `connect.cpp`
restores server negotiation and login. `kailleraclient.cpp` restores DllMain and
the eight public wrappers. The remaining instruction differences are in the configuration constructor, socket
polling, and game-input loop.
`session.cpp` and `serverbrowser.cpp` restore the
partyline, packet dispatcher, game panels, input synchronization and server
selection. `listview.cpp` and `resizer.cpp` restore shared window helpers.

All nine dialogs are editable RC source. Compiling `kaillera.rc` with the
original VC6 resource compiler reproduces every resource payload, including the
two images and group directory represented by `kaillera.ico`. French language
ID 1036 is retained. Resource identifiers that were not preserved as names are
numeric.

Remaining work is instruction refinement, complete layout and packing.
Diagnostic signature matches do not certify
unresolved relocation identities or final executable bytes.

The tracked `toolchains/upx120` is the unmodified historical packer, not a
reference-program payload. UPX 1.20 `-9` was identified as the original setting.
Raw decompression of the original packed stream identifies the original
`SizeOfHeaders` and IAT directory values that UPX's unpack operation changes.
Restoring only those PE header fields before repacking a decompressed reference
reproduces all original packed bytes. The proof is in
[`analysis/kaillera-upx.md`](../../analysis/kaillera-upx.md). The source-built
candidate must still reproduce the program and layout before any guarded
container metadata restoration can apply.

`python3 scripts/smoke_kaillera.py bin/kailleraclient.dll` runs the original and
rebuilt public API under Wine/Xvfb, checking version, initialization, repeated
game-list replacement, idle input, chat and shutdown. It creates its own scratch
directory and Wine prefix. The live server/game path still requires additional
validation; the smoke test does not establish instruction or full-file equality.

The necessary unmodified VC6 RTM C++/Win32 toolchain extension is documented in
`toolchains/kaillera-toolchain.json`; `install_vc6rtm` in
`scripts/unpack_toolchains.py` reproduces it from the original ISO. Previously
installed compiler files remain unchanged. `toolchains/kaillera-packer.json`
records the historical packer archive and file hashes.
