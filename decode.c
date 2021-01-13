#include <stdint.h>
#include <stdio.h>

typedef struct CPU
{
    uint32_t pc;
    uint32_t reg[32];

    uint8_t (*ReadByte)(uint32_t addr);
    uint16_t (*ReadHalfword)(uint32_t addr);
    uint32_t (*ReadWord)(uint32_t addr);

    void (*WriteByte)(uint32_t addr, uint8_t byte);
    void (*WriteHalfword)(uint32_t addr, uint16_t halfword);
    void (*WriteWord)(uint32_t addr, uint32_t word);
} CPU;

enum
{
    OP_LUI = 0b0110111,
    OP_AUIPC = 0b0010111,
    OP_JAL = 0b1101111,
    OP_JALR = 0b1100111,
    OP_BRANCH = 0b1100011,
    OP_LOAD = 0b0000011,
    OP_STORE = 0b0100011,
    OP_OPIMM = 0b0010011,
    OP_OP = 0b0110011,
    OP_MISCMEM = 0b0001111,
    OP_SYSTEM = 0b1110011
};

static inline int32_t IImmediate(uint32_t instruction)
{
    return (int32_t)instruction >> 20; // inst[31:20]
}

static inline int32_t SImmediate(uint32_t instruction)
{
    return ((int32_t)(instruction & 0xfe000000) >> 20) // inst[31:25]
            | (instruction & 0x00000f80) >> 7          // inst[11:7]
            ;
}

static inline int32_t BImmediate(uint32_t instruction)
{
    return ((int32_t)(instruction & 0x80000000) >> 19) // inst[31]
            | (instruction & 0x00000080) << 4          // inst[7]
            | (instruction & 0x7e000000) >> 20         // inst[30:25]
            | (instruction & 0x00000f00) >> 7          // inst[11:8]
            ;
}

static inline int32_t UImmediate(uint32_t instruction)
{
    return instruction & 0xfffff000; // inst[31:12]
}

static inline int32_t JImmediate(uint32_t instruction)
{
    return ((int32_t)(instruction & 0x80000000) >> 11) // inst[31]
            | (instruction & 0x000ff000)               // inst[19:12]
            | (instruction & 0x00100000) >> 9          // inst[20]
            | (instruction & 0x7fe00000) >> 20         // inst[30:21]
            ;
}

