# Kaillera native link inputs and layout

The matching build links under the original basename `kailleraclient.dll`.
LINK writes that name into the export directory; linking as
`kailleraclient.unpacked.dll` added nine bytes to `.rdata` and changed the
export library name. A descriptive unpacked alias is copied after linking.

The target imports `KERNEL32.DLL`, while the pinned VC6 RTM import archive
spells the DLL `KERNEL32.dll`. `build_kaillera_link_inputs.py` derives a scratch
import archive by changing only this symbolic DLL spelling. It validates the
input hash, member boundaries, import headers, function names, and executable
sections. It does not copy instructions from the target or patch linked code.
With these inputs the native `.rdata` virtual size is the target's 3,770 bytes,
and all seven imported DLL names have the target spelling.

Native function placement also supplies source-structure evidence. The
reliable transport's `receive` precedes `dataAvailable`. The UDP registry is
a class static member, with its definition after the methods: VC6 emits the
observed guarded destructor for a class static object, unlike a file static
object. This also places the initialization wrappers after UDP polling.
The maximum socket member is explicitly initialized to zero. The target places
the destructor guard at `0x1000CA74` and the socket at `0x1000CA78`.
VC6 allocates uninitialized symbols using its internal symbol order, so the
remaining inferred identifiers also affect the resulting layout. Recovering
the original transport class names changed these offsets again; matching
individual layout experiments does not settle the remaining private names.

These are link-layout and component findings. Full packed DLL equality is
still required by `verify_kaillera.py`; relocation-masked instruction matches
alone do not establish it.

## Uninitialized data

`kaillera-bss-layout.json` records the recovered offsets and declaration sizes
of 71 globals and compiler-generated guards. The offsets are inferred from
absolute operands in complete, uniquely matched native function signatures.
Every observed operand for a given declaration agrees. Names remain inferred
except where publisher binaries or recovered source establish them.

`build_kaillera_layout_inputs.py` supplies this layout to LINK by changing only
COFF symbol values, uninitialized section allocation sizes, and their matching
section auxiliary records. It records and guards all 71 source symbol offsets, COFF types and storage
classes, together with source allocation sizes and section attributes. It rejects
missing, additional or duplicate declarations, changed source offsets, overlapping
source or destination extents, initialized data, executable allocations, duplicate
input basenames, invalid section identifiers, and unexpected auxiliary records.
The extents are the reconstructed declarations' reserved sizes; COFF does not
encode full C++ array or class type sizes. Changes within alignment padding still
require the independent source/native-code and complete DLL checks. All initialized and
executable section contents and every relocation record remain unchanged.
The build consumes the declarative layout, without reading target bytes during
compilation or linking. The ordinary Makefile build uses the source directly.

The resulting native image reproduces all 59 public addresses in these blocks
and all 525 checked absolute BSS operands, including private globals and
member offsets. These 525 observations are also recorded individually by count
in the layout manifest; the earlier 517-observation inventory excluded the eight
operands in socket polling before that function matched. The `.data` virtual size
becomes the target's 3,744 bytes.
Differential public-API smoke testing also passed after applying the layout.
These checks establish the data-address reconstruction, not complete DLL
equality or the live multiplayer path.

`python3 scripts/test_kaillera_layout_inputs.py` exercises the metadata-only
mutation boundary and negative guards using synthetic COFF input, without a
compiler, Wine, or reference-program bytes.

## Code contribution groups

The original project and private symbol names are unavailable. The matching
driver therefore describes the recovered contribution order explicitly rather
than claiming to have recovered the original project settings.
`build_kaillera_code_inputs.py` changes only eight-byte COFF section-name fields:
queue helpers and shared destructors form one group, followed by browser code.
The native LINK `/ORDER` facility orders public helper COMDATs inside that group.
Compiler exception funclets keep their existing `.text$x` group. The normalizer
verifies that every section payload, relocation record, and symbol is unchanged.
The ordinary Makefile does not use these link-input transformations.

This reproduces the public API addresses starting at `0x10005420`, the lobby
and gameplay function addresses, and the complete helper address sequence from
`0x10008C10` through `0x10008E70`. The source bodies still require independent
verification at those addresses. In particular, an identical instruction
signature at a different function is insufficient evidence for a helper match.

`build_kaillera_crt_inputs.py` makes 27 selected SDK import records available
in the full COFF format emitted by the pinned native LIB `/LINK50COMPAT` tool.
It expands symbolic import declarations using freshly generated named and
ordinal import templates: the 22 Winsock thunks and five CRT thunks retain the
native six-byte indirect-jump payload and its symbolic IAT relocation. Import
names, hints, and ordinals come from the pinned SDK declarations. All other
SDK import records remain unchanged. Native CRT section names supply the
observed code grouping; CRT instructions and data payloads remain unchanged.
The first `crtdll.obj` COMDAT, `__CRT_INIT@12`, also needs the observed 16-byte
alignment rather than the SDK member's byte alignment; its immediately following
entry-point COMDAT retains byte alignment. This changes only section metadata.
The import chronology separately recovers the 99 IAT slots using ordinary
archive indexes and `/INCLUDE` roots. Neither stage reads the target DLL.

