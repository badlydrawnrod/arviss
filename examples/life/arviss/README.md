# Conway's "Life"

Before building this example, build Arviss, then [install the pre-requisite RISC-V toolchain](../../README.md). Note, if
you want to build the [Zig](https://ziglang.org/) example, then install **Zig** by following the
instructions [further down the same page]((../../README.md)).

## Windows

### Building

Configure **CMake** to use **Clang** as shown here.

```
C:> cd examples\life\arviss
C:> cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang
C:> cmake --build build
```

Built samples are output to `life\arviss\bin` in a format that can be loaded into this example's Arviss runner.

### Running

After building the example, run it using `run_life.exe` from the relevant Arviss `build` directory.

```
C:> cd build\examples\life\runner
C:> run_life
```

## Linux

### Building

Configure **CMake** to use **Clang** as shown here.

```shell
$ cd examples/life/arviss
$ cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-11
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

Built samples are output to `life/arviss/bin` in a form that can be loaded into this example's Arviss runner.

### Running

After building the example, run it using `run_life` from the relevant Arviss `build` directory.

```shell
$ cd build/examples/life/runner
$ ./run_life
```

## Zig

### Building

Build the runner as described above under **Windows** or **Linux**, then build the example itself.

To build the example, run `zig build-exe` setting the target to `riscv32-freestanding` and the CPU and features to
`generic_rv32+f+m`.

```
C:> cd examples\life\arviss
C:> zig build-exe cell.zig --strip --single-threaded -target riscv32-freestanding -Tminimal.ld -mcpu generic_rv32+f+m -O ReleaseSmall -femit-bin=bin\cell
```

After building the example, run it using `run_life` (Linux) or `run_life.exe` (Windows)  from the relevant Arviss
`build` directory.

```
C:> cd build\examples\life\runner
C:> run_life
```
