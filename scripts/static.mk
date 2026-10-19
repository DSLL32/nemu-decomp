# Optional user build: MSVC LINK produces the executable directly from core
# objects and a native static library of all bundled components.
.PHONY: static
static: $(OUT)/nemu64-static.exe

STATIC_BUILD_FILES := $(BUILD_RECIPE) $(ROOT)/scripts/static.mk
STATIC_SOURCES := $(shell find $(ROOT)/src/static $(ROOT)/src/gs_dll $(ROOT)/src/audio $(ROOT)/src/kailleraclient $(ROOT)/src/lemd3d $(ROOT)/src/input $(ROOT)/src/debugger $(ROOT)/src/dialogs $(ROOT)/src/profile $(ROOT)/src/lemcombinedebugger -type f) \
    $(addprefix $(ROOT)/scripts/,build_static.py static_objects.py static_build_cache.py static_resources.py static_crash_symbols.py static_fpo.py omf_to_coff.py delphi_dfm.dpr run_toolchain.py) \
    $(shell find $(ROOT)/toolchains/d6u2/source/rtl $(ROOT)/toolchains/d6u2/source/vcl -type f \( -name '*.pas' -o -name '*.inc' \))
STATIC_CORE_FLAGS := $(filter-out /ML,$(NEMU_FLAGS)) /MT /Zi /DNEMU_STATIC
STATIC_CORE_OBJS := $(addprefix $(WORK)/static/core/,$(addsuffix .obj,$(NEMU_UNITS) ByteSwap))
STATIC_ZLIB_OBJS := $(addprefix $(WORK)/static/zlib/,$(addsuffix .obj,$(notdir $(ZLIB_UNITS))))
# /GL + /Zi objects refer to these build-only compiler PDBs. Regenerate an
# object if its PDB was deleted, even when the source timestamp is unchanged.
.PHONY: static-missing-pdb
# MSPDB lowercases filenames under Wine; match its sidecars case-insensitively.
STATIC_MISSING_PDB := $(shell $(PYTHON) -c 'from pathlib import Path; import sys; present={p.name.lower() for p in Path(sys.argv[1]).glob("*.pdb")}; print(" ".join(n for n in sys.argv[2:] if n.lower()+".pdb" not in present))' "$(WORK)/static/core" $(NEMU_UNITS) ByteSwap)
ifneq ($(strip $(STATIC_MISSING_PDB)),)
$(addprefix $(WORK)/static/core/,$(addsuffix .obj,$(STATIC_MISSING_PDB))): static-missing-pdb
endif
STATIC_DEFAULTS := $(addprefix $(ROOT)/master/,config.ini nemu64.ini LemD3DCombine.dat LemD3DCombine.new)
STATIC_CORE_HEADERS := $(wildcard $(NEMU_SRC)/*.h $(NEMU_SRC)/*.inc $(NEMU_SRC)/res/*.inc) $(ROOT)/src/static/StaticRuntime.h $(ROOT)/src/static/StaticFiles.h $(ROOT)/src/static/StaticCrash.h $(ROOT)/src/static/StaticCrashCore.inc

$(addprefix $(WORK)/static/core/,$(addsuffix .obj,$(NEMU_UNITS))): $(WORK)/static/core/%.obj: $(NEMU_SRC)/%.cpp $(STATIC_CORE_HEADERS) $(WORK)/nemu64/PifTables.inc $(STATIC_BUILD_FILES)
	@cd "$(ROOT)"
	mkdir -p obj/static/core
	$(call tool,vc7,cl) $(STATIC_CORE_FLAGS) /Fdobj/static/core/$*.pdb /Foobj/static/core/$*.obj src/nemu64/$*.cpp

$(WORK)/static/core/ByteSwap.obj: $(NEMU_SRC)/ByteSwap.c $(STATIC_BUILD_FILES)
	@cd "$(ROOT)"
	mkdir -p obj/static/core
	$(call tool,vc7,cl) $(STATIC_CORE_FLAGS) /Fdobj/static/core/ByteSwap.pdb /Foobj/static/core/ByteSwap.obj src/nemu64/ByteSwap.c

define static_zlib_object
$(WORK)/static/zlib/$(notdir $(1)).obj: $(ZLIB_SRC)/$(1).c $(wildcard $(ZLIB_SRC)/*.h) $(STATIC_BUILD_FILES)
	@cd "$(ROOT)"
	mkdir -p obj/static/zlib
	$$(call tool,vc7,cl) /nologo /c /O2 /Z7 /MT /DWIN32 /DNDEBUG /Isrc/nemu64/vendor/zlib-1.1.3 /Foobj/static/zlib/$(notdir $(1)).obj src/nemu64/vendor/zlib-1.1.3/$(1).c
endef
$(foreach unit,$(ZLIB_UNITS),$(eval $(call static_zlib_object,$(unit))))

$(OUT)/nemu64-static.exe: $(STATIC_CORE_OBJS) $(STATIC_ZLIB_OBJS) $(WORK)/nemu64/nemu64.res $(STATIC_SOURCES) $(STATIC_DEFAULTS) $(STATIC_BUILD_FILES)
	cd "$(ROOT)"
	$(PYTHON) scripts/build_static.py $(STATIC_CORE_OBJS) $(STATIC_ZLIB_OBJS)
