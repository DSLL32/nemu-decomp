# Kaillera client 0.84 reconstruction — incomplete

The reference is the 28,160-byte UPX 1.20-packed Kaillera **0.84** client,
SHA-256 `410503fdfd1e931adf6024df7b8d88c725260b568688c5a5a1ab9897c83ff72d`.
Its PE timestamp is June 2, 2001. It is an earlier version than the currently
published 0.9 SDK. The target contains 15 VC6 RTM C++ compiler contributions.

There is no reconstructed replacement DLL yet. This command compiles the
recovered components into a library and compares their instruction signatures
and resource payloads with the reference:

```sh
python3 scripts/build_kaillera.py --report analysis/kaillera-components.json
```

The command deliberately exits nonzero while the application is incomplete.
It allocates scratch space and its own Wine prefix under `~/.ai-tmp`;
`--work ~/.ai-tmp/<directory>` reuses a build directory and prefix.
The component library, resources, copied sources, object files and logs remain
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
application directory and private C++ names are inferred.

`config.cpp` reconstructs cached INI reads/writes. `message.cpp` reconstructs
message serialization. `udpconnection.cpp` restores socket multiplexing, peer
lookup and transport. `reliableconnection.cpp` and `packetcache.h` restore the
two embedded message caches and resend/receive algorithms. `connect.cpp`
restores server negotiation and login. `kailleraclient.cpp` restores DllMain and
the eight public wrappers. Some component instruction differences remain.

All nine dialogs are editable RC source. Compiling `kaillera.rc` with the
original VC6 resource compiler reproduces every resource payload, including the
two images and group directory represented by `kaillera.ico`. French language
ID 1036 is retained. Resource identifiers that were not preserved as names are
numeric.

The remaining application work includes the lobby, protocol dispatcher,
game synchronization, server browser and shared window helpers, followed by
complete layout and packing. Diagnostic signature matches do not certify
unresolved relocation identities or final executable bytes.

The tracked `toolchains/upx120` is the unmodified historical packer, not a
reference-program payload. UPX 1.20 `-9` was identified as the original setting.
Repacking a decompressed reference still differs in a small packed header/tail
region because decompression does not preserve every original PE header byte.
No metadata restoration or post-build patch is currently applied.

The necessary unmodified VC6 RTM C++/Win32 toolchain extension is documented in
`toolchains/kaillera-toolchain.json`; `install_vc6rtm` in
`scripts/unpack_toolchains.py` reproduces it from the original ISO. Previously
installed compiler files remain unchanged. `toolchains/kaillera-packer.json`
records the historical packer archive and file hashes.
