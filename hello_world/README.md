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

## Building

### Build the examples with batch files (quick and easy)

```
C:> cd hello_world
C:> build.bat
```

### Build the examples with CMake (preferred)

```
C:> cd hello_world
C:> cmake -G Ninja -B build -DCMAKE_C_COMPILER=clang -DCMAKE_OBJCOPY=%USERPROFILE%\scoop\shims\llvm-objcopy .
C:> cmake --build build
```

## Running

- Build Arviss using a native toolchain.
- Copy `hello.bin` from the build directory.
- Run "run_hello".
