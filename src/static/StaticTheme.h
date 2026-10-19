#ifndef NEMU_STATIC_THEME_H
#define NEMU_STATIC_THEME_H

/* Runtime-only UI theming, with native and VCL client palette support. */
extern "C" void __cdecl NemuStaticThemeInitialize(void);
extern "C" void __cdecl NemuStaticThemeVclReady(BOOL ready);
extern "C" void __cdecl NemuStaticThemeShutdown(void);

#endif
