clang --std=c11 \
--target=riscv32 -ffreestanding -nostartfiles -nostdlib -nodefaultlibs -march=rv32imf -mabi=ilp32f \
-ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,-T,minimal.ld -fuse-ld=lld \
-D PRINTF_DISABLE_SUPPORT_FLOAT -D PRINTF_DISABLE_SUPPORT_LONG_LONG crt0.s main.c printf.c putchar.c \
-o hello

llvm-objcopy hello -O binary hello.bin
