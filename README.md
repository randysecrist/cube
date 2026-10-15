# SoFI Gradient Cube

A spinning 3D cube rendered in pure C with legacy OpenGL and GLUT.
Each face displays a smooth color gradient via per-vertex colors, lit
by a single directional light.  An on-screen overlay shows real-time
stats (FPS, frame time, vertex/triangle counts, draw calls, elapsed time).

Press **Esc** or **q** to quit.

## Prerequisites

| Platform | Packages |
|----------|----------|
| macOS    | Xcode Command Line Tools (ships OpenGL + GLUT frameworks) |
| Linux    | `build-essential`, `freeglut3-dev`, `libgl-dev`, `libglu-dev` |
| Windows  | MinGW-w64 with [freeglut](https://freeglut.sourceforge.net/) |

Autotools (`autoconf`, `automake`) are required to generate the build
system.  On macOS: `brew install autoconf automake`.  On Debian/Ubuntu:
`sudo apt install autoconf automake`.

## Building (standalone app)

```sh
./autogen.sh        # bootstrap autotools (only needed once)
./configure         # detect platform and libraries
make                # compile
./cube              # run
```

## Cross-compiling

The `configure` script accepts a `--host` flag for cross-compilation.

**Linux target** (from macOS, requires a Linux cross-toolchain):

```sh
./configure --host=x86_64-linux-gnu
make
```

**Windows target** (requires MinGW-w64):

```sh
./configure --host=x86_64-w64-mingw32
make
```

## macOS Screen Saver

The core rendering code (`src/mesh.c`, `src/render.c`, `src/logo.c`,
`src/font.c`) is shared between the standalone GLUT app and a macOS
screen saver.  The `screensaver/` directory contains an Xcode project
that wraps these modules as a native `.saver` bundle, with
`SoFICubeView` replacing `main.c` as the entry point.

### Building

```sh
cd screensaver
xcodebuild -project SoFICube.xcodeproj -configuration Release
```

### Installing

```sh
cp -R screensaver/build/Release/SoFICube.saver ~/Library/Screen\ Savers/
```

Then open **System Settings > Screen Saver** and select **SoFICube**.

## Windows Screen Saver

The same shared rendering modules build a native `.scr`, with
`src/win32_scr.c` replacing `main.c` as the entry point. It drives WGL
directly rather than GLUT, so the resulting binary depends only on system
DLLs and can be copied anywhere.

### Building

`configure` enables this target automatically on a MinGW-w64 host, so the
standard build produces it alongside `cube.exe`:

```sh
./autogen.sh
./configure
make            # builds both cube.exe and cube.scr
```

### Installing

Copy `cube.scr` and `sofi.png` into the same directory, then right-click
`cube.scr` and choose **Install**, or copy both to `C:\Windows\System32`.
Select **cube** under **Settings > Personalization > Lock screen > Screen
saver**.

The screen saver responds to the standard flags: `/s` runs full screen,
`/p <hwnd>` renders into the settings preview pane, and `/c` reports that
there are no configurable options. Any key or mouse movement exits.

### Assets

Windows launches a `.scr` from `System32`, so assets are resolved relative
to the executable rather than the working directory: first
`screensaver\Resources\` beside the binary (the build tree layout), then
flat alongside it. Because `larsseit.ttf` is not distributable and this
build has no GLUT bitmap-font fallback, the stats overlay falls back to a
stock Windows face so it stays visible.

## Stats overlay

The upper-left corner displays:

| Line | Metric |
|------|--------|
| 1 | **FPS** -- frames per second (updated every 500 ms) |
| 2 | **Frame** -- milliseconds per frame |
| 3 | **Verts** -- vertices submitted per frame |
| 4 | **Tris** -- triangles drawn per frame |
| 5 | **Draw calls** -- `glBegin`/`glEnd` batches per frame |
| 6 | **Elapsed** -- wall-clock run time (m:ss) |

## Project structure

```
.
├── src/
│   ├── cube.h          # shared types, constants, and function declarations
│   ├── main.c          # GLUT window setup and event loop
│   ├── mesh.c          # rounded-cube geometry generation
│   ├── render.c        # drawing, stats tracking, and overlay
│   ├── logo.c          # texture loading for the SoFi logo
│   ├── font.c          # bitmap font rendering via stb_truetype
│   └── win32_scr.c     # Windows .scr entry point (WGL, no GLUT)
├── screensaver/
│   ├── SoFICube.xcodeproj/   # Xcode project for the .saver bundle
│   ├── SoFICubeView.h        # ScreenSaverView subclass (header)
│   ├── SoFICubeView.m        # ScreenSaverView subclass (implementation)
│   ├── Info.plist             # bundle metadata
│   └── Resources/             # logo and font assets
├── configure.ac    # autoconf input (platform & library detection)
├── Makefile.am     # automake input (build rules)
├── autogen.sh      # bootstrap script (runs autoreconf)
└── build-aux/      # autotools helper scripts (generated)
```

## Controls

| Key | Action |
|-----|--------|
| Esc | Quit |
| q   | Quit |
