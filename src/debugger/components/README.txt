Reconstructed Theme Manager sources for debugger.dll
====================================================

Original authors: Mike Lischke (Theme Manager/Theme Services); Marcel van
Brakel, Mike Lischke and the credited contributors for the JEDI API headers.
Author and license notices are preserved in each source file.

These units share the linked April 2002 implementation reconstructed for
../../dialogs/components.
The unchanged newer reference and its source provenance are preserved under
../../dialogs/vendor/theme-manager-1.10.1. The version/history comments in
the source headers describe that newer reference, not an identified original
release number. This component directory is an inferred source layout.

The debugger contains the same linked implementation as dialogs.dll, including:

- TThemeServices (128 bytes), TWindowProcList (36 bytes) and TThemeManager
  (112 bytes), with the same fields, virtual methods and twelve theme options.
- Per-manager ThemeServices, original JwaUxTheme/JwaTmSchema unit names,
  API initialization, themed-element ordinals and UseThemes expression.
- Immediate list sorting and binary search, the original painting paths,
  form collection and manager election, and nine symbolic assembler window
  procedure adapters. No target machine-code arrays are embedded.
- The original API-loader mistake: GetThemeBackgroundExtent resolves
  GetThemeBackgroundContentRect. Correcting it changes the original behavior.

Compiled with the tracked Delphi 6 Update 2 DCUs, the complete CODE spans have
identical lengths and identical bytes after diagnostic masking of relocations
and external relative calls/jumps:

  Unit          Original RVA   Length
  JwaUxTheme       000BAF54     000009B7
  JwaTmSchema      000BB90C     00000038
  ThemeSrv         000BB944     000008AC
  ThemeMgr         000BC1F0     000024D3

Address masking is an investigation aid; whole-file byte equality is the
completion test. The sources require no implementation changes from the
already byte-exact dialogs reconstruction. The debugger build uses the
unmodified Update 2 Personal compiler with the tracked Update 2 DCUs. Its
DVCLAL edition resource matches the original debugger.dll. Personal and
Enterprise Update 2 compilers produce identical executable sections in the
component reference probe; their edition resources differ.

An earlier identification experiment also used the Update 1 Personal compiler
with Update 2 DCUs and matched the Update 2 Enterprise probe's CODE, DATA,
imports, exports and relocations. That experiment is historical evidence,
not the compiler configuration used by scripts/build_debugger.py.

Unlinked declarations and routines remain inherited reference material where
no binary evidence survives; their historical textual identity is unknown.
