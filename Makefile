# sadx-macos build. Layout: src/<area>/ (cpu d3dx ninja game player input runtime platform/mac),
# tests/{unit,probes,fixtures,python}, tools/ (+ tools/oracles), third_party/, game/ (ignored inputs),
# reference/ (ignored large inputs + tracked provenance). Add a module by adding its files; add a unit
# test by adding tests/unit/<name>.cpp plus one line in tests/unit/manifest.tsv; add an oracle by
# adding tests/probes/native_<x>_probe.cpp, tools/oracles/verify_original_<x>.py and one manifest line.
CXX := xcrun clang++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Werror -ffp-contract=off -fno-fast-math -arch arm64 -mmacosx-version-min=14.0
MACFLAGS := -fobjc-arc
INCLUDES := -Isrc -Itests/fixtures
GAME_DIR ?= game/Sonic Adventure DX
GAME_EXE ?= $(GAME_DIR)/sonic.exe
GAME_SYSTEM ?= $(GAME_DIR)/system
INPUT_MOD_DLL ?= $(GAME_DIR)/mods/sadx-input-mod/sadx-input-mod.dll
# The pinned "updated speedrun" profile install (reference/updated-pc-reference): the DLLs the oracles pin.
INPUT_DLL ?= reference/updated-pc-reference/game/mods/sadx-input-mod/sadx-input-mod.dll
LOADER_DLL ?= reference/updated-pc-reference/game/mods/.modloader/SADXModLoader.dll
SPEEDRUN_DLL ?= reference/updated-pc-reference/unpacked/speedrun/sadx-speedrun-mod/sadx-speedrun-mod.dll
REFERENCE_PYTHON ?= PYTHONPATH=tools reference/python/bin/python
REFERENCE_ALTERNATE_VARIANT ?= attribute4000
# Unicorn 2.1.4 (the oracle engine) as the in-process guest CPU; the arm64 static library ships in the
# reference venv's wheel. Point UNICORN_DIR at another install (include/ + lib/libunicorn.a) if needed.
UNICORN_DIR ?= $(firstword $(wildcard reference/python/lib/python3.*/site-packages/unicorn))

