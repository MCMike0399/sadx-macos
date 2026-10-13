# sadx-macos build. Layout (docs/ARCHITECTURE.md): src/<layer>/ with layers cpu loader math engine input
# player game runtime guest platform/mac apps; tests/{unit,probes,fixtures,python}; tools/{lib,gate,analysis,
# emit,oracles}; third_party/; game/ and the large parts of reference/ are ignored inputs.
#   add a module      : src/<layer>/<name>.{hpp,cpp} + tests/unit/<name>.cpp + one line in tests/unit/manifest.tsv
#   add an oracle     : tests/probes/native_<slug>_probe.cpp + tools/oracles/verify_original_<slug>.py
#                       (target oracle-<slug-with-dashes> appears by itself; manifest.tsv only for non-default inputs)
#   add a mac program : src/apps/<name>.mm + a rule next to sadx-app below
CXX := xcrun clang++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Werror -ffp-contract=off -fno-fast-math -arch arm64 -mmacosx-version-min=14.0
MACFLAGS := -fobjc-arc
MAC_FRAMEWORKS := -framework Foundation -framework AppKit -framework Metal -framework MetalKit -framework QuartzCore -framework CoreGraphics -framework ImageIO -framework GameController
INCLUDES := -Isrc -Itests/fixtures
GAME_DIR ?= game/Sonic Adventure DX
GAME_EXE ?= $(GAME_DIR)/sonic.exe
GAME_SYSTEM ?= $(GAME_DIR)/system
INPUT_MOD_DLL ?= $(GAME_DIR)/mods/sadx-input-mod/sadx-input-mod.dll
# The pinned "updated speedrun" profile install (reference/updated-pc-reference): the DLLs the oracles pin.
INPUT_DLL ?= reference/updated-pc-reference/game/mods/sadx-input-mod/sadx-input-mod.dll
LOADER_DLL ?= reference/updated-pc-reference/game/mods/.modloader/SADXModLoader.dll
SPEEDRUN_DLL ?= reference/updated-pc-reference/unpacked/speedrun/sadx-speedrun-mod/sadx-speedrun-mod.dll
# Python for the oracles and tools: the reference venv (Unicorn 2.1.4) with tools/lib + tools/oracles importable.
REFERENCE_PYTHON ?= PYTHONPATH=tools/lib:tools/oracles reference/python/bin/python
PY ?= PYTHONPATH=tools/lib:tools/oracles python3
REFERENCE_ALTERNATE_VARIANT ?= attribute4000
# Unicorn 2.1.4 (the oracle engine) as the in-process guest CPU; the arm64 static library ships in the
# reference venv's wheel. Point UNICORN_DIR at another install (include/ + lib/libunicorn.a) if needed.
UNICORN_DIR ?= $(firstword $(wildcard reference/python/lib/python3.*/site-packages/unicorn))

