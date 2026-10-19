# Kaillera historical transport class names

The publisher's Kaillera **server 0.86** binaries retain GCC RTTI names
`UDPCom` and `KLRCom`. These are historical names recovered from original
binaries. Their correspondence to the reconstructed client 0.84 transport
classes is an inference supported by the same inheritance, object fields,
virtual methods, and transport behavior. This does not recover every original
client identifier or establish full client byte equality.

The original publisher downloads are:

- [Linux archive](http://www.kaillera.com/files/kaillerasrv-0.86-linux.tgz)
- [FreeBSD archive](http://www.kaillera.com/files/kaillerasrv-0.86-freebsd.tgz)

Both were downloaded on 2026-10-19 and inspected without execution. Archive and
executable sizes and SHA-256 hashes are in
[`kaillera-original-names.json`](kaillera-original-names.json). Neither reference
executable is imported into the reconstructed source or build.

| Reference | `6UDPCom` file offset | `6KLRCom` file offset |
|---|---:|---:|
| Linux | `0xf014` | `0xf074` |
| FreeBSD | `0x10f4d` | `0x10f64` |

The length prefix is GCC's historical encoding of the class name. Linux RTTI
factory `0x0804eed8` supplies `6UDPCom` to the runtime. Factory `0x0804f66c`
first calls that factory at `0x0804f67b`, then passes its resulting base RTTI
and `6KLRCom` to the single-inheritance runtime initializer. This establishes
that the historical `KLRCom` derives from `UDPCom`.

The Linux UDP vtable prefix is at file offset `0xefe0`; its RTTI factory pointer
is followed by eleven virtual-method pointers. The destructor at `0x0804e900`
closes its socket, removes it from the shared socket set, recomputes the maximum
socket, and removes its instance from a pointer list. The sharing method at
`0x0804ee04` copies the port and socket fields. These operations agree with the
client reconstruction. GCC places this class's vptr after its data fields,
whereas the Microsoft client layout places it first.

The Linux polling routine at `0x0804eb90` also caches the list count before
iterating, checks the index against the live list count inside its getter,
and marks objects selected by `select`. This is additional shared-source
evidence, not an exact code comparison across compilers. Server 0.86 has a
second timeout parameter converted from milliseconds, so its implementation
must not be substituted wholesale for client 0.84's one-argument routine.

Both ELF files are stripped. Their remaining dynamic symbol tables contain
runtime symbols and external dependencies but no original application-global
names. No application source filenames or assertion strings were found.
The two transport classes are the only application RTTI names found; other
unprefixed names belong to the GCC runtime.

Reproduce the hash-guarded extraction with downloaded publisher archives:

```sh
python3 scripts/check_kaillera_server_names.py \
  --linux-archive /path/to/kaillerasrv-0.86-linux.tgz \
  --freebsd-archive /path/to/kaillerasrv-0.86-freebsd.tgz \
  --output analysis/kaillera-original-names.json
```

Further inspection uses `nm -D -C`, `strings -tx`, and `objdump -d` on the
extracted reference executables. Inspection scratch is
`/root/.ai-tmp/kaillera-serverbrowser.AbTIBeCe/source-research`.

The fresh public-source search also inspected SupraclientC, SSClient,
OpenKaillera/okai, nyx, kailpy, and the Google Code `kaillera` archive. These
expose later replacement clients or binaries, not the original JNetLib-based
client implementation. SSClient's PDBs accompany modern replacement builds.
The Google Code archive contains a DLL and empty repository directories.
