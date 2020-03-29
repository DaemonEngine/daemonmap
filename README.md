DaemonMap
=========

The navmesh compiler for [Unvanquished](https://unvanquished.net/).

DaemonMap is a q3map2 fork from [NetRadiant](https://netradiant.gitlab.io) tree, with navmesh computation
code by Fuma using [recastnavigation](https://github.com/recastnavigation/recastnavigation).

Everything but navmesh code was removed but original file layout was
kept, allowing code exchange with NetRadiant upstream if required.

Use [`q3map2` from Xonotic's NetRadiant tree](https://gitlab.com/xonotic/netradiant/) for every other task
like bsp compilation, vis computation, light casting etc.

The navmesh settings are currently hardcoded with Unvanquished models and sizes.


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
cmake -G "Unix Makefiles" -H. -Bbuild -DCMAKE_BUILD_TYPE=Debug
cmake --build build -- -j$(nproc) install
```


### Release build

```sh
cmake -G "Unix Makefiles" -H. -Bbuild -DCMAKE_BUILD_TYPE=Release
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


## No future

This tool is made to be deleted one day because the only way for a game
to get proper navmeshes after having modified model size or added or
removed models is to get the navmeshes produced by engine using game code
settings. That's why there is no effort to merge this code into
NetRadiant upstream since the existence of this tool is a temporary hack.
