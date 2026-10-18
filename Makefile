# GNU Make build of the recovered sources. Run `make` to populate bin/ with
# the same layout as master/. Unrecovered files are copied from master/.
# Intermediate files live in obj/; Wine uses the user's configuration.
# This is the ordinary user build; scripts/build_*.py handle byte matching.

SHELL := /bin/bash
.SHELLFLAGS := -eu -o pipefail -c
.ONESHELL:
.DELETE_ON_ERROR:
.DEFAULT_GOAL := all

ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
OUT := $(ROOT)/bin
WORK := $(ROOT)/obj
PYTHON ?= python3
WINE ?= wine
TOOLCHAINS := toolchains

vc6_INCLUDE := include mfc/include atl/include
vc6_LIB := lib mfc/lib
vc7_INCLUDE := include atlmfc/include platformsdk/include
vc7_LIB := lib atlmfc/lib platformsdk/lib
empty :=
space := $(empty) $(empty)

# Set only the historical tools' search paths; inherit Wine configuration.
# All commands run from the repository root, except Delphi links in obj/<name>.
# The optional third argument is the relative path back to the repository root.
# Echo the native command, then run it through Wine with the required paths.
define tool
run_tool() {
    set -- $(if $(filter d6%,$(1)),-U'$(subst /,\,$(3)$(TOOLCHAINS)/$(1)/lib)' -I'$(subst /,\,$(3)$(TOOLCHAINS)/$(1)/lib)' -R'$(subst /,\,$(3)$(TOOLCHAINS)/$(1)/lib)') "$$@"
    printf -v tool_command '%q ' '$(2)' "$$@"
    printf '%s\n' "$${tool_command% }"
    env WINEPATH="$(3)$(TOOLCHAINS)/$(1)/bin;$(3)$(TOOLCHAINS)/$(1)/runtime" \
        INCLUDE="$(subst $(space),;,$(addprefix $(3)$(TOOLCHAINS)/$(1)/,$($(1)_INCLUDE)))" \
        LIB="$(subst $(space),;,$(addprefix $(3)$(TOOLCHAINS)/$(1)/,$($(1)_LIB)))" \
        $(WINE) "$(3)$(TOOLCHAINS)/$(1)/bin/$(2).exe" "$$@"
}
run_tool
endef

REBUILT := nemu64.exe gs_dll.dll Plugin/audio.dll Plugin/Input.dll \
           Plugin/debugger.dll Plugin/dialogs.dll Plugin/profile.dll
MASTER_FILES := $(shell cd "$(ROOT)/master" && find . -type f -printf '%P\n')
COPIED := $(filter-out $(REBUILT),$(MASTER_FILES))
BUILD_RECIPE := $(ROOT)/Makefile

.PHONY: all clean nemu64 gs audio input debugger dialogs profile
all: $(addprefix $(OUT)/,$(REBUILT) $(COPIED))
nemu64: $(OUT)/nemu64.exe
gs: $(OUT)/gs_dll.dll
audio: $(OUT)/Plugin/audio.dll
input: $(OUT)/Plugin/Input.dll
debugger: $(OUT)/Plugin/debugger.dll
dialogs: $(OUT)/Plugin/dialogs.dll
profile: $(OUT)/Plugin/profile.dll

# Only files without a recovered build target use this fallback rule.
$(addprefix $(OUT)/,$(COPIED)): $(OUT)/%: $(ROOT)/master/%
	mkdir -p "$(@D)"
	cp "$<" "$@"

NEMU_UNITS := RSP RSPReciprocal DynarecTracking DynarecEmitter Dynarec CPU \
              CPUFPU CPUExecution DynarecOpcodes DynarecMemoryOps DynarecFPU \
              CpuControl Memory Plugins State WinMain FrontendIni Rom RomBrowser \
              RuntimeSignatures CIC PIF SaveDevices Globals LegacyData
ZLIB_UNITS := uncompr compress contrib/minizip/unzip inflate deflate crc32 \
              infblock adler32 zutil trees infcodes inftrees infutil inffast
