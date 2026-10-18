The LemD3D archive refers to `amdlib.h` and an external AMD3D library which it
does not include. This directory reconstructs the interface and retained parts
of that dependency. The release target enables its `DOITALLAMD` matrix path.
The complete DLL comparison verifies the resulting code and data bytes.

`detect.c` reconstructs the older 13-query detector at VA `0x10027950`, including
its cached CPUID state, vendor comparisons, feature bits, processor cases and
historical Cyrix spelling. AMD's later public `detect.c`/`adetect.h` sample
supplies the preserved notice and processor-switch source:

```
https://web.archive.org/web/20010101000000id_/http://www.amd.com/products/cpg/bin/getcpu_caps.zip
```

The later sample's additional strings, cache queries and processor aliases are
absent from the earlier implementation. Separate repeated switch cases are
needed to reproduce its original jump table. The unreferenced constant mask
array is recovered from retained read-only data; its original name and module
ownership are inferred.

`matrix.asm` adapts AMD's 3x3/4x4 Direct3D kernels from `matrx_lib.c` on the
[March 1999 AMD SDK disc](https://archive.org/details/amd-sdk), with the earlier
entry stubs, `FEMMS`, and column-major GL kernel recovered from the DLL. It is
readable instruction source for originally handwritten assembly. The complete
1,540-byte contribution, including retained unused kernels and alignment,
matches. MASM's PAGE alignment metadata is reduced to 32-byte alignment in the
matching build; no instruction bytes are edited.

`matrix.c` reconstructs the scalar `D3DMul_4x4` fallback as sixteen ordinary
floating-point dot products and a temporary matrix. Its entire 576-byte native
contribution matches. `math_lib.c` contains the retained packed float/sign-mask
initializer `ones`; the spelling and separate data-module arrangement are
inferred. Its archive is selected after the CRT to recover initialized-data
ordering. These files are not presented as a verbatim recovered AMD library.

The matching recipe compiles native C with VC6 and assembly with the tracked
MASM, while LemD3D itself uses VC7 LTCG. Original native compiler provenance is
preserved separately as archival Rich metadata. URLs and hashes of the samples
and disc sources are recorded in `analysis/lemd3d-upstream.json`.
