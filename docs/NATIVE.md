# Native application bring-up

`build/sadx-native` is the separate Apple Silicon application target. It links compiled
native game/platform code and consumes imported resources. The hybrid `sadx-app --guest`
is still the development reference.

Current verified scope: the native Start handler, save initialization, both logos and their
fades, and the opening movie with decoded video, native audio, fades and normal-input Skipma.
Both a complete intro and an Enter-key skip reach the explicit `clear_game` boundary after
releasing the movie resources. Post-movie cleanup, title and the connected Sonic story remain
unfinished. This target is not yet a playable port.

Import from your own supported PC installation once:

```sh
python3 tools/import/native_assets.py /path/to/SADX game/imported
```

For the currently qualified setup, use the pinned reference installation with its Input-mod
configuration and `sonicDX.ini`. The importer validates the original character archive,
converts its startup action graph into typed pointer-free records, copies textures and movie
files, and records the source movie playlists, volume and input settings. The patched installation's
`CHRMODELS_orig.dll` is the asset archive; `CHRMODELS.dll` may be the mod-loader proxy.
Neither executable is copied into the imported output.
Importing an unmodified disc installation with a separately supplied profile is not yet
qualified. Build and production execution use the imported files and native code.

Build with Xcode command-line tools and CMake available:

```sh
make -j10 build/sadx-native
./build/sadx-native --assets game/imported
```

If CMake is outside PATH, pass `NATIVE_CONTROLLER_CMAKE=/absolute/path/to/cmake` to make.
The pinned SDL3/libusb sources and small macOS USB sharing patch build locally under
`build/native-controller-*`. They supply native Switch 2 Pro USB support without original
Windows DLL execution. Bluetooth and multiple identical Switch 2 controllers are unverified.

For reproducible startup images and state:

```sh
./build/sadx-native --assets game/imported --offscreen --exit-after 120 \
  --screenshot build/native-logo.ppm
python3 tools/journey/compare_native_startup.py
```

The comparison runs both applications and checks 16 states and complete image bytes through
frame 388. It requires the development reference installation. Offscreen startup checks do
not establish gameplay frame pacing, physical controller operation, or game audio scheduling.

The following exercises the normal native input path during the intro:

```sh
./build/sadx-native --assets game/imported --offscreen --exit-after 520 \
  --key-events 500:13:down,501:13:up
```

It currently exits 1 at `clear_game`; this is a reported missing implementation, not a successful
complete journey. Without key events the full intro plays before reaching the same boundary.
Movie texture padding was uninitialized in the source renderer; native padding is explicitly
cleared and reported as undefined in the source. Movie pixel parity is not claimed.

Physical controller and native AudioQueue qualification are separate from game fidelity.
The USB device has been exercised for buttons, both sticks, triggers, reconnect and rumble;
the user confirmed the rumble. Effect, music and voice PCM reached the native device queue.
Those results do not establish that unfinished native gameplay schedules the correct events.
