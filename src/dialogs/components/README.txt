Reconstructed Theme Manager sources for dialogs.dll
===================================================

Original authors: Mike Lischke (Theme Manager/Theme Services); Marcel van
Brakel, Mike Lischke and the credited contributors for the JEDI API headers.
Author and license notices are preserved in each source file.

These files reconstruct the linked April 2002 implementation from the newer
Theme Manager 1.10.1 sources preserved under ../vendor/theme-manager-1.10.1.
That directory's provenance.json records the source URLs, mirror commit, and
hashes. The vendor copy remains unchanged and is not used by the build.
The version/history comments in the reference-derived headers describe the
newer reference, not an identified release number for the original DLL.

The reconstruction restores:

- The original JwaUxTheme/JwaTmSchema unit names, themed-element ordinals,
  API initialization, UseThemes expression and per-manager ThemeServices.
- TThemeServices (128 bytes), TWindowProcList (36 bytes) and TThemeManager
  (112 bytes), including their original virtual methods and field layout.
- The original twelve theme options, control lists, message handlers,
  painting paths, form collection and manager election behavior.
- Immediate list sorting and binary search, without the later caches.
- The nine original assembler window-procedure adapters. These are symbolic
  Pascal assembler routines, not embedded target byte arrays.

Later listener/event machinery, control subclasses, singleton service access,
and recreation caches have been removed where absent from the linked binary.
The original API-loader mistake is retained: GetThemeBackgroundExtent loads
GetThemeBackgroundContentRect. Fixing it would change the reconstructed DLL.
The runtime copyright text is restored to the original 2001, 2002 string;
reference source copyright notices remain intact.

The complete linked byte ranges of JwaUxTheme, JwaTmSchema, ThemeSrv and
ThemeMgr match exactly, including addresses and literals. Whole-DLL equality
also validates their data, resources, initialization and dependencies. Code
and declarations discarded by Delphi's linker are inherited reference material
where no original evidence survives; no claim is made that they reproduce the
entire historical library release. This component directory is an inferred
source layout.
