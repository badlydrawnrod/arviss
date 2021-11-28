# Arviss examples

Arviss ships with a handful of examples that show how to use it. Each example is divided into two parts:

1. a native "runner" that will run the example. **These are built as part of Arviss**.
2. RISC-V examples, designed to be run with a native runner.

The RISC-V examples are designed to be run with **Arviss**, so they must be compiled for RISC-V. This needs a
cross-compiler toolchain, such as a **gcc** toolchain that targets RISC-V, or **clang** which can compile for many
targets including RISC-V.

> **Recommendation:** use clang rather than gcc, as it's already a cross-compiler.

Similarly, the Zig example requires [Zig](https://ziglang.org) to be installed.

## Links to examples

To run these examples, first build Arviss and the native runners. When you have done that, come back to these
instructions and install the pre-requisites (see below) then follow these links to the individual examples.

- [The canonical hello world example.](hello_world/arviss/README.md)
- [A turtle graphics demonstration using Raylib.](turtles/arviss/README.md)
- [A simple arcade game where the enemies are controlled by Arviss](very_angry_robots/arviss/README.md)
- [Conway's "Life" in which every cell is an Arviss VM](life/arviss/README.md) - comes with both C
  and [Zig](https://ziglang.org/) implementations.

# Installing the pre-requisites

Building the examples requires:

- a cross-compiler toolchain that targets RISC-V
- CMake
- Ninja (optional)

These instructions are written in terms of **clang**, as it is much simpler to set up an appropriate toolchain.

The Zig example requires the Zig compiler to be installed.

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

To build the **Zig** example, first install **Zig** by [downloading a pre-built binary](https://ziglang.org/download/)
or by using a package manager such as scoop.

```
C:> scoop install ziglang
```

And once you've tried Zig, you may never go back.

## Linux pre-requisites

On Linux, you'll need **clang-11** or later. The instructions shown here are for Ubuntu 20.04 where the default **
clang**
is **clang-10**.

### Install CMake, Ninja, Clang, and lld

Install **CMake**, **Ninja**, **clang-11** or later, and the **lld** linker that corresponds to the **clang** version.

```shell
$ sudo apt install cmake
$ sudo apt install ninja-build
$ sudo apt install clang-12 lld-12
```

To build the **Zig** example, first install **Zig** by [downloading a pre-built binary](https://ziglang.org/download/),
or [installing it with a package manager](https://github.com/ziglang/zig/wiki/Install-Zig-from-a-Package-Manager).

And once you've tried Zig, you may never go back.