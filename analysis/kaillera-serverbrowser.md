# Kaillera server browser reconstruction

`src/kailleraclient/serverbrowser.cpp` reconstructs the server selection dialog,
HTTP list download, row parsing, UDP pings, direct-address dialog, About dialog,
and username validation. The private source filename, type names, and variable
names are inferred. The code uses ordinary C++ and the recovered JNetLib source;
no original executable instructions or opaque instruction arrays are included.

All 24 native COFF functions, including 16 global construction/destruction and
`atexit` wrappers, match their expected original addresses outside their declared
four-byte COFF relocation operands. Their padded COFF spans cover the complete
4,448-byte range `0x10008EA0–0x1000A000`. The eight application function spans are:

| Original VA | Source function | Padded COFF bytes |
| --- | --- | ---: |
| `0x10008FC0` | `selectServerDialog` | 32 |
| `0x10008FE0` | `browserDialog` | 2,048 |
| `0x100097E0` | `updateServerList` | 1,552 |
| `0x10009DF0` | `setBrowserStatus` | 32 |
| `0x10009E10` | `browserMessage` | 64 |
| `0x10009E50` | `addressDialog` | 272 |
| `0x10009F60` | `aboutDialog` | 80 |
| `0x10009FB0` | `readUsername` | 80 |

The compiler is the unchanged VC6 RTM toolchain with `/O2 /MD /GX /Gy /DWIN32
/DNDEBUG`. The final HTTP cleanup schedules require a file-local request pointer
and plain `delete`. The connection hostname overload uses original UDP vtable
slot `+0x14`; VC6 reverses the declaration order of those overloaded virtual
functions. `ListView` has an inline constructor and empty destructor, and the
two simple dialog controls have inline zero initialization and empty destructors,
accounting for the native lifetime wrappers.

`analysis/kaillera-serverbrowser-instructions.json` records the source hashes,
reference hash, expected addresses, and every masked relocation's source symbol
and original operand/destination. This is a component diagnostic, not a claim of
complete DLL equality. Final link layout, relocation identities, resource bytes,
and UPX reproduction remain part of the complete Kaillera verification.

Validation used an isolated scratch directory and reused Wine prefix at
`/root/.ai-tmp/kaillera-serverbrowser.AbTIBeCe`. To reproduce the component check,
compile `serverbrowser.cpp` with `scripts/run_toolchain.py vc6rtm cl` and those
flags, then run `scripts/check_kaillera.py --reference <unpacked120.dll> --objects
<serverbrowser.obj> --report <report.json>`. That diagnostic intentionally exits
1 because it does not certify a complete DLL.
