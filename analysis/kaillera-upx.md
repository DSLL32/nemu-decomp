# Kaillera UPX 1.20 container

The target is reproduced by the pinned UPX 1.20w with `-9`. Packing its ordinary
UPX decompression initially appeared to leave 116–140 differing packed bytes,
despite both files decompressing to the identical 56,320-byte image. This is
loss of original PE header metadata during decompression, not a different
compressor, hidden program code, or nondeterministic padding.

Decoding the raw NRV2B stream preserves the extra original PE header which UPX
stores after its filtered program image. The original and repacked filtered
inputs are both 64,622 bytes and differ in exactly four bytes, all inside that
extra header:

| PE field | Original link image | Ordinary UPX decompression |
| --- | --- | --- |
| `SizeOfHeaders` | `0x400` | `0x1000` |
| IAT directory RVA | `0xB000` | `0` |
| IAT directory size | `0x1A8` | `0` |

The original stream's extra PE header begins at filtered offset `0xFA9B`.
Changing only those fields in the decompressed reference and repacking it with
the unchanged pinned `upx.exe -9` reproduces **all 28,160 target bytes**, SHA-256
`410503fdfd1e931adf6024df7b8d88c725260b568688c5a5a1ab9897c83ff72d`.
This diagnostic does not certify a source reconstruction. The matching source
build must still reproduce the complete native program, data, resources,
imports, exports, relocations, and final packed bytes. Normal VC6 linking emits
these header fields directly; no packed-stream edits are needed.

The historical implementation documents the mechanism in `p_w32pe.cpp`:
packing copies `ih` and the original section table into its extra data;
unpacking explicitly sets `oh.headersize=rvamin` and clears the IAT directory.
Source: [UPX 1.20 source archive](https://sourceforge.net/projects/upx/files/upx/1.20/upx-1.20-src.tar.gz/download).

The diagnostic used `/root/.ai-tmp/kaillera-helpers.hJeGTE`, its reused Wine
prefix, and an isolated extraction of Debian's `libucl1` to decode raw NRV2B
streams. The normal matching build needs only the already pinned historical
packer. No executable instructions were inserted or patched.
