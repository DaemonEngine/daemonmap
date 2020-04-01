NetRadiant
==========

![NetRadiant logo](setup/data/tools/bitmaps/splash.png)

The open source, cross platform level editor for id Tech-derivated games (has lineage to GtkRadiant).


## Compatibility matrix

|System   |Build    |Bundle    |Run      |Build requirements                   |
|---------|---------|----------|---------|-------------------------------------|
|Linux    |**Yes**  |**Yes**   |**Yes**  |_GCC or Clang_                       |
|FreeBSD  |**Yes**  |_not yet_ |**Yes**  |_GCC_                                |
|Windows  |**Yes**  |**Yes**   |**Yes**  |_MSYS2/Mingw64 or Mingw32_           |
|Wine     |-        |-         |**Yes**  |-                                    |
|macOS    |**Yes**  |_not yet_ |_mostly_ |_Homebrew, GCC and patched GtkGLExt_ |

NetRadiant is known to build and run properly on Linux, FreeBSD and Windows using MSYS2. NetRadiant is known to build on macOS using Homebrew, but can't display things properly without a modified GtkGLExt which is yet to be upstreamed, and issues are known. Windows build is known to work well on wine, which can be used as a fallback on macOS.

At this time library bundling is only supported on Windows/MSYS2 and Linux. Since bundling copies things from the host, a clean build environment has to be used in order to get a clean bundle. Linux bundle does not ship GTK (users are expected to have a working GTK environment with GtkGlExt installed).


## Getting the sources

Source browser, issues and more can be found on the gitlab project: https://gitlab.com/xonotic/netradiant/

The latest source is available from the git repository: `https://gitlab.com/xonotic/netradiant.git`

The `git` client can be obtained from your distribution repository or from the Git website: http://git-scm.org

A copy of the source tree can be obtained using the command line `git` client this way:

```sh
git clone --recursive https://gitlab.com/xonotic/netradiant.git
cd netradiant
```


## Dependencies

* OpenGL, LibXml2, GTK2, GtkGLExt, LibJpeg, LibPng, LibWebp, Minizip, ZLib.

To fetch default game packages you'll need Git, Subversion and `unzip`.


### Ubuntu:

```sh
apt-get install --reinstall build-essential cmake \
    lib{x11,gtk2.0,gtkglext1,xml2,jpeg,webp,minizip}-dev \
    git subversion unzip wget
```

If you plan to build a bundle, you also need to install `uuid-runtime patchelf`

This is enough to build NetRadiant but you may also install those extra packages to get proper GTK2 graphical and sound themes: `gnome-themes-extra gtk2-engines-murrine libcanberra-gtk-module`


### MSYS2:

Under MSYS2, the mingw shell must be used.

If you use MSYS2 over SSH, add `mingw64` to the path this way (given you compile for 64 bit Windows, replace with `mingw32` if you target 32 bit Windows instead): 

```sh
export PATH="/mingw64/bin:${PATH}"`
```

Install the dependencies this way:

```sh
pacman -S --needed base-devel git \
    mingw-w64-$(uname -m)-{ntldd-git,subversion,unzip,toolchain,cmake,make,gtk2,gtkglext,libwebp,minizip-git}