# Layers that go into libsadx-core.a (portable, no platform frameworks), lowest first.
CORE_LAYERS := cpu loader math engine input player game runtime
CORE_SOURCES := $(foreach a,$(CORE_LAYERS),$(wildcard src/$(a)/*.cpp))
CORE_OBJECTS := $(patsubst src/%.cpp,build/obj/%.o,$(CORE_SOURCES))
GUEST_SOURCES := $(wildcard src/guest/*.cpp src/hle/*.cpp)
GUEST_OBJECTS := $(patsubst src/%.cpp,build/obj/%.o,$(GUEST_SOURCES))
GUEST_LINK := build/libsadx-guest.a $(UNICORN_DIR)/lib/libunicorn.a -lpthread
MAC_SOURCES := $(wildcard src/platform/mac/*.cpp src/platform/mac/*.mm)
MAC_OBJECTS := $(patsubst src/%.cpp,build/obj/%.o,$(filter %.cpp,$(MAC_SOURCES))) $(patsubst src/%.mm,build/obj/%.o,$(filter %.mm,$(MAC_SOURCES)))
UNIT_SOURCES := $(wildcard tests/unit/*.cpp)
UNIT_NAMES := $(patsubst tests/unit/%.cpp,%,$(UNIT_SOURCES))
UNIT_TARGETS := $(foreach n,$(UNIT_NAMES),build/$(subst _,-,$(n))-test)
UNIT_MM_SOURCES := $(wildcard tests/unit/*.mm)
UNIT_MM_TARGETS := $(foreach n,$(patsubst tests/unit/%.mm,%,$(UNIT_MM_SOURCES)),build/$(subst _,-,$(n))-test)
PROBE_SOURCES := $(wildcard tests/probes/native_*_probe.cpp)
PROBE_NAMES := $(patsubst tests/probes/native_%_probe.cpp,%,$(PROBE_SOURCES))
PROBE_TARGETS := $(foreach n,$(PROBE_NAMES),build/native-$(subst _,-,$(n))-probe.dylib)

SOFTFLOAT_FILES := $(wildcard third_party/softfloat/source/*.c third_party/softfloat/source/include/*.h third_party/softfloat/source/8086/*.c third_party/softfloat/source/8086/*.h) third_party/softfloat/platform.h third_party/softfloat/sources.txt third_party/softfloat/build.sh
SOFTFLOAT_SSE_FILES := $(wildcard third_party/softfloat_sse/source/*.c third_party/softfloat_sse/source/8086-SSE/*) third_party/softfloat_sse/sources.txt third_party/softfloat_sse/build.sh
BOCHS_FILES := $(wildcard third_party/bochs_fpatan/source/fpu/* third_party/bochs_fpatan/source/softfloat3e/*.cc third_party/bochs_fpatan/source/softfloat3e/include/*.h) third_party/bochs_fpatan/math.hpp third_party/bochs_fpatan/config.h third_party/bochs_fpatan/sources.txt third_party/bochs_fpatan/build.sh

.PHONY: all assets test unit-tests run-unit-tests probes oracles oracles-verdict render controllers check gate clean manifests app-smoke status
all: build/sadx-metal build/libsadx-core.a build/libsadx-macos.a build/libsadx-guest.a build/sadx-boot build/sadx-app

build:
	mkdir -p build build/obj

# Objects: per-file dependency tracking (-MMD) instead of "every header rebuilds everything".
build/obj/%.o: src/%.cpp Makefile | build
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP $(INCLUDES) -c $< -o $@
build/obj/%.o: src/%.mm Makefile | build
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(MACFLAGS) -MMD -MP $(INCLUDES) -c $< -o $@
build/obj/cpu/x87_transcendental.o: CXXFLAGS += -Ithird_party/bochs_fpatan -Ithird_party/bochs_fpatan/source -Ithird_party/bochs_fpatan/source/softfloat3e/include
build/obj/cpu/sse_math.o: third_party/softfloat/source/include/softfloat.h third_party/softfloat/source/include/softfloat_types.h
build/obj/guest/%.o: CXXFLAGS += -I$(UNICORN_DIR)/include
build/obj/hle/%.o: CXXFLAGS += -I$(UNICORN_DIR)/include
-include $(CORE_OBJECTS:.o=.d) $(MAC_OBJECTS:.o=.d) $(GUEST_OBJECTS:.o=.d)

build/softfloat/softfloat.a: $(SOFTFLOAT_FILES) | build
	sh third_party/softfloat/build.sh build/softfloat
build/softfloat_sse/softfloat-sse.a: $(SOFTFLOAT_SSE_FILES) $(SOFTFLOAT_FILES) | build
	sh third_party/softfloat_sse/build.sh build/softfloat_sse
build/bochs_fpatan/libbochs_fpatan.a: $(BOCHS_FILES) | build
	CXX="$(shell xcrun --find clang++)" sh third_party/bochs_fpatan/build.sh build/bochs_fpatan

# The three archives: portable core, mac platform, guest CPU (+ the hle layer above it).
build/libsadx-core.a: $(CORE_OBJECTS) build/softfloat_sse/softfloat-sse.a build/softfloat/softfloat.a build/bochs_fpatan/libbochs_fpatan.a
	xcrun libtool -static -o $@ $^
build/libsadx-macos.a: $(MAC_OBJECTS)
	xcrun libtool -static -o $@ $^
build/libsadx-guest.a: $(GUEST_OBJECTS)
	xcrun libtool -static -o $@ $^

# Programs (src/apps).
build/sadx-boot: src/apps/sadx_boot.cpp build/libsadx-macos.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-macos.a build/libsadx-core.a -o $@
build/sadx-app: src/apps/sadx_app.mm build/libsadx-macos.a build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) -I$(UNICORN_DIR)/include $< build/libsadx-macos.a $(GUEST_LINK) build/libsadx-core.a $(MAC_FRAMEWORKS) -o $@
build/sadx-metal: src/apps/metal_stage.mm Makefile | build
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $< -framework Foundation -framework Metal -framework CoreGraphics -framework GameController -o $@
# The window smoke test of the roadmap (M2 app-smoke): the guest in the app for 120 frames, a screenshot, exit 0
# (in the gate; it opens a window, so the gate needs a logged-in GUI session).
REFERENCE_GAME_DIR ?= reference/updated-pc-reference/game
WINDOWS_SYSTEM_DIR ?= reference/windows-system
app-smoke: build/sadx-app
	./build/sadx-app --guest "$(REFERENCE_GAME_DIR)" "$(WINDOWS_SYSTEM_DIR)" --exit-after 120 --screenshot build/app-smoke.ppm > build/app-smoke.log && test -s build/app-smoke.ppm && python3 tools/gate/check_app_smoke.py build/app-smoke.ppm build/app-smoke.log

assets:
	python3 tools/lib/extract.py "$(GAME_EXE)" --output build/assets
render: all assets
	./build/sadx-metal --render build/assets/emerald-coast.mesh build/emerald-coast.ppm
controllers: all
	./build/sadx-metal --controllers

# Unit tests: tests/unit/<name>.cpp -> build/<name-with-dashes>-test against libsadx-core; tests/unit/<name>.mm
# additionally link libsadx-macos and the frameworks; tests/unit/guest_<name>.cpp link the guest + Unicorn.
.SECONDEXPANSION:
# Apple make 3.81 picks the first matching pattern rule: the guest rule must precede the generic one.
build/guest-%-test: tests/unit/guest_$$(subst -,_,$$*).cpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I$(UNICORN_DIR)/include $< $(GUEST_LINK) build/libsadx-core.a -o $@
# Tests named mac-* / metal-* exercise src/platform/mac and link that archive too.
build/mac-%-test: tests/unit/mac_$$(subst -,_,$$*).cpp build/libsadx-macos.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-macos.a build/libsadx-core.a $(MAC_FRAMEWORKS) -o $@
build/metal-%-test: tests/unit/metal_$$(subst -,_,$$*).cpp build/libsadx-macos.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-macos.a build/libsadx-core.a $(MAC_FRAMEWORKS) -o $@
build/%-test: tests/unit/$$(subst -,_,$$*).cpp build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-core.a -o $@
build/%-test: tests/unit/$$(subst -,_,$$*).mm build/libsadx-macos.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-macos.a build/libsadx-core.a $(MAC_FRAMEWORKS) -o $@
# Fixture headers some tests include.
build/input-test build/mac-input-test: tests/fixtures/input_keyboard_goldens.hpp
build/ninja-queue-consumer-test: tests/fixtures/ninja_sprite_test_fixture.hpp
build/native-player-path-probe.dylib build/native-player-direction-reset-probe.dylib: tests/fixtures/path_probe_data.hpp

# Native probes: tests/probes/native_<x>_probe.cpp -> build/native-<x>-probe.dylib, called by the Unicorn oracles.
build/native-%-probe.dylib: tests/probes/native_$$(subst -,_,$$*)_probe.cpp build/libsadx-core.a
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) $< build/libsadx-core.a -o $@
probes: $(PROBE_TARGETS)

# Generated recipes (remade when a manifest or an oracle script is added/removed).
build/tests.mk build/oracles.mk: tests/unit/manifest.tsv tools/oracles/manifest.tsv tools/gate/gen_make.py $(wildcard tools/oracles/verify_original_*.py) | build
	python3 tools/gate/gen_make.py
manifests: build/tests.mk build/oracles.mk
include build/tests.mk
include build/oracles.mk

run-unit-tests: assets $(UNIT_TARGETS) $(UNIT_MM_TARGETS)
unit-tests: run-unit-tests
test: unit-tests
	$(PY) -m unittest discover -s tests/python -p 'test_*.py'
	python3 tools/gate/check_global_owners.py
# Every oracle; reports in build/oracles/<slug>.json. `make oracles-verdict` reads them (tools/oracles/verdict.py).
oracles: $(ORACLE_TARGETS)
oracles-verdict:
	python3 tools/oracles/verdict.py build/oracles/*.json
# The checkers every gate must pass: layer rules, 0 duplicate owners, 0 UNHANDLED profile patches, repo hygiene.
check:
	python3 tools/gate/check_layers.py
	python3 tools/gate/check_global_owners.py
	python3 tools/gate/check_profile_patches.py
	python3 tools/gate/check_repo_hygiene.py
# The full gate command recorded by tools/gate/record_gate.py (every program built and run, then the checkers).
gate: all test probes $(UNIT_MM_TARGETS) check app-smoke
status:
	python3 tools/gate/gen_status.py
clean:
	rm -rf build
