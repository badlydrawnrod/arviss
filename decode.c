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
        int32_t upper = (int32_t)instruction & 0xfffff000;
        printf("LUI r%d, %d\n", rd, upper >> 12);
    }
    break;

    case OP_AUIPC: {
        int32_t upper = (int32_t)instruction & 0xfffff000;
        printf("AUIPC r%d, %d\n", rd, upper >> 12);
    }
    break;
        
    case OP_JAL:
        break;
    case OP_JALR:
        break;
    case OP_BRANCH:
        break;
    case OP_LOAD:
        break;
    case OP_STORE:
        break;

    case OP_OPIMM: {
        uint32_t funct3 = (instruction >> 12) & 7;
        int32_t imm = (int32_t)instruction >> 20;
        uint32_t shamt = imm & 0x1f;
        uint32_t bit30 = instruction >> 25;
        switch (funct3)
        {
        case 0b000: // ADDI
            printf("ADDI r%d, r%d, %d\n", rd, rs1, imm);
            break;
        case 0b010: // SLTI
            printf("SLTI r%d, r%d, %d\n", rd, rs1, imm);
            break;
        case 0b011: // SLTIU
            printf("SLTIU r%d, r%d, %u\n", rd, rs1, imm);
            break;
        case 0b100: // XORI
            printf("XORI r%d, r%d, %d\n", rd, rs1, imm);
            break;
        case 0b110: // ORI
            printf("ORI r%d, r%d, %d\n", rd, rs1, imm);
            break;
        case 0b111: // ANDI
            printf("ANDI r%d, r%d, %d\n", rd, rs1, imm);
            break;
        case 0b001: // SLLI / SRAI
            printf("SLLI r%d, r%d, %d\n", rd, rs1, shamt);
            break;
        case 0b101:
            switch (bit30)
            {
            case 0b0000000: // SRLI
                printf("SRLI r%d, r%d, %d\n", rd, rs1, shamt);
                break;
            case 0b0100000: // SRAI
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
        uint32_t bit30 = instruction >> 25;
        switch (funct3)
        {
        case 0b000:     // ADD / SUB
            if (!bit30) // ADD
            {
                printf("ADD r%d, r%d, r%d\n", rd, rs1, rs2);
            }
            else // SUB
            {
                printf("SUB r%d, r%d, r%d\n", rd, rs1, rs2);
            }
            break;
        case 0b001: // SLL
            printf("SLL r%d, r%d, r%d\n", rd, rs1, rs2);
            break;
        case 0b010: // SLT
            printf("SLT r%d, r%d, r%d\n", rd, rs1, rs2);
            break;
        case 0b011: // SLTU
            printf("SLTU r%d, r%d, r%d\n", rd, rs1, rs2);
            break;
        case 0b100: // XOR
            printf("XOR r%d, r%d, r%d\n", rd, rs1, rs2);
            break;
        case 0b101: // SRL / SRA
            switch (bit30)
            {
            case 0b0000000: // SRL
                printf("SRL r%d, r%d, r%d\n", rd, rs1, rs2);
                break;
            case 0b0100000: // SRA
                printf("SRA r%d, r%d, r%d\n", rd, rs1, rs2);
            default:
                break;
            }
            break;
        case 0b110: // OR
            printf("OR r%d, r%d, r%d\n", rd, rs1, rs2);
            break;
        case 0b111: // AND
            printf("AND r%d, r%d, r%d\n", rd, rs1, rs2);
            break;
        default:
            break;
        }
    }
    break;
    case OP_MISCMEM:
        break;
    case OP_SYSTEM:
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

    return 0;
}
