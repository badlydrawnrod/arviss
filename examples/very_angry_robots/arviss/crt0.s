    .section .init
    .global _start
    .type   _start, @function

_start:
    .cfi_startproc
    .cfi_undefined ra

.option push
.option norelax
    # Initialise the global pointer.
    la		gp,__global_pointer$
.option pop

    # Initialise the stack.
    la      sp, __stack_top
    add     s0, sp, zero

    # Clear the BSS segment.
    la      a0, _edata
    la      a1, _end
    li      a2, 0
clear_bss:
    bgeu	a0, a1, finish_bss
    sb		a2, 0(a0)
    addi	a0, a0, 1
    beq		x0, x0, clear_bss

finish_bss:
    li		a0,0		# a0 = argc = 0
    li		a1,0		# a1 = argv = NULL
    li		a2,0		# a2 = envp = NULL
    call    main

    # Abort execution here.
    ebreak

    .cfi_endproc

    .size  _start, .-_start

    .end
