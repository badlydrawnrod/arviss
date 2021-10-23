// This is a Zig version of cell.c. It should be compiled with the same linker script as cell.c
//
// zig build-exe cell.zig --strip --single-threaded -target riscv32-freestanding -Tminimal.ld -mcpu generic_rv32+f+m \
//         -O ReleaseSmall -femit-bin=bin\cell

// Sets up a small stack in the .bss section. See: https://github.com/andrewrk/HellOS/blob/master/hellos.zig
export var stack_bytes: [128]u8 align(16) linksection(".bss") = undefined;
const stack_bytes_slice = stack_bytes[0..];

// By convention, the entry point is _start().
export fn _start() callconv(.Naked) noreturn {
    // Call main(), telling it where to find the stack.
    @call(.{ .stack = stack_bytes_slice }, main, .{});
    while (true) {}
}

// These are our syscalls.
const SYS = enum(u32) {
    count = 0,
    getstate = 1,
    setstate = 2,
    _,
};

// Invokes a syscall with zero parameters.
fn syscall0(number: SYS) usize {
    return asm volatile ("ecall"
        : [ret] "={x10}" (-> usize),
        : [number] "{x17}" (@enumToInt(number)),
        : "memory"
    );
}

// Invokes a sycall with one parameter.
fn syscall1(number: SYS, arg1: usize) usize {
    return asm volatile ("ecall"
        : [ret] "={x10}" (-> usize),
        : [number] "{x17}" (@enumToInt(number)),
          [arg1] "{x10}" (arg1),
        : "memory"
    );
}

fn countNeighbours() usize {
    return syscall0(.count);
}

fn getState() bool {
    return syscall0(.getstate) != 0;
}

fn setState(state: bool) void {
    _ = syscall1(.setstate, @boolToInt(state));
}

fn main() void {
    while (true) {
        var wasAlive = getState();
        var neighbours = countNeighbours();
        setState((wasAlive and neighbours == 2) or neighbours == 3);
    }
}
