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

## Conventional object-linking experiment

`scripts/omf_to_coff.py` is an independent Python implementation that converts
Delphi 6 OMF objects into Microsoft i386 COFF objects. It does not use the PE
merger or a third-party object converter. For example:

```sh
python3 scripts/omf_to_coff.py input.obj output.obj \
    --import-lib toolchains/vc7/lib/kernel32.lib \
    --aliases aliases.json --report conversion.json
```

Use Delphi's `-JP` output when compiling units with overloaded routines. The
compiler's internal mangled names remain intact; a JSON mapping can expose the
chosen C-compatible entry points, for example `{"@Add$qii": "_Add"}`. The optional
Microsoft import libraries supply the spelling of foreign C/stdcall symbols.
Aliases do not change calling conventions or implement a C++ ABI bridge.

The converter handles Borland virtual function/data segments as COFF COMDATs,
32-bit absolute/relative fixups, public/external symbols, iterated data,
external-data pointer cells, and initialization/finalization records. Threadvar
offsets become COFF section-relative relocations into `.tls$D`, between the
Microsoft CRT's TLS sentinels. Unsupported semantic records cause an error;
the tool deliberately drops compiler debug data, not executable code.

Run the reproducible end-to-end check with:

```sh
python3 scripts/check_omf_to_coff.py --report obj/static/omf-to-coff.json
```

It compiles disposable copies of the real `System` and `SysInit` sources plus
two probe units, converts them, archives them with Microsoft's `LIB`, and links
a C host with Microsoft's `LINK`. It runs at image bases `0x400000` and
`0x10000000`, checking globals, cross-unit data imports, callbacks, managed
strings, initialization/finalization, exception unwinding/finally, and isolated
thread-local storage on the main and worker threads. No source-built DLLs are
used. It also checks rejection of a bad checksum and an unsupported import
record. Scratch files and its Wine prefix stay under `~/.ai-tmp`.

This proves conventional object/static-library linking for the runtime and
those probes. It has not yet replaced `make static` or integrated the full VCL
plugins. The C host currently dispatches unit initializers explicitly and
connects `SysInit.TlsIndex` to the Microsoft CRT's TLS index. A complete build
still needs dependency-ordered startup, VCL/form resources, component symbol
isolation, and the frontend's built-in plugin registration. Native Windows has
not been tested.
