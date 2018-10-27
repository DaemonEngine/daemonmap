DaemonMap
=========

The navmesh compiler for [Unvanquished](https://www.unvanquished.net/).

DaemonMap is a q3map2 fork from [NetRadiant](https://gitlab.com/xonotic/netradiant) tree, with navmesh computation
code by Fuma using [recastnavigation](https://github.com/recastnavigation/recastnavigation).

Everything else than navmesh code was removed but original file layoutd was
kept, allowing code exchange with NetRadiant upstream if required.

Use [`q3map2` from Xonotic's NetRadiant tree](https://gitlab.com/xonotic/netradiant/) for every other task
like bsp compilation, vis computation, light casting etc.

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
to get proper navmeshes after having modified model size or added or
removed models is to get the navmeshes produced by engine using game code
settings. That's why there is no effort to merge this code into
NetRadiant upstream since this tool is a temporary hack.
