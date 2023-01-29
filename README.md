DaemonMap
=========

## Deprecated

⚠️ This tool is deprecated since 2023-01-29 and the release of [Unvanquished 0.54.0](https://unvanquished.net/unvanquished-0-54-armed-and-dangerous/).

⚠️ Navigation mesh generation is now implemented in game.

⚠️ Files produced with this tool are expected to be incompatibles with the game.

This repository and this tool are only of interest for historical purpose.

The maintained navigation mesh generation code can now be found in Unvanquished repository:

- https://github.com/Unvanquished/Unvanquished

## No future

The precious navmesh code was meant to be moved from this tool to the game itself because the only way for a game to get proper navmeshes after having modified model size or added or removed models is to get the navmeshes produced by the engine or the game itself using game settings.

The navmesh code itself is meant to live and get a bright future, in a more convenient place which is the game iself.

This tool hosting the navmesh code was meant to be left over once such migration would have been done. Which is now done.

## Not merged in NetRadiant upstream

This tool was a temporary hack. There was no effort to merge this tool into NetRadiant upstream and there will be be no effort done on that purpose in the future. The only project having used this tool stopped using it after having imported the navigation mesh generation code elsewhere.

## What was daemonmap

Daemonmap was an external navigation mesh (navmesh) compiler for [Unvanquished](https://unvanquished.net/).

DaemonMap is a q3map2 fork from [NetRadiant](https://netradiant.gitlab.io) tree, with navmesh computation code by Fuma using [recastnavigation](https://github.com/recastnavigation/recastnavigation).

Everything but navmesh code was removed but original file layout was kept, allowing code exchange with NetRadiant upstream if required.

The [`q3map2` tool from Xonotic's NetRadiant tree](https://gitlab.com/xonotic/netradiant/) should be used for every other task done on maps like BSP compilation, visibility computation, light casting, etc.

The navmesh settings are hardcoded with Unvanquished models and sizes.


## Getting the sources

```sh
git clone --recurse-submodules https://github.com/Unvanquished/daemonmap.git
cd daemonmap
```


## Simple compilation

### Easy builder assistant

If you have standard needs and use well-known platform and operating system, you may try the provided `easy-builder` script which may be enough for you:

```sh
./easy-builder
```

If everything went right, you'll find your daemonmap build in `install/` subdirectory.

If you need to build a debug build (to get help from a developer, for example), you can do it that way:

```sh
./easy-builder --debug
```

By default, build tools and compilers are using the `build/` directory as workspace.


## Advanced compilation

This project uses the usual CMake workflow:


### Debug build

```sh
cmake -G "Unix Makefiles" -S. -Bbuild -DCMAKE_BUILD_TYPE=Debug
cmake --build build -- -j$(nproc) install
```


### Release build

```sh
cmake -G "Unix Makefiles" -S. -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build -- -j$(nproc) install
```


## Usage

```sh
daemonmap -game unvanquished -nav /path/to/maps/mapname.bsp
```


## Help

```sh
daemonmap --help
daemonmap --help nav
```