# Optional static user build

Run `make static` to produce `bin/nemu64-static.exe`. It includes GameShark,
Kaillera, audio, input, LemD3D8, debugger, dialogs, profiler, and the combiner
debugger. Ship it with `config.ini`, `nemu64.ini`, `LemD3DCombine.dat`, and
`LemD3DCombine.new`. No bundled DLLs or `Plugin/` directory are required.
Windows and DirectX DLLs remain system dependencies.

`Plugin/` is optional. The frontend registers built-in implementations first,
then scans that directory for external dynamic plugins using the existing ABI.
Case-insensitive matches for bundled filenames are skipped to avoid duplicate
registration when both user builds share a directory. External graphics plugins
appear in Settings; audio/input providers retain the frontend's load-order
behavior. An external DLL that itself imports a bundled DLL still requires
that dependency as a loose DLL.

## Ordinary object and library linking

`scripts/build_static.py` builds relocatable objects, archives the components
with Microsoft `LIB`, and invokes Microsoft `LINK` to produce the executable.
There is no PE merger, embedded DLL image, module lookup interception, or
post-link executable patching.

- Delphi 6 Update 2 compiles the Pascal plugins and shared RTL/VCL with `-JPN`.
  `scripts/omf_to_coff.py` converts their OMF objects into Microsoft i386 COFF.
  The static build uses this one Delphi version for all Pascal components.
- Generated copies of the Pascal units have private unit names so repeated
  names such as `UnitMain` and `RCP` stay independent. Identical theme component
  units are shared between debugger and dialogs. Reconstructed sources are
  unchanged.
- C/C++ components use VC7 and its static multithreaded CRT/MFC. Their external
  COFF symbols receive component prefixes to isolate repeated globals and
  plugin entry points. Compiler relocations and static constructors are kept.
- Generated COFF jump thunks expose the existing plugin ABI; pointer cells
  satisfy the core's Kaillera call declarations. A function table registers
  built-ins directly. Only external plugins go through `LoadLibrary`.
- `StaticRuntime.cpp` initializes Delphi units in dependency order, connects
  Delphi threadvars to the native CRT's TLS block, initializes MFC, and calls
  the native components' attach/detach functions. Every component uses the
  executable's real `HINSTANCE` and resource namespace.
- Win32 `.res` inputs and Delphi form resources are collected before linking.
  Native components get distinct numeric resource ranges; dialog control IDs
  remain unchanged. Delphi resource strings use the RTL's literal-pointer mode.
- Emulated RAM is reserved before VCL/MFC initialization. Direct3D and
  DirectSound use MSVC's standard delay-load support so their drivers load
  later, after that reservation.

Generated sources, objects, `builtins.lib`, linker response files, the link map,
and conversion metadata are under `obj/static/`. The compiler log is
`obj/static/build.log`; the build report is `obj/static/link-report.json`.

This is an ordinary user build, not a byte-matching reconstruction. `make` and
the historical `scripts/build_*.py` workflows retain their separate DLL builds.

## Object converter

The Python converter uses only the standard library:

```sh
python3 scripts/omf_to_coff.py input.obj output.obj \
    --import-lib toolchains/vc7/lib/kernel32.lib \
    --aliases aliases.json --report conversion.json
```

It supports Borland virtual segments as COFF COMDATs, public/external symbols,
32-bit absolute/relative fixups, iterated data, external-data pointer cells,
resource/dependency metadata, initialization/finalization tables, and native
TLS section-relative relocations. Unsupported semantic records fail with an
error. Compiler debug data is discarded. Aliases change symbol spelling, not
calling conventions or C++ object layouts.

## Validation

```sh
python3 scripts/check_omf_to_coff.py --report obj/static/omf-to-coff.json
python3 scripts/check_static.py --components --report obj/static/smoke.json
```

The converter check links and runs actual Delphi runtime/probe objects at two
image bases. It exercises managed strings, callbacks, cross-unit globals,
initialization/finalization, exception unwinding, and thread-local storage.

The static executable check uses disposable Wine/Xvfb fixtures with an absent
plugin directory, an empty one, and an external tool alongside invalid duplicate
bundled filenames. It checks built-in menus, About/Disclaimer, Delphi
settings/input and native audio/video dialogs, external tool initialization/cleanup and DLL unload,
and normal shutdown. It also rejects imports of bundled DLLs. The optional
`--components` check links the existing GameShark fixture against
`obj/static/builtins.lib` and exercises MFC dialogs, cheat editing, and writes
to synthetic RAM. Scratch files and
Wine prefixes are allocated under `~/.ai-tmp`.

The build remains experimental. Validation is under Wine rather than native
Windows. These checks do not establish ROM execution, rendering accuracy, audio
playback, or a live netplay session.
