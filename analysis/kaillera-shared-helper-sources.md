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

`itemlist.h` is the surviving original helper source. The two-field representation, allocation increments, returned added pointer, and shrinking/memcpy deletion behavior all agree with the target. The recovered public API and original file/class names replace the earlier inferred `PtrList` implementation. Local changes normalize line endings/trailing whitespace and include the allocation/string headers directly, making the original header self-contained. The diagnostic `void` specialization established the shared implementation independently. The integrated consumers retain concrete element types (`UDPCom`, the resizer item, and `ByteQueue`); derived cache helpers use the protected `m_size`.

The target list-view helper differs from the surviving WASTE revision: column widths are percentages of the client rectangle; `GetParam` zero-initializes its `LVITEM`; numeric comparison treats values beginning with `>` specially. Those behaviors remain reconstructed from Kaillera's instructions. Unused later WASTE methods are not required by the target. Public direct field access in the reconstructed header remains an inference where target inlining cannot establish the original access specifier or accessor spelling.

The WASTE `C_Config` has a portable file parser and only a string cache; Kaillera instead uses Windows profile APIs, separate integer/string caches, per-entry dirty flags, a fixed filename buffer, and a section buffer. The later source establishes shared lineage and the `C_Config` name but is not substituted for Kaillera's implementation. WASTE `C_SHBuf` is a reference-counted shared buffer and is unrelated to Kaillera's growable byte queue; it is not imported. No original byte-queue helper source has been recovered.

No downloaded binary or source archive is required to build the project. The diagnostic report is source evidence and is not a byte-exact DLL certificate.

## Additional Config source search

The [original WASTE 1.0 source archive](http://slackerbitch.free.fr/waste/waste-source.tar.gz)
is still available: 214,730 bytes, SHA-256
`a928dac65c0ca165652193f6b70cdca29fa7828fadb51fd52e3de7020926bc8d`.
Its SHA-1 `93583fb2577ee08f331d438bc1aa84aaa83d6386` also agrees with the
[historical NetBSD package inventory](https://mail-index.netbsd.org/pkgsrc-changes/2017/03/23/msg154598.html).
Both `config.h` and `config.cpp` are byte-identical to the 1.1 files listed above.
Thus the oldest located release already uses the portable, string-only parser;
checking the earlier release did not recover Kaillera's two-cache constructor.

Further inspected source families did not supply that implementation:

| Source | Inspected scope | Finding |
| --- | --- | --- |
| [Wasabi source](https://github.com/bizzehdee/wasabi/tree/49d293042ce17b0d137b2e4c6fc82a52bdb05091/src) | `bfc/util/inifile.cpp/.h` and `api/config/config.cpp/.h` | `IniFile` calls profile APIs directly; `ConfigFile` uses an XML/string-pair representation. Neither has the two caches. |
| [Winamp Wasabi config sources](https://github.com/alexfreud/winamp/tree/0954e03b3acbc11b6c90298598db6b6469568d72/Src/Wasabi/api/config) | 42 config-related C++/header files, including the bundled input-plugin Wasabi interfaces | No `C_Config` or matching integer/string cache implementation. |
| [SHOUTcast DSP source](https://github.com/alexfreud/winamp/tree/0954e03b3acbc11b6c90298598db6b6469568d72/Src/Plugins/DSP/dsp_sc) | 30 C++/header files outside bundled LAME | Settings use profile API calls and encoder-specific wrappers, without the matching cache. |
| [Nullsoft SafeSex 0.35](https://www.1014.org/code/nullsoft/safesex/safesex-setup.exe) | Source embedded in the original installer | Direct `GetPrivateProfileInt`/`WritePrivateProfileString` wrappers; no cached Config class. |
| [Nullsoft Beep 0.3](https://www.1014.org/code/nullsoft/nbeep/nbeep03.exe) | The original installer's complete `Source` directory | No Config/profile cache implementation. |

The inspected Wasabi `inifile.cpp`/`.h` hashes are
`773c680ca06251c498f60f15c86f66378803b24bdcef858bdd92376f9b901e7b` and
`1bff1204fd213a2f3b7f18220d8673d0c6c01d77362d3d05d41f8b0f28fa577a`;
its `config.cpp`/`.h` hashes are
`0f1d4ad74c1660f73378578f500e67456a0a80c5f6c2182e1c956bac1f365181` and
`2d7b014129f336b41533cc7266bb941807e05e03eff912014f1a59db61e8e176`.
The SafeSex and Beep installer SHA-256 values are
`72b6523b25fd3d8dfc70af4ddd2761de3a571079e5b83534c79a720fd96da01b` and
`6a406096ceb58864b74f24799004b79b1d5654e3c0822a4f1075b7b4d3bd8a56`.
These are negative search results, not a claim that the original helper never
survived elsewhere. None of these differing implementations is vendored or used
to justify a constructor change.
