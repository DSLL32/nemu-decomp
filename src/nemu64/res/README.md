The `.ico` and `.bmp` files contain the original image payloads, reconstructed
as ordinary editable image files. Their filenames are inferred. The XML manifest,
menu, accelerators, five dialogs, version information, and string table are
represented as editable source in the parent directory. `nemu64.rc` uses the
original Windows-1252 code page, including the copyright symbol.

`pif-table.bin` is generated in the build directory; it is not a checked-in copy
of the original resource. Run:

```sh
python3 res/generate_pif_table.py res/pif-table.bin
```

Resource 187 contains the CIC-NUS-6105 responses to every five-nibble challenge
prefix, followed by three zero challenge nibbles. The generator emits all
1,048,576 entries in ascending prefix order. Its 4,194,304 bytes match the original
resource exactly, with SHA-256
`f7c883d9bd2ef4910665dd6deccae79e0d2f33b385795eca5eaa8e423720bc0c`.

The algorithm is adapted from [X-Scale's 2011 CIC implementation](https://github.com/mupen64plus/mupen64plus-core/blob/master/src/device/pif/n64_cic_nus_6105.c).
Its license is in [CIC6105-LICENSE.txt](CIC6105-LICENSE.txt). This later algorithm
reconstructs the data; it is not presented as Nemu64's original table generator.
The executable itself still uses its reconstructed lookup-table consumer.

The resource is lookup data, not executable code. Startup code at `0x41BB06`
loads resource type 10, ID 187, then copies 1,048,576 DWORDs into the allocation
held at `0x44CC518`. The consumer at `0x420420` expands a 15-byte challenge into
nibbles, indexes the table using the first 20 challenge bits, and continues the
response using smaller nibble/history tables. Both the access pattern and full
algorithmic regeneration establish its data role.

The VC7 resource compiler reproduces all 29 payloads. A resource-only DLL also
reproduces the entire 4,296,704-byte `.rsrc` section, including resource ordering
and padding, when its data RVAs are compared at the original section address.
See [the inventory and comparison report](../../../analysis/nemu-resources.json).

The executable also initializes two 256-byte transition tables and a 32,768-entry
history table in `.data`. Generate their C definitions separately:

```sh
python3 res/generate_pif_tables.py PifTables.inc
```

Include that generated file in exactly one translation unit. It defines
`PifResponseTable1`, `PifResponseTable0`, and `PifResponseHistory` in the original
order. Their combined 131,584 bytes match `0x440B10..0x460D10`, including when
compiled with the tracked VC7 compiler.

The transition tables enumerate `(key_table[previous_response] + 5 * challenge)
& 15`. The history generator enumerates four-nibble challenges in ascending
order, starting with CIC key `0xB` and table 0, and retains those that finish in
table 1. Each entry also records the first fifth response in `{1, 9, B, E}` found
when trying fifth challenges `0..15`. This reproduces every history entry and its
original ordering. The consumer ignores that fifth response nibble when
classifying the preceding four responses. As with the larger resource table,
this is inferred data-generation logic; the original generator's source text is
unknown.
