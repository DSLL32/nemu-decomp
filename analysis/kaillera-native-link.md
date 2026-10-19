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
