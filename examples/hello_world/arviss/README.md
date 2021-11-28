# Hello World

Before building this example, build Arviss, then [install the pre-requisite RISC-V toolchain](../../README.md).

## Windows

### Building

Configure **CMake** to use **Clang** as shown here.

```
C:> cd examples\hello_world\arviss
C:> cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang
C:> cmake --build build
```

Built samples are output to `hello_world\arviss\bin` in a format that can be loaded into this example's Arviss runner.

### Running

After building the example, run it using `run_hello.exe` from the relevant Arviss `build` directory.

```
C:> cd build\examples\hello_world\runner
C:> run_hello
```

## Linux

### Building

Configure **CMake** to use **Clang** as shown here.

```shell
$ cd examples/hello_world/arviss
$ cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang
$ cmake --build build
```

Note that this needs **clang-11** or later. If you're on a system such as Ubuntu 20.04 where the default **clang** is
lower than required then set `CMAKE_C_COMPILER` to a later version of **clang** (you'll need to install it first) and
set `ARVISS_LINKER` to the corresponding linker version. For example, if you have **clang-12** installed then set
`CMAKE_C_COMPILER` to `clang-12` and set `ARVISS_LINKER` to `lld-12`.

e.g.,

```shell
$ cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-12 -DARVISS_LINKER=lld-12
```

Built samples are output to `hello_world/arviss/bin` in a form that can be loaded into this example's Arviss runner.

### Running

After building the example, run it using `run_hello.exe` from the relevant Arviss `build` directory.

```shell
$ cd build/examples/hello_world/runner
$ ./run_hello
```
