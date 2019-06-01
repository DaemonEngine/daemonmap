NetRadiant
==========

![NetRadiant logo](setup/data/tools/bitmaps/splash.png)

The open source, cross platform level editor for idtech games (Radiant fork).

# Getting the sources

The latest source is available from the git repository:
https://gitlab.com/xonotic/netradiant.git

The git client can be obtained from the Git website:
http://git-scm.org

To get a copy of the source using the command line git client:

```
git clone --recursive https://gitlab.com/xonotic/netradiant.git
cd netradiant
```

See also https://gitlab.com/xonotic/netradiant/ for a source browser, issues and more.

# Dependencies

* OpenGL
* LibXml2
* GTK2
* GtkGLExt
* LibJpeg
* LibPng
* LibWebp
* Minizip
* ZLib

## msys2

Under MSYS2, the mingw shell must be used

### 32 bit:

```
pacman -S --needed base-devel mingw-w64-i686-{toolchain,cmake,make,gtk2,gtkglexti,libwebp}
```

### 64 bit:

```
pacman -S --needed base-devel mingw-w64-x86_64-{toolchain,cmake,make,gtk2,gtkglext,libwebp}
```

## macOS:

```
brew install gcc cmake Caskroom/cask/xquartz gtkglext pkgconfig minizip webp coreutils gnu-sed
brew link --force gettext
```

# Submodules

 * Crunch (optional, disabled by default, only supported with CMake build)

If you have not used `--recursive` option at `git clone` time, you can fetch Crunch this way (run it within the NetRadiant repository):


```
git submodule update --init --recursive
```

# Compiling

This project uses the usual CMake workflow:

## Debug

```
cmake -G "Unix Makefiles" -H. -Bbuild && cmake --build build -- -j$(nproc)
```

## Release

```
cmake -G "Unix Makefiles" -H. -Bbuild -DCMAKE_BUILD_TYPE=Release && cmake --build build -- -j$(nproc)
```

On Mac you have to add this to the first cmake call:

```
-DCMAKE_C_COMPILER=/usr/local/bin/gcc-9 -DCMAKE_CXX_COMPILER=/usr/local/bin/g++-9 -DOPENGL_INCLUDE_DIR=/opt/X11/include -DOPENGL_gl_LIBRARY=/opt/X11/lib/libGL.dylib
```

## Build and installation details

### Compilation details

options:

* `BUILD_RADIANT=OFF`  
   Do not build NetRadiant (default: `ON`, build radiant graphical editor)
* `BUILD_TOOLS=OFF`  
   Do not build q3map2 and other tools (default: `ON`, build command line tools)
* `BUILD_CRUNCH=ON`  
   Enable crunch support (default: `OFF`, disable crunch support)
* `RADIANT_ABOUTMSG="Custom build"`  
   A message shown in the about dialog

targets:

* `radiant`    Compiles the radiant core binary
* `modules`    Compiles all modules (each module has its own target as well)
* `plugins`    Compiles all plugins (each plugin has its own target as well)
* `quake3`     Compiles all the Quake3 tools
  - `q3map2`   Compiles the quake3 map compiler
  - `q3data`   Compiles the q3data tool

### Download details

options:

* `DOWNLOAD_GAMEPACKS=OFF`  
   Do not automatically download the gamepack data during the first compilation (default: `ON`)
* `GAMEPACKS_LICENSE_LIST=all`  
   Download all gamepacks whatever the license (default: `free`, download free gamepacks)
* `GAMEPACKS_NAME_LIST=Xonotic Unvanquished`  
   Download gamepacks for the given games (default: `none`, do not select more gamepacks to download)

target:

* `game_packs` Downloads the game pack data

Run `./gamepacks-manager -h` to know about available licenses and other available games. Both lists are merged, for example setting `GAMEPACKS_LICENSE_LIST=GPL` and `GAMEPACKS_NAME_LIST=Q3` will install both GPL gamepacks and proprietary Quake 3 one.

### Installation details

options:

* `FHS_INSTALL=ON`  
  Install files following the Filesystem Hierarchy Standard (bin, lib, share, etc.), also setup XDG mime and application support on POSIX systems (default: `OFF`, install like in 1999)
* `CMAKE_INSTALL_PREFIX=/usr`  
  Install system-wide on Posix systems, always set `FHS_INSTALL` to `ON` when doing this (default: install in `install/` directory within source tree)

target:

* `install`  
  Install files

## Note about Crunch

The crnlib used to decode `.crn` files is the one from [Dæmon](http://github.com/DaemonEngine/Daemon) which is the one by [Unity](https://github.com/Unity-Technologies/crunch/tree/unity) made cross-platform and slightly improved. Since Unity brokes compatibility with [BinomialLLC's legacy tree](https://github.com/BinomialLLC/crunch) it's required to use either the `crunch` tool from Dæmon or the one from Unity to compress textures that have to be read by radiant or q3map2.