```

Explicitely use `mingw-w64-x86_64-` or `mingw-w64-i686-` prefix instead of `mingw-w64-$(uname -m)` if you need to target a non-default architecture.


### macOS:

```sh
brew install gcc cmake gtkglext pkgconfig minizip webp coreutils gnu-sed
brew link --force gettext
```


## Submodules

 * Crunch (optional, not built if submodule is not present)

If you have not used `--recursive` option at `git clone` time, you can fetch Crunch this way (run this within the `netradiant` repository):

```sh
git submodule update --init --recursive
```


## Simple compilation

### Easy builder assistant

If you have standard needs and use well-known platform and operating system, you may try the provided `easy-builder` script which may be enough for you:

```sh
./easy-builder
```

If everything went right, you'll find your netradiant build in `install/` subdirectory.

If you need to build a debug build (to get help from a developer, for example), you can do it that way:

```sh
./easy-builder --debug
```

By default, build tools and compilers are using the `build/` directory as workspace.


## Advanced compilation

### Initial build

This project uses the usual CMake workflow:


#### Debug build

```sh
cmake -G "Unix Makefiles" -S. -Bbuild -DCMAKE_BUILD_TYPE=Debug
cmake --build build -- -j$(nproc)
```


#### Release build

```sh
cmake -G "Unix Makefiles" -S. -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build -- -j$(nproc)
```

On macOS you have to add this to the first cmake call:

```sh
-DCMAKE_C_COMPILER=/usr/local/bin/gcc-9 -DCMAKE_CXX_COMPILER=/usr/local/bin/g++-9
```

On FreeBSD you have to add this to the first cmake call:

```sh
-DCMAKE_C_COMPILER=/usr/local/bin/gcc8 -DCMAKE_CXX_COMPILER=/usr/local/bin/g++8
```


### Subsequent builds

The initial build will download the gamepacks and build NetRadiant and tools. If you frequently recompile you can skip downloading the gamepacks:

```sh
cmake --build build --target binaries -- -j$(nproc)
```

You should still periodically update gamepacks:

```sh
cmake --build build --target gamepacks
```


### Build and installation details

#### Compilation details

Options:

* `BUILD_RADIANT=OFF`  
  Do not build NetRadiant (default: `ON`, build netradiant graphical editor);
* `BUILD_TOOLS=OFF`  
  Do not build q3map2 and other tools (default: `ON`, build command line tools);
* `BUILD_DAEMONMAP=OFF`  
  Do not build daemonmap tool (default: `ON` if submodule is there, buils daemonmap navigation mesh generator);
* `BUILD_CRUNCH=OFF`  
  Disable crunch support (default: `ON` if submodule is there, enable crunch support);
* `RADIANT_ABOUTMSG="Custom build by $(whoami)"`  
  A message shown in the about dialog (default: `Custom build`).

Targets:

* `binaries`            Compile all binaries;
  - `netradiant`        Compile the netradiant editor;
  - `modules`           Compile all modules (each module has its own target as well);
  - `plugins`           Compile all plugins (each plugin has its own target as well);
  - `tools`             Compile all tools (each tool has its own target as well);
     * `quake2`         Compile all the Quake 2 tools: `q2map`, `qdata3`;
     * `heretic2`       Compile all the Heretic2 tools: `q2map`, `h2data`;
     * `quake3`         Compile all the Quake 3 tools:
         - `q3map2`     Compile the Quake 3 map compiler;
         - `q3data`     Compile the q3data tool;
     * `unvanquished`   Compile all the Unvanquished tool: `daemonmap`, `q3map3`, `q4data`;
         - `daemonmap`  Compile the daemonmap navigation mesh generator.

Type `make help` to get an exhaustive list of targets.


#### Download details

Options:

* `DOWNLOAD_GAMEPACKS=OFF`  
  Do not automatically download the gamepack data on each compilation and do not install game packs already downloaded (default: `ON`);
* `GAMEPACKS_LICENSE_LIST=all`  
  Download all gamepacks whatever the license (default: `free`, download free gamepacks, can be set to `none` to only filter by name);
* `GAMEPACKS_NAME_LIST="Xonotic Unvanquished"`  
  Download gamepacks for the given games (default: `none`, do not select more gamepacks to download).

Target:

* `gamepacks` Downloads the game pack data.

Run `./gamepacks-manager -h` to know about available licenses and other available games. Both lists are merged, for example setting `GAMEPACKS_LICENSE_LIST=GPL` and `GAMEPACKS_NAME_LIST=Q3` will install both GPL gamepacks and the proprietary Quake 3 gamepack.


#### Installation details

Options:

* `BUNDLE_LIBRARIES=ON`  
  Bundle libraries, only MSYS2 is supported at this time (default: `OFF`);
* `FHS_INSTALL=ON` (available on POSIX systems)  
  Install files following the Filesystem Hierarchy Standard (`bin`, `lib`, `share`, etc.)  
  Also setup XDG mime and application support on Linux-like systems (default: `OFF`, install like in 1999);
* `CMAKE_INSTALL_PREFIX=/usr`  
  Install system-wide on Posix systems, always set `FHS_INSTALL` to `ON` when doing this (default: `install/` directory within source tree).

Target:

* `install` Install files.


## Additonnal notes

### About Crunch

The crnlib used to decode `.crn` files is the one from [Dæmon](http://github.com/DaemonEngine/Daemon) which is the one by [Unity](https://github.com/Unity-Technologies/crunch/tree/unity) made cross-platform and slightly improved. Since Unity brokes compatibility with [BinomialLLC's legacy tree](https://github.com/BinomialLLC/crunch) it's required to use either the `crunch` tool from Dæmon or the one from Unity to compress textures that have to be read by radiant or q3map2.
