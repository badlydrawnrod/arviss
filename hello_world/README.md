# To run this with Arviss

## Install clang

Install clang. It's much easier than getting the right gcc toolchain.

## Build with batch files / scripts

- Run `build.bat`

## Build with CMake

- Build the example with `cmake`

```
    cmake -G ninja -B build -D CMAKE_C_COMPILER=clang
    cmake --build build
```

## Run

- Build Arviss
- Run "run_hello"
