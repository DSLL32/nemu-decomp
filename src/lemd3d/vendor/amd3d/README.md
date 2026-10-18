The source archive refers to `amdlib.h` and an external AMD3D library that it
does not include. Only `GetCPUCaps` is called in the release configuration;
the optional `DOITALLAMD` matrix paths are disabled, as in the archived source.

`detect.c` reconstructs the older detector at master VA `0x10027950`. The
13-entry query switch, vendor comparisons, three cached CPUID words, initialization
sequence, feature bits and CPU model cases are read from that function. The
header exposes the corresponding earlier query values, including CPU_MFG=0,
HAS_MMX=11 and HAS_3DNOW=12. All switch cases are implemented.

AMD's later public `detect.c`/`adetect.h` sample supplies the preserved copyright
notice, CPU model names and model-switch source. It was recovered from:

```
https://web.archive.org/web/20010101000000id_/http://www.amd.com/products/cpg/bin/getcpu_caps.zip
```

The newer sample also has strings, cache information and additional feature
queries absent from the target. Its separate `detect_base` function is folded
into the older initialization flow, and its later K6 model aliases are removed.
This is an adapted reconstruction, not an untouched copy of the original AMD
library or a byte-exact claim. The source is compiled as ordinary C without
LTCG; the exact earlier compiler/library remains unresolved. Input hashes and
the archived download URL are in `analysis/lemd3d-upstream.json`.
