# Kaillera reliable UDP layer

All nine functions emitted from `src/kailleraclient/reliableconnection.cpp`
match complete native COFF signatures outside declared relocation operands.
These functions cover the original padded range `0x10001000–0x10001630`.
`kaillera-reliableconnection-instructions.json` records source hashes, compiler
flags, and expected original destinations for each unresolved relocation.
This is a component diagnostic; final linked bytes remain mandatory.

The original private file and class names are inferred. The cache owns two
256-record arrays, sequence numbers, and heap payloads. Its recovered inline
`get` method searches for a sequence number and copies a payload; using it
before and after a UDP read reproduces the compiler's shared success block
and separate error returns. The UDP receive length has the enclosing function
scope. Resending indexes the cache record directly in its `pack` expression;
introducing a local record pointer changes VC6 instruction scheduling.
No assembly, executable byte arrays, or code patches are used.

The UDP hostname overload is declared before the address overload because
VC6 places overloaded virtual members into the vtable in reverse declaration
order. This restores the observed address slot `+0x10` and hostname slot
`+0x14`; instruction matching with unresolved COFF operands cannot by itself
prove these identities.

Validation compiled with the original VC6 RTM toolchain using `/O2 /MD /GX
/Gy /DWIN32 /DNDEBUG`, in `/root/.ai-tmp/kaillera-helpers.hJeGTE` with its reused
Wine prefix, then checked all complete native functions against the historical
UPX 1.20 decompression of the pinned master DLL.
