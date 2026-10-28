# Build and verification

Install the official [CE C/C++ toolchain v11.2](https://github.com/CE-Programming/toolchain/releases/tag/v11.2)
and put its `bin` directory on PATH. Python 3, Pillow and a native C++14 compiler
are required for verification. On Ubuntu 22.04 the toolchain needs `libc6-i386`.

```sh
make gfx
python3 tools/test.py
make
python3 tools/verify.py
```

The calculator program is `bin/CELESTE.8xp`. GitHub Actions runs these commands
on Linux, downloads the official CE runtime libraries, and produces a checked
install ZIP. It uses no repository secrets or local paths. To package manually,
place the official v15.0 `clibs.8xg` in `dist/`, then run `python3 tools/package.py`.

Host tests exercise the record tracker, actual AppVar/HUD adapter, key edge
detection, and actual runtime update/pause loop with a fake SDK and two
framebuffers. They check that pause freezes simulation and draw-side effects,
preserves hitstop and the screen, and supports reset while paused. Long pauses
discard rendering debt. These tests do not emulate the LCD driver or OS.

Binary checks verify the TI checksum/name, all atlas tiles, room map and font.
Selected player/entity, collision and movement source sections must match the
upstream commit recorded in `upstream-hashes.json`.

Before calling a release device-tested, check on a calculator or ROM-backed
emulator: jump/dash feel; pause during room changes, camera shake and hitstop;
resume; Del held and tapped; Clear while paused and relaunch; full-run finish;
PB persistence after RAM reset; legacy saves; and built-in practice controls.
Device testing has not been performed for this release.
