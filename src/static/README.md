# Optional static user build

Run `make static` to produce `bin/nemu64-static.exe`. The toolchains and Wine
requirements are the same as for `make`. Both targets can coexist; the ordinary
executable and the historical reconstruction scripts retain their DLL builds.

The executable includes GameShark, Kaillera, audio, input, LemD3D8, debugger,
dialogs, profiler, and the combiner debugger. Ship it with `config.ini`,
`nemu64.ini`, `LemD3DCombine.dat`, and `LemD3DCombine.new`. The loose bundled DLLs
are intermediate build products and are not needed by this executable.

`Plugin/` is optional. Built-in plugins are registered first, then additional
DLLs in that directory are loaded through the existing plugin interface.
Case-insensitive matches for bundled plugin filenames are skipped, so leaving
the normal build's DLLs in place does not register them twice. Additional
graphics plugins appear in Settings; additional audio/input providers follow
the frontend's existing load-order behavior.

## Linking

The Delphi 6/VCL projects do not produce Microsoft COFF objects, and several
components need different compiler runtimes. Instead of porting those projects,
`scripts/link_static.py` links their source-built PE images offline:

- It places their sections in the executable, applies x86 relocations, resolves
  imports between bundled components, and combines system import descriptors.
- Each component retains a private PE header and resource/export namespace.
  Numeric resource IDs, repeated Delphi unit names, and separate CRT/VCL globals
  therefore remain independent. Section permissions are preserved.
- `StaticRuntime.c` provides module lookup for those namespaces and delivers
  process/thread startup and teardown through an executable TLS callback.
  Component calls to `VirtualQuery` retain the allocation-base convention used
  by Delphi to find a class's form resources.
- RAM is reserved before component initialization. Direct3D/DirectSound system
  imports are resolved afterward, preserving the normal frontend's ordering;
  loading their Wine drivers first can exhaust contiguous 32-bit address space.

The resulting executable is a fixed-address PE32 image. Windows maps its code
and data directly; it neither extracts DLL files nor maps embedded DLLs at
runtime. Windows/DirectX DLLs are still dependencies. This is a user-build link,
not a byte-matching reconstruction. The build records input hashes, component
locations, resolved imports, and output identity in `obj/static/link.json`.

## Validation and limits

```sh
python3 scripts/check_static.py --report obj/static/smoke.json
```

The disposable Wine/Xvfb fixtures test an absent plugin directory, an empty
one, and an external tool DLL alongside invalid duplicate bundled filenames.
They inspect built-in menus, open About/Disclaimer and the Delphi settings/input
dialogs, and require normal shutdown. The external plugin must receive process
attach/detach and tool initialization/cleanup calls. The PE import table is
checked for loose bundled dependencies. Scratch files and a dedicated Wine
prefix are created under `~/.ai-tmp`.

This remains experimental: it has been checked under Wine, not native Windows.
The smoke test does not cover ROM execution, rendered output, audio playback,
or a live netplay session. The custom linker intentionally rejects input static
TLS directories, delay-import tables, forwarded exports, and relocation types
other than x86 HIGHLOW rather than silently producing an invalid executable.

The private component handles are understood by the executable and bundled
components. They are not registered as DLLs with the Windows loader. External
plugins using the normal host interface work; an external DLL with a load-time
import of a bundled DLL still needs that dependency supplied as a loose DLL.
