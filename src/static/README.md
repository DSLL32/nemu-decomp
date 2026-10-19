# Optional static user build

Run `make -j8 static` to produce `bin/nemu64-static.exe`. It includes GameShark,
Kaillera, audio, input, LemD3D8, debugger, dialogs, profiler, and the combiner
debugger. The executable also embeds the tracked `master/` copies of
`config.ini`, `nemu64.ini`, `LemD3DCombine.dat`, and `LemD3DCombine.new`.
These files, the bundled DLLs, and the `Plugin/` directory are optional.
Windows and DirectX DLLs remain system dependencies.

`Plugin/` is optional. The frontend registers built-in implementations first,
then scans that directory for external dynamic plugins using the existing ABI.
Case-insensitive matches for bundled filenames are skipped to avoid duplicate
registration when both user builds share a directory. External graphics plugins
appear in Settings; audio/input providers retain the frontend's load-order
behavior. An external DLL that itself imports a bundled DLL still requires
that dependency as a loose DLL.

## Build performance

`make -j8 static` runs native plugin compilers in parallel as well as the host
compilers. `STATIC_JOBS=N` overrides the plugin worker count; `make -j1 static`
is serial. The shared Delphi RTL/VCL and plugin unit graph is compiled as one
group, then reused until its inputs change.

Content hashes under `obj/static/cache/` track sources, headers, flags, build
scripts, toolchains, and generated outputs. Native source edits rebuild their
object; header edits rebuild the affected component. A theme-only C++ edit
reuses Delphi and all bundled plugin objects. Deleted or modified cached outputs
are regenerated. Remove `obj/static/cache/` to force the component build again.
`obj/static/build.log` records command timing and overlapping tool invocations;
the final build message reports elapsed time, reuse, and peak concurrency.

## Embedded defaults and disk overrides

Reads use an existing disk file in its entirety, including an empty file.
Only a missing file falls back to its embedded bytes. A missing key in an
existing INI uses the caller's usual default; it does not inherit that key from
the embedded INI. Other files such as `Cheats.ini` retain their normal behavior.

The frontend, Pascal settings dialogs, video plugin, and combiner editor share
this behavior. Default INIs and combiner scripts are read directly from memory.
Reading or opening a dialog does not extract the four files.

Settings continue to save to disk. On the first INI write, a missing file is
seeded from its embedded version before applying the edit, preserving the rest
of its defaults. Normal frontend shutdown saves browser settings and therefore
creates `config.ini` if needed. Existing disk files are edited normally. The
combiner editor saves its edits to `LemD3DCombine.dat` using its existing `.new`
working-file/rename path; the embedded `.new` default is empty.

`make static` builds only the executable. `make` still produces the ordinary
DLL build and copies its four data files. Changes to the tracked default files
invalidate the static executable's build target.

## Native Windows dark mode

The static build opts into native dark window frames and eligible native popup
menus on Windows 10 1809 and later, following the Windows **app** theme setting.
High contrast takes precedence. Light/dark and accessibility changes are picked
up while the application runs. The ordinary DLL build retains its original UI.

`StaticTheme.cpp` resolves the optional DWM and UxTheme functions at runtime,
loading DLLs by their system-directory paths. Neither DLL is a startup import.
Older Windows returns before looking up dark-mode ordinals. The actual NT
version is queried dynamically, and the two signatures of UxTheme ordinal 135
are handled separately for Windows 10 1809 and 1903+. Missing DLLs, missing
exports and unsupported DWM attributes are tolerated.

A thread-local window hook catches new and recreated handles. Optional common
control subclass helpers are resolved at runtime. Pushbuttons, combo boxes,
edit fields, lists, trees and their scrollbars retain native rendering, with
per-control theme names and supported color messages. Custom painting is limited
to the menu bar, header/status/tab controls, checkbox/radio/group-box gaps,
and application artwork. Light mode restores the
control colors; high contrast uses the original Windows colors. Keyboard input,
control commands and focus cues continue through the original window procedures.

`StaticThemeControls.inc` supplies dark faces/edges for VCL's own classic
painters and removes the toolbar's light divider. It does not paint native
scrollbars or install scrollbar repaint timers. Eject/pause/stop/reset icons use an inverted,
masked copy of the toolbar image list; magenta remains transparent. A separate
masked gray image list keeps disabled glyphs visible. Toolbar dropdown arrows
use the dark foreground after native painting, including their disabled state. The shared browser images
remain intact. The original image lists and divider flag are restored in light mode.

Generated debugger painters use `StaticThemeColor` for explicit backgrounds,
register-label resets, current instructions, selections, search results and
read/write watches. Read watches remain yellow, writes blue, and combined
watches green, with dark backgrounds and readable foregrounds. The memory
painter restores its brush color after transparent text, including the address
and ASCII columns. Other bitmap/image data keeps its original colors.

The adapter detaches before plugin cleanup starts, while the VCL forms are
still alive, so destruction cannot reenter its UI hooks.

Bundled Delphi code resolves `GetSysColor` and `GetSysColorBrush` to static-build
wrappers. `StaticVclTheme.pas` invalidates VCL's cached GDI resources and updates
forms, including labels and panels without HWNDs. Ordinary black text and white
edit backgrounds use the corresponding system colors; explicit semantic colors
and artwork retain their meanings. The generated copy of the old XP theme
component uses classic VCL painting while dark, so it cannot repaint clients
with a light theme. The reconstructed plugin and Delphi toolchain sources are
unchanged. Dynamically loaded plugins can still have their own custom painters.

