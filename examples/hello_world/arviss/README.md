# Building the examples

The examples are designed to be run with **Arviss**, so they must be compiled for RISC-V. This requires a cross-compiler
toolchain, such as a **gcc** toolchain that targets RISC-V, or **clang** which can compile for many targets.

## Windows

The simplest way to build the examples on Windows is to install **clang** with a package manager such as **scoop** or
**chocolatey**.

### Install scoop

Install **scoop**, as documented here: https://scoop.sh/ and add it to the `PATH`.

### Install CMake, Ninja, and LLVM

Use scoop to install **CMake**, **Ninja**, and **LLVM**.

```
C:> scoop install cmake
C:> scoop install ninja
C:> scoop install llvm
```

### Build the examples with batch files (quick and easy)

```
C:> cd demos\hello_world\arviss
C:> build.bat
```

### Build the examples with CMake (preferred)

Built samples are output to `hello_world/bin` in a format that can be loaded into ARVISS.
```
C:> cd demos\hello_world\arviss
C:> cmake -G Ninja -B build -DCMAKE_C_COMPILER=clang -DCMAKE_OBJCOPY=%USERPROFILE%\scoop\shims\llvm-objcopy .
C:> cmake --build build
```

## Linux

These instructions are for Ubuntu 20.04. It appears to ship with clang-10, so an upgrade to clang-11 or later is
required.

### Install clang

Install **clang-11** and the **lld-11** linker.

```shell
$ sudo apt install clang-11 lld-11
```

### Build the examples with CMake

Built samples are output to `hello_world/bin` in a format that can be loaded into ARVISS.

```shell
$ cd demos/hello_world/arviss
$ cmake -G Ninja -B build -DCMAKE_C_COMPILER=clang-11 -DCMAKE_OBJCOPY=/usr/bin/llvm-objcopy
$ cmake --build build
```

## Termux (on Android)

TODO: but works with clang-12 and lld-12.

# Running

- Build Arviss (and the demo runner) using a native toolchain as described in the parent `README.md`.
- Run `run_hello`.

e.g.,

```
C:> cd build\demos\hello_world\runner
C:> run_hello
```
