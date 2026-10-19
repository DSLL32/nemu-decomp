# Recovered shared helper source

The archived [WASTE 1.1 source release](https://sourceforge.net/projects/waste/files/WASTE%20Source/v1.1/), dated 2003-07-06, contains shared Nullsoft helpers whose native VC6 output establishes their relationship to the older Kaillera 0.84 client. This is a later surviving source snapshot, not a recovered complete Kaillera source tree. The original Kaillera helper revision remains unknown.

The [source ZIP](https://downloads.sourceforge.net/project/waste/WASTE%20Source/v1.1/waste-source.zip) is 409,324 bytes, SHA-256 `4e487b38877d8aea66c69ad98e7876dd35e3160209f228000ad87c98fb62d835`. The files below are also byte-identical in the [immutable mirror revision](https://github.com/m12n/waste/tree/c4e1a202ba4d6d2bd4f12e2b372fa341c066156f/src).

| Archive file | SHA-256 |
| --- | --- |
| `itemlist.h` | `9e77a18a28bd2c5a725e5d3ed6940f132b14b0ffc70fbdfbcb03a35cef64aecd` |
| `listview.h` | `f4beefe4302745a51e07a4fdd60662852c70a722f41a8293b11b2ed1dcc61815` |
| `listview.cpp` | `ecddb18a974c3791358ed81276671d125c5033bdaad304ffd9fbd6293cd7edf8` |
| `config.h` | `37cf60937cfec318423be2bd7d89996e3d362c5e6b3c860dbec8b96530809be4` |
| `config.cpp` | `939f7c052c050d7f8a13787df737a0257087eeb2a12ead70555a43e1aa4a2c04` |

The source notices and GNU GPL version 2 license are retained in the imported files and `src/kailleraclient/COPYING.waste`.

## Native source diagnostics

The unmodified recovered files were compiled with the pinned Visual C++ 6 RTM compiler using `/O2 /MD /GX /Gy /DWIN32 /DNDEBUG`. `itemlist.h` was explicitly instantiated as `C_ItemList<void>` in a scratch translation unit which included the standard allocation/string headers. `listview.cpp` used a scratch `platform.h` containing only Windows and standard-library includes. The report [kaillera-shared-helper-sources.json](kaillera-shared-helper-sources.json) records all emitted methods, including methods that do not match. Only COFF-declared relocation operands were excluded from these comparisons; this does not prove their final linked addresses.

| Unmodified recovered method | Whole COFF size | Kaillera address |
| --- | ---: | --- |
| `C_ItemList<void>::C_ItemList` | 16 | `0x10008D90` |
| `C_ItemList<void>::~C_ItemList` | 16 | `0x10008E60` |
| `C_ItemList<void>::Add` | 64 | `0x10008C20` |
| `C_ItemList<void>::Del` | 112 | `0x10008CF0` |
| `C_ItemList<void>::GetSize` | 16 | `0x10002380` |
| `W_ListView::InsertItem` | 96 | `0x10004240` |
| `W_ListView::SetItemText` | 96 | `0x100042A0` |
| `W_ListView::sortfunc` | 320 | `0x10004390` |
| `W_ListView::Resort` | 32 | `0x100044D0` |

A separate global `W_ListView` instance also generates the exact 48-byte initialization wrapper at `0x10008EB0`, using the recovered constructor unchanged. The empty destructor generates the observed destruction registration and empty destructor wrappers. `C_ItemList` exposes its array and count as protected members; this source evidence explains direct count access in the derived Kaillera byte-queue cache and the associated compiler bounds elimination.

## Scope and differences

`itemlist.h` is the surviving original helper source. The two-field representation, allocation increments, returned added pointer, and shrinking/memcpy deletion behavior all agree with the target. The recovered public API and original file/class names replace the earlier inferred `PtrList` implementation. Local changes normalize line endings and include the allocation/string headers directly, making the original header self-contained. All consumers use the single `C_ItemList<void>` specialization; derived cache helpers use its protected `m_size`.

The target list-view helper differs from the surviving WASTE revision: column widths are percentages of the client rectangle; `GetParam` zero-initializes its `LVITEM`; numeric comparison treats values beginning with `>` specially. Those behaviors remain reconstructed from Kaillera's instructions. Unused later WASTE methods are not required by the target. Public direct field access in the reconstructed header remains an inference where target inlining cannot establish the original access specifier or accessor spelling.

The WASTE `C_Config` has a portable file parser and only a string cache; Kaillera instead uses Windows profile APIs, separate integer/string caches, per-entry dirty flags, a fixed filename buffer, and a section buffer. The later source establishes shared lineage and the `C_Config` name but is not substituted for Kaillera's implementation. WASTE `C_SHBuf` is a reference-counted shared buffer and is unrelated to Kaillera's growable byte queue; it is not imported. No original byte-queue helper source has been recovered.

No downloaded binary or source archive is required to build the project. The diagnostic report is source evidence and is not a byte-exact DLL certificate.
