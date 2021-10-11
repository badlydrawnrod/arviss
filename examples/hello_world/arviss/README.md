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
$ cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-11
$ cmake --build build
```

Built samples are output to `hello_world/arviss/bin` in a form that can be loaded into this example's Arviss runner.

### Running

After building the example, run it using `run_hello.exe` from the relevant Arviss `build` directory.

```shell
$ cd build/examples/hello_world/runner
$ ./run_hello
```
