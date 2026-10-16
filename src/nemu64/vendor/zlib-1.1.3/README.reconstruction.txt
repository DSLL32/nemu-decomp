Nemu64 reconstruction: zlib 1.1.3 and minizip unzip 0.15

The C sources, headers, and original documentation in this directory are
unchanged files from the July 9, 1998 zlib 1.1.3 release. The contrib/minizip
subdirectory retains the archive's original layout. The enclosing vendor path
is an inferred reconstruction location, not a recovered original Nemu path.

Upstream archive:
https://www.zlib.net/fossils/zlib-1.1.3.tar.gz
SHA-256: cae5847bc0e1cf113d3f70d037400da3e47c2e2b7b1c96b0b08447a5fbb906f4

provenance.json records each copied file's original SHA-256. The archive's
examples, build projects, unrelated contributions, and alternative platform
sources are omitted. The zlib license remains in zlib.h and the original
source headers; the unzip license remains in contrib/minizip/unzip.h.

Compiler: toolchains/vc6rtm, Microsoft C 12.00.8168.
Matching configuration: /c /O2 /ML /DWIN32 /DNDEBUG, with this directory on the
include path. C sources must remain C translation units. Nemu's final link uses
the VC7 linker and static CRT. No source changes or executable byte patches are
needed. /MD changes the generated stdio calls and fails to match unzip; /MT may
emit identical code for these functions, so /ML also relies on the final CRT
selection rather than code generation alone.

The 14 linked translation units are uncompr.c, compress.c,
contrib/minizip/unzip.c, inflate.c, deflate.c, crc32.c, infblock.c, adler32.c,
zutil.c, trees.c, infcodes.c, inftrees.c, infutil.c, and inffast.c. gzio.c is
retained as part of the upstream library but has no identified linked functions
in the target. Unreferenced functions in the linked translation units are also
removed by the release link.

analysis/nemu-zlib-addresses.json identifies 72 unambiguous complete COFF
function-block matches. Together they cover all 27,104 bytes from 0x421210 to
0x427BF0, including function alignment and jump tables. Only recorded COFF
relocation fields are excluded from this diagnostic comparison; internal
branches remain exact. A call from _tr_init also identifies the empty
tr_static_init helper folded at 0x41FEF0, outside this contiguous block.
Corresponding relocation operands yield 65 consistent
external symbol addresses without conflicts. This is source/compiler evidence;
whole-executable equality remains the final reconstruction criterion.

The generic scripts/map_nemu_libraries.py matcher was also run with all compiled
zlib objects and toolchains/vc7/lib/libc.lib. It independently validates every
one of the 257 code relocations in the 73 identified functions against callee
bodies or import identities. The remaining 93 DIR32 data operands require the
complete link; no zlib candidate was rejected.