These transformations are linker-input metadata reconstruction. They do not
patch a linked executable or substitute target instruction bytes. Complete
packed equality remains the acceptance criterion.

## Native import chronology

`kaillera-import-chronology.json` records a static selection order for 99 SDK
import members and seven DLL descriptor groups. `build_kaillera_import_inputs.py`
pins every SDK archive hash, selected member offset/name/hash, archive index
symbol list, and `/INCLUDE` root. It generates ordinary archives with unchanged
selected member bodies. The only stock spelling adjustment remains the guarded
`KERNEL32.DLL` metadata adaptation described above.

Archive order and final IAT order are not identical in this LINK version. A
scratch diagnostic first supplied imports in final-table order, observed the
native permutation, and inverted that positional permutation once. The resulting
static input order reproduces all 99 import names/ordinals, descriptor order, and
IAT addresses. The builder does not perform this experiment, read target bytes,
or reorder a linked image. `check_inventory` verifies the native result against
the static symbolic recipe, including each DLL's first-thunk address and the
whole IAT directory (`0xB000`, 424 bytes). Descriptor order differs from the order
of DLL blocks within the IAT.

One additional unchanged, unrooted `inet_ntoa` import member resolves a reference
in an unused JNetLib utility before `/OPT:REF` discards that utility. It does not
appear in the final import inventory. This explicit singleton replaces a fallback
search of the entire WinSock archive.

The same recipe accepts the separately guarded `build_kaillera_crt_inputs.py`
output for MSVCRT/WinSock native thunk placement. Every derived archive must match
its conversion report's output hash; member order/names/index definitions must
match the original SDK, all unlisted members must remain unchanged, and every
changed member must match its recorded original/derived hashes. Thus the import
chronology and native code placement remain separate, inspectable linker-input
operations. The ordinary user Makefile continues to link the source directly.

The native chronology also roots three SDK CRT members in explicit singleton
archives: `chkstk.obj` and `atonexit.obj` precede the new/EH import records;
`crtdll.obj` follows EH and precedes the exit/init import thunks. Each member's
original SDK identity, hash and index declarations are guarded by the same
recipe. This controls native contribution order without copying reference
instructions or editing linked bytes. CRT code-group and first-entry alignment
metadata remain the separate guarded converter's responsibility.

After BSS layout, `build_kaillera_code_inputs.py` exposes the observed application
COMDAT groups and emits a helper ordering file. The build combines that file
with the native thunk ordering, rejecting duplicate symbols. Application and
native import grouping change COFF section metadata; their instruction payloads,
relocation records and source declarations remain independently inspected.

The combined native build reproduces all 22 Winsock thunk addresses beginning
at `0x1000A000`, deletion at `0x1000A084`, stack probing at `0x1000A090`,
`_onexit`/`atexit` at `0x1000A0BF`/`0x1000A0EB`, allocation/EH thunks at
`0x1000A0FE`/`0x1000A104`, CRT initialization/entry at
`0x1000A110`/`0x1000A1BB`, and the final exit/init thunks at
`0x1000A258`/`0x1000A25E`. The `.text` virtual size is 38,598 bytes.

A linked image cannot be compared directly with an UPX-unpacked reference to
judge import metadata equality. UPX 1.20 reconstructs the import lookup area,
zero hints, name packing, and resource directory layout when unpacking. Packing
and unpacking the complete candidate with the pinned tool reproduces every
import and resource byte in the reference. The only remaining `.rdata` difference
in that diagnostic roundtrip is its four-byte export timestamp. No import-name
or resource-directory patch is needed. Source instruction differences and archival
metadata still prevent complete packed equality.

The four native CRT COMMON declarations also require their recovered discovery
order. The CRT adapter reads the original declarations from the pinned SDK's
`atonexit.obj`, `crtdll.obj`, and `adjustfd.obj`, requiring each to be a unique
four-byte external COMMON with its original type and storage class. It emits a
sectionless COFF object containing just these declarations, before selecting CRT
code members. LINK allocates them in reverse declaration order, producing
`__adjust_fdiv` at `0x1000CE90`, `__pRawDllMain` at `0x1000CE94`, and
`___onexitend`/`___onexitbegin` at `0x1000CE98`/`0x1000CE9C`. This input has no
initialized or executable bytes and does not replace any SDK instruction. With
that discovery order, every native byte from `0x1000A090` through
`0x1000A266` matches, including all 13 previously differing absolute operands.
