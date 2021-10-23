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

## Building Arviss

All of the following instructions assume that **CMake** is installed and is on the path.

### Building with MSVC and MSBuild on Windows

Run the following from a Visual Studio Developer Command Prompt.

```
C:> cmake -G "Visual Studio 16 2019" -B build -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl .
C:> cmake --build build
C:> ctest --test-dir build --verbose
```

### Building with MSVC and Ninja on Windows

This assumes that **CMake** and **Ninja** are installed and are on the path.

Run the following from a Visual Studio Developer Command Prompt.

```
C:> cmake -G Ninja -B build -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl .
C:> cmake --build build
C:> ctest --test-dir build --verbose
```

### Building with clang and Ninja on Windows

This assumes that **CMake**, **clang** and **Ninja** are installed and are on the path.

Run the following from a command prompt.

```
C:> cmake -G Ninja -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ .
C:> cmake --build build
C:> ctest --test-dir build --verbose
```

### Building with gcc and make on Linux

This assumes that **CMake** is installed and is on the path.

Run the following from a shell prompt.

```shell
cmake -G "Unix Makefiles" -B build -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ .
cmake --build build
ctest --test-dir build --verbose
```

### Building with gcc and Ninja on Linux

This assumes that **CMake** and **Ninja** are installed and are on the path.

Run the following from a shell prompt.

```shell
cmake -G Ninja -B build -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ .
cmake --build build
ctest --test-dir build --verbose
```

### Building with clang and Ninja on Linux

This assumes that **CMake** and **Ninja** are installed and are on the path.

Run the following from a shell prompt.

```shell
cmake -G Ninja -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ .
cmake --build build
ctest --test-dir build --verbose
```

## Building the examples

See [this readme](examples/README.md) to learn how to build and run the examples.
