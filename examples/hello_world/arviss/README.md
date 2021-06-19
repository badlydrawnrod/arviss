# Hello World

Before building this example, build Arviss, then [install the pre-requisite RISC-V toolchain](../../README.md).

## Windows

### Building

Configure **CMake** to use **Clang** and **llvm-objcopy** as shown here.

```
C:> cd examples\hello_world\arviss
C:> cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_OBJCOPY=%USERPROFILE%\scoop\shims\llvm-objcopy
C:> cmake --build build
```

Built samples are output to `hello_world\arviss\bin` in a format that can be loaded into this example's Arviss runner.

### Running

After building the example, run it using `run_hello.exe` from the runner's `build` directory.

```
C:> cd build
C:> run_hello
```

## Linux

### Building

Configure **CMake** to use **Clang** and **llvm-objcopy** as shown here.

```shell
$ cd examples/hello_world/arviss
$ cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-11 -DCMAKE_OBJCOPY=/usr/bin/llvm-objcopy
$ cmake --build build
```

Built samples are output to `hello_world/arviss/bin` in a form that can be loaded into this example's Arviss runner.

### Running

After building the example, run it using `run_hello` from the runner's `build` directory.

```shell
$ cd build
$ ./run_hello
```