// See: http://www.five-embeddev.com/riscv-isa-manual/latest/gmaps.html#rv3264g-instruction-set-listings
// or riscv-spec-209191213.pdf.
void Decode(CPU* cpu, uint32_t instruction)
{
    uint32_t opcode = instruction & 0x7f;
    uint32_t rd = (instruction >> 7) & 0x1f;
    uint32_t rs1 = (instruction >> 15) & 0x1f;

    switch (opcode)
    {
    case OP_LUI: {
        // rd <- imm_u, pc += 4
        int32_t upper = UImmediate(instruction);
        printf("LUI x%d, %d\n", rd, upper >> 12);
        cpu->reg[rd] = upper;
        cpu->pc += 4;
    }
    break;

    case OP_AUIPC: {
        // rd <- pc + imm_u, pc += 4
        int32_t upper = UImmediate(instruction);
        printf("AUIPC x%d, %d\n", rd, upper >> 12);
        cpu->reg[rd] = cpu->pc + upper;
        cpu->pc += 4;
    }
    break;

    case OP_JAL: {
        // rd <- pc + 4, pc <- pc + imm_j
        int32_t imm = JImmediate(instruction);
        printf("JAL x%d, %d\n", rd, imm);
        cpu->reg[rd] = cpu->pc + 4;
        cpu->pc += imm;
    }
    break;

    case OP_JALR: {
        // rd <- pc + 4, pc <- (rs1 + imm_i) & ~1
        int32_t imm = IImmediate(instruction);
        uint32_t funct3 = (instruction >> 12) & 7;
        if (funct3 == 0b000)
        {
            printf("JALR x%d, x%d, %d\n", rd, rs1, imm);
            cpu->reg[rd] = cpu->pc + 4;
            cpu->pc = (cpu->reg[rs1] + imm) & ~1;
        }
        else
        {
        }
    }
    break;

    case OP_BRANCH: {
        uint32_t funct3 = (instruction >> 12) & 7;
        uint32_t rs2 = (instruction >> 20) & 0x1f;
        int32_t imm = BImmediate(instruction);
        switch (funct3)
        {
        case 0b000: // BEQ
            // pc <- pc + ((rs1 == rs2) ? imm_b : 4)
            printf("BEQ x%d, x%d, %d\n", rs1, rs2, imm);
            cpu->pc += ((rs1 == rs2) ? imm : 4);
            break;
        case 0b001: // BNE
            // pc <- pc + ((rs1 != rs2) ? imm_b : 4)
            printf("BNE x%d, x%d, %d\n", rs1, rs2, imm);
            cpu->pc += ((rs1 != rs2) ? imm : 4);
            break;
        case 0b100: // BLT
            // pc <- pc + ((rs1 < rs2) ? imm_b : 4)
            printf("BLT x%d, x%d, %d\n", rs1, rs2, imm);
            cpu->pc += (((int32_t)rs1 < (int32_t)rs2) ? imm : 4);
            break;
        case 0b101: // BGE
            // pc <- pc + ((rs1 >= rs2) ? imm_b : 4)
            printf("BGE x%d, x%d, %d\n", rs1, rs2, imm);
            cpu->pc += (((int32_t)rs1 >= (int32_t)rs2) ? imm : 4);
            break;
        case 0b110: // BLTU
            // pc <- pc + ((rs1 < rs2) ? imm_b : 4)
            printf("BLTU x%d, x%d, %d\n", rs1, rs2, imm);
            cpu->pc += ((rs1 < rs2) ? imm : 4);
            break;
        case 0b111: // BGEU
            // pc <- pc + ((rs1 >= rs2) ? imm_b : 4)
            printf("BGEU x%d, x%d, %d\n", rs1, rs2, imm);
            cpu->pc += ((rs1 >= rs2) ? imm : 4);
            break;
        default:
            break;
        }
    }
    break;

    case OP_LOAD: {
        uint32_t funct3 = (instruction >> 12) & 7;
        int32_t imm = IImmediate(instruction);
        switch (funct3)
        {
        case 0b000: // LB
            // rd <- sx(m8(rs1 + imm_i)), pc += 4
            printf("LB x%d, %d(x%d)\n", rd, imm, rs1);
            cpu->reg[rd] = (int32_t)(int16_t)(int8_t)cpu->ReadByte(cpu->reg[rs1] + imm);
            break;
        case 0b001: // LH
            // rd <- sx(m16(rs1 + imm_i)), pc += 4
            printf("LH x%d, %d(x%d)\n", rd, imm, rs1);
            cpu->reg[rd] = (int32_t)(int16_t)cpu->ReadHalfword(cpu->reg[rs1] + imm);
            break;
        case 0b010: // LW
            // rd <- sx(m32(rs1 + imm_i)), pc += 4
            printf("LW x%d, %d(x%d)\n", rd, imm, rs1);
            cpu->reg[rd] = (int32_t)cpu->ReadWord(cpu->reg[rs1] + imm);
            break;
        case 0b100: // LBU
            // rd <- zx(m8(rs1 + imm_i)), pc += 4
            printf("LBU x%d, %d(x%d)\n", rd, imm, rs1);
            cpu->reg[rd] = cpu->ReadByte(cpu->reg[rs1] + imm);
            break;
        case 0b101: // LHU
            // rd <- zx(m16(rs1 + imm_i)), pc += 4
            printf("LHU x%d, %d(x%d)\n", rd, imm, rs1);
            cpu->reg[rd] = cpu->ReadHalfword(cpu->reg[rs1] + imm);
            break;
        default:
            break;
        }
    }
    break;

    case OP_STORE: {
        uint32_t funct3 = (instruction >> 12) & 7;
        uint32_t rs2 = (instruction >> 20) & 0x1f;
        int32_t imm = SImmediate(instruction);
        switch (funct3)
        {
        case 0b000: // SB
            // m8(rs1 + imm_s) <- rs2[7:0], pc += 4
            printf("SB x%d, %d(x%d)\n", rs2, imm, rs1);
            cpu->WriteByte(cpu->reg[rs1] + imm, cpu->reg[rs2] & 0xff);
            break;
        case 0b001: // SH
            // m16(rs1 + imm_s) <- rs2[15:0], pc += 4
            printf("SH x%d, %d(x%d)\n", rs2, imm, rs1);
            cpu->WriteHalfword(cpu->reg[rs1] + imm, cpu->reg[rs2] & 0xffff);
            break;
        case 0b010: // SW
            // m32(rs1 + imm_s) <- rs2[31:0], pc += 4
            printf("SW x%d, %d(x%d)\n", rs2, imm, rs1);
            cpu->WriteWord(cpu->reg[rs1] + imm, cpu->reg[rs2]);
            break;
        default:
            break;
        }
    }
    break;

    case OP_OPIMM: {
        uint32_t funct3 = (instruction >> 12) & 7;
        int32_t imm = IImmediate(instruction);
        uint32_t shamt = imm & 0x1f;
        uint32_t funct7 = instruction >> 25;
        switch (funct3)
        {
        case 0b000: // ADDI
            // rd <- rs1 + imm_i, pc += 4
            printf("ADDI x%d, x%d, %d\n", rd, rs1, imm);
            cpu->reg[rd] = cpu->reg[rs1] + imm;
            break;
        case 0b010: // SLTI
            // rd <- (rs1 < imm_i) ? 1 : 0, pc += 4
            printf("SLTI x%d, x%d, %d\n", rd, rs1, imm);
            cpu->reg[rd] = ((int32_t)cpu->reg[rs1] < imm) ? 1 : 0;
            break;
        case 0b011: // SLTIU
            // rd <- (rs1 < imm_i) ? 1 : 0, pc += 4
            printf("SLTIU x%d, x%d, %u\n", rd, rs1, imm);
            cpu->reg[rd] = (cpu->reg[rs1] < imm) ? 1 : 0;
            break;
        case 0b100: // XORI
            // rd <- rs1 ^ imm_i, pc += 4
            printf("XORI x%d, x%d, %d\n", rd, rs1, imm);
            cpu->reg[rd] = cpu->reg[rs1] ^ imm;
            break;
        case 0b110: // ORI
            // rd <- rs1 | imm_i, pc += 4
            printf("ORI x%d, x%d, %d\n", rd, rs1, imm);
            cpu->reg[rd] = cpu->reg[rs1] | imm;
            break;
        case 0b111: // ANDI
            // rd <- rs1 & imm_i, pc += 4
            printf("ANDI x%d, x%d, %d\n", rd, rs1, imm);
            cpu->reg[rd] = cpu->reg[rs1] & imm;
            break;
        case 0b001: // SLLI / SRAI
            // rd <- rs1 << shamt_i, pc += 4
            printf("SLLI x%d, x%d, %d\n", rd, rs1, shamt);
            cpu->reg[rd] = cpu->reg[rd] << shamt;
            break;
        case 0b101:
            switch (funct7)
            {
            case 0b0000000: // SRLI
                // rd <- rs1 >> shamt_i, pc += 4
                printf("SRLI x%d, x%d, %d\n", rd, rs1, shamt);
                cpu->reg[rd] = cpu->reg[rd] >> shamt;
                break;
            case 0b0100000: // SRAI
                // rd <- rs1 >> shamt_i, pc += 4
                printf("SRAI x%d, x%d, %d\n", rd, rs1, shamt);
                cpu->reg[rd] = (int32_t)cpu->reg[rd] >> shamt;
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }
    break;

    case OP_OP: {
        uint32_t funct3 = (instruction >> 12) & 7;
        uint32_t rs2 = (instruction >> 20) & 0x1f;
        uint32_t funct7 = instruction >> 25;
        switch (funct3)
        {
        case 0b000: // ADD / SUB
            switch (funct7)
            {
            case 0b0000000: // ADD
                // rd <- rs1 + rs2, pc += 4
                printf("ADD x%d, x%d, x%d\n", rd, rs1, rs2);
                cpu->reg[rd] = cpu->reg[rs1] + cpu->reg[rs2];
                break;
            case 0b0000001: // SUB
                // rd <- rs1 - rs2, pc += 4
                printf("SUB x%d, x%d, x%d\n", rd, rs1, rs2);
                cpu->reg[rd] = cpu->reg[rs1] - cpu->reg[rs2];
                break;
            default:
                break;
            }
            break;
        case 0b001: // SLL
            // rd <- rs1 << (rs2 % XLEN), pc += 4
            printf("SLL x%d, x%d, x%d\n", rd, rs1, rs2);
            cpu->reg[rd] = cpu->reg[rs1] << (cpu->reg[rs2] % 32);
            break;
        case 0b010: // SLT
            // rd <- (rs1 < rs2) ? 1 : 0, pc += 4
            printf("SLT x%d, x%d, x%d\n", rd, rs1, rs2);
            cpu->reg[rd] = ((int32_t)cpu->reg[rs1] < (int32_t)cpu->reg[rs2]) ? 1 : 0;
            break;
        case 0b011: // SLTU
            // rd <- (rs1 < rs2) ? 1 : 0, pc += 4
            printf("SLTU x%d, x%d, x%d\n", rd, rs1, rs2);
            cpu->reg[rd] = (cpu->reg[rs1] < cpu->reg[rs2]) ? 1 : 0;
            break;
        case 0b100: // XOR
            // rd <- rs1 ^ rs2, pc += 4
            printf("XOR x%d, x%d, x%d\n", rd, rs1, rs2);
            cpu->reg[rd] = cpu->reg[rs1] ^ cpu->reg[rs2];
            break;
        case 0b101: // SRL / SRA
            switch (funct7)
            {
            case 0b0000000: // SRL
                // rd <- rs1 >> (rs2 % XLEN), pc += 4
                printf("SRL x%d, x%d, x%d\n", rd, rs1, rs2);
                cpu->reg[rd] = cpu->reg[rs1] >> (cpu->reg[rs2] % 32);
                break;
            case 0b0100000: // SRA
                // rd <- rs1 >> (rs2 % XLEN), pc += 4
                printf("SRA x%d, x%d, x%d\n", rd, rs1, rs2);
                cpu->reg[rd] = (int32_t)cpu->reg[rs1] >> (cpu->reg[rs2] % 32);
            default:
                break;
            }
            break;
        case 0b110: // OR
            // rd <- rs1 | rs2, pc += 4
            printf("OR x%d, x%d, x%d\n", rd, rs1, rs2);
            cpu->reg[rd] = cpu->reg[rs1] | cpu->reg[rs2];
            break;
        case 0b111: // AND
            // rd <- rs1 & rs2, pc += 4
            printf("AND x%d, x%d, x%d\n", rd, rs1, rs2);
            cpu->reg[rd] = cpu->reg[rs1] & cpu->reg[rs2];
            break;
        default:
            break;
        }
    }
    break;

    case OP_MISCMEM: {
        uint32_t funct3 = (instruction >> 12) & 7;
        if (funct3 == 0b000)
        {
            printf("FENCE\n");
            // TODO:
        }
        else
        {
        }
    }
    break;

    case OP_SYSTEM: {
        if ((instruction & 0b00000000000011111111111110000000) == 0)
        {
            int32_t imm = (int32_t)instruction >> 20;
            switch (imm)
            {
            case 0b000000000000:
                printf("ECALL\n");
                // TODO:
                break;
            case 0b000000000001:
                printf("EBREAK\n");
                // TODO:
                break;
            default:
                break;
            }
        }
        else
        {
        }
    }
    break;

    default:
        break;
    }
}

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
    Decode(&cpu, (123 << 20) | (1 << 15) | (0b000 < 12) | (2 << 7) | OP_JALR);
    Decode(&cpu, (-123 << 20) | (1 << 15) | (0b000 < 12) | (2 << 7) | OP_JALR);

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
    Decode(&cpu, 0xfe010113); // 13 01 01 fe   addi    sp, sp, -32      ADDI x2, x2, -32
    Decode(&cpu, 0x00112e23); // 23 2e 11 00   sw      ra, 28(sp)       SW x1, 28(x2)
    Decode(&cpu, 0x00812c23); // 23 2c 81 00   sw      s0, 24(sp)       SW x8, 24(x2)
    Decode(&cpu, 0x02010413); // 13 04 01 02   addi    s0, sp, 32       ADDI x8, x2, 32
    Decode(&cpu, 0x00000613); // 13 06 00 00   mv      a2, zero         ADDI x12, x0, 0
    Decode(&cpu, 0xfec42a23); // 23 2a c4 fe   sw      a2, -12(s0)      SW x12, -12(x8)
    Decode(&cpu, 0xfea42823); // 23 28 a4 fe   sw      a0, -16(s0)      SW x10, -16(x8)
    Decode(&cpu, 0xfeb42623); // 23 26 b4 fe   sw      a1, -20(s0)      SW x11, -20(x8)
    Decode(&cpu, 0x00001537); // 37 15 00 00   lui     a0, 1            LUI x10, 1
    Decode(&cpu, 0x24350513); // 13 05 35 24   addi    a0, a0, 579      ADDI x10, x10, 579
    Decode(&cpu, 0x01812403); // 03 24 81 01   lw      s0, 24(sp)       LW x8, 24(x2)
    Decode(&cpu, 0x01c12083); // 83 20 c1 01   lw      ra, 28(sp)       LW x1, 28(x2)
    Decode(&cpu, 0x02010113); // 13 01 01 02   addi    sp, sp, 32       ADDI x2, x2, 32
    Decode(&cpu, 0x00008067); // 67 80 00 00   ret                      JALR x0, x1, 0
}