The DWM frame API is described in [Microsoft's theme guidance](https://learn.microsoft.com/en-us/windows/apps/desktop/modernize/ui/apply-windows-themes).
The optional UxTheme app/menu APIs are undocumented; a current Microsoft example
is [PowerToys' implementation](https://github.com/microsoft/PowerToys/blob/main/src/modules/ZoomIt/ZoomIt/Utility.cpp).

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

## Crash reports (static build only)

Fatal exceptions display a small Win32 error dialog with expandable Details,
Copy report, and Close. Details include a build identifier, exception/access
address, x86 registers, and stack locations as module/function/offset. Closing
ends the faulted process. The reporter is prepared on its own thread during
startup, after the emulated address space has been reserved.

The reporter caches the application's dark/light preference while the UI is
healthy. Windows owns its pushbuttons and the report edit control, including
the border and scrollbars. The dialog supplies background/text colors and
requests native dark themes when available; it has no replacement control
painters, edit subclasses, or scrollbar repaint timers.
It does not depend on the faulted UI thread's hooks or VCL. Optional DWM/UxTheme
exports are resolved at startup only on supported Windows versions. Dialog
text uses the system message font; details use Lucida Console at the same
size, with DPI-scaled controls. The fault is the first stack entry, marked
`[fault]`, rather than repeated in a separate fault-location section.

The runtime uses XP-era kernel32/user32/gdi32 APIs. It needs no PDB, MAP,
DbgHelp, redistributable crash library, symbol server, or extracted file.
Optional vectored-exception entry points are resolved from kernel32 at runtime.
The observer records context but never treats a handled exception as fatal.
Generated VCL code forwards unhandled hardware exceptions with a first-chance
stack snapshot; ordinary validation exceptions and EAbort keep their handlers.
The core forwards fatal native errors from its CPU exception filter while
retaining recoverable generated-code memory accesses and memory breakpoints.

The build keeps `/O2`, `/GL`, and frame-pointer omission. `/Zi` on the core and
`/Z7` on native components provide metadata; each core compilation has a private
PDB so parallel compiler jobs don't contend for one file. The linker emits
`obj/static/nemu64-static.pdb`. It is a build artifact, and may be used for native
C/C++ debugging, but is unnecessary when distributing the EXE.

`scripts/static_fpo.py` extracts the VC7 linker's x86 FrameData unwind recipes.
The recipes and a compact index of public/local C/C++ and Delphi code symbols
are embedded as ordinary RCDATA. A second link verifies that code bytes,
function RVAs, and recipes are unchanged before accepting the executable.
The runtime evaluates the compiler's bounded postfix recipes. Frames requiring
a bounded return-address search are marked; code without metadata can use saved
frame links, which may skip frameless functions. Unsupported/partial-prologue
states stop that walk. Separately labelled stack candidates are heuristic.
Inlined/tail-called functions and arbitrary hand-written stack manipulation
cannot always be reconstructed. Dynrec locations report the code-buffer offset
and current guest PC; loose DLLs retain module offsets and nearest export names.

Stack walking checks memory and stack bounds and limits all searches. Reporting
is still best effort if the process is too damaged to run its reporting thread
or create Windows controls. There is no claim of recovery after a fatal fault.

Run `python3 scripts/check_static_crash.py` for actual optimized main/worker
thread faults, damaged-frame-pointer handling, handled-exception behavior, the
Delphi VCL bridge, an external plugin crash, dark/light painting, report font
size, scrolling, and dialog/copy/close checks. These
fixtures use a disposable Wine prefix configured as XP and an isolated launch
folder with no PDB/MAP files. Native Windows XP testing remains outstanding.

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
and normal shutdown. An additional fixture starts with only the executable
and verifies that the read-only databases stay absent. It also rejects imports
of bundled DLLs. The optional
`--components` check links the existing GameShark fixture against
`obj/static/builtins.lib` and exercises MFC dialogs, cheat editing, and writes
to synthetic RAM. Another archive fixture compares every embedded INI section
and key against Windows, exercises Delphi `TIniFile` and text I/O, runs the
renderer's actual combiner parser with embedded/disk/empty inputs, and verifies
first-save behavior. The theme compatibility probe uses simulated OS exports
with real windows, dialogs, thread hooks and message delivery. It checks old
Windows versions, both ordinal-135 signatures, missing capabilities, live theme
changes and high contrast, with assertions for rendered control pixels.
A second fixture links the real static UI against a test-only OS preference
adapter, checks dark client pixels and live light/high-contrast restoration, and
captures the main window and Delphi settings/input dialogs as BMPs in scratch.
The same fixture opens the actual commands, memory and register windows using
a test memory reader, checks all three watch types plus search highlighting,
refreshes GPR labels, and verifies both dark and original light memory colors.
It also checks icon inversion/ownership, VCL classic faces, and native
ROM-list scrolling. Native button/combo/nonclient paint probes ensure the adapter
does not replace Windows' drawing; Wine pixels are not used as evidence that a
native dark theme exists. This synthetic reader is confined to the test executable.
The shipped executable contains no forced preference and is checked for
accidental DWM/UxTheme startup imports. Native Windows rendering still needs
visual verification. Scratch files and
Wine prefixes are allocated under `~/.ai-tmp`.

The build remains experimental. Validation is under Wine rather than native
Windows. These checks do not establish ROM execution, rendering accuracy, audio
playback, or a live netplay session.
