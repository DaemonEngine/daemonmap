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

# Submodules

 * Crunch

If you forgot to add `--recursive` option at `git clone` time, fetch it this way:


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

## Note about Crunch

The crnlib used to decode `.crn` files is the one from [Dæmon](http://github.com/DaemonEngine/Daemon) which is just the one by [Unity](https://github.com/Unity-Technologies/crunch/tree/unity) made cross-platform. Since Unity brokes compatibility with [BinomialLLC's legacy tree](https://github.com/BinomialLLC/crunch) it's required to use either crunch from Dæmon or the one from Unity to compress textures that have to be read by radiant or q3map2.