NEMU_OBJS := $(addprefix $(WORK)/nemu64/,$(addsuffix .obj,$(NEMU_UNITS) ByteSwap))
ZLIB_OBJS := $(addprefix $(WORK)/nemu64/zlib/,$(addsuffix .obj,$(notdir $(ZLIB_UNITS))))
NEMU_FLAGS := /nologo /c /O2 /Ob1 /ML /GL /W3 /DWIN32 /DNDEBUG /D_WINDOWS \
              /DWINVER=0x0500 /D_WIN32_WINNT=0x0400 \
              /Isrc/nemu64/vendor/zlib-1.1.3 /Iobj/nemu64

$(WORK)/nemu64/%.obj: $(ROOT)/src/nemu64/%.cpp $(BUILD_RECIPE)
	@cd "$(ROOT)"
	mkdir -p obj/nemu64
	$(call tool,vc7,cl) $(NEMU_FLAGS) /Foobj/nemu64/$*.obj src/nemu64/$*.cpp

$(WORK)/nemu64/ByteSwap.obj: $(ROOT)/src/nemu64/ByteSwap.c $(BUILD_RECIPE)
	@cd "$(ROOT)"
	mkdir -p obj/nemu64
	$(call tool,vc7,cl) $(NEMU_FLAGS) /Foobj/nemu64/ByteSwap.obj src/nemu64/ByteSwap.c

# Generate data directly into obj/; the compiler finds it through /Iobj/nemu64.
$(WORK)/nemu64/PifTables.inc: $(ROOT)/src/nemu64/res/generate_pif_table.py $(ROOT)/src/nemu64/res/generate_pif_tables.py $(BUILD_RECIPE)
	cd "$(ROOT)"
	mkdir -p obj/nemu64
	$(PYTHON) src/nemu64/res/generate_pif_tables.py obj/nemu64/PifTables.inc

$(WORK)/nemu64/Globals.obj: $(WORK)/nemu64/PifTables.inc

$(WORK)/nemu64/res/pif-table.bin: $(ROOT)/src/nemu64/res/generate_pif_table.py $(BUILD_RECIPE)
	cd "$(ROOT)"
	mkdir -p obj/nemu64/res
	$(PYTHON) src/nemu64/res/generate_pif_table.py obj/nemu64/res/pif-table.bin

define zlib_object
$(WORK)/nemu64/zlib/$(notdir $(1)).obj: $(ROOT)/src/nemu64/vendor/zlib-1.1.3/$(1).c $(BUILD_RECIPE)
	@cd "$(ROOT)"
	mkdir -p obj/nemu64/zlib
	$$(call tool,vc7,cl) /nologo /c /O2 /ML /DWIN32 /DNDEBUG /Isrc/nemu64/vendor/zlib-1.1.3 /Foobj/nemu64/zlib/$(notdir $(1)).obj src/nemu64/vendor/zlib-1.1.3/$(1).c
endef
$(foreach unit,$(ZLIB_UNITS),$(eval $(call zlib_object,$(unit))))

$(WORK)/nemu64/imports/kailleraclient_symbols.obj: $(ROOT)/src/nemu64/imports/kailleraclient_symbols.asm $(BUILD_RECIPE)
	@cd "$(ROOT)"
	mkdir -p obj/nemu64/imports
	$(call tool,vc7,ml) /nologo /c /coff /Foobj/nemu64/imports/kailleraclient_symbols.obj src/nemu64/imports/kailleraclient_symbols.asm

$(WORK)/nemu64/imports/kailleraclient.lib: $(ROOT)/src/nemu64/imports/kailleraclient.def $(WORK)/nemu64/imports/kailleraclient_symbols.obj $(BUILD_RECIPE)
	@cd "$(ROOT)"
	$(call tool,vc7,lib) /nologo /machine:I386 /def:src/nemu64/imports/kailleraclient.def obj/nemu64/imports/kailleraclient_symbols.obj /out:obj/nemu64/imports/kailleraclient.lib

$(WORK)/nemu64/imports/gs_dll.lib: $(ROOT)/src/nemu64/imports/gs_dll.def $(BUILD_RECIPE)
	@cd "$(ROOT)"
	mkdir -p obj/nemu64/imports
	$(call tool,vc7,lib) /nologo /machine:I386 /def:src/nemu64/imports/gs_dll.def /out:obj/nemu64/imports/gs_dll.lib