AREAS := cpu d3dx ninja game player input runtime
CORE_SOURCES := $(filter-out src/runtime/sadx_boot.cpp,$(foreach a,$(AREAS),$(wildcard src/$(a)/*.cpp)))
CORE_OBJECTS := $(patsubst src/%.cpp,build/obj/%.o,$(CORE_SOURCES))
GUEST_SOURCES := $(wildcard src/guest/*.cpp)
GUEST_OBJECTS := $(patsubst src/%.cpp,build/obj/%.o,$(GUEST_SOURCES))
GUEST_LINK := build/libsadx-guest.a $(UNICORN_DIR)/lib/libunicorn.a -lpthread
MAC_OBJECTS := build/obj/platform/mac/mac_input.o build/obj/platform/mac/metal_texture.o build/obj/platform/mac/metal_sprite.o build/obj/platform/mac/mac_clock.o
UNIT_SOURCES := $(wildcard tests/unit/*.cpp)
UNIT_NAMES := $(patsubst tests/unit/%.cpp,%,$(UNIT_SOURCES))
UNIT_TARGETS := $(foreach n,$(UNIT_NAMES),build/$(subst _,-,$(n))-test)
UNIT_MM_TARGETS := build/mac-input-test build/mac-clock-test build/metal-texture-test build/metal-sprite-test build/metal-sprite-composition-test
PROBE_SOURCES := $(wildcard tests/probes/native_*_probe.cpp)
PROBE_NAMES := $(patsubst tests/probes/native_%_probe.cpp,%,$(PROBE_SOURCES))
PROBE_TARGETS := $(foreach n,$(PROBE_NAMES),build/native-$(subst _,-,$(n))-probe.dylib)

SOFTFLOAT_FILES := $(wildcard third_party/softfloat/source/*.c third_party/softfloat/source/include/*.h third_party/softfloat/source/8086/*.c third_party/softfloat/source/8086/*.h) third_party/softfloat/platform.h third_party/softfloat/sources.txt third_party/softfloat/build.sh
SOFTFLOAT_SSE_FILES := $(wildcard third_party/softfloat_sse/source/*.c third_party/softfloat_sse/source/8086-SSE/*) third_party/softfloat_sse/sources.txt third_party/softfloat_sse/build.sh
BOCHS_FILES := $(wildcard third_party/bochs_fpatan/source/fpu/* third_party/bochs_fpatan/source/softfloat3e/*.cc third_party/bochs_fpatan/source/softfloat3e/include/*.h) third_party/bochs_fpatan/math.hpp third_party/bochs_fpatan/config.h third_party/bochs_fpatan/sources.txt third_party/bochs_fpatan/build.sh

.PHONY: all assets test unit-tests probes oracles render controllers check gate clean manifests app-smoke
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
build/libsadx-guest.a: $(GUEST_OBJECTS)
	xcrun libtool -static -o $@ $^
-include $(CORE_OBJECTS:.o=.d) $(MAC_OBJECTS:.o=.d) $(GUEST_OBJECTS:.o=.d)

build/softfloat/softfloat.a: $(SOFTFLOAT_FILES) | build
	sh third_party/softfloat/build.sh build/softfloat
build/softfloat_sse/softfloat-sse.a: $(SOFTFLOAT_SSE_FILES) $(SOFTFLOAT_FILES) | build
	sh third_party/softfloat_sse/build.sh build/softfloat_sse
build/bochs_fpatan/libbochs_fpatan.a: $(BOCHS_FILES) | build
	CXX="$(shell xcrun --find clang++)" sh third_party/bochs_fpatan/build.sh build/bochs_fpatan

build/libsadx-core.a: $(CORE_OBJECTS) build/softfloat_sse/softfloat-sse.a build/softfloat/softfloat.a build/bochs_fpatan/libbochs_fpatan.a
	xcrun libtool -static -o $@ $^
build/libsadx-macos.a: $(MAC_OBJECTS)
	xcrun libtool -static -o $@ $^

build/sadx-boot: src/runtime/sadx_boot.cpp build/libsadx-macos.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-macos.a build/libsadx-core.a -o $@
build/sadx-app: src/platform/mac/sadx_app.mm build/libsadx-macos.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-macos.a build/libsadx-core.a -framework AppKit -framework Metal -framework MetalKit -framework QuartzCore -framework GameController -o $@
# The window smoke test of the roadmap (M2 app-smoke): 120 frames, a screenshot, exit 0.
app-smoke: build/sadx-app
	./build/sadx-app "$(GAME_EXE)" "$(INPUT_MOD_DLL)" --frames 5 --survey --exit-after 120 --screenshot build/app-smoke.ppm > build/app-smoke.log && test -s build/app-smoke.ppm
build/sadx-metal: src/platform/mac/metal_stage.mm Makefile | build
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $< -framework Foundation -framework Metal -framework CoreGraphics -framework GameController -o $@

assets:
	python3 tools/extract.py "$(GAME_EXE)" --output build/assets
render: all assets
	./build/sadx-metal --render build/assets/emerald-coast.mesh build/emerald-coast.ppm
controllers: all
	./build/sadx-metal --controllers

# Unit tests: tests/unit/<name>.cpp -> build/<name-with-dashes>-test, linked against the production archive.
.SECONDEXPANSION:
# Apple make 3.81 picks the first matching pattern rule: the guest rule must precede the generic one.
build/guest-%-test: tests/unit/guest_$$(subst -,_,$$*).cpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I$(UNICORN_DIR)/include $< $(GUEST_LINK) build/libsadx-core.a -o $@
build/%-test: tests/unit/$$(subst -,_,$$*).cpp build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-core.a -o $@
build/mac-input-test: tests/unit/mac_input.mm src/platform/mac/mac_input.mm build/libsadx-core.a | build
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) tests/unit/mac_input.mm src/platform/mac/mac_input.mm build/libsadx-core.a -framework AppKit -framework GameController -o $@
build/mac-clock-test: tests/unit/mac_clock.cpp build/libsadx-macos.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-macos.a build/libsadx-core.a -o $@
build/metal-texture-test: tests/unit/metal_texture.mm build/libsadx-macos.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-macos.a build/libsadx-core.a -framework Foundation -framework Metal -framework CoreGraphics -framework ImageIO -o $@
build/metal-sprite-test: tests/unit/metal_sprite.mm build/libsadx-macos.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-macos.a build/libsadx-core.a -framework Foundation -framework Metal -framework CoreGraphics -framework ImageIO -o $@
build/metal-sprite-composition-test: tests/unit/metal_sprite_composition.mm build/libsadx-macos.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-macos.a build/libsadx-core.a -framework Foundation -framework Metal -framework CoreGraphics -framework ImageIO -o $@
# Fixture headers some tests include.
build/input-test build/mac-input-test: tests/fixtures/input_keyboard_goldens.hpp
build/ninja-queue-consumer-test: tests/fixtures/ninja_sprite_test_fixture.hpp
build/native-player-path-probe.dylib build/native-player-direction-reset-probe.dylib: tests/fixtures/path_probe_data.hpp

# Native probes: tests/probes/native_<x>_probe.cpp -> build/native-<x>-probe.dylib, called by the Unicorn oracles.
build/native-%-probe.dylib: tests/probes/native_$$(subst -,_,$$*)_probe.cpp build/libsadx-core.a
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) $< build/libsadx-core.a -o $@
probes: $(PROBE_TARGETS)

# Generated recipes from the manifests (remade automatically when a manifest changes).
build/tests.mk build/oracles.mk: tests/unit/manifest.tsv tools/oracles/manifest.tsv tools/gen_make.py | build
	python3 tools/gen_make.py
manifests: build/tests.mk build/oracles.mk
include build/tests.mk
include build/oracles.mk

run-unit-tests: assets $(UNIT_TARGETS) $(UNIT_MM_TARGETS)
unit-tests: run-unit-tests
test: unit-tests
	python3 -m unittest discover -s tests/python -p 'test_*.py'
	python3 tools/check_global_owners.py
oracles: $(ORACLE_TARGETS)
# The checkers every gate must pass (0 duplicate owners, 0 UNHANDLED profile patches).
check:
	python3 tools/check_global_owners.py
	python3 tools/check_profile_patches.py
	python3 tools/check_repo_hygiene.py
# The full gate command recorded by tools/record_gate.py.
gate: all test probes build/metal-texture-test build/metal-sprite-test build/metal-sprite-composition-test check
clean:
	rm -rf build
