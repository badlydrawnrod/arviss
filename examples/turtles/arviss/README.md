# Turtles

Before building this example, build Arviss, then [install the pre-requisite RISC-V toolchain](../../README.md).

## Windows

### Building

Configure **CMake** to use **Clang** as shown here.

```
C:> cd examples\turtles\arviss
C:> cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang
C:> cmake --build build
```

Built samples are output to `turtles\arviss\bin` in a format that can be loaded into this example's Arviss runner.

### Running

After building the example, run it using `run_turtles.exe` from the runner's `build` directory.

```
C:> cd build
C:> run_turtles
```

## Linux

### Building

Configure **CMake** to use **Clang** as shown here.

```shell
$ cd examples/turtles/arviss
$ cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-11
$ cmake --build build
```

Built samples are output to `turtles/arviss/bin` in a form that can be loaded into this example's Arviss runner.

### Running

After building the example, run it using `run_turtles` from the runner's `build` directory.

```shell
$ cd build
$ ./run_turtles
```