NEMU_ASSETS := $(wildcard $(ROOT)/src/nemu64/res/*.ico $(ROOT)/src/nemu64/res/*.bmp)
$(WORK)/nemu64/nemu64.res: $(ROOT)/src/nemu64/nemu64.rc $(ROOT)/src/nemu64/resource.h $(ROOT)/src/nemu64/nemu64.manifest $(NEMU_ASSETS) $(WORK)/nemu64/res/pif-table.bin $(BUILD_RECIPE)
	@cd "$(ROOT)"
	$(call tool,vc7,rc) /Isrc/nemu64 /Iobj/nemu64 /foobj/nemu64/nemu64.res src/nemu64/nemu64.rc

$(OUT)/nemu64.exe: $(NEMU_OBJS) $(ZLIB_OBJS) $(WORK)/nemu64/nemu64.res $(WORK)/nemu64/imports/kailleraclient.lib $(WORK)/nemu64/imports/gs_dll.lib $(BUILD_RECIPE)
	@cd "$(ROOT)"
	mkdir -p bin
	$(call tool,vc7,link) /nologo /LTCG /SUBSYSTEM:WINDOWS /INCREMENTAL:NO /MACHINE:I386 \
	    /OUT:bin/nemu64.exe \
	    $(patsubst $(ROOT)/%,%,$(NEMU_OBJS) $(ZLIB_OBJS)) obj/nemu64/nemu64.res \
	    comctl32.lib winmm.lib obj/nemu64/imports/kailleraclient.lib obj/nemu64/imports/gs_dll.lib \
	    kernel32.lib user32.lib gdi32.lib comdlg32.lib shell32.lib

GS_UNITS := Addsubcode gs_dll gs_exeDlg GS_interpret NewCodeDlg PropertiesDlg RW_INI_files StdAfx
GS_OBJS := $(addprefix $(WORK)/gs_dll/,$(addsuffix .obj,$(GS_UNITS)))
GS_FLAGS := /nologo /c /O2 /MT /W3 /GX /DWIN32 /DNDEBUG /D_WINDOWS /D_USRDLL

$(GS_OBJS): $(WORK)/gs_dll/%.obj: $(ROOT)/src/gs_dll/%.cpp $(BUILD_RECIPE)
	@cd "$(ROOT)"
	mkdir -p obj/gs_dll
	$(call tool,vc6,cl) $(GS_FLAGS) /Foobj/gs_dll/$*.obj src/gs_dll/$*.cpp

GS_ASSETS := $(wildcard $(ROOT)/src/gs_dll/res/*)
$(WORK)/gs_dll/gs_dll.res: $(ROOT)/src/gs_dll/gs_dll.rc $(ROOT)/src/gs_dll/resource.h $(GS_ASSETS) $(BUILD_RECIPE)
	@cd "$(ROOT)"
	mkdir -p obj/gs_dll
	$(call tool,vc6,rc) /Isrc/gs_dll /l 0x409 /foobj/gs_dll/gs_dll.res src/gs_dll/gs_dll.rc

$(OUT)/gs_dll.dll: $(GS_OBJS) $(WORK)/gs_dll/gs_dll.res $(ROOT)/src/gs_dll/gs_dll.def $(BUILD_RECIPE)
	@cd "$(ROOT)"
	mkdir -p bin
	$(call tool,vc6,link) /nologo /subsystem:windows /dll /incremental:no /machine:I386 \
	    /def:src/gs_dll/gs_dll.def /out:bin/gs_dll.dll /implib:obj/gs_dll/gs_dll.lib \
	    $(patsubst $(ROOT)/%,%,$(GS_OBJS)) obj/gs_dll/gs_dll.res

AUDIO_UNITS := ABI1 ABI2 AudioHLE ABI3 AudioLegacy DirectSound AudioLog CPUFeatures \
               Jpeg Mp3 AudioProcessing AudioResample AudioPlugin
AUDIO_OBJS := $(addprefix $(WORK)/audio/,$(addsuffix .obj,$(AUDIO_UNITS) RSP))
AUDIO_FLAGS := /nologo /c /O2 /Ob1 /MT /W3 /DWIN32 /DNDEBUG /D_WINDOWS /D_USRDLL

$(WORK)/audio/%.obj: $(ROOT)/src/audio/ahle_ds/%.c $(BUILD_RECIPE)
	@cd "$(ROOT)"
	mkdir -p obj/audio
	$(call tool,vc7,cl) $(AUDIO_FLAGS) /Foobj/audio/$*.obj src/audio/ahle_ds/$*.c

$(WORK)/audio/RSP.obj: $(ROOT)/src/audio/ahle_ds/RSP.cpp $(BUILD_RECIPE)
	@cd "$(ROOT)"
	mkdir -p obj/audio
	$(call tool,vc7,cl) $(AUDIO_FLAGS) /Foobj/audio/RSP.obj src/audio/ahle_ds/RSP.cpp

$(WORK)/audio/Audio.res: $(ROOT)/src/audio/ahle_ds/Audio.rc $(BUILD_RECIPE)
	@cd "$(ROOT)"
	mkdir -p obj/audio
	$(call tool,vc7,rc) /Isrc/audio/ahle_ds /l 0x409 /foobj/audio/Audio.res src/audio/ahle_ds/Audio.rc

$(OUT)/Plugin/audio.dll: $(AUDIO_OBJS) $(WORK)/audio/Audio.res $(ROOT)/src/audio/ahle_ds/audio.def $(ROOT)/toolchains/vc6/lib/dsound.lib $(BUILD_RECIPE)
	@cd "$(ROOT)"
	mkdir -p bin/Plugin
	$(call tool,vc7,link) /nologo /dll /incremental:no \
	    /subsystem:windows /machine:I386 \
	    /out:bin/Plugin/audio.dll /implib:obj/audio/audio.lib \
	    /def:src/audio/ahle_ds/audio.def $(patsubst $(ROOT)/%,%,$(AUDIO_OBJS)) obj/audio/Audio.res \
	    toolchains/vc6/lib/dsound.lib winmm.lib kernel32.lib user32.lib gdi32.lib

# Explicit source and project-header prerequisites for incremental builds.
NEMU_SRC := $(ROOT)/src/nemu64
$(WORK)/nemu64/ByteSwap.obj: $(addprefix $(NEMU_SRC)/,ByteSwap.c)
$(WORK)/nemu64/CIC.obj: $(addprefix $(NEMU_SRC)/,CIC.cpp CPU.h Memory.h PIF.h Plugins.h)
$(WORK)/nemu64/CPU.obj: $(addprefix $(NEMU_SRC)/, \
    CPU.cpp CPU.h Dynarec.h Memory.h Plugins.h RSP.h RomBrowser.h State.h WinMain.h)
$(WORK)/nemu64/CPUExecution.obj: $(addprefix $(NEMU_SRC)/, \
    CPUExecution.cpp CPU.h Dynarec.h Memory.h Plugins.h RSP.h RomBrowser.h State.h WinMain.h)
$(WORK)/nemu64/CPUFPU.obj: $(addprefix $(NEMU_SRC)/,CPUFPU.cpp CPU.h)
$(WORK)/nemu64/CpuControl.obj: $(addprefix $(NEMU_SRC)/, \
    CpuControl.cpp CPU.h CpuControl.h Dynarec.h DynarecFPU.h DynarecOpcodes.h FrontendIni.h Memory.h \
    PIF.h Plugins.h RSP.h Rom.h RomBrowser.h State.h WinMain.h res/BootRspImem.inc \
    res/DefaultMemPack.inc)
$(WORK)/nemu64/Dynarec.obj: $(addprefix $(NEMU_SRC)/, \
    Dynarec.cpp CPU.h CpuControl.h Dynarec.h DynarecFPU.h DynarecMemoryOps.h FrontendIni.h Memory.h \
    PIF.h Plugins.h RSP.h Rom.h RomBrowser.h WinMain.h)
$(WORK)/nemu64/DynarecEmitter.obj: $(addprefix $(NEMU_SRC)/, \
    DynarecEmitter.cpp CPU.h CpuControl.h Dynarec.h DynarecFPU.h DynarecMemoryOps.h FrontendIni.h \
    Memory.h PIF.h Plugins.h RSP.h Rom.h RomBrowser.h WinMain.h)
$(WORK)/nemu64/DynarecFPU.obj: $(addprefix $(NEMU_SRC)/, \
    DynarecFPU.cpp CPU.h CpuControl.h Dynarec.h DynarecFPU.h Plugins.h)
$(WORK)/nemu64/DynarecMemoryOps.obj: $(addprefix $(NEMU_SRC)/, \
    DynarecMemoryOps.cpp CPU.h Dynarec.h DynarecFPU.h DynarecMemoryOps.h DynarecOpcodes.h Memory.h \
    Plugins.h)
$(WORK)/nemu64/DynarecOpcodes.obj: $(addprefix $(NEMU_SRC)/, \
    DynarecOpcodes.cpp CPU.h CpuControl.h Dynarec.h DynarecFPU.h DynarecMemoryOps.h DynarecOpcodes.h \
    Memory.h Plugins.h Rom.h RomBrowser.h)
$(WORK)/nemu64/DynarecTracking.obj: $(addprefix $(NEMU_SRC)/, \
    DynarecTracking.cpp CPU.h CpuControl.h Dynarec.h DynarecFPU.h DynarecMemoryOps.h FrontendIni.h \
    Memory.h PIF.h Plugins.h RSP.h Rom.h RomBrowser.h WinMain.h)
$(WORK)/nemu64/FrontendIni.obj: $(addprefix $(NEMU_SRC)/, \
    FrontendIni.cpp CPU.h Dynarec.h FrontendIni.h Memory.h PIF.h Plugins.h Rom.h RomBrowser.h \
    WinMain.h)
$(WORK)/nemu64/Globals.obj: $(addprefix $(NEMU_SRC)/,Globals.cpp CPU.h PIF.h Plugins.h RSP.h)
$(WORK)/nemu64/LegacyData.obj: $(addprefix $(NEMU_SRC)/,LegacyData.cpp)
$(WORK)/nemu64/Memory.obj: $(addprefix $(NEMU_SRC)/, \
    Memory.cpp CPU.h Dynarec.h Memory.h PIF.h Plugins.h Rom.h RomBrowser.h WinMain.h)
$(WORK)/nemu64/PIF.obj: $(addprefix $(NEMU_SRC)/,PIF.cpp CPU.h Memory.h PIF.h Plugins.h)
$(WORK)/nemu64/Plugins.obj: $(addprefix $(NEMU_SRC)/, \
    Plugins.cpp CPU.h FrontendIni.h Memory.h Plugins.h RSP.h RomBrowser.h WinMain.h)
$(WORK)/nemu64/RSP.obj: $(addprefix $(NEMU_SRC)/,RSP.cpp CPU.h Memory.h RSP.h)
$(WORK)/nemu64/RSPReciprocal.obj: $(addprefix $(NEMU_SRC)/,RSPReciprocal.cpp CPU.h RSP.h)
$(WORK)/nemu64/Rom.obj: $(addprefix $(NEMU_SRC)/, \
    Rom.cpp CPU.h Dynarec.h FrontendIni.h Memory.h PIF.h Plugins.h Rom.h RomBrowser.h WinMain.h \
    vendor/zlib-1.1.3/contrib/minizip/unzip.h vendor/zlib-1.1.3/zconf.h vendor/zlib-1.1.3/zlib.h)
$(WORK)/nemu64/RomBrowser.obj: $(addprefix $(NEMU_SRC)/, \
    RomBrowser.cpp FrontendIni.h Rom.h RomBrowser.h WinMain.h resource.h)
$(WORK)/nemu64/RuntimeSignatures.obj: $(addprefix $(NEMU_SRC)/, \
    RuntimeSignatures.cpp CPU.h CpuControl.h Dynarec.h DynarecFPU.h DynarecMemoryOps.h \
    DynarecSignatures.inc FrontendIni.h Memory.h PIF.h Plugins.h RSP.h Rom.h RomBrowser.h WinMain.h)
$(WORK)/nemu64/SaveDevices.obj: $(addprefix $(NEMU_SRC)/,SaveDevices.cpp CPU.h Memory.h PIF.h Plugins.h)
$(WORK)/nemu64/State.obj: $(addprefix $(NEMU_SRC)/, \
    State.cpp CPU.h CpuControl.h DynarecFPU.h Memory.h PIF.h Plugins.h RSP.h RomBrowser.h State.h \
    WinMain.h resource.h vendor/zlib-1.1.3/zconf.h vendor/zlib-1.1.3/zlib.h)
$(WORK)/nemu64/WinMain.obj: $(addprefix $(NEMU_SRC)/, \
    WinMain.cpp CPU.h Dynarec.h EasterText.h FrontendIni.h Kaillera.h Memory.h PIF.h Plugins.h RSP.h \
    Rom.h RomBrowser.h State.h WinMain.h resource.h)

ZLIB_SRC := $(ROOT)/src/nemu64/vendor/zlib-1.1.3
$(WORK)/nemu64/zlib/adler32.obj: $(addprefix $(ZLIB_SRC)/,adler32.c zconf.h zlib.h)
$(WORK)/nemu64/zlib/compress.obj: $(addprefix $(ZLIB_SRC)/,compress.c zconf.h zlib.h)
$(WORK)/nemu64/zlib/unzip.obj: $(addprefix $(ZLIB_SRC)/, \
    contrib/minizip/unzip.c contrib/minizip/unzip.h zconf.h zlib.h)
$(WORK)/nemu64/zlib/crc32.obj: $(addprefix $(ZLIB_SRC)/,crc32.c zconf.h zlib.h)
$(WORK)/nemu64/zlib/deflate.obj: $(addprefix $(ZLIB_SRC)/,deflate.c deflate.h zconf.h zlib.h zutil.h)
$(WORK)/nemu64/zlib/infblock.obj: $(addprefix $(ZLIB_SRC)/, \
    infblock.c infblock.h infcodes.h inftrees.h infutil.h zconf.h zlib.h zutil.h)
$(WORK)/nemu64/zlib/infcodes.obj: $(addprefix $(ZLIB_SRC)/, \
    infcodes.c infblock.h infcodes.h inffast.h inftrees.h infutil.h zconf.h zlib.h zutil.h)
$(WORK)/nemu64/zlib/inffast.obj: $(addprefix $(ZLIB_SRC)/, \
    inffast.c infblock.h infcodes.h inffast.h inftrees.h infutil.h zconf.h zlib.h zutil.h)
$(WORK)/nemu64/zlib/inflate.obj: $(addprefix $(ZLIB_SRC)/,inflate.c infblock.h zconf.h zlib.h zutil.h)
$(WORK)/nemu64/zlib/inftrees.obj: $(addprefix $(ZLIB_SRC)/,inftrees.c inffixed.h inftrees.h zconf.h zlib.h zutil.h)
$(WORK)/nemu64/zlib/infutil.obj: $(addprefix $(ZLIB_SRC)/, \
    infutil.c infblock.h infcodes.h inftrees.h infutil.h zconf.h zlib.h zutil.h)
$(WORK)/nemu64/zlib/trees.obj: $(addprefix $(ZLIB_SRC)/,trees.c deflate.h trees.h zconf.h zlib.h zutil.h)
$(WORK)/nemu64/zlib/uncompr.obj: $(addprefix $(ZLIB_SRC)/,uncompr.c zconf.h zlib.h)
$(WORK)/nemu64/zlib/zutil.obj: $(addprefix $(ZLIB_SRC)/,zutil.c zconf.h zlib.h zutil.h)

GS_SRC := $(ROOT)/src/gs_dll
$(WORK)/gs_dll/Addsubcode.obj: $(addprefix $(GS_SRC)/, \
    Addsubcode.cpp Addsubcode.h StdAfx.h gs_dll.h resource.h)
$(WORK)/gs_dll/GS_interpret.obj: $(addprefix $(GS_SRC)/, \
    GS_interpret.cpp GS_interpret.h RW_INI_files.h StdAfx.h gs_dll.h gs_exeDlg.h resource.h)
$(WORK)/gs_dll/NewCodeDlg.obj: $(addprefix $(GS_SRC)/, \
    NewCodeDlg.cpp NewCodeDlg.h StdAfx.h gs_dll.h resource.h)
$(WORK)/gs_dll/PropertiesDlg.obj: $(addprefix $(GS_SRC)/, \
    PropertiesDlg.cpp PropertiesDlg.h StdAfx.h gs_dll.h resource.h)
$(WORK)/gs_dll/RW_INI_files.obj: $(addprefix $(GS_SRC)/,RW_INI_files.cpp RW_INI_files.h StdAfx.h)
$(WORK)/gs_dll/StdAfx.obj: $(addprefix $(GS_SRC)/,StdAfx.cpp StdAfx.h)
$(WORK)/gs_dll/gs_dll.obj: $(addprefix $(GS_SRC)/,gs_dll.cpp StdAfx.h gs_dll.h resource.h)
$(WORK)/gs_dll/gs_exeDlg.obj: $(addprefix $(GS_SRC)/, \
    gs_exeDlg.cpp Addsubcode.h GS_interpret.h NewCodeDlg.h PropertiesDlg.h RW_INI_files.h StdAfx.h \
    gs_dll.h gs_exeDlg.h resource.h)

AUDIO_SRC := $(ROOT)/src/audio/ahle_ds
$(WORK)/audio/ABI1.obj: $(addprefix $(AUDIO_SRC)/, \
    ABI1.c AudioCommands.h AudioHLE.h AudioPlugin.h AudioSIMD.h transform.h)
$(WORK)/audio/ABI2.obj: $(addprefix $(AUDIO_SRC)/, \
    ABI2.c AudioCommands.h AudioHLE.h AudioPlugin.h AudioSIMD.h transform.h)
$(WORK)/audio/ABI3.obj: $(addprefix $(AUDIO_SRC)/, \
    ABI3.c AudioCommands.h AudioHLE.h AudioPlugin.h AudioSIMD.h RSPMicrocode.inc transform.h)
$(WORK)/audio/AudioHLE.obj: $(addprefix $(AUDIO_SRC)/, \
    AudioHLE.c AudioCommands.h AudioHLE.h AudioPlugin.h AudioProcessing.h DirectSound.h transform.h)
$(WORK)/audio/AudioLegacy.obj: $(addprefix $(AUDIO_SRC)/, \
    AudioLegacy.c AudioHLE.h AudioLegacy.h AudioPlugin.h transform.h)
$(WORK)/audio/AudioLog.obj: $(addprefix $(AUDIO_SRC)/,AudioLog.c transform.h)
$(WORK)/audio/AudioPlugin.obj: $(addprefix $(AUDIO_SRC)/,AudioPlugin.c AudioPlugin.h transform.h)
$(WORK)/audio/AudioProcessing.obj: $(addprefix $(AUDIO_SRC)/, \
    AudioProcessing.c AudioADPCMBlock.h AudioHLE.h AudioPlugin.h AudioProcessing.h transform.h)
$(WORK)/audio/AudioResample.obj: $(addprefix $(AUDIO_SRC)/, \
    AudioResample.c AudioADPCMBlock.h AudioADPCMLegacy.h AudioHLE.h AudioPlugin.h AudioProcessing.h \
    AudioResampleTables.h transform.h)
$(WORK)/audio/CPUFeatures.obj: $(addprefix $(AUDIO_SRC)/,CPUFeatures.c transform.h)
$(WORK)/audio/DirectSound.obj: $(addprefix $(AUDIO_SRC)/,DirectSound.c DirectSound.h transform.h)
$(WORK)/audio/Jpeg.obj: $(addprefix $(AUDIO_SRC)/,Jpeg.c transform.h)
$(WORK)/audio/Mp3.obj: $(addprefix $(AUDIO_SRC)/, \
    Mp3.c AudioADPCMBlock.h AudioEnvelope.h AudioHLE.h AudioPlugin.h AudioProcessing.h transform.h)
$(WORK)/audio/RSP.obj: $(addprefix $(AUDIO_SRC)/, \
    RSP.cpp AudioHLE.h AudioPlugin.h RSP.h RSPData.inc RSPKernel.inc RSPMicrocode.inc transform.h)

# Convert text forms to resources in obj/. DCC runs there so {$R *.dfm}
# finds these resources, while Pascal source is read directly from src/.
define delphi_project
$(WORK)/$(1)/delphi_dfm.exe: $(ROOT)/scripts/delphi_dfm.dpr $(BUILD_RECIPE)
	@mkdir -p "$(WORK)/$(1)"
	cd "$(WORK)/$(1)"
	$$(call tool,$(2),dcc32-personal,../../) -Q -M -N. -E. '..\..\scripts\delphi_dfm.dpr'

$(WORK)/$(1)/%.dfm: $(ROOT)/src/$(1)/%.dfm $(WORK)/$(1)/delphi_dfm.exe
	cd "$(ROOT)"
	$(WINE) obj/$(1)/delphi_dfm.exe resource src/$(1)/$$*.dfm obj/$(1)/$$*.dfm
	rm -f "$(WORK)/$(1)/$$*.dcu"

$(WORK)/$(1)/XPManifest.res: $(ROOT)/src/$(1)/XPManifest.rc $(ROOT)/src/$(1)/$(1).manifest $(BUILD_RECIPE)
	@cd "$(ROOT)"
	mkdir -p obj/$(1)
	$$(call tool,vc7,rc) /Isrc/$(1) /foobj/$(1)/XPManifest.res src/$(1)/XPManifest.rc

$(OUT)/Plugin/$(3).dll: $(wildcard $(ROOT)/src/$(1)/*.pas $(ROOT)/src/$(1)/*.dpr $(ROOT)/src/$(1)/components/*.pas $(ROOT)/src/$(1)/components/*.inc) \
    $(patsubst $(ROOT)/src/$(1)/%,$(WORK)/$(1)/%,$(wildcard $(ROOT)/src/$(1)/*.dfm)) \
    $(if $(filter dialogs debugger,$(1)),$(WORK)/$(1)/XPManifest.res) $(BUILD_RECIPE)
	@mkdir -p "$(WORK)/$(1)" "$(OUT)/Plugin"
	cd "$(WORK)/$(1)"
	# Include-file edits invalidate cached local units; normal Pascal edits are
	# handled by Delphi's make mode and its DCU dependency information.
	for inc in ../../src/$(1)/components/*.inc; do
	    if [[ -f "$$$$inc" && ( ! -f "$$@" || "$$$$inc" -nt "$$@" ) ]]; then
	        rm -f ./*.dcu
	        break
	    fi
	done
	# Explicit project "uses ... in" entries are relative to this working
	# directory. Compile their source units first, then link their cached DCUs.
	for unit in ../../src/$(1)/*.pas ../../src/$(1)/components/*.pas; do
	    [[ -f "$$$$unit" ]] || continue
	    name=$$$${unit##*/}
	    dcu=$$$${name%.pas}.dcu
	    if [[ ! -f "$$$$dcu" || "$$$$unit" -nt "$$$$dcu" || ../../Makefile -nt "$$$$dcu" ]]; then
	        $$(call tool,$(2),dcc32-personal,../../) $(4) -Q -M -N. -E. \
	            -U'.;..\..\src\$(1);..\..\src\$(1)\components;..\..\toolchains\$(2)\lib' \
	            -I'..\..\src\$(1)\components;..\..\toolchains\$(2)\lib' \
	            -R'.;..\..\toolchains\$(2)\lib' "$$$${unit//\//\\}"
	    fi
	done
	$$(call tool,$(2),dcc32-personal,../../) $(4) -Q -M -N. -E. \
	    -U'.;..\..\src\$(1);..\..\src\$(1)\components;..\..\toolchains\$(2)\lib' \
	    -I'..\..\src\$(1)\components;..\..\toolchains\$(2)\lib' \
	    -R'.;..\..\toolchains\$(2)\lib' '..\..\src\$(1)\$(3).dpr'
	cp $(3).dll "$$@"
endef
$(eval $(call delphi_project,dialogs,d6u1,dialogs))
$(eval $(call delphi_project,debugger,d6u2,debugger))
$(eval $(call delphi_project,input,d6u2,Input,-DNEMU_FIX_HID_ENUMERATION))
$(eval $(call delphi_project,profile,d6rtm,profile))


clean:
	rm -rf "$(OUT)" "$(WORK)"
