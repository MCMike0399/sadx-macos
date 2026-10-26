#!/usr/bin/env python3
"""Build pinned native SDL3 + libusb locally; no global installation or game binaries.

Usage: python3 third_party/native_controller/build.py --cmake /path/to/cmake
Downloads verified source into build/native-controller-sources and installs under
build/native-controller-deps. SDL3 is a hardware compatibility layer; it does not
replace the pinned Input mod's recovered mapping/deadzones.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[2]
SOURCES = (
    ("libusb-1.0.29.tar.bz2", "https://github.com/libusb/libusb/releases/download/v1.0.29/libusb-1.0.29.tar.bz2",
     "5977fc950f8d1395ccea9bd48c06b3f808fd3c2c961b44b0c2e6e29fc3a70a85", "libusb-1.0.29"),
    ("SDL-3.4.2.tar.gz", "https://codeload.github.com/libsdl-org/SDL/tar.gz/683181b47cfabd293e3ea409f838915b8297a4fd",
     "f5b3a3b27ae296da479ab4642c456ff620d268e65d642bdfc934aa210107593f",
     "SDL-683181b47cfabd293e3ea409f838915b8297a4fd"),
)


def run(command, directory):
    subprocess.run([str(item) for item in command], cwd=directory.resolve(), check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cmake", default=shutil.which("cmake"))
    parser.add_argument("--jobs", type=int, default=6)
    args = parser.parse_args()
    if not args.cmake:
        parser.error("provide --cmake /path/to/local/cmake (3.16 or newer)")
    source_root = ROOT / "build/native-controller-sources"
    prefix = ROOT / "build/native-controller-deps"
    source_root.mkdir(parents=True, exist_ok=True)
    for filename, url, digest, directory in SOURCES:
        archive = source_root / filename
        if not archive.exists():
            with urllib.request.urlopen(url) as response, archive.open("wb") as output:
                shutil.copyfileobj(response, output)
        if hashlib.sha256(archive.read_bytes()).hexdigest() != digest:
            raise ValueError(f"source SHA256 mismatch: {archive}")
        if not (source_root / directory).exists():
            with tarfile.open(archive) as bundle:
                bundle.extractall(source_root, filter="data")
    usb_build = ROOT / "build/native-controller-libusb"
    usb_build.mkdir(exist_ok=True)
    run([source_root / SOURCES[0][3] / "configure", f"--prefix={prefix}",
         "--disable-shared", "--enable-static", "CFLAGS=-O2 -mmacosx-version-min=14.0",
         # New SDKs can weak-link pipe2 even though the deployment OS lacks it.
         # libusb calls it unconditionally when configure's link-only test succeeds.
         # Its existing pipe + fcntl path supports our macOS 14 deployment target.
         "ac_cv_func_pipe2=no"], usb_build)
    run(["make", f"-j{args.jobs}", "install"], usb_build)
    sdl_build = ROOT / "build/native-controller-sdl3"
    sdl_build.mkdir(exist_ok=True)
    sdl_source = source_root / SOURCES[1][3]
    patched_files = json.loads((Path(__file__).parent / "upstream.json").read_text())["sdl3"]["platform_patch"]["files"]
    identities = {name: hashlib.sha256((sdl_source / name).read_bytes()).hexdigest()
                  for name in patched_files}
    if all(identities[name] == facts["original"] for name, facts in patched_files.items()):
        run(["patch", "-p1", "--batch", "-i", Path(__file__).parent / "shared-iohid.patch"], sdl_source)
    for name, facts in patched_files.items():
        if hashlib.sha256((sdl_source / name).read_bytes()).hexdigest() != facts["patched"]:
            raise ValueError(f"unexpected SDL platform patch source: {name}")
    run([args.cmake, "-S", source_root / SOURCES[1][3], "-B", sdl_build,
         "-DCMAKE_BUILD_TYPE=Release", f"-DCMAKE_INSTALL_PREFIX={prefix}",
         "-DCMAKE_OSX_DEPLOYMENT_TARGET=14.0", "-DSDL_SHARED=ON", "-DSDL_STATIC=OFF",
         "-DSDL_AUDIO=OFF", "-DSDL_VIDEO=OFF", "-DSDL_RENDER=OFF", "-DSDL_GPU=OFF",
         "-DSDL_CAMERA=OFF", "-DSDL_TEST_LIBRARY=OFF", "-DSDL_TESTS=OFF",
         "-DSDL_HIDAPI_LIBUSB=ON", "-DSDL_HIDAPI_LIBUSB_SHARED=OFF",
         f"-DCMAKE_PREFIX_PATH={prefix}", f"-DLibUSB_LIBRARY={prefix}/lib/libusb-1.0.a",
         f"-DLibUSB_INCLUDE_PATH={prefix}/include/libusb-1.0",
         "-DLibUSB_LINK_LIBRARIES=-framework IOKit;-framework CoreFoundation;-framework Security"], sdl_build)
    run([args.cmake, "--build", sdl_build, "--parallel", args.jobs], sdl_build)
    run([args.cmake, "--install", sdl_build], sdl_build)
    licenses = prefix / "licenses"
    licenses.mkdir(exist_ok=True)
    shutil.copyfile(source_root / SOURCES[0][3] / "COPYING", licenses / "libusb-LGPL-2.1.txt")
    shutil.copyfile(source_root / SOURCES[1][3] / "LICENSE.txt", licenses / "SDL3-zlib.txt")
    # CMake preserves header source timestamps on install. This Make target records
    # successful rebuilding after a build-script or platform-patch change.
    (prefix / "include/SDL3/SDL.h").touch()
    print(prefix)


if __name__ == "__main__":
    main()
