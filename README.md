DaemonMap
=========

The navmesh compiler for [Unvanquished](https://www.unvanquished.net/).

DaemonMap is a q3map2 fork from [NetRadiant](https://gitlab.com/xonotic/netradiant) tree, with navmesh computation
code by Fuma using [recastnavigation](https://github.com/recastnavigation/recastnavigation).

Everything else than navmesh code was removed but original file layoutd was
kept, allowing code exchange with NetRadiant upstream if required.

The navmesh settings are currently hardcoded with Unvanquished models and sizes.

Fetch & Build
-------------

```sh
git clone --recurse-submodules https://github.com/Unvanquished/daemonmap.git
cd daemonmap
cmake -H'.' -B'build' -G'Unix Makefiles'
cmake --build 'build' -- -j"$(nproc)"
```

Usage
-----

```sh
build/daemonmap -game unvanquished -nav /path/to/maps/mapname.bsp
```

Help
----

```sh
build/daemonmap --help
build/daemonmap --help nav
```

No future
---------

This tool is made to be deleted one day because the only way for a game
to get proper navmeshes after having changed model size or added or
removed models is to get the navmesh produced by engine using game code
settings. That's why there is no effort to merge this code into
NetRadiant upstream since this tool must be considered as temporary hack.
