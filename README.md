# ARVISS - A RISC-V Instruction Set Simulator

![Builds](https://github.com/badlydrawnrod/arviss/actions/workflows/cmake.yml/badge.svg)
![Builds](https://github.com/badlydrawnrod/arviss/actions/workflows/examples.yml/badge.svg)

## Building Arviss

### Building with MSVC and MSBuild on Windows

This assumes that **CMake** is installed and is on the path.

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
C:> cmake -G Ninja -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang .
C:> cmake --build build
C:> ctest --test-dir build --verbose
```

## Building the examples

See the [this readme](hello_world/README.md) to learn how to build the examples.
