# ARVISS - A RISC-V Instruction Set Simulator

![Builds](https://github.com/badlydrawnrod/arviss/actions/workflows/cmake.yml/badge.svg)
![Builds](https://github.com/badlydrawnrod/arviss/actions/workflows/examples.yml/badge.svg)

## Introduction

Arviss is an [instruction set simulator](https://en.wikipedia.org/wiki/Instruction_set_simulator)
for [RISC-V](https://en.wikipedia.org/wiki/RISC-V). At the time of writing it supports **RV32IMF**, which comprises the
32-bit base integer instruction set (**RV32I**), the integer multiplication extension (**M**), and the 32-bit floating
point extension (**F**).

It comes with [examples](examples/README.md) written in [C](https://en.wikipedia.org/wiki/C_(programming_language))
and
[Zig](https://en.wikipedia.org/wiki/Zig_(programming_language)).

# Building Arviss

By default, building Arviss will also build the native portion of the examples. To inhibit this, define
`INHIBIT_ARVISS_EXAMPLES=ON`, e.g., with `-D` on the **CMake** command line.

## Windows Pre-requisites

The instructions assume that you have some form of Visual Studio 2019 build tools installed.

## Linux Pre-requisites

The graphical examples use [Raylib](https://www.raylib.com/). To build these on Linux you will need to install the
following, as described [on the Raylib wiki](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux).

```shell
$ sudo apt install libasound2-dev mesa-common-dev libx11-dev libxrandr-dev libxi-dev xorg-dev libgl1-mesa-dev libglu1-mesa-dev
```

## Building with MSVC and MSBuild on Windows

These instructions assume that **CMake** is installed and is on the path.

Run the following from a Visual Studio Developer Command Prompt.

```
C:> cmake -G "Visual Studio 16 2019" -B build -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl .
C:> cmake --build build
C:> ctest --test-dir build --verbose
```

## Building with MSVC and Ninja on Windows

These instructions assume that **CMake** and **Ninja** are installed and are on the path.

Run the following from a Visual Studio Developer Command Prompt.

```
C:> cmake -G Ninja -B build -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl .
C:> cmake --build build
C:> ctest --test-dir build --verbose
```

## Building with clang and Ninja on Windows

These instructions assume that **CMake**, **clang** and **Ninja** are installed and are on the path.

Run the following from a command prompt.

```
C:> cmake -G Ninja -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ .
C:> cmake --build build
C:> ctest --test-dir build --verbose
```

## Building with gcc and make on Linux

These instructions assume that **CMake** is installed and is on the path.

Run the following from a shell prompt.

```shell
cmake -G "Unix Makefiles" -B build -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ .
cmake --build build
ctest --test-dir build --verbose
```

## Building with gcc and Ninja on Linux

These instructions assume that **CMake** and **Ninja** are installed and are on the path.

Run the following from a shell prompt.

```shell
cmake -G Ninja -B build -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ .
cmake --build build
ctest --test-dir build --verbose
```

## Building with clang and Ninja on Linux

These instructions assume that **CMake** and **Ninja** are installed and are on the path.

Run the following from a shell prompt.

```shell
cmake -G Ninja -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ .
cmake --build build
ctest --test-dir build --verbose
```

## Building the RISC-V portion of the examples

See [this readme](examples/README.md) to learn how to build and run the RISC-V portion of the examples.
