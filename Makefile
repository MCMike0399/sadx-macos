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
MAC_FRAMEWORKS := -framework Foundation -framework AppKit -framework Metal -framework MetalKit -framework QuartzCore -framework CoreGraphics -framework ImageIO -framework GameController -framework AudioToolbox -framework AVFoundation -framework CoreMedia -framework CoreVideo
NATIVE_CONTROLLER_CMAKE ?= cmake
INCLUDES := -Isrc -Itests/fixtures
GAME_DIR ?= game/Sonic Adventure DX
GAME_EXE ?= $(GAME_DIR)/sonic.exe
game_path_empty :=
game_path_space := $(game_path_empty) $(game_path_empty)
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
# Shipping native closure: maintained game/profile sources only, without loader/guest/HLE objects.
NATIVE_SOURCES := $(sort $(shell cat native-sources.txt) $(wildcard src/engine/*_native.cpp src/runtime/native_*.cpp))
NATIVE_OBJECTS := $(patsubst src/%.cpp,build/obj/%.o,$(NATIVE_SOURCES))
NATIVE_PLATFORM_OBJECTS := build/obj/platform/mac/mac_clock.o build/obj/platform/mac/mac_audio.o \
 build/obj/platform/mac/hid_switch_controller.o build/obj/platform/mac/mac_movie.o build/obj/platform/mac/metal_device_frame.o
NATIVE_PLATFORM_OBJECTS += $(patsubst src/%.mm,build/obj/%.o,$(wildcard src/platform/mac/mac_audio_playback.mm src/platform/mac/mac_sound_bank.mm src/platform/mac/mac_sound_effects.mm))
NATIVE_PLATFORM_OBJECTS += $(patsubst src/%.cpp,build/obj/%.o,$(wildcard src/platform/mac/mac_sound_bank.cpp src/platform/mac/mac_save_files.cpp src/platform/mac/mac_save_writer.cpp))
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

.PHONY: m1 all assets test unit-tests run-unit-tests probes oracles oracles-verdict render controllers check gate clean manifests app-smoke status
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
build/obj/platform/mac/hid_switch_controller.o: CXXFLAGS += -Ibuild/native-controller-deps/include
build/obj/platform/mac/hid_switch_controller.o: build/native-controller-deps/include/SDL3/SDL.h
build/native-controller-deps/include/SDL3/SDL.h: third_party/native_controller/build.py third_party/native_controller/upstream.json third_party/native_controller/shared-iohid.patch
	python3 third_party/native_controller/build.py --cmake "$(NATIVE_CONTROLLER_CMAKE)"
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
build/libsadx-native.a: $(NATIVE_OBJECTS) build/softfloat_sse/softfloat-sse.a build/softfloat/softfloat.a build/bochs_fpatan/libbochs_fpatan.a native-sources.txt
	xcrun libtool -static -o $@ $(filter %.o %.a,$^)

# Programs (src/apps).
build/sadx-boot: src/apps/sadx_boot.cpp build/libsadx-macos.a build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-macos.a build/libsadx-guest.a build/libsadx-core.a -o $@
build/sadx-app: src/apps/sadx_app.mm build/libsadx-macos.a build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) -I$(UNICORN_DIR)/include $< build/libsadx-macos.a $(GUEST_LINK) build/libsadx-core.a $(MAC_FRAMEWORKS) -o $@
build/sadx-metal: src/apps/metal_stage.mm Makefile | build
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $< -framework Foundation -framework Metal -framework CoreGraphics -framework GameController -o $@
build/sadx-audio-qualify: src/apps/audio_qualify.mm build/libsadx-macos.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-macos.a build/libsadx-core.a $(MAC_FRAMEWORKS) -o $@
build/sadx-controller-qualify: src/apps/controller_qualify.mm build/libsadx-macos.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-macos.a build/libsadx-core.a $(MAC_FRAMEWORKS) -o $@
build/sadx-native: src/apps/sadx_native.mm build/libsadx-native.a $(NATIVE_PLATFORM_OBJECTS)
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< $(NATIVE_PLATFORM_OBJECTS) build/libsadx-native.a $(MAC_FRAMEWORKS) -o $@
# Force-load every member: ordinary app linkage can hide a guest-only dependency
# in an archive member the current frontier has not reached yet.
build/sadx-native-closure: src/apps/sadx_native.mm build/libsadx-native.a $(NATIVE_PLATFORM_OBJECTS)
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< $(NATIVE_PLATFORM_OBJECTS) -Wl,-force_load,build/libsadx-native.a $(MAC_FRAMEWORKS) -o $@
.PHONY: native-closure
native-closure: build/sadx-native-closure
	python3 tools/gate/check_native_closure.py --archive build/libsadx-native.a --platform $(NATIVE_PLATFORM_OBJECTS) --sources $(NATIVE_SOURCES) --linked build/sadx-native-closure
NATIVE_ASSETS ?= game/imported
.PHONY: voice-playback-fixture
voice-playback-fixture:
	python3 tools/gate/prepare_voice_playback_fixture.py --assets "$(NATIVE_ASSETS)" --output build/voice-playback-fixture
.PHONY: native-startup-diff
native-startup-diff: build/sadx-native build/sadx-app
	python3 tools/journey/compare_native_startup.py --assets "$(NATIVE_ASSETS)" --game "$(REFERENCE_GAME_DIR)" --windows "$(WINDOWS_SYSTEM_DIR)" --output "$$(mktemp -d build/native-startup-journey.XXXXXX)"
.PHONY: native-journeys
native-journeys: build/sadx-native
	python3 tools/gate/run_native_journeys.py --app build/sadx-native --assets "$(NATIVE_ASSETS)"
.PHONY: play-native
play-native: build/sadx-native
	./build/sadx-native --assets "$(NATIVE_ASSETS)"
# The window smoke test of the roadmap (M2 app-smoke): the guest in the app for 120 frames, a screenshot, exit 0
# (in the gate; it opens a window, so the gate needs a logged-in GUI session).
REFERENCE_GAME_DIR ?= reference/updated-pc-reference/game
WINDOWS_SYSTEM_DIR ?= reference/windows-system
.PHONY: play play-title play-character-select play-world play-help journeys
play-help:
	@echo 'Enter / pad Start: skip the black intro movie and confirm; arrows / left stick: move and select; Esc / pad B: back.'
play: build/sadx-app play-help
	./build/sadx-app --guest "$(REFERENCE_GAME_DIR)" "$(WINDOWS_SYSTEM_DIR)"
play-title: build/sadx-app play-help
	./build/sadx-app --guest "$(REFERENCE_GAME_DIR)" "$(WINDOWS_SYSTEM_DIR)" --keys 450:13
play-character-select: build/sadx-app play-help
	./build/sadx-app --guest "$(REFERENCE_GAME_DIR)" "$(WINDOWS_SYSTEM_DIR)" --keys 450:13,800:13,950:13,1100:13,1250:13,1400:13,1550:13
play-world: build/sadx-app play-help
	@echo 'Auto-navigation to Station Square; keep input neutral until the world appears.'
	./build/sadx-app --guest "$(REFERENCE_GAME_DIR)" "$(WINDOWS_SYSTEM_DIR)" --keys 450:13,800:13,950:13,1100:13,1250:13,1400:13,1550:13,1700:13,1850:13
journeys: build/sadx-app
	python3 tools/gate/run_journeys.py --game-dir "$(REFERENCE_GAME_DIR)" --system-dir "$(WINDOWS_SYSTEM_DIR)"
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
# Address-shaped boot/mode interpreters are reference adapters.
REFERENCE_UNIT_TARGETS := build/music-state-reference-test build/ninja-texture-selection-test build/saved-fov-reference-test build/save-name-reference-test build/library-initial-state-test build/game-library-init-test build/player-scene-position-test build/ninja-fog-initial-state-test build/game-frame-owners-test build/game-view-initial-state-test build/ninja-fog-test build/ninja-queue-consumer-test build/game-view-test \
 build/game-frame-control-test build/reference-frame-control-test build/game-logic-test build/game-logic-support-test build/game-loop-test build/game-startup-test build/ninja-queue-lifecycle-test \
 build/game-mode-test build/game-mode-leaves-test build/game-runtime-test \
 build/player-acceleration-ground-test build/player-acceleration-alternate-test \
 build/player-auxiliary-effect-test build/player-direction-reset-test \
 build/input-constants-boot-test build/startup-initial-boot-test build/library-initial-boot-test
$(REFERENCE_UNIT_TARGETS): build/%-test: tests/unit/$$(subst -,_,$$*).cpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(EXTRA_REFERENCE_SOURCES) build/libsadx-guest.a build/libsadx-core.a -o $@
build/game-runtime-test: EXTRA_REFERENCE_SOURCES := tests/probes/native_session_control_defaults_probe.cpp tests/probes/native_input_initial_state_probe.cpp tests/probes/native_ninja_sine_table_probe.cpp tests/probes/native_ninja_fog_initial_state_probe.cpp tests/probes/native_ninja_screen_initial_state_probe.cpp tests/probes/native_sound_initial_state_probe.cpp tests/probes/native_game_view_initial_state_probe.cpp tests/probes/native_ninja_queue_initial_state_probe.cpp
build/game-runtime-test: tests/probes/native_session_control_defaults_probe.cpp tests/probes/session_control_probe.hpp tests/probes/native_input_initial_state_probe.cpp tests/probes/input_initial_state_probe.hpp tests/probes/native_ninja_sine_table_probe.cpp tests/probes/ninja_sine_table_probe.hpp tests/probes/native_ninja_fog_initial_state_probe.cpp tests/probes/ninja_fog_initial_state_probe.hpp tests/probes/native_ninja_screen_initial_state_probe.cpp tests/probes/ninja_screen_initial_state_probe.hpp tests/probes/native_sound_initial_state_probe.cpp tests/probes/sound_initial_state_probe.hpp tests/probes/native_game_view_initial_state_probe.cpp tests/probes/game_view_initial_state_probe.hpp tests/probes/native_ninja_queue_initial_state_probe.cpp tests/probes/ninja_queue_initial_state_probe.hpp
build/input-constants-boot-test: EXTRA_REFERENCE_SOURCES := tests/probes/native_input_mod_constants_probe.cpp
build/input-constants-boot-test: tests/probes/native_input_mod_constants_probe.cpp tests/probes/input_mod_constants_probe.hpp
build/startup-initial-boot-test: EXTRA_REFERENCE_SOURCES := tests/probes/native_startup_initial_state_probe.cpp
build/startup-initial-boot-test: tests/probes/native_startup_initial_state_probe.cpp tests/probes/startup_initial_state_probe.hpp
build/library-initial-boot-test: EXTRA_REFERENCE_SOURCES := tests/probes/native_library_initial_state_probe.cpp
build/library-initial-boot-test: tests/probes/native_library_initial_state_probe.cpp tests/probes/library_initial_state_probe.hpp
# Native frame-control composition deliberately excludes the reference/core archive.
build/native-frame-control-test: tests/unit/native_frame_control.cpp src/engine/game_frame_control.cpp src/engine/game_frame_control.hpp src/engine/game_frame_control_internal.hpp src/engine/game_frame_control_native.cpp src/engine/speedrun.cpp src/engine/speedrun.hpp
	$(CXX) $(CXXFLAGS) -Isrc tests/unit/native_frame_control.cpp src/engine/game_frame_control.cpp src/engine/game_frame_control_native.cpp src/engine/speedrun.cpp -o $@
# The semantic mode core and its native seams build without reference archives or PE inputs.
build/native-game-mode-test: tests/unit/native_game_mode.cpp src/game/game_mode.cpp src/game/game_mode.hpp src/game/game_mode_internal.hpp src/game/game_mode_native.cpp
	$(CXX) $(CXXFLAGS) -Isrc tests/unit/native_game_mode.cpp src/game/game_mode.cpp src/game/game_mode_native.cpp -o $@
# Apple make 3.81 picks the first matching pattern rule: the guest rule must precede the generic one.
build/guest-%-test: tests/unit/guest_$$(subst -,_,$$*).cpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I$(UNICORN_DIR)/include $< $(GUEST_LINK) build/libsadx-core.a -o $@
# HLE module tests use the same guest CPU/import boundary as guest-* tests.
build/hle-%-test: tests/unit/hle_$$(subst -,_,$$*).cpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I$(UNICORN_DIR)/include $< $(GUEST_LINK) build/libsadx-core.a -o $@
# Tests named mac-* / metal-* exercise src/platform/mac and link that archive too.
build/mac-%-test: tests/unit/mac_$$(subst -,_,$$*).cpp build/libsadx-macos.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-macos.a build/libsadx-core.a $(MAC_FRAMEWORKS) -o $@
build/metal-%-test: tests/unit/metal_$$(subst -,_,$$*).cpp build/libsadx-macos.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-macos.a build/libsadx-core.a $(MAC_FRAMEWORKS) -o $@
# Legacy startup fixtures use their reference adapter; the product uses the new native interface.
build/player-startup-test: tests/unit/player_startup.cpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-guest.a build/libsadx-core.a -o $@
# Legacy animation fixtures and PE import use their registered reference adapters.
REFERENCE_ANIMATION_UNITS := build/player-animation-decode-test build/player-animation-test build/sonic-animation-table-test build/animation-characterization-test
$(REFERENCE_ANIMATION_UNITS): build/%-test: tests/unit/$$(subst -,_,$$*).cpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-guest.a build/libsadx-core.a -o $@
build/%-test: tests/unit/$$(subst -,_,$$*).cpp build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-core.a -o $@
build/%-test: tests/unit/$$(subst -,_,$$*).mm build/libsadx-macos.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-macos.a build/libsadx-core.a $(MAC_FRAMEWORKS) -o $@
# Queue composition calls reference fog diagnostics; its native renderer stays in libsadx-macos.
build/metal-sprite-composition-test: tests/unit/metal_sprite_composition.mm build/libsadx-macos.a build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-macos.a build/libsadx-guest.a build/libsadx-core.a $(MAC_FRAMEWORKS) -o $@
# Fixture headers some tests include.
build/fidelity-float-test: src/cpu/fidelity_float.hpp
build/input-test build/mac-input-test: tests/fixtures/input_keyboard_goldens.hpp
build/ninja-queue-consumer-test: tests/fixtures/ninja_sprite_test_fixture.hpp
build/native-player-path-probe.dylib build/native-player-direction-reset-probe.dylib: tests/fixtures/path_probe_data.hpp

# Native probes: tests/probes/native_<x>_probe.cpp -> build/native-<x>-probe.dylib, called by the Unicorn oracles.
REFERENCE_PLAYER_PROBES := build/native-player-animation-probe.dylib build/native-player-animation-blend-probe.dylib build/native-sonic-animation-table-probe.dylib build/native-animation-table-characterization-probe.dylib build/native-game-reset-composed-probe.dylib build/native-ninja-texture-selection-probe.dylib build/native-ninja-sprite-display-probe.dylib build/native-library-init-probe.dylib build/native-player-scene-helpers-probe.dylib build/native-player-scene-position-probe.dylib build/native-fov-probe.dylib build/native-ninja-queue-color-probe.dylib build/native-game-frame-owners-probe.dylib build/native-fog-probe.dylib build/native-ninja-queue-consumer-probe.dylib build/native-game-view-probe.dylib  build/native-game-frame-control-probe.dylib build/native-speedrun-limiter-probe.dylib build/native-game-startup-probe.dylib build/native-game-logic-support-probe.dylib build/native-game-logic-probe.dylib build/native-game-loop-probe.dylib build/native-frame-completion-probe.dylib build/native-ninja-queue-lifecycle-probe.dylib build/native-ground-acceleration-probe.dylib build/native-alternate-acceleration-probe.dylib build/native-player-startup-probe.dylib
$(REFERENCE_PLAYER_PROBES): build/native-%-probe.dylib: tests/probes/native_$$(subst -,_,$$*)_probe.cpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) $< build/libsadx-guest.a build/libsadx-core.a -o $@

build/native-game-mode-probe.dylib: tests/probes/native_game_mode_probe.cpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) $< build/libsadx-guest.a build/libsadx-core.a -o $@
build/native-game-mode-leaves-probe.dylib: tests/probes/native_game_mode_leaves_probe.cpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) $< build/libsadx-guest.a build/libsadx-core.a -o $@
# This native code-table probe needs only maintained table source.
build/native-engine-code-tables-probe.dylib: tests/probes/native_engine_code_tables_probe.cpp src/runtime/engine_code_tables.cpp src/runtime/engine_code_tables.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib -Isrc tests/probes/native_engine_code_tables_probe.cpp src/runtime/engine_code_tables.cpp -o $@
build/native-input-mod-constants-probe.dylib: tests/probes/native_input_mod_constants_probe.cpp tests/probes/input_mod_constants_probe.hpp src/input/input_mod_constants.cpp src/input/input_mod_constants.hpp src/input/input_mod_pad.hpp src/input/input_mod_axes.hpp src/input/input.hpp src/engine/game_controller.hpp src/cpu/sse_math.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib -Isrc tests/probes/native_input_mod_constants_probe.cpp src/input/input_mod_constants.cpp -o $@
build/native-ninja-queue-initial-state-probe.dylib: tests/probes/native_ninja_queue_initial_state_probe.cpp tests/probes/ninja_queue_initial_state_probe.hpp src/engine/ninja_queue_initial_state.cpp src/engine/ninja_queue_initial_state.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib -Isrc tests/probes/native_ninja_queue_initial_state_probe.cpp src/engine/ninja_queue_initial_state.cpp -o $@
build/native-game-view-initial-state-probe.dylib: tests/probes/native_game_view_initial_state_probe.cpp tests/probes/game_view_initial_state_probe.hpp src/game/game_view_initial_state.cpp src/game/game_view_initial_state.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib -Isrc tests/probes/native_game_view_initial_state_probe.cpp src/game/game_view_initial_state.cpp -o $@
build/native-sound-initial-state-probe.dylib: tests/probes/native_sound_initial_state_probe.cpp tests/probes/sound_initial_state_probe.hpp src/game/sound_initial_state.cpp src/game/sound_initial_state.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib -Isrc tests/probes/native_sound_initial_state_probe.cpp src/game/sound_initial_state.cpp -o $@
build/native-ninja-screen-initial-state-probe.dylib: tests/probes/native_ninja_screen_initial_state_probe.cpp tests/probes/ninja_screen_initial_state_probe.hpp src/engine/ninja_screen_initial_state.cpp src/engine/ninja_screen_initial_state.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib -Isrc tests/probes/native_ninja_screen_initial_state_probe.cpp src/engine/ninja_screen_initial_state.cpp -o $@
build/native-ninja-fog-initial-state-probe.dylib: tests/probes/native_ninja_fog_initial_state_probe.cpp tests/probes/ninja_fog_initial_state_probe.hpp src/engine/ninja_fog_initial_state.cpp src/engine/ninja_fog_initial_state.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib -Isrc tests/probes/native_ninja_fog_initial_state_probe.cpp src/engine/ninja_fog_initial_state.cpp -o $@
build/native-ninja-sine-table-probe.dylib: tests/probes/native_ninja_sine_table_probe.cpp tests/probes/ninja_sine_table_probe.hpp src/math/ninja_sine_table.cpp src/math/ninja_sine_table.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib -Isrc tests/probes/native_ninja_sine_table_probe.cpp src/math/ninja_sine_table.cpp -o $@
build/native-input-initial-state-probe.dylib: tests/probes/native_input_initial_state_probe.cpp tests/probes/input_initial_state_probe.hpp src/input/input_initial_state.cpp src/input/input_initial_state.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib -Isrc tests/probes/native_input_initial_state_probe.cpp src/input/input_initial_state.cpp -o $@
build/native-session-control-defaults-probe.dylib: tests/probes/native_session_control_defaults_probe.cpp tests/probes/session_control_probe.hpp src/runtime/session_control_defaults.cpp src/runtime/session_control_defaults.hpp src/engine/game_frame_control.hpp src/game/game_mode.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib -Isrc tests/probes/native_session_control_defaults_probe.cpp src/runtime/session_control_defaults.cpp -o $@
build/native-%-probe.dylib: tests/probes/native_$$(subst -,_,$$*)_probe.cpp build/libsadx-core.a
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) $< build/libsadx-core.a -o $@
probes: $(PROBE_TARGETS)

# Generated recipes (remade when a manifest or an oracle script is added/removed).
build/tests.mk build/oracles.mk: tests/unit/manifest.tsv tools/oracles/manifest.tsv tools/gate/gen_make.py $(wildcard tools/oracles/verify_original_*.py) | build
	python3 tools/gate/gen_make.py
manifests: build/tests.mk build/oracles.mk
include build/tests.mk
include build/oracles.mk

# guest-overrides replays these oracle corpora (each verify script writes its <slug>.cases.bin next to the report).
OVERRIDE_CORPORA := build/oracles/screen_polygon.cases.bin build/oracles/screen_init.cases.bin build/oracles/collision_shapes.cases.bin build/oracles/controller_init.cases.bin build/oracles/game_startup.cases.bin build/oracles/ninja_queue_lifecycle.cases.bin build/oracles/task_lifecycle.cases.bin build/oracles/task_work.cases.bin build/oracles/ninja_memory.cases.bin build/oracles/fog.cases.bin build/oracles/ninja_render.cases.bin build/oracles/game_frame_control.cases.bin build/oracles/projection_range.cases.bin build/oracles/fov.cases.bin build/oracles/ninja_view.cases.bin build/oracles/d3dx_perspective.cases.bin build/oracles/d3dx_matrix_inverse.cases.bin build/oracles/d3dx_project.cases.bin build/oracles/ninja_queue_color.cases.bin build/oracles/ninja_projected_depth.cases.bin
build/oracles/%.cases.bin: tools/oracles/verify_original_%.py build/native-$$(subst _,-,$$*)-probe.dylib
	$(MAKE) oracle-$(subst _,-,$*)
# A poisoned reference image proves explicit native code tables do not reread PE bytes.
build/engine-code-tables-poisoned.exe: $(subst $(game_path_space),\$(game_path_space),$(GAME_EXE)) tools/oracles/verify_original_engine_code_tables.py build/native-engine-code-tables-probe.dylib
	$(REFERENCE_PYTHON) tools/oracles/verify_original_engine_code_tables.py "$(GAME_EXE)" build/native-engine-code-tables-probe.dylib --output build/oracles/engine_code_tables.json --poison-output $@

# Reader fixtures only: original virtual-zero storage materialized for control injections.
.PHONY: session-control-fixtures
session-control-fixtures: $(subst $(game_path_space),\$(game_path_space),$(GAME_EXE)) tools/oracles/verify_original_session_control_defaults.py build/native-session-control-defaults-probe.dylib
	$(REFERENCE_PYTHON) tools/oracles/verify_original_session_control_defaults.py "$(GAME_EXE)" build/native-session-control-defaults-probe.dylib --output build/oracles/session_control_defaults.json --poison-output build/session-control-poisoned.exe --varied-output build/session-control-varied.exe --partial-output build/session-control-partial.exe

.PHONY: input-initial-fixtures
input-initial-fixtures: build/native-input-initial-state-probe.dylib
	$(REFERENCE_PYTHON) tools/oracles/verify_original_input_initial_state.py "$(GAME_EXE)" "$(INPUT_MOD_DLL)" build/native-input-initial-state-probe.dylib --output build/oracles/input_initial_state.json --fixture-prefix build/input-initial

.PHONY: sine-initial-fixtures
sine-initial-fixtures: build/native-ninja-sine-table-probe.dylib
	$(REFERENCE_PYTHON) tools/oracles/verify_original_ninja_sine_table.py "$(GAME_EXE)" build/native-ninja-sine-table-probe.dylib --output build/oracles/ninja_sine_table.json --fixture-prefix build/sine-initial
	$(REFERENCE_PYTHON) tools/emit/emit_ninja_sine_table.py "$(GAME_EXE)" --check

.PHONY: fog-initial-fixtures
fog-initial-fixtures: build/native-ninja-fog-initial-state-probe.dylib
	$(REFERENCE_PYTHON) tools/oracles/verify_original_ninja_fog_initial_state.py "$(GAME_EXE)" build/native-ninja-fog-initial-state-probe.dylib --output build/oracles/ninja_fog_initial_state.json --poison-output build/fog-initial-poisoned.exe --varied-output build/fog-initial-varied.exe --partial-output build/fog-initial-partial.exe

.PHONY: input-constants-fixtures
input-constants-fixtures: build/native-input-mod-constants-probe.dylib
	$(REFERENCE_PYTHON) tools/oracles/verify_original_input_mod_constants.py "$(INPUT_MOD_DLL)" build/native-input-mod-constants-probe.dylib --output build/oracles/input_mod_constants.json --poison-output build/input-constants-poisoned.dll --varied-output build/input-constants-varied.dll --partial-output build/input-constants-partial.dll

.PHONY: queue-initial-fixtures
queue-initial-fixtures: build/native-ninja-queue-initial-state-probe.dylib
	$(REFERENCE_PYTHON) tools/oracles/verify_original_ninja_queue_initial_state.py "$(GAME_EXE)" build/native-ninja-queue-initial-state-probe.dylib --output build/oracles/ninja_queue_initial_state.json --poison-output build/queue-initial-poisoned.exe --varied-output build/queue-initial-varied.exe --partial-output build/queue-initial-partial.exe

.PHONY: view-initial-fixtures
view-initial-fixtures: build/native-game-view-initial-state-probe.dylib
	$(REFERENCE_PYTHON) tools/oracles/verify_original_game_view_initial_state.py "$(GAME_EXE)" build/native-game-view-initial-state-probe.dylib --output build/oracles/game_view_initial_state.json --poison-output build/view-initial-poisoned.exe --varied-output build/view-initial-varied.exe --partial-output build/view-initial-partial.exe

.PHONY: sound-initial-fixtures
sound-initial-fixtures: build/native-sound-initial-state-probe.dylib
	$(REFERENCE_PYTHON) tools/oracles/verify_original_sound_initial_state.py "$(GAME_EXE)" build/native-sound-initial-state-probe.dylib --output build/oracles/sound_initial_state.json --poison-output build/sound-initial-poisoned.exe --varied-output build/sound-initial-varied.exe --partial-output build/sound-initial-partial.exe

.PHONY: screen-initial-fixtures
screen-initial-fixtures: build/native-ninja-screen-initial-state-probe.dylib
	$(REFERENCE_PYTHON) tools/oracles/verify_original_ninja_screen_initial_state.py "$(GAME_EXE)" build/native-ninja-screen-initial-state-probe.dylib --output build/oracles/ninja_screen_initial_state.json --poison-output build/screen-initial-poisoned.exe --varied-output build/screen-initial-varied.exe --partial-output build/screen-initial-partial.exe

run-unit-tests: voice-playback-fixture library-initial-fixtures startup-initial-fixtures assets $(UNIT_TARGETS) $(UNIT_MM_TARGETS) $(OVERRIDE_CORPORA) build/engine-code-tables-poisoned.exe session-control-fixtures input-initial-fixtures sine-initial-fixtures fog-initial-fixtures screen-initial-fixtures sound-initial-fixtures view-initial-fixtures queue-initial-fixtures input-constants-fixtures
unit-tests: run-unit-tests
test: unit-tests
	$(REFERENCE_PYTHON) -m unittest discover -s tests/python -p 'test_*.py'
	python3 tools/gate/check_global_owners.py
# Every oracle; reports in build/oracles/<slug>.json. `make oracles-verdict` reads them (tools/oracles/verdict.py).
oracles: $(ORACLE_TARGETS)
oracles-verdict:
	python3 tools/oracles/verdict.py build/oracles/*.json
# The checkers every gate must pass: layer rules, 0 duplicate owners, 0 UNHANDLED profile patches, repo hygiene.
symbols:
	python3 tools/symbols/build_symbol_db.py
.PHONY: symbols check-symbols
check-symbols: symbols
	python3 tools/symbols/build_symbol_db.py --check
.PHONY: native-frame-control
native-frame-control:
	python3 tools/gate/check_native_frame_control.py
.PHONY: native-engine-code-tables
native-engine-code-tables:
	python3 tools/gate/check_engine_code_tables.py
.PHONY: native-session-control-defaults
native-session-control-defaults:
	python3 tools/gate/check_session_control_defaults.py
.PHONY: native-input-initial-state
native-input-initial-state:
	python3 tools/gate/check_input_initial_state.py
.PHONY: native-ninja-sine-table
native-ninja-sine-table:
	python3 tools/gate/check_ninja_sine_table.py
.PHONY: native-ninja-fog-initial-state
native-ninja-fog-initial-state:
	python3 tools/gate/check_ninja_fog_initial_state.py
.PHONY: native-ninja-screen-initial-state
native-ninja-screen-initial-state:
	python3 tools/gate/check_ninja_screen_initial_state.py
.PHONY: native-sound-initial-state
native-sound-initial-state:
	python3 tools/gate/check_sound_initial_state.py
.PHONY: native-game-view-initial-state
native-game-view-initial-state:
	python3 tools/gate/check_game_view_initial_state.py
.PHONY: native-ninja-queue-initial-state
native-ninja-queue-initial-state:
	python3 tools/gate/check_ninja_queue_initial_state.py
.PHONY: native-input-mod-constants
native-input-mod-constants:
	python3 tools/gate/check_input_mod_constants.py
check: native-library-initial-state native-startup-initial-state native-input-mod-constants native-ninja-queue-initial-state native-game-view-initial-state native-sound-initial-state check-symbols native-frame-control native-engine-code-tables native-session-control-defaults native-input-initial-state native-ninja-sine-table native-ninja-fog-initial-state native-ninja-screen-initial-state
	python3 tools/gate/check_layers.py
	python3 tools/gate/check_global_owners.py
	python3 tools/gate/check_provenance.py
	python3 tools/gate/check_readability.py
	python3 tools/gate/check_constexpr_tables.py
	python3 tools/gate/check_address_dispatch.py
	python3 tools/gate/check_judge.py
	python3 tools/gate/check_profile_patches.py
	python3 tools/gate/check_repo_hygiene.py
# The full gate command recorded by tools/gate/record_gate.py (every program built and run, then the checkers).
gate: all test probes $(UNIT_MM_TARGETS) check app-smoke build/sadx-native native-closure
	$(MAKE) journeys
	$(MAKE) native-journeys
	$(MAKE) native-startup-diff
status:
	python3 tools/gate/gen_status.py
clean:
	rm -rf build

# The roadmap's M1 tests (docs/ROADMAP.md), nothing else: guest-imports, guest-dlls (profile-applied at mod init),
# guest-boot (profile-applied, live overrides), guest-overrides + HLE bridges (override-equivalence), guest-fpu-shims, guest-frames
# (SEGA logo -> title, determinism, guest-fps), guest-input (input-script determinism), guest-timing (profile cadence) and app-smoke.
M1_TESTS := hle-screen-init-bridge guest-imports guest-dlls guest-boot guest-overrides hle-device-bridge hle-heap-bridge hle-task-bridge hle-queue-bridge guest-fpu-shims guest-frames guest-input guest-timing guest-controller
m1: $(foreach t,$(M1_TESTS),build/$(t)-test) $(OVERRIDE_CORPORA) app-smoke
	./build/hle-screen-init-bridge-test "$(GAME_EXE)" build/oracles/screen_init.cases.bin
	./build/guest-imports-test "$(GAME_EXE)"
	./build/guest-dlls-test $(REFERENCE_GAME_DIR) $(WINDOWS_SYSTEM_DIR)
	./build/guest-boot-test $(REFERENCE_GAME_DIR) $(WINDOWS_SYSTEM_DIR)
	./build/guest-overrides-test "$(GAME_EXE)" build/oracles
	./build/hle-device-bridge-test "$(GAME_EXE)" build/oracles
	./build/hle-heap-bridge-test "$(GAME_EXE)" build/oracles/ninja_memory.cases.bin
	./build/hle-task-bridge-test "$(GAME_EXE)" build/oracles/task_work.cases.bin build/oracles/task_lifecycle.cases.bin
	./build/hle-queue-bridge-test "$(GAME_EXE)" build/oracles/ninja_queue_lifecycle.cases.bin
	./build/guest-fpu-shims-test "$(GAME_EXE)" reference/inventory/fpu-sites.json
	./build/guest-frames-test $(REFERENCE_GAME_DIR) $(WINDOWS_SYSTEM_DIR) 60
	./build/guest-input-test $(REFERENCE_GAME_DIR) $(WINDOWS_SYSTEM_DIR)
	./build/guest-timing-test $(REFERENCE_GAME_DIR) $(WINDOWS_SYSTEM_DIR)
	./build/guest-controller-test $(REFERENCE_GAME_DIR) $(WINDOWS_SYSTEM_DIR)

build/native-startup-initial-state-probe.dylib: tests/probes/native_startup_initial_state_probe.cpp tests/probes/startup_initial_state_probe.hpp src/game/startup_initial_state.cpp src/game/startup_initial_state.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib -Isrc tests/probes/native_startup_initial_state_probe.cpp src/game/startup_initial_state.cpp -o $@
.PHONY: startup-initial-fixtures native-startup-initial-state
startup-initial-fixtures: build/native-startup-initial-state-probe.dylib
	$(REFERENCE_PYTHON) tools/oracles/verify_original_startup_initial_state.py "$(GAME_EXE)" build/native-startup-initial-state-probe.dylib --output build/oracles/startup_initial_state.json --poison-output build/startup-initial-poisoned.exe --varied-output build/startup-initial-varied.exe --partial-output build/startup-initial-partial.exe
native-startup-initial-state:
	python3 tools/gate/check_startup_initial_state.py

build/native-library-initial-state-probe.dylib: tests/probes/native_library_initial_state_probe.cpp tests/probes/library_initial_state_probe.hpp src/game/library_initial_state.cpp src/game/library_initial_state.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib -Isrc tests/probes/native_library_initial_state_probe.cpp src/game/library_initial_state.cpp -o $@
.PHONY: library-initial-fixtures native-library-initial-state
library-initial-fixtures: build/native-library-initial-state-probe.dylib
	$(REFERENCE_PYTHON) tools/oracles/verify_original_library_initial_state.py "$(GAME_EXE)" build/native-library-initial-state-probe.dylib --output build/oracles/library_initial_state.json --poison-output build/library-initial-poisoned.exe --varied-output build/library-initial-varied.exe --partial-output build/library-initial-partial.exe
native-library-initial-state:
	python3 tools/gate/check_library_initial_state.py

build/mac-switch-backend-test: tests/unit/mac_switch_backend.mm build/libsadx-macos.a build/libsadx-core.a build/native-controller-deps/include/SDL3/SDL.h
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) -Ibuild/native-controller-deps/include $< build/libsadx-macos.a build/libsadx-core.a $(MAC_FRAMEWORKS) -o $@

# Native media decoder qualification; source is introduced by the following movie integration.
build/sadx-movie-qualify: src/apps/movie_qualify.mm build/libsadx-macos.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-macos.a build/libsadx-core.a $(MAC_FRAMEWORKS) -o $@

build/guest-draw-history-test: tests/unit/guest_draw_history.cpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(GUEST_LINK) build/libsadx-core.a -o $@

build/native-game-view-native-probe.dylib: tests/probes/native_game_view_native_probe.cpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) $< build/libsadx-guest.a build/libsadx-core.a -o $@

build/native-load-selected-save-probe.dylib: tests/probes/native_load_selected_save_probe.cpp tests/probes/native_save_file_list_probe.cpp build/libsadx-core.a
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) $< build/libsadx-core.a -o $@

# Save writer filesystem qualification links the maintained backend directly.
build/save-write-test: tests/unit/save_write.cpp src/platform/mac/mac_save_writer.cpp build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) tests/unit/save_write.cpp src/platform/mac/mac_save_writer.cpp build/libsadx-core.a -o $@

build/object-draw-test: tests/unit/object_draw.cpp tests/probes/object_draw_compat.cpp tests/probes/object_draw_compat.hpp build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -include tests/probes/object_draw_compat.hpp $< tests/probes/object_draw_compat.cpp build/libsadx-core.a -o $@
build/native-object-draw-probe.dylib: tests/probes/native_object_draw_probe.cpp tests/probes/object_draw_compat.cpp tests/probes/object_draw_compat.hpp build/libsadx-core.a
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) -include tests/probes/object_draw_compat.hpp $< tests/probes/object_draw_compat.cpp build/libsadx-core.a -o $@

# Original normal/UV strip packet: isolated source closure plus link-time device receipts.
build/normal-uv-strip-test: tests/unit/normal_uv_strip.cpp tests/probes/native_normal_uv_strip_probe.cpp tests/probes/normal_uv_strip_probe.hpp src/engine/normal_uv_strip.cpp src/engine/normal_uv_strip.hpp src/cpu/x87_math.cpp build/softfloat/softfloat.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -Itests/probes tests/unit/normal_uv_strip.cpp tests/probes/native_normal_uv_strip_probe.cpp src/engine/normal_uv_strip.cpp src/cpu/x87_math.cpp build/softfloat/softfloat.a -o $@
build/native-normal-uv-strip-probe.dylib: tests/probes/native_normal_uv_strip_probe.cpp tests/probes/normal_uv_strip_probe.hpp src/engine/normal_uv_strip.cpp src/engine/normal_uv_strip.hpp src/cpu/x87_math.cpp build/softfloat/softfloat.a
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) -Itests/probes tests/probes/native_normal_uv_strip_probe.cpp src/engine/normal_uv_strip.cpp src/cpu/x87_math.cpp build/softfloat/softfloat.a -o $@

# The concrete material probe supplies link-time device receipts.
build/native-material-correction-probe.dylib: tests/probes/native_material_correction_probe.cpp src/engine/material_correction.cpp src/engine/material_correction.hpp build/libsadx-core.a
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) tests/probes/native_material_correction_probe.cpp src/engine/material_correction.cpp build/libsadx-core.a -o $@
build/material-correction-test: tests/unit/material_correction.cpp src/engine/material_correction.cpp src/engine/material_correction.hpp build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) tests/unit/material_correction.cpp src/engine/material_correction.cpp build/libsadx-core.a -o $@

# Only frozen reference fixture translation units receive the compatibility definitions.
POLYBUFF_REFERENCE_UNITS := build/ninja-system-init-test build/ninja-device-init-test build/ninja-frame-calls-test build/ninja-screen-test build/ninja-view-test build/polybuff-ownership-test
$(POLYBUFF_REFERENCE_UNITS): build/%-test: tests/unit/$$(subst -,_,$$*).cpp src/guest/polybuff_owners.hpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -include guest/polybuff_owners.hpp $< build/libsadx-guest.a build/libsadx-core.a -o $@
POLYBUFF_REFERENCE_PROBES := build/native-system-init-probe.dylib build/native-device-setup-probe.dylib build/native-ninja-frame-calls-probe.dylib build/native-screen-init-probe.dylib build/native-screen-polygon-probe.dylib build/native-ninja-projected-depth-probe.dylib build/native-ninja-view-probe.dylib
$(POLYBUFF_REFERENCE_PROBES): build/native-%-probe.dylib: tests/probes/native_$$(subst -,_,$$*)_probe.cpp src/guest/polybuff_owners.hpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) -include guest/polybuff_owners.hpp $< build/libsadx-guest.a build/libsadx-core.a -o $@
# Standalone native initialization receipts never link runtime/reference Context definitions.
build/native-polybuff-initialization-probe.dylib: tests/probes/native_polybuff_initialization_probe.cpp src/engine/polybuff_initialization.cpp src/engine/polybuff_initialization.hpp src/engine/normal_uv_strip.cpp src/cpu/x87_math.cpp build/softfloat/softfloat.a
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) tests/probes/native_polybuff_initialization_probe.cpp src/engine/polybuff_initialization.cpp src/engine/normal_uv_strip.cpp src/cpu/x87_math.cpp build/softfloat/softfloat.a -o $@
build/polybuff-initialization-test: tests/unit/polybuff_initialization.cpp tests/probes/native_polybuff_initialization_probe.cpp src/engine/polybuff_initialization.cpp src/engine/polybuff_initialization_binding.cpp src/engine/polybuff_initialization.hpp src/engine/normal_uv_strip.cpp src/cpu/x87_math.cpp build/softfloat/softfloat.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -DPOLYBUFF_INITIALIZATION_NATIVE_BINDING tests/unit/polybuff_initialization.cpp tests/probes/native_polybuff_initialization_probe.cpp src/engine/polybuff_initialization.cpp src/engine/polybuff_initialization_binding.cpp src/engine/normal_uv_strip.cpp src/cpu/x87_math.cpp build/softfloat/softfloat.a -o $@

# Frozen current-publication fixtures receive observation syntax only in their own TU.
CURRENT_REFERENCE_UNITS := build/sonic-player-test build/sonic-actions-test build/player-scene-position-binding-test
$(CURRENT_REFERENCE_UNITS): build/%-test: tests/unit/$$(subst -,_,$$*).cpp tests/reference/actor_current_fixture.hpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -include tests/reference/actor_current_fixture.hpp $< build/libsadx-guest.a build/libsadx-core.a -o $@
CURRENT_REFERENCE_PROBES := build/native-sonic-player-probe.dylib build/native-sonic-action-probe.dylib
$(CURRENT_REFERENCE_PROBES): build/native-%-probe.dylib: tests/probes/native_$$(subst -,_,$$*)_probe.cpp tests/reference/actor_current_fixture.hpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) -include tests/reference/actor_current_fixture.hpp $< build/libsadx-guest.a build/libsadx-core.a -o $@
build/actor-current-test: tests/unit/actor_current.cpp src/player/actor_current.cpp src/player/actor_current.hpp src/engine/task_work.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) tests/unit/actor_current.cpp src/player/actor_current.cpp src/engine/task_work.cpp -o $@

# Bounded actual Sonic updater/startup composition and imported Gamma ownership.
build/animation-sonic-composition-test: tests/unit/animation_sonic_composition.cpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -DANIMATION_TYPED $< build/libsadx-guest.a build/libsadx-core.a -o $@
build/animation-gamma-shared-binding-test: tests/unit/animation_gamma_shared_binding.mm src/apps/native_gamma_actor_assets.hpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-native.a -framework Foundation -o $@

# Native Gamma binding test uses actual imported metadata and the shipping closure.
build/native-gamma-binding-test: tests/unit/native_gamma_binding.mm src/apps/native_gamma_actor_assets.hpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-native.a -framework Foundation -o $@

# Native bootstrap predicates, isolated from reference runtime and original binaries.
build/native-selection-player-queries-probe.dylib: tests/probes/native_selection_player_queries_probe.cpp src/player/selection_player_queries.cpp src/player/selection_player_queries.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) tests/probes/native_selection_player_queries_probe.cpp src/player/selection_player_queries.cpp -o $@
build/selection-player-queries-test: tests/unit/selection_player_queries.cpp src/player/selection_player_queries.cpp src/player/selection_player_queries.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) tests/unit/selection_player_queries.cpp src/player/selection_player_queries.cpp -o $@

# Ordinary character texture wrapper: isolate the original receipt probe and real lower-loader unit.
build/character-textures-test: tests/unit/character_textures.cpp src/game/character_textures.cpp src/game/character_textures.hpp src/game/regular_textures.cpp src/game/regular_textures.hpp
	$(CXX) $(CXXFLAGS) -Isrc tests/unit/character_textures.cpp src/game/character_textures.cpp src/game/regular_textures.cpp -o $@
build/native-character-textures-probe.dylib: tests/probes/native_character_textures_probe.cpp src/game/character_textures.cpp src/game/character_textures.hpp src/game/regular_textures.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib -Isrc tests/probes/native_character_textures_probe.cpp src/game/character_textures.cpp -o $@

# Exact cursor expansion: native arena and raw-copy slice without runtime/reference adapters.
build/gamma-cursors-test: tests/unit/gamma_cursors.cpp src/player/gamma_cursors.cpp src/player/gamma_cursors.hpp src/engine/task_work.cpp src/engine/task_work.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) tests/unit/gamma_cursors.cpp src/player/gamma_cursors.cpp src/engine/task_work.cpp -o $@
build/native-gamma-cursors-probe.dylib: tests/probes/native_gamma_cursors_probe.cpp src/player/gamma_cursors.cpp src/player/gamma_cursors.hpp src/engine/task_work.cpp src/engine/task_work.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) tests/probes/native_gamma_cursors_probe.cpp src/player/gamma_cursors.cpp src/engine/task_work.cpp -o $@
build/native-gamma-weld-cursors-test: tests/unit/native_gamma_weld_cursors.mm src/apps/native_gamma_actor_assets.hpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-native.a -framework Foundation -o $@

# Same ninja_render module, independent source leaves and explicit private bridge.
build/native-ninja-render-depth-probe.dylib: tests/probes/native_ninja_render_depth_probe.cpp tests/probes/ninja_render_depth_probe.cpp src/engine/ninja_render.cpp src/engine/ninja_render.hpp src/engine/ninja_depth_internal.hpp src/engine/ninja_blend_internal.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) tests/probes/native_ninja_render_depth_probe.cpp src/engine/ninja_render.cpp -o $@

# Bounded PE32 image contract: synthetic bytes, no guest runtime or external assets.
build/executable-image-test: tests/unit/executable_image.cpp src/loader/executable_image.cpp src/loader/executable_image.hpp | build
	$(CXX) $(CXXFLAGS) $(INCLUDES) tests/unit/executable_image.cpp src/loader/executable_image.cpp -o $@
build/executable-image-edges-test: tests/unit/executable_image_edges.cpp src/loader/executable_image.cpp src/loader/executable_image.hpp | build
	$(CXX) $(CXXFLAGS) $(INCLUDES) tests/unit/executable_image_edges.cpp src/loader/executable_image.cpp -o $@

# Asset-free MPEG metadata contract and actual HleDshow consumer.
build/mpeg-timing-contract-test: tests/unit/mpeg_timing_contract.cpp tests/fixtures/mpeg_timing_stream.hpp src/loader/mpeg_timing.cpp src/loader/mpeg_timing.hpp | build
	$(CXX) $(CXXFLAGS) $(INCLUDES) tests/unit/mpeg_timing_contract.cpp src/loader/mpeg_timing.cpp -o $@
build/mpeg-timing-hle-test: tests/unit/mpeg_timing_hle.cpp tests/fixtures/mpeg_timing_stream.hpp src/loader/mpeg_timing.hpp src/loader/mpeg_timing.cpp src/hle/hle_dshow.cpp build/libsadx-guest.a build/libsadx-core.a | build
	$(CXX) $(CXXFLAGS) $(INCLUDES) tests/unit/mpeg_timing_hle.cpp src/hle/hle_dshow.cpp src/loader/mpeg_timing.cpp $(GUEST_LINK) build/libsadx-core.a -o $@

# The publication judge includes the original-hook fixture directly.
build/animation-table-publication-test: tests/fixtures/animation_table_publications.hpp src/player/animation/sonic_animation_initialize.hpp src/player/animation/sonic_animation_content.hpp
# Task scheduler characterization uses unchanged maintained source and real reference transport.
build/task-scheduler-contract-test: tests/unit/task_scheduler_contract.cpp tests/probes/task_scheduler_contract.hpp src/engine/task.cpp src/engine/task.hpp src/engine/task_work.cpp src/engine/task_work.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) tests/unit/task_scheduler_contract.cpp src/engine/task.cpp src/engine/task_work.cpp -o $@
build/task-bridge-contract-test: tests/unit/task_bridge_contract.cpp tests/probes/task_bridge_contract.hpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I$(UNICORN_DIR)/include tests/unit/task_bridge_contract.cpp $(GUEST_LINK) build/libsadx-core.a -o $@

# Inline reference conversion is outside the native archive. Rebuild the actual
# probe/unit consumers when diagnostic mapping or pointer encoding changes.
TASK_REFERENCE_TARGETS := build/native-collision-component-probe.dylib \
 build/native-game-scene-logic-probe.dylib build/native-player-auxiliary-effect-probe.dylib \
 build/native-player-startup-probe.dylib build/native-task-lifecycle-probe.dylib \
 build/native-task-work-probe.dylib build/player-auxiliary-effect-test build/player-task-test \
 build/task-arena-transport-test build/game-scene-logic-test build/task-lifecycle-test build/task-work-test \
 build/task-scheduler-contract-test
$(TASK_REFERENCE_TARGETS): src/guest/task_reference.hpp src/player/player_auxiliary_effect.hpp

# Before-source save owner characterization; explicit transitive headers.
build/save-io-characterization-test: src/cpu/crt_integer.hpp src/cpu/fidelity_float.hpp src/cpu/x87_math.hpp src/cpu/x87_transcendental.hpp src/engine/character_resources.hpp src/engine/graphics_frame.hpp src/engine/native_texture_lifetime.hpp src/engine/ninja_texture.hpp src/engine/sprite_immediate.hpp src/engine/task.hpp src/engine/task_work.hpp src/game/file_select.cpp src/game/file_select.hpp src/game/game_bootstrap.hpp src/game/game_cleanup.hpp src/game/music_state.hpp src/game/save_data.cpp src/game/save_data.hpp src/game/save_file_list.cpp src/game/save_file_list.hpp src/game/save_file_name.hpp src/game/save_write.cpp src/game/save_write.hpp src/game/sound_initial_state.hpp src/game/title_menu.hpp src/math/ninja_math.hpp tests/unit/save_io_characterization.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -Wl,-dead_strip tests/unit/save_io_characterization.cpp src/game/save_write.cpp src/game/save_file_list.cpp src/game/save_data.cpp src/game/file_select.cpp -o $@

# Before-source save owner characterization; explicit transitive headers.
build/save-frame-characterization-test: src/cpu/x87_math.hpp src/engine/character_resources.hpp src/engine/controller_values.hpp src/engine/frame_control_fwd.hpp src/engine/game_controller.hpp src/engine/game_frame_control.hpp src/engine/game_logic.hpp src/engine/game_logic_support.hpp src/engine/graphics_frame.hpp src/engine/native_texture_lifetime.hpp src/engine/ninja_texture.hpp src/engine/speedrun.hpp src/engine/sprite_immediate.hpp src/engine/task.hpp src/engine/task_work.hpp src/game/game_bootstrap.hpp src/game/game_cleanup.hpp src/game/game_loop.hpp src/game/menu_caption.hpp src/game/music_state.hpp src/game/save_data.hpp src/game/save_frame.cpp src/game/save_frame.hpp src/game/sound_initial_state.hpp src/guest/frame_control_reference.hpp src/guest/game_loop.cpp tests/unit/save_frame_characterization.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -Wl,-dead_strip tests/unit/save_frame_characterization.cpp src/game/save_frame.cpp src/guest/game_loop.cpp -o $@

# Finite native mode forwarding and complete tagged reference diagnostics.
build/game-mode-forwarding-test: tests/unit/game_mode_forwarding.cpp src/game/game_mode.cpp src/game/game_mode_native.cpp src/game/game_mode.hpp src/game/game_mode_internal.hpp src/game/game_mode_services.hpp | build
	$(CXX) $(CXXFLAGS) $(INCLUDES) tests/unit/game_mode_forwarding.cpp src/game/game_mode.cpp src/game/game_mode_native.cpp -o $@
build/game-mode-diagnostics-test: tests/unit/game_mode_diagnostics.cpp src/game/game_mode.cpp src/guest/game_mode_reference.cpp src/guest/game_mode_reference.hpp src/game/game_mode.hpp src/game/game_mode_internal.hpp src/game/game_mode_services.hpp src/game/game_loop.hpp src/engine/game_logic.hpp | build
	$(CXX) $(CXXFLAGS) $(INCLUDES) tests/unit/game_mode_diagnostics.cpp src/game/game_mode.cpp src/guest/game_mode_reference.cpp -o $@

# Exact native axes: partial stores, aliases and caller-selected SSE controls.
build/input-mod-axes-contract-test: tests/unit/input_mod_axes_contract.cpp src/input/input_mod_axes.cpp src/input/input_mod_axes.hpp src/cpu/sse_math.cpp src/cpu/sse_math.hpp build/softfloat_sse/softfloat-sse.a build/softfloat/softfloat.a | build
	$(CXX) $(CXXFLAGS) $(INCLUDES) tests/unit/input_mod_axes_contract.cpp src/input/input_mod_axes.cpp src/cpu/sse_math.cpp build/softfloat_sse/softfloat-sse.a build/softfloat/softfloat.a -o $@

build/sse-math-contract-test: tests/unit/sse_math_contract.cpp src/cpu/sse_math.cpp src/cpu/sse_math.hpp src/cpu/x87_math.cpp src/cpu/x87_math.hpp build/softfloat_sse/softfloat-sse.a build/softfloat/softfloat.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I. tests/unit/sse_math_contract.cpp src/cpu/sse_math.cpp src/cpu/x87_math.cpp build/softfloat_sse/softfloat-sse.a build/softfloat/softfloat.a -o $@

# Explicit sentinel identity and reentrant save scheduling; no guest linkage.
build/save-policy-boundaries-test: $(shell xcrun --sdk macosx --show-sdk-path)/SDKSettings.json src/cpu/crt_integer.hpp src/cpu/fidelity_float.hpp src/cpu/x87_math.hpp src/cpu/x87_transcendental.hpp src/engine/character_resources.hpp src/engine/graphics_frame.hpp src/engine/native_texture_lifetime.hpp src/engine/ninja_texture.hpp src/engine/sprite_immediate.hpp src/engine/task.hpp src/engine/task_work.hpp src/game/file_select.cpp src/game/file_select.hpp src/game/game_bootstrap.hpp src/game/game_cleanup.hpp src/game/menu_caption.hpp src/game/music_state.hpp src/game/save_data.cpp src/game/save_data.hpp src/game/save_file_list.cpp src/game/save_file_list.hpp src/game/save_frame.cpp src/game/save_frame.hpp src/game/sound_initial_state.hpp src/game/title_menu.hpp src/math/ninja_math.hpp tests/unit/save_policy_boundaries.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -Wl,-dead_strip tests/unit/save_policy_boundaries.cpp src/game/save_file_list.cpp src/game/save_frame.cpp src/game/save_data.cpp src/game/file_select.cpp -o $@

# Gamma setup witnesses keep their reentrant allocator seam separate from production CAlloc.
build/sprite-registry-test: tests/unit/sprite_registry.cpp src/engine/sprite_registry.cpp src/engine/task_work.cpp src/engine/sprite_registry.hpp src/engine/task_work.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(filter %.cpp,$^) -o $@
build/gamma-setup-resource-test: tests/unit/gamma_setup_resource.cpp src/player/gamma_setup_resource.cpp src/engine/sprite_registry.cpp src/engine/task_work.cpp src/player/gamma_setup_resource.hpp src/engine/sprite_definition.hpp src/engine/sprite_registry.hpp src/engine/task_work.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(filter %.cpp,$^) -o $@
build/gamma-setup-allocation-test: tests/unit/gamma_setup_allocation.cpp src/player/gamma_setup_resource.cpp src/engine/sprite_registry.cpp src/engine/gamma_setup_allocation_native.cpp src/engine/task_work.cpp src/player/gamma_setup_resource.hpp src/engine/sprite_definition.hpp src/engine/sprite_registry.hpp src/engine/task_work.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(filter %.cpp,$^) -o $@
build/gamma-setup-caller-test: tests/unit/gamma_setup_caller.cpp tests/probes/native_gamma_selection_core_probe.cpp src/player/gamma_setup_resource.cpp src/engine/sprite_registry.cpp src/engine/gamma_setup_allocation_native.cpp src/player/gamma_selection_core.cpp src/player/player_startup.cpp src/player/player_state.cpp src/engine/task.cpp src/engine/task_work.cpp src/cpu/x87_math.cpp src/cpu/crt_integer.cpp build/softfloat/softfloat.a src/player/gamma_setup_resource.hpp src/engine/sprite_definition.hpp src/engine/sprite_registry.hpp src/engine/task_work.hpp src/player/gamma_selection_core.hpp src/player/player_initialization.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< src/player/gamma_setup_resource.cpp src/engine/sprite_registry.cpp src/engine/gamma_setup_allocation_native.cpp src/player/gamma_selection_core.cpp src/player/player_startup.cpp src/player/player_state.cpp src/engine/task.cpp src/engine/task_work.cpp src/cpu/x87_math.cpp src/cpu/crt_integer.cpp build/softfloat/softfloat.a -o $@
build/native-sprite-registry-probe.dylib: tests/probes/native_sprite_registry_probe.cpp src/engine/sprite_registry.cpp src/engine/task_work.cpp src/engine/sprite_registry.hpp src/engine/task_work.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) $(filter %.cpp,$^) -o $@
build/native-sprite-registry-prefix-probe.dylib: tools/oracles/build_sprite_registry_probe.py tests/probes/native_sprite_registry_probe.cpp src/engine/sprite_registry.cpp src/engine/task_work.cpp src/engine/sprite_registry.hpp src/engine/task_work.hpp
	$(REFERENCE_PYTHON) tools/oracles/build_sprite_registry_probe.py --output $@
build/native-gamma-setup-resource-probe.dylib: tests/probes/native_gamma_setup_resource_probe.cpp src/player/gamma_setup_resource.cpp src/engine/sprite_registry.cpp src/engine/task_work.cpp src/player/gamma_setup_resource.hpp src/engine/sprite_definition.hpp src/engine/sprite_registry.hpp src/engine/task_work.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) $(filter %.cpp,$^) -o $@

build/native-preview-events-test: tests/unit/native_preview_events.mm src/apps/native_character_selection_assets.hpp src/apps/native_gamma_actor_assets.hpp src/runtime/native_event_actions_context.hpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-native.a -framework Foundation -o $@

build/native-gamma-binding-test: tests/fixtures/character_material_consumer.hpp

# Accepted shared Sonic body: native witnesses exclude the reference/core archive.
build/sonic-controller-native-test: tests/unit/sonic_controller_native.cpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-native.a -o $@
build/sonic-controller-reference-test: tests/unit/sonic_controller_reference.cpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-guest.a build/libsadx-core.a -o $@
build/sonic-physics-capture-reference-test: tests/unit/sonic_physics_capture_reference.cpp build/libsadx-guest.a build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-guest.a build/libsadx-core.a -o $@
# Private yaw observation TU throws at the uncomposed boundary; never shipping behavior.
build/sonic-ground-observer.o: src/player/ground_acceleration_native.cpp src/player/ground_acceleration_internal.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -Dturn_ground_yaw=unobserved_ground_yaw_stub -c $< -o $@
build/sonic-physics-capture-native-test: tests/unit/sonic_physics_capture_native.cpp build/sonic-ground-observer.o build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/sonic-ground-observer.o build/libsadx-native.a -o $@
build/native-sonic-callback-test: tests/unit/native_sonic_callback.mm build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-native.a -framework Foundation -o $@

build/native-gamma-effects-probe.dylib: tests/probes/native_gamma_effects_probe.cpp src/player/gamma_effects.cpp src/player/gamma_effects.hpp src/engine/particle_pool.cpp src/engine/particle_pool.hpp src/engine/task.cpp src/engine/task.hpp src/engine/task_work.cpp src/engine/task_work.hpp src/cpu/x87_math.cpp src/cpu/crt_integer.cpp build/softfloat/softfloat.a
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) tests/probes/native_gamma_effects_probe.cpp src/player/gamma_effects.cpp src/engine/particle_pool.cpp src/engine/task.cpp src/engine/task_work.cpp src/cpu/x87_math.cpp src/cpu/crt_integer.cpp build/softfloat/softfloat.a -o $@
build/gamma-effects-test: tests/unit/gamma_effects.cpp src/player/gamma_effects.hpp src/engine/particle_pool.hpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-native.a -o $@
build/native-gamma-effects-test: tests/unit/native_gamma_effects.mm src/apps/native_gamma_actor_assets.hpp src/runtime/native_player_system.hpp src/player/gamma_effects.hpp src/engine/particle_pool.hpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-native.a -framework Foundation -o $@

build/sonic-animation-assets-test: tests/unit/sonic_animation_assets.mm src/apps/native_sonic_animation_assets.hpp src/apps/native_gamma_actor_assets.hpp src/runtime/native_player_system.hpp src/runtime/sonic_animation_assets.hpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-native.a -framework Foundation -o $@
build/sonic-animation-integration-test: tests/unit/sonic_animation_integration.mm src/apps/native_sonic_animation_assets.hpp src/apps/native_gamma_actor_assets.hpp src/runtime/native_session.hpp src/runtime/native_player_system.hpp src/runtime/native_title_scene.hpp src/runtime/native_sonic_context.hpp build/libsadx-native.a $(NATIVE_PLATFORM_OBJECTS)
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< $(NATIVE_PLATFORM_OBJECTS) build/libsadx-native.a $(MAC_FRAMEWORKS) -o $@
# App metadata loaders are header implementations, outside per-object dependency generation.
build/sadx-native build/sadx-native-closure: src/apps/native_sonic_animation_assets.hpp src/runtime/sonic_animation_assets.hpp src/runtime/native_session.hpp src/runtime/native_player_system.hpp
# These existing Objective-C++ test recipes also include shared owners directly.
build/native-sonic-callback-test build/native-preview-events-test build/native-gamma-binding-test build/native-gamma-weld-cursors-test build/native-gamma-effects-test build/animation-gamma-shared-binding-test: src/runtime/native_session.hpp src/runtime/native_player_system.hpp src/runtime/sonic_animation_assets.hpp

build/native-gamma-motion-test: tests/unit/native_gamma_motion.mm src/apps/native_gamma_actor_assets.hpp src/runtime/native_player_system.hpp src/runtime/gamma_motion.hpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-native.a -framework Foundation -o $@

build/particle-lifecycle-test: tests/unit/particle_lifecycle.cpp src/engine/particle_pool.hpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< build/libsadx-native.a -o $@
build/native-particle-lifecycle-probe.dylib: tests/probes/native_particle_lifecycle_probe.cpp src/engine/particle_pool.hpp src/engine/particle_pool_lifecycle_native.cpp src/engine/task.cpp src/engine/task_work.cpp src/cpu/x87_math.cpp src/cpu/crt_integer.cpp build/softfloat/softfloat.a
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) tests/probes/native_particle_lifecycle_probe.cpp src/engine/particle_pool_lifecycle_native.cpp src/engine/task.cpp src/engine/task_work.cpp src/cpu/x87_math.cpp src/cpu/crt_integer.cpp build/softfloat/softfloat.a -o $@

build/editor-state-test: tests/unit/editor_state.cpp src/game/editor_state.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@
build/native-editor-state-probe.dylib: tests/probes/native_editor_state_probe.cpp src/game/editor_state.hpp
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) $< -o $@
build/native-editor-binding-test: tests/unit/native_editor_binding.mm build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-native.a -framework Foundation -o $@

# PJoinVertexes initialization/backup lifetime and exact object-node traversal.
build/player-vertex-weld-test: tests/unit/player_vertex_weld.cpp src/player/player_vertex_weld.cpp src/player/player_vertex_weld.hpp src/engine/character_pack.cpp src/engine/character_geometry.cpp src/engine/task_work.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) tests/unit/player_vertex_weld.cpp src/player/player_vertex_weld.cpp src/engine/character_pack.cpp src/engine/character_geometry.cpp src/engine/task_work.cpp -o $@
build/native-player-vertex-weld-probe.dylib: tests/probes/native_player_vertex_weld_probe.cpp tests/probes/vertex_weld_fixture.hpp src/player/player_vertex_weld.cpp src/player/player_vertex_weld.hpp src/engine/character_pack.cpp src/engine/character_geometry.cpp src/engine/task_work.cpp
	$(CXX) $(CXXFLAGS) -dynamiclib $(INCLUDES) tests/probes/native_player_vertex_weld_probe.cpp src/player/player_vertex_weld.cpp src/engine/character_pack.cpp src/engine/character_geometry.cpp src/engine/task_work.cpp -o $@
build/native-gamma-vertex-weld-test: tests/unit/native_gamma_vertex_weld.mm src/apps/native_gamma_actor_assets.hpp src/runtime/native_player_system.hpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-native.a -framework Foundation -o $@

build/native-gamma-input-buffer-test: tests/unit/native_gamma_input_buffer.mm src/apps/native_gamma_actor_assets.hpp src/runtime/native_player_system.hpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/libsadx-native.a -framework Foundation -o $@

build/native-gamma-input-buffer-probe.dylib: tests/probes/native_gamma_input_buffer_probe.cpp src/player/gamma_input_buffer.cpp src/player/gamma_input_buffer.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -shared -fPIC $< src/player/gamma_input_buffer.cpp -o $@

# Sonic inline model prefix; independent new original judge.
build/native-sonic-model-probe.dylib: tests/probes/native_sonic_model_probe.mm src/apps/native_sonic_model_assets.hpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) -dynamiclib $< build/libsadx-native.a -framework Foundation -o $@
build/sonic-model-selection-preview.cpp: src/runtime/native_selection_preview.cpp reference/packets/sonic-model-native/prepare_wiring.py
	python3 reference/packets/sonic-model-native/prepare_wiring.py
build/sonic-model-callback.o: build/sonic-model-selection-preview.cpp Makefile
	$(CXX) $(CXXFLAGS) -MMD -MP $(INCLUDES) -c $< -o $@
-include build/sonic-model-callback.d
build/sonic-model-native-test: tests/unit/sonic_model_native.mm src/apps/native_sonic_model_assets.hpp build/sonic-model-callback.o build/libsadx-native.a $(NATIVE_PLATFORM_OBJECTS)
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< build/sonic-model-callback.o $(NATIVE_PLATFORM_OBJECTS) build/libsadx-native.a $(MAC_FRAMEWORKS) -o $@

# Shared inline loader consumed by the app and existing Sonic integration witnesses.
build/sadx-native build/sadx-native-closure build/sonic-animation-assets-test build/sonic-animation-integration-test build/native-editor-binding-test: src/apps/native_sonic_model_assets.hpp

build/native-model-matrices-probe.dylib: tests/probes/native_model_matrices_probe.cpp src/engine/model_matrices.cpp src/engine/ninja_matrix_composition.cpp src/player/model_motion_matrices.cpp build/libsadx-core.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -dynamiclib $^ -o $@

build/model-motion-matrices-test: tests/unit/model_motion_matrices.mm src/engine/model_matrices.cpp src/engine/ninja_matrix_composition.cpp src/player/model_motion_matrices.cpp src/player/model_motion_source.cpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $^ -framework Foundation -o $@

# Gamma fan source body and real common-tail binding.
build/native-gamma-fan-probe.dylib: tests/probes/native_gamma_fan_probe.cpp src/player/gamma_fan.cpp src/player/gamma_fan.hpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -dynamiclib $< src/player/gamma_fan.cpp build/libsadx-native.a -o $@
build/native-gamma-fan-test: tests/unit/native_gamma_fan.mm src/apps/native_gamma_actor_assets.hpp src/runtime/native_player_system.hpp src/runtime/native_display.hpp build/libsadx-native.a $(NATIVE_PLATFORM_OBJECTS)
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< $(NATIVE_PLATFORM_OBJECTS) build/libsadx-native.a $(MAC_FRAMEWORKS) -o $@
# Gamma display source prefix, render-state leaves and the actual constructor caller.
build/native-gamma-display-probe.dylib: tests/probes/native_gamma_display_probe.cpp src/player/gamma_display.cpp src/player/gamma_display.hpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -dynamiclib $< src/player/gamma_display.cpp build/libsadx-native.a -o $@
build/native-render-state-snapshot-probe.dylib: tests/probes/native_render_state_snapshot_probe.cpp src/engine/render_state_snapshot.cpp src/engine/render_state_snapshot.hpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -dynamiclib $< src/engine/render_state_snapshot.cpp build/libsadx-native.a -o $@
build/native-gamma-display-test: tests/unit/native_gamma_display.mm src/apps/native_gamma_actor_assets.hpp src/runtime/native_player_system.hpp src/runtime/native_display.hpp src/runtime/native_gamma_display_context.hpp src/engine/render_state_snapshot.hpp build/libsadx-native.a $(NATIVE_PLATFORM_OBJECTS)
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< $(NATIVE_PLATFORM_OBJECTS) build/libsadx-native.a $(MAC_FRAMEWORKS) -o $@
# Sonic mode-zero continuation and exact build-only Root callback wiring.
build/native-sonic-initialization-probe.dylib: tests/probes/native_sonic_initialization_probe.cpp src/player/sonic_model.hpp build/libsadx-native.a Makefile
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -dynamiclib $< build/libsadx-native.a -o $@
build/sonic-initialization-selection-preview.cpp: src/runtime/native_selection_preview.cpp reference/packets/sonic-initialization-native/prepare_wiring.py
	python3 reference/packets/sonic-initialization-native/prepare_wiring.py
build/sonic-initialization-callback.o: build/sonic-initialization-selection-preview.cpp Makefile
	$(CXX) $(CXXFLAGS) -MMD -MP $(INCLUDES) -c $< -o $@
-include build/sonic-initialization-callback.d
build/sonic-initialization-native-test: tests/unit/sonic_initialization_native.mm src/apps/native_sonic_model_assets.hpp src/apps/native_regular_texture_assets.hpp src/runtime/native_session.hpp src/runtime/native_player_system.hpp src/runtime/native_title_scene.hpp src/runtime/native_sonic_context.hpp Makefile build/sonic-initialization-callback.o build/libsadx-native.a $(NATIVE_PLATFORM_OBJECTS)
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) -MMD -MP $< build/sonic-initialization-callback.o $(NATIVE_PLATFORM_OBJECTS) build/libsadx-native.a $(MAC_FRAMEWORKS) -o $@

-include build/native-sonic-initialization-probe.d build/sonic-initialization-native-test.d
build/sonic-model-selection-preview.cpp: reference/packets/sonic-initialization-native/prepare_wiring.py
build/sonic-model-native-test: src/apps/native_regular_texture_assets.hpp

# Open N3b checkpoint: intentionally outside gate/native-journeys until an actor frame is qualified.
CS_DISPLAY_ASSETS ?= build/cs-private-assets
CS_DISPLAY_OUTPUT ?= build/cs-display-check
CS_DISPLAY_ARGS ?=
CS_SAVE_SOURCE ?= $(REFERENCE_GAME_DIR)/savedata
.PHONY: native-character-select-display private-native-assets build/cs-display-assets
# The append-only Gamma display records are imported from the pinned original
# character archive. They stay a private build fixture; the canonical pack is unchanged.
build/display-append/sonic-selection.actions: tools/import/gamma_display_resources.py tools/import/gamma_actor_resources.py tools/import/character_actions.py | build
	mkdir -p build/display-append
	$(REFERENCE_PYTHON) tools/import/gamma_display_resources.py "$(GAME_SYSTEM)/CHRMODELS_orig.dll" "$(GAME_EXE)" $@ > build/display-append/import.json
private-native-assets:
	python3 tools/testing/private_native_assets.py --source "$(NATIVE_ASSETS)" --output "$(CS_DISPLAY_ASSETS)" $(CS_ASSET_ARGS)
build/cs-private-assets/.private-native-assets.json: tools/testing/private_native_assets.py build/display-append/sonic-selection.actions $(NATIVE_ASSETS)/characters/sonic-selection.actions
	rm -rf "$(CS_DISPLAY_ASSETS).new"
	python3 tools/testing/private_native_assets.py --source "$(NATIVE_ASSETS)" --output "$(CS_DISPLAY_ASSETS).new" --append build/display-append/sonic-selection.actions
	rm -rf "$(CS_DISPLAY_ASSETS)"
	mv "$(CS_DISPLAY_ASSETS).new" "$(CS_DISPLAY_ASSETS)"
build/cs-display-assets: build/cs-private-assets/.private-native-assets.json
native-character-select-display: build/sadx-native build/cs-private-assets/.private-native-assets.json
	python3 tools/testing/check_native_character_select_display.py --assets "$(CS_DISPLAY_ASSETS)" --save-source "$(CS_SAVE_SOURCE)" --output "$(CS_DISPLAY_OUTPUT)" $(CS_DISPLAY_ARGS)

build/private-native-assets-test: tests/unit/private_native_assets.cpp
	$(CXX) $(CXXFLAGS) $< -o $@
build/selection-pack-append-test: tests/unit/selection_pack_append.mm src/apps/native_character_selection_assets.hpp
	$(CXX) $(CXXFLAGS) $(MACFLAGS) $(INCLUDES) $< -framework Foundation -o $@

build/animated-model-test: tests/unit/animated_model.cpp tests/probes/native_animated_model_probe.cpp src/engine/animated_model.cpp src/engine/object_draw.cpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

build/native-gamma-ordinary-body-probe.dylib: tests/probes/native_gamma_object_callback_probe.cpp src/player/gamma_display_action.cpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -dynamiclib $^ -o $@

build/normal-uv-quad-test: tests/unit/normal_uv_quad.cpp tests/probes/native_normal_uv_quad_probe.cpp src/engine/normal_uv_quad.cpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -Itests/probes $^ -o $@

build/normal-primitives-test: tests/unit/normal_primitives.cpp tests/probes/native_normal_primitives_probe.cpp src/engine/normal_primitives.cpp src/engine/normal_uv_quad.cpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -Itests/probes $^ -o $@

build/native-gamma-tail-probe.dylib: tests/probes/native_gamma_tail_probe.cpp src/player/gamma_display_tail.cpp build/libsadx-native.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) -dynamiclib $^ -o $@
