NetRadiant
==========

The open source, cross platform level editor for idtech games (Radiant fork)

# Getting the Sources

The latest source is available from the git repository:
https://gitlab.com/xonotic/netradiant.git

The git client can be obtained from the Git website:
http://git-scm.org

To get a copy of the source using the commandline git client:
```
git clone https://gitlab.com/xonotic/netradiant.git
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
 * Minizip
 * ZLib

## msys2

### 32 bit:

```
pacman -S --needed base-devel mingw-w64-i686-{toolchain,cmake,make,gtk2,gtkglext}
```

### 64 bit:

```
pacman -S --needed base-devel mingw-w64-x86_64-{toolchain,cmake,make,gtk2,gtkglext}
```

## OS X:

```
brew install gtkglext
brew install Caskroom/cask/xquartz
brew link --force gettext
```

# Compiling

This project uses the usual CMake workflow:

`cmake -G "Unix Makefiles" -H . -B build && cmake --build build -- -j$(nproc)`

## More Compilation Details

options:
 * `DOWNLOAD_GAMEPACKS=ON`
   Automatically download the gamepack data during the first compilation
 * `RADIANT_ABOUTMSG="Custom build"`
   A message shown in the about dialog

targets:
 * `radiant`    Compiles the radiant core binary
 * `modules`    Compiles all modules (each module has its own target as well)
 * `plugins`    Compiles all plugins (each plugin has its own target as well)
 * `game_packs` Downloads the game pack data
 * `quake3`     Compiles all the Quake3 tools
   - `q3map2`     Quake3 map compiler
   - `q3data`
