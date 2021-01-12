#include <stdint.h>
#include <stdio.h>

typedef struct CPU
{
    uint32_t pc;
    uint32_t reg[32];
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
    return ((int32_t)instruction & 0xfe000000) >> 20 // inst[31:25]
            | (instruction & 0x00000f80) >> 7        // inst[11:7]
            ;
}

static inline int32_t BImmediate(uint32_t instruction)
{
    return ((int32_t)instruction & 0x80000000) >> 19 // inst[31]
            | (instruction & 0x00000080) << 4        // inst[7]
            | (instruction & 0x7e000000) >> 20       // inst[30:25]
            | (instruction & 0x00000f00) >> 7        // inst[11:8]
            ;
}

static inline int32_t UImmediate(uint32_t instruction)
{
    return instruction & 0xfffff000; // inst[31:12]
}

static inline int32_t JImmediate(uint32_t instruction)
{
    return ((int32_t)instruction & 0x80000000) >> 11 // inst[31]
            | (instruction & 0x000ff000)             // inst[19:12]
            | (instruction & 0x00100000) >> 9        // inst[20]
            | (instruction & 0x7fe00000) >> 20       // inst[30:21]
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
        printf("LUI r%d, %d\n", rd, upper >> 12);
    }
    break;

    case OP_AUIPC: {
        // rd <- pc + imm_u, pc += 4
        int32_t upper = UImmediate(instruction);
        printf("AUIPC r%d, %d\n", rd, upper >> 12);
    }
    break;

    case OP_JAL: {
        // rd <- pc + 4, pc <- pc + imm_j
        int32_t imm = JImmediate(instruction);
        printf("JAL r%d, %d\n", rd, imm);
    }
    break;

    case OP_JALR: {
        // rd <- pc + 4, pc <- (rs1 + imm_i) & ~1
        int32_t imm = IImmediate(instruction);
        uint32_t funct3 = (instruction >> 12) & 7;
        if (funct3 == 0b000)
        {
            printf("JALR r%d, r%d, %d\n", rd, rs1, imm);
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
            printf("BEQ r%d, r%d, %d\n", rs1, rs2, imm);
            break;
        case 0b001: // BNE
            // pc <- pc + ((rs1 != rs2) ? imm_b : 4)
            printf("BNE r%d, r%d, %d\n", rs1, rs2, imm);
            break;
        case 0b100: // BLT
            // pc <- pc + ((rs1 < rs2) ? imm_b : 4)
            printf("BLT r%d, r%d, %d\n", rs1, rs2, imm);
            break;
        case 0b101: // BGE
            // pc <- pc + ((rs1 >= rs2) ? imm_b : 4)
            printf("BGE r%d, r%d, %d\n", rs1, rs2, imm);
            break;
        case 0b110: // BLTU
            // pc <- pc + ((rs1 < rs2) ? imm_b : 4)
            printf("BLTU r%d, r%d, %d\n", rs1, rs2, imm);
            break;
        case 0b111: // BGEU
            // pc <- pc + ((rs1 >= rs2) ? imm_b : 4)
            printf("BGEU r%d, r%d, %d\n", rs1, rs2, imm);
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
            printf("LB r%d, %d(r%d)\n", rd, imm, rs1);
            break;
        case 0b001: // LH
            // rd <- sx(m16(rs1 + imm_i)), pc += 4
            printf("LH r%d, %d(r%d)\n", rd, imm, rs1);
            break;
        case 0b010: // LW
            // rd <- sx(m32(rs1 + imm_i)), pc += 4
            printf("LW r%d, %d(r%d)\n", rd, imm, rs1);
            break;
        case 0b100: // LBU
            // rd <- zx(m8(rs1 + imm_i)), pc += 4
            printf("LBU r%d, %d(r%d)\n", rd, imm, rs1);
            break;
        case 0b101: // LHU
            // rd <- zx(m16(rs1 + imm_i)), pc += 4
            printf("LHU r%d, %d(r%d)\n", rd, imm, rs1);
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
            printf("SB r%d, %d(r%d)\n", rs2, imm, rs1);
            break;
        case 0b001: // SH
            // m16(rs1 + imm_s) <- rs2[15:0], pc += 4
            printf("SH r%d, %d(r%d)\n", rs2, imm, rs1);
            break;
        case 0b010: // SW
            // m32(rs1 + imm_s) <- rs2[31:0], pc += 4
            printf("SW r%d, %d(r%d)\n", rs2, imm, rs1);
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
            printf("ADDI r%d, r%d, %d\n", rd, rs1, imm);
            break;
        case 0b010: // SLTI
            // rd <- (rs1 < imm_i) ? 1 : 0, pc += 4
            printf("SLTI r%d, r%d, %d\n", rd, rs1, imm);
            break;
        case 0b011: // SLTIU
            // rd <- (rs1 < imm_i) ? 1 : 0, pc += 4
            printf("SLTIU r%d, r%d, %u\n", rd, rs1, imm);
            break;
        case 0b100: // XORI
            // rd <- rs1 ^ imm_i, pc += 4
            printf("XORI r%d, r%d, %d\n", rd, rs1, imm);
            break;
        case 0b110: // ORI
            // rd <- rs1 | imm_i, pc += 4
            printf("ORI r%d, r%d, %d\n", rd, rs1, imm);
            break;
        case 0b111: // ANDI
            // rd <- rs1 & imm_i, pc += 4
            printf("ANDI r%d, r%d, %d\n", rd, rs1, imm);
            break;
        case 0b001: // SLLI / SRAI
            // rd <- rs1 << shamt_i, pc += 4
            printf("SLLI r%d, r%d, %d\n", rd, rs1, shamt);
            break;
        case 0b101:
            switch (funct7)
            {
            case 0b0000000: // SRLI
                // rd <- rs1 >> shamt_i, pc += 4
                printf("SRLI r%d, r%d, %d\n", rd, rs1, shamt);
                break;
            case 0b0100000: // SRAI
                // rd <- rs1 >> shamt_i, pc += 4
                printf("SRAI r%d, r%d, %d\n", rd, rs1, shamt);
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
                printf("ADD r%d, r%d, r%d\n", rd, rs1, rs2);
                break;
            case 0b0000001: // SUB
                // rd <- rs1 - rs2, pc += 4
                printf("SUB r%d, r%d, r%d\n", rd, rs1, rs2);
                break;
            default:
                break;
            }
            break;
        case 0b001: // SLL
            // rd <- rs1 << (rs2 % XLEN), pc += 4
            printf("SLL r%d, r%d, r%d\n", rd, rs1, rs2);
            break;
        case 0b010: // SLT
            // rd <- (rs1 < rs2) ? 1 : 0, pc += 4
            printf("SLT r%d, r%d, r%d\n", rd, rs1, rs2);
            break;
        case 0b011: // SLTU
            // rd <- (rs1 < rs2) ? 1 : 0, pc += 4
            printf("SLTU r%d, r%d, r%d\n", rd, rs1, rs2);
            break;
        case 0b100: // XOR
            // rd <- rs1 ^ rs2, pc += 4
            printf("XOR r%d, r%d, r%d\n", rd, rs1, rs2);
            break;
        case 0b101: // SRL / SRA
            switch (funct7)
            {
            case 0b0000000: // SRL
                // rd <- rs1 >> (rs2 % XLEN), pc += 4
                printf("SRL r%d, r%d, r%d\n", rd, rs1, rs2);
                break;
            case 0b0100000: // SRA
                // rd <- rs1 >> (rs2 % XLEN), pc += 4
                printf("SRA r%d, r%d, r%d\n", rd, rs1, rs2);
            default:
                break;
            }
            break;
        case 0b110: // OR
            // rd <- rs1 | rs2, pc += 4
            printf("OR r%d, r%d, r%d\n", rd, rs1, rs2);
            break;
        case 0b111: // AND
            // rd <- rs1 & rs2, pc += 4
            printf("AND r%d, r%d, r%d\n", rd, rs1, rs2);
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
            // TODO: fm, pred, succ.
            printf("FENCE\n");
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
                break;
            case 0b000000000001:
                printf("EBREAK\n");
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

int main()
{
    CPU cpu;

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

    return 0;
}
