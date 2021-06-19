# Arviss examples

Arviss ships with a handful of examples that show how to use it. Each example is divided into two parts:

1. a native "runner" that will run the example. These are built as part of Arviss.
2. RISC-V examples, designed to be run with a native runner.

The RISC-V examples are designed to be run with **Arviss**, so they must be compiled for RISC-V. This needs a
cross-compiler toolchain, such as a **gcc** toolchain that targets RISC-V, or **clang** which can compile for many
targets including RISC-V.

## Links to examples

To run these examples, first build Arviss and the native runners. Install the pre-requisites (see below) then follow
these links to the individual examples.

- [The canonical hello world example.](hello_world/arviss/README.md)
- [A turtle graphics demonstration using Raylib.](turtles/arviss/README.md)

# Installing the pre-requisites

Building the examples requires:

- a cross-compiler toolchain that targets RISC-V
- CMake
- Ninja (optional)

These instructions are written in terms of **clang**, as it is much simpler to set up an appropriate toolchain.

## Windows pre-requisites

The simplest way to install the pre-requisites on Windows is to use a package manager such as **scoop** or
**chocolatey**.

### Install scoop

Install **scoop**, as documented here: https://scoop.sh/ and add it to the `PATH`.

### Install CMake, Ninja, and LLVM

Use scoop to install **CMake**, **Ninja**, and **LLVM** (for **clang**).

```
C:> scoop install cmake
C:> scoop install ninja
C:> scoop install llvm
```

## Linux pre-requisites

On Linux, install **clang-11** or later. These instructions are for Ubuntu 20.04 which appears to ship with **clang-10**
.

### Install CMake, Ninja, Clang, and lld

Install **CMake**, **Ninja**, **clang-11**, and the **lld-11** linker.

```shell
$ sudo apt install cmake
$ sudo apt install ninja-build
$ sudo apt install clang-11 lld-11
```
