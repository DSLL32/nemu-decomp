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
