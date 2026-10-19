# Optional user build: combine the source-built components at link time. Keep
# the normal executable and all reconstruction compiler settings independent.
.PHONY: static
static: $(OUT)/nemu64-static.exe $(addprefix $(OUT)/,$(COPIED))

STATIC_RUNTIME := $(ROOT)/src/static/StaticRuntime.c
STATIC_BUILD_FILES := $(BUILD_RECIPE) $(ROOT)/scripts/static.mk
$(WORK)/static/nemu_static.dll: $(STATIC_RUNTIME) $(STATIC_BUILD_FILES)
	@cd "$(ROOT)"
	mkdir -p obj/static
	$(call tool,vc6,cl) /nologo /c /O2 /Zl /W3 /Foobj/static/StaticRuntime.obj src/static/StaticRuntime.c
	$(call tool,vc7,link) /nologo /DLL /NOENTRY /NODEFAULTLIB /INCREMENTAL:NO /MACHINE:I386 \
	    /OUT:obj/static/nemu_static.dll /IMPLIB:obj/static/nemu_static.lib \
	    obj/static/StaticRuntime.obj kernel32.lib user32.lib

$(WORK)/static/Plugins.obj: $(addprefix $(NEMU_SRC)/,Plugins.cpp Plugins.h CPU.h FrontendIni.h Memory.h RSP.h RomBrowser.h WinMain.h) $(ROOT)/src/static/StaticRuntime.h $(STATIC_BUILD_FILES)
	@cd "$(ROOT)"
	mkdir -p obj/static
	$(call tool,vc7,cl) $(NEMU_FLAGS) /DNEMU_STATIC /Foobj/static/Plugins.obj src/nemu64/Plugins.cpp

$(WORK)/static/WinMain.obj: $(addprefix $(NEMU_SRC)/,WinMain.cpp CPU.h Dynarec.h EasterText.h FrontendIni.h Kaillera.h Memory.h PIF.h Plugins.h RSP.h Rom.h RomBrowser.h State.h WinMain.h resource.h) $(ROOT)/src/static/StaticRuntime.h $(STATIC_BUILD_FILES)
	@cd "$(ROOT)"
	mkdir -p obj/static
	$(call tool,vc7,cl) $(NEMU_FLAGS) /DNEMU_STATIC /Foobj/static/WinMain.obj src/nemu64/WinMain.cpp

STATIC_NEMU_OBJS := $(filter-out $(WORK)/nemu64/Plugins.obj $(WORK)/nemu64/WinMain.obj,$(NEMU_OBJS)) $(WORK)/static/Plugins.obj $(WORK)/static/WinMain.obj
$(WORK)/static/nemu64.exe: $(STATIC_NEMU_OBJS) $(ZLIB_OBJS) $(WORK)/nemu64/nemu64.res $(WORK)/nemu64/imports/kailleraclient.lib $(WORK)/nemu64/imports/gs_dll.lib $(WORK)/static/nemu_static.dll $(STATIC_BUILD_FILES)
	@cd "$(ROOT)"
	$(call tool,vc7,link) /nologo /LTCG /SUBSYSTEM:WINDOWS /INCREMENTAL:NO /MACHINE:I386 \
	    /OUT:obj/static/nemu64.exe \
	    $(patsubst $(ROOT)/%,%,$(STATIC_NEMU_OBJS) $(ZLIB_OBJS)) obj/nemu64/nemu64.res \
	    obj/static/nemu_static.lib comctl32.lib winmm.lib \
	    obj/nemu64/imports/kailleraclient.lib obj/nemu64/imports/gs_dll.lib \
	    kernel32.lib user32.lib gdi32.lib comdlg32.lib shell32.lib

$(OUT)/nemu64-static.exe: $(WORK)/static/nemu64.exe $(addprefix $(OUT)/,$(filter-out nemu64.exe,$(REBUILT))) $(ROOT)/scripts/link_static.py $(ROOT)/scripts/pe_fingerprints.py
	cd "$(ROOT)"
	$(PYTHON) scripts/link_static.py
