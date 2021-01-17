#include "decode.h"

#include <stdint.h>
#include <stdio.h>

uint8_t ReadByte(uint32_t addr)
{
    return 0;
}

uint16_t ReadHalfword(uint32_t addr)
{
    return 0;
}

uint32_t ReadWord(uint32_t addr)
{
    return 0;
}

void WriteByte(uint32_t addr, uint8_t byte)
{
}

void WriteHalfword(uint32_t addr, uint16_t halfword)
{
}

void WriteWord(uint32_t addr, uint32_t word)
{
}

int main()
{
    CPU cpu = {.ReadByte = ReadByte,
               .ReadHalfword = ReadHalfword,
               .ReadWord = ReadWord,
               .WriteByte = WriteByte,
               .WriteHalfword = WriteHalfword,
               .WriteWord = WriteWord};

    // LUI
    Decode(&cpu, (65535 << 12) | (2 << 7) | OP_LUI);

    // AUIPC
    Decode(&cpu, (65535 << 12) | (2 << 7) | OP_AUIPC);

    // JAL
    Decode(&cpu, (2 << 7) | OP_JAL);

    // JALR
    Decode(&cpu, (123 << 20) | (1 << 15) | (0b000 << 12) | (2 << 7) | OP_JALR);
    Decode(&cpu, (-123 << 20) | (1 << 15) | (0b000 << 12) | (2 << 7) | OP_JALR);

    // BEQ
    Decode(&cpu, (3 << 20) | (6 << 15) | (0b000 << 12) | OP_BRANCH);

    // BNE
    Decode(&cpu, (3 << 20) | (6 << 15) | (0b001 << 12) | OP_BRANCH);

    // BLT
    Decode(&cpu, (3 << 20) | (6 << 15) | (0b100 << 12) | OP_BRANCH);

    // BGE
    Decode(&cpu, (3 << 20) | (6 << 15) | (0b101 << 12) | OP_BRANCH);

    // BLTU
    Decode(&cpu, (3 << 20) | (6 << 15) | (0b110 << 12) | OP_BRANCH);

    // BGEU
    Decode(&cpu, (3 << 20) | (6 << 15) | (0b111 << 12) | OP_BRANCH);

    // LB
    Decode(&cpu, (55 << 20) | (9 << 15) | (0b000 << 12) | (2 << 7) | OP_LOAD);

    // LH
    Decode(&cpu, (55 << 20) | (9 << 15) | (0b001 << 12) | (2 << 7) | OP_LOAD);

    // LW
    Decode(&cpu, (55 << 20) | (9 << 15) | (0b010 << 12) | (2 << 7) | OP_LOAD);

    // LBU
    Decode(&cpu, (55 << 20) | (9 << 15) | (0b100 << 12) | (2 << 7) | OP_LOAD);

    // LHU
    Decode(&cpu, (55 << 20) | (9 << 15) | (0b101 << 12) | (2 << 7) | OP_LOAD);

    // SB
    Decode(&cpu, (3 << 20) | (5 << 15) | (0b000 << 12) | (2 << 7) | OP_STORE);

    // SH
    Decode(&cpu, (3 << 20) | (5 << 15) | (0b001 << 12) | (2 << 7) | OP_STORE);

    // SW
    Decode(&cpu, (3 << 20) | (5 << 15) | (0b010 << 12) | (2 << 7) | OP_STORE);

    // ADDI
    Decode(&cpu, (123 << 20) | (1 << 15) | (0b000 << 12) | (2 << 7) | OP_OPIMM);
    Decode(&cpu, (-123 << 20) | (1 << 15) | (0b000 << 12) | (2 << 7) | OP_OPIMM);

    // SLTI
    Decode(&cpu, (456 << 20) | (1 << 15) | (0b010 << 12) | (2 << 7) | OP_OPIMM);
    Decode(&cpu, (-456 << 20) | (5 << 15) | (0b010 << 12) | (31 << 7) | OP_OPIMM);

    // SLTIU
    Decode(&cpu, (456 << 20) | (1 << 15) | (0b011 << 12) | (2 << 7) | OP_OPIMM);
    Decode(&cpu, (-1 << 20) | (5 << 15) | (0b011 << 12) | (31 << 7) | OP_OPIMM);

    // XORI
    Decode(&cpu, (123 << 20) | (1 << 15) | (0b100 << 12) | (2 << 7) | OP_OPIMM);
    Decode(&cpu, (-123 << 20) | (1 << 15) | (0b100 << 12) | (2 << 7) | OP_OPIMM);

    // ORI
    Decode(&cpu, (123 << 20) | (1 << 15) | (0b110 << 12) | (2 << 7) | OP_OPIMM);
    Decode(&cpu, (-123 << 20) | (31 << 15) | (0b110 << 12) | (2 << 7) | OP_OPIMM);

    // ANDI
    Decode(&cpu, (123 << 20) | (1 << 15) | (0b111 << 12) | (2 << 7) | OP_OPIMM);
    Decode(&cpu, (-123 << 20) | (31 << 15) | (0b111 << 12) | (2 << 7) | OP_OPIMM);

    // SLLI
    Decode(&cpu, (0 << 25) | (15 << 20) | (0b001 << 12) | (5 << 7) | OP_OPIMM);

    // SRLI
    Decode(&cpu, (0 << 25) | (12 << 20) | (0b101 << 12) | (3 << 7) | OP_OPIMM);

    // SRAI
    Decode(&cpu, (0b0100000 << 25) | (12 << 20) | (0b101 << 12) | (3 << 7) | OP_OPIMM);

    // ADD
    Decode(&cpu, (0 << 25) | (8 << 20) | (3 << 15) | (0b000 << 12) | (2 << 7) | OP_OP);

    // SUB
    Decode(&cpu, (0b0100000 << 25) | (8 << 20) | (3 << 15) | (0b000 << 12) | (2 << 7) | OP_OP);

    // SLL
    Decode(&cpu, (0 << 25) | (8 << 20) | (3 << 15) | (0b001 << 12) | (2 << 7) | OP_OP);

    // SLT
    Decode(&cpu, (0 << 25) | (8 << 20) | (3 << 15) | (0b010 << 12) | (2 << 7) | OP_OP);

    // SLTU
    Decode(&cpu, (0 << 25) | (8 << 20) | (3 << 15) | (0b011 << 12) | (2 << 7) | OP_OP);

    // XOR
    Decode(&cpu, (0 << 25) | (8 << 20) | (3 << 15) | (0b100 << 12) | (2 << 7) | OP_OP);

    // SRL
    Decode(&cpu, (0 << 25) | (8 << 20) | (3 << 15) | (0b101 << 12) | (2 << 7) | OP_OP);

    // SRL
    Decode(&cpu, (0b0100000 << 25) | (8 << 20) | (3 << 15) | (0b101 << 12) | (2 << 7) | OP_OP);

    // OR
    Decode(&cpu, (0 << 25) | (8 << 20) | (3 << 15) | (0b110 << 12) | (2 << 7) | OP_OP);

    // AND
    Decode(&cpu, (0 << 25) | (8 << 20) | (3 << 15) | (0b111 << 12) | (2 << 7) | OP_OP);

    // FENCE
    Decode(&cpu, (0b000 << 12) | OP_MISCMEM);

    // ECALL
    Decode(&cpu, (0 << 20) | (0 << 15) | (0b000 << 12) | (0 << 7) | OP_SYSTEM);

    // EBREAK
    Decode(&cpu, (1 << 20) | (0 << 15) | (0b000 << 12) | (0 << 7) | OP_SYSTEM);

    printf("Decoding a compiled program.\n");
    Decode(&cpu, 0xfe010113); // 13 01 01 fe   addi    sp, sp, -32      ADDI sp, sp, -32
    Decode(&cpu, 0x00112e23); // 23 2e 11 00   sw      ra, 28(sp)       SW ra, 28(sp)
    Decode(&cpu, 0x00812c23); // 23 2c 81 00   sw      s0, 24(sp)       SW s0, 24(sp)
    Decode(&cpu, 0x02010413); // 13 04 01 02   addi    s0, sp, 32       ADDI s0, sp, 32
    Decode(&cpu, 0x00000613); // 13 06 00 00   mv      a2, zero         ADDI a2, zero, 0
    Decode(&cpu, 0xfec42a23); // 23 2a c4 fe   sw      a2, -12(s0)      SW a2, -12(s0)
    Decode(&cpu, 0xfea42823); // 23 28 a4 fe   sw      a0, -16(s0)      SW a0, -16(s0)
    Decode(&cpu, 0xfeb42623); // 23 26 b4 fe   sw      a1, -20(s0)      SW a1, -20(s0)
    Decode(&cpu, 0x00001537); // 37 15 00 00   lui     a0, 1            LUI a0, 1
    Decode(&cpu, 0x24350513); // 13 05 35 24   addi    a0, a0, 579      ADDI a0, a0, 579
    Decode(&cpu, 0x01812403); // 03 24 81 01   lw      s0, 24(sp)       LW s0, 24(sp)
    Decode(&cpu, 0x01c12083); // 83 20 c1 01   lw      ra, 28(sp)       LW ra, 28(sp)
    Decode(&cpu, 0x02010113); // 13 01 01 02   addi    sp, sp, 32       ADDI sp, sp, 32
    Decode(&cpu, 0x00008067); // 67 80 00 00   ret                      JALR zero, ra, 0
}