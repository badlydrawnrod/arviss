# Hello World

Before building this example, [install the pre-requisite RISC-V toolchain](../../README.md).

## Building the example for Arviss

### Windows

Configure **CMake** to use **Clang** as shown here.

```
C:> cd examples\hello_world\arviss
C:> cmake -G Ninja -B build -DCMAKE_C_COMPILER=clang -DCMAKE_OBJCOPY=%USERPROFILE%\scoop\shims\llvm-objcopy .
C:> cmake --build build
```

Built samples are output to `hello_world\arviss\bin` in a format that can be loaded into this example's ARVISS runner.

### Linux

```shell
$ cd examples/hello_world/arviss
$ cmake -G Ninja -B build -DCMAKE_C_COMPILER=clang-11 -DCMAKE_OBJCOPY=/usr/bin/llvm-objcopy
$ cmake --build build
```

Built samples are output to `hello_world/arviss/bin` in a form that can be loaded into this example's ARVISS runner.

## Building the ARVISS runner

TODO
