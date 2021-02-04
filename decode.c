#include "decode.h"

#include <stdint.h>

#define BDR_TRACE_ENABLED

#if defined(BDR_TRACE_ENABLED)
#include <math.h>
#include <stdio.h>
#define TRACE(...)                                                                                                                 \
    do                                                                                                                             \
    {                                                                                                                              \
        printf(__VA_ARGS__);                                                                                                       \
    } while (0)
#else
#define TRACE(...)                                                                                                                 \
    do                                                                                                                             \
    {                                                                                                                              \
    } while (0)
#endif

static char* abiNames[] = {"zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
                           "a6",   "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

static char* roundingModes[] = {"rne", "rtz", "rdn", "rup", "rmm", "reserved5", "reserved6", "dyn"};

static inline int32_t IImmediate(uint32_t instruction)
{
    return (int32_t)instruction >> 20; // inst[31:20] -> sext(imm[11:0])
}

static inline int32_t SImmediate(uint32_t instruction)
{
    return ((int32_t)(instruction & 0xfe000000) >> 20) // inst[31:25] -> sext(imm[11:5])
            | (instruction & 0x00000f80) >> 7          // inst[11:7]  -> imm[4:0]
            ;
}

static inline int32_t BImmediate(uint32_t instruction)
{
    return ((int32_t)(instruction & 0x80000000) >> 19) // inst[31]    -> sext(imm[12])
            | (instruction & 0x00000080) << 4          // inst[7]     -> imm[11]
            | (instruction & 0x7e000000) >> 20         // inst[30:25] -> imm[10:5]
            | (instruction & 0x00000f00) >> 7          // inst[11:8]  -> imm[4:1]
            ;
}

static inline int32_t UImmediate(uint32_t instruction)
{
    return instruction & 0xfffff000; // inst[31:12] -> imm[31:12]
}

static inline int32_t JImmediate(uint32_t instruction)
{
    return ((int32_t)(instruction & 0x80000000) >> 11) // inst[31]    -> sext(imm[20])
            | (instruction & 0x000ff000)               // inst[19:12] -> imm[19:12]
            | (instruction & 0x00100000) >> 9          // inst[20]    -> imm[11]
            | (instruction & 0x7fe00000) >> 20         // inst[30:21] -> imm[10:1]
            ;
}

static inline uint32_t Funct3(uint32_t instruction)
{
    return (instruction >> 12) & 7;
}

static inline uint32_t Funct7(uint32_t instruction)
{
    return instruction >> 25;
}

static inline uint32_t Funct12(uint32_t instruction)
{
    return instruction >> 20;
}

static inline uint32_t Opcode(uint32_t instruction)
{
    return instruction & 0x7f;
}

static inline uint32_t Rd(uint32_t instruction)
{
    return (instruction >> 7) & 0x1f;
}

static inline uint32_t Rs1(uint32_t instruction)
{
    return (instruction >> 15) & 0x1f;
}

static inline uint32_t Rs2(uint32_t instruction)
{
    return (instruction >> 20) & 0x1f;
}

static inline uint32_t Rs3(uint32_t instruction)
{
    return (instruction >> 27) & 0x1f;
}

static inline uint32_t Rm(uint32_t instruction)
{
    return (instruction >> 12) & 7;
}

void Reset(CPU* cpu, uint32_t sp)
{
    cpu->pc = 0;
    for (int i = 0; i < 32; i++)
    {
        cpu->xreg[i] = 0;
    }
    cpu->xreg[2] = sp;
    cpu->mepc = 0;
    cpu->mcause = 0;
    cpu->mtval = 0;
}

// See: http://www.five-embeddev.com/riscv-isa-manual/latest/gmaps.html#rv3264g-instruction-set-listings
// or riscv-spec-209191213.pdf.
CpuResult Decode(CPU* cpu, uint32_t instruction)
{
    uint32_t opcode = Opcode(instruction);
    uint32_t rd = Rd(instruction);
    uint32_t rs1 = Rs1(instruction);

    switch (opcode)
    {
    case OP_LUI: {
        // rd <- imm_u, pc += 4
        int32_t upper = UImmediate(instruction);
        TRACE("LUI %s, %d\n", abiNames[rd], upper >> 12);
        cpu->xreg[rd] = upper;
        cpu->pc += 4;
        cpu->xreg[0] = 0;
    }
    break;

    case OP_AUIPC: {
        // rd <- pc + imm_u, pc += 4
        int32_t upper = UImmediate(instruction);
        TRACE("AUIPC %s, %d\n", abiNames[rd], upper >> 12);
        cpu->xreg[rd] = cpu->pc + upper;
        cpu->pc += 4;
        cpu->xreg[0] = 0;
    }
    break;

    case OP_JAL: {
        // rd <- pc + 4, pc <- pc + imm_j
        int32_t imm = JImmediate(instruction);
        TRACE("JAL %s, %d\n", abiNames[rd], imm);
        cpu->xreg[rd] = cpu->pc + 4;
        cpu->pc += imm;
        cpu->xreg[0] = 0;
    }
    break;

    case OP_JALR: {
        // rd <- pc + 4, pc <- (rs1 + imm_i) & ~1
        int32_t imm = IImmediate(instruction);
        uint32_t funct3 = Funct3(instruction);
        if (funct3 == 0b000)
        {
            TRACE("JALR %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
            uint32_t rs1Before = cpu->xreg[rs1]; // Because rd and rs1 might be the same register.
            cpu->xreg[rd] = cpu->pc + 4;
            cpu->pc = (rs1Before + imm) & ~1;
            cpu->xreg[0] = 0;
        }
        else
        {
            return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
        }
    }
    break;

    case OP_BRANCH: {
        uint32_t funct3 = Funct3(instruction);
        uint32_t rs2 = Rs2(instruction);
        int32_t imm = BImmediate(instruction);
        switch (funct3)
        {
        case 0b000: // BEQ
            // pc <- pc + ((rs1 == rs2) ? imm_b : 4)
            TRACE("BEQ %s, %s, %d\n", abiNames[rs1], abiNames[rs2], imm);
            cpu->pc += ((cpu->xreg[rs1] == cpu->xreg[rs2]) ? imm : 4);
            break;
        case 0b001: // BNE
            // pc <- pc + ((rs1 != rs2) ? imm_b : 4)
            TRACE("BNE %s, %s, %d\n", abiNames[rs1], abiNames[rs2], imm);
            cpu->pc += ((cpu->xreg[rs1] != cpu->xreg[rs2]) ? imm : 4);
            break;
        case 0b100: // BLT
            // pc <- pc + ((rs1 < rs2) ? imm_b : 4)
            TRACE("BLT %s, %s, %d\n", abiNames[rs1], abiNames[rs2], imm);
            cpu->pc += (((int32_t)cpu->xreg[rs1] < (int32_t)cpu->xreg[rs2]) ? imm : 4);
            break;
        case 0b101: // BGE
            // pc <- pc + ((rs1 >= rs2) ? imm_b : 4)
            TRACE("BGE %s, %s, %d\n", abiNames[rs1], abiNames[rs2], imm);
            cpu->pc += (((int32_t)cpu->xreg[rs1] >= (int32_t)cpu->xreg[rs2]) ? imm : 4);
            break;
        case 0b110: // BLTU
            // pc <- pc + ((rs1 < rs2) ? imm_b : 4)
            TRACE("BLTU %s, %s, %d\n", abiNames[rs1], abiNames[rs2], imm);
            cpu->pc += ((cpu->xreg[rs1] < cpu->xreg[rs2]) ? imm : 4);
            break;
        case 0b111: // BGEU
            // pc <- pc + ((rs1 >= rs2) ? imm_b : 4)
            TRACE("BGEU %s, %s, %d\n", abiNames[rs1], abiNames[rs2], imm);
            cpu->pc += ((cpu->xreg[rs1] >= cpu->xreg[rs2]) ? imm : 4);
            break;
        default:
            return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
        }
    }
    break;

    case OP_LOAD: {
        uint32_t funct3 = Funct3(instruction);
        int32_t imm = IImmediate(instruction);
        switch (funct3)
        {
        case 0b000: { // LB
            // rd <- sx(m8(rs1 + imm_i)), pc += 4
            TRACE("LB %s, %d(%s)\n", abiNames[rd], imm, abiNames[rs1]);
            CpuResult byteResult = ReadByte(cpu->memory, cpu->xreg[rs1] + imm);
            if (!ResultIsByte(byteResult))
            {
                return byteResult;
            }
            cpu->xreg[rd] = (int32_t)(int16_t)(int8_t)ResultAsByte(byteResult);
            cpu->pc += 4;
            cpu->xreg[0] = 0;
        }
        break;
        case 0b001: { // LH
            // rd <- sx(m16(rs1 + imm_i)), pc += 4
            TRACE("LH %s, %d(%s)\n", abiNames[rd], imm, abiNames[rs1]);
            CpuResult halfwordResult = ReadHalfword(cpu->memory, cpu->xreg[rs1] + imm);
            if (!ResultIsHalfword(halfwordResult))
            {
                return halfwordResult;
            }
            cpu->xreg[rd] = (int32_t)(int16_t)ResultAsHalfword(halfwordResult);
            cpu->pc += 4;
            cpu->xreg[0] = 0;
        }
        break;
        case 0b010: { // LW
            // rd <- sx(m32(rs1 + imm_i)), pc += 4
            TRACE("LW %s, %d(%s)\n", abiNames[rd], imm, abiNames[rs1]);
            CpuResult wordResult = ReadWord(cpu->memory, cpu->xreg[rs1] + imm);
            if (!ResultIsWord(wordResult))
            {
                return wordResult;
            }
            cpu->xreg[rd] = (int32_t)ResultAsWord(wordResult);
            cpu->pc += 4;
            cpu->xreg[0] = 0;
        }
        break;
        case 0b100: { // LBU
            // rd <- zx(m8(rs1 + imm_i)), pc += 4
            TRACE("LBU x%d, %d(x%d)\n", rd, imm, rs1);
            CpuResult byteResult = ReadByte(cpu->memory, cpu->xreg[rs1] + imm);
            if (!ResultIsByte(byteResult))
            {
                return byteResult;
            }
            cpu->xreg[rd] = ResultAsByte(byteResult);
            cpu->pc += 4;
            cpu->xreg[0] = 0;
        }
        break;
        case 0b101: { // LHU
            // rd <- zx(m16(rs1 + imm_i)), pc += 4
            TRACE("LHU %s, %d(%s)\n", abiNames[rd], imm, abiNames[rs1]);
            CpuResult halfwordResult = ReadHalfword(cpu->memory, cpu->xreg[rs1] + imm);
            if (!ResultIsHalfword(halfwordResult))
            {
                return halfwordResult;
            }
            cpu->xreg[rd] = ResultAsHalfword(halfwordResult);
            cpu->pc += 4;
            cpu->xreg[0] = 0;
        }
        break;
        default:
            return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
        }
    }
    break;

    case OP_STORE: {
        uint32_t funct3 = Funct3(instruction);
        uint32_t rs2 = Rs2(instruction);
        int32_t imm = SImmediate(instruction);
        switch (funct3)
        {
        case 0b000: // SB
            // m8(rs1 + imm_s) <- rs2[7:0], pc += 4
            TRACE("SB %s, %d(%s)\n", abiNames[rs2], imm, abiNames[rs1]);
            WriteByte(cpu->memory, cpu->xreg[rs1] + imm, cpu->xreg[rs2] & 0xff);
            cpu->pc += 4;
            break;
        case 0b001: // SH
            // m16(rs1 + imm_s) <- rs2[15:0], pc += 4
            TRACE("SH %s, %d(%s)\n", abiNames[rs2], imm, abiNames[rs1]);
            WriteHalfword(cpu->memory, cpu->xreg[rs1] + imm, cpu->xreg[rs2] & 0xffff);
            cpu->pc += 4;
            break;
        case 0b010: // SW
            // m32(rs1 + imm_s) <- rs2[31:0], pc += 4
            TRACE("SW %s, %d(%s)\n", abiNames[rs2], imm, abiNames[rs1]);
            WriteWord(cpu->memory, cpu->xreg[rs1] + imm, cpu->xreg[rs2]);
            cpu->pc += 4;
            break;
        default:
            return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
        }
    }
    break;

    case OP_OPIMM: {
        uint32_t funct3 = Funct3(instruction);
        int32_t imm = IImmediate(instruction);
        uint32_t shamt = imm & 0x1f;
        uint32_t funct7 = Funct7(instruction);
        switch (funct3)
        {
        case 0b000: // ADDI
            // rd <- rs1 + imm_i, pc += 4
            TRACE("ADDI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
            cpu->xreg[rd] = cpu->xreg[rs1] + imm;
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            break;
        case 0b010: // SLTI
            // rd <- (rs1 < imm_i) ? 1 : 0, pc += 4
            TRACE("SLTI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
            cpu->xreg[rd] = ((int32_t)cpu->xreg[rs1] < imm) ? 1 : 0;
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            break;
        case 0b011: // SLTIU
            // rd <- (rs1 < imm_i) ? 1 : 0, pc += 4
            TRACE("SLTIU %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
            cpu->xreg[rd] = (cpu->xreg[rs1] < (uint32_t)imm) ? 1 : 0;
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            break;
        case 0b100: // XORI
            // rd <- rs1 ^ imm_i, pc += 4
            TRACE("XORI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
            cpu->xreg[rd] = cpu->xreg[rs1] ^ imm;
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            break;
        case 0b110: // ORI
            // rd <- rs1 | imm_i, pc += 4
            TRACE("ORI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
            cpu->xreg[rd] = cpu->xreg[rs1] | imm;
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            break;
        case 0b111: // ANDI
            // rd <- rs1 & imm_i, pc += 4
            TRACE("ANDI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
            cpu->xreg[rd] = cpu->xreg[rs1] & imm;
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            break;
        case 0b001: // SLLI
            // rd <- rs1 << shamt_i, pc += 4
            TRACE("SLLI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
            cpu->xreg[rd] = cpu->xreg[rs1] << shamt;
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            break;
        case 0b101:
            switch (funct7)
            {
            case 0b0000000: // SRLI
                // rd <- rs1 >> shamt_i, pc += 4
                TRACE("SRLI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
                cpu->xreg[rd] = cpu->xreg[rs1] >> shamt;
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            case 0b0100000: // SRAI
                // rd <- rs1 >> shamt_i, pc += 4
                TRACE("SRAI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
                cpu->xreg[rd] = (int32_t)cpu->xreg[rs1] >> shamt;
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            default:
                return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
            }
            break;
        default:
            return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
        }
    }
    break;

    case OP_OP: {
        uint32_t funct3 = Funct3(instruction);
        uint32_t rs2 = Rs2(instruction);
        uint32_t funct7 = Funct7(instruction);
        switch (funct3)
        {
        case 0b000:
            switch (funct7)
            {
            case 0b0000000: // ADD
                // rd <- rs1 + rs2, pc += 4
                TRACE("ADD %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                cpu->xreg[rd] = cpu->xreg[rs1] + cpu->xreg[rs2];
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            case 0b0100000: // SUB
                // rd <- rs1 - rs2, pc += 4
                TRACE("SUB %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                cpu->xreg[rd] = cpu->xreg[rs1] - cpu->xreg[rs2];
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            case 0b00000001: // MUL (RV32M)
                TRACE("MUL %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                cpu->xreg[rd] = cpu->xreg[rs1] * cpu->xreg[rs2];
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            default:
                return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
            }
            break;

        case 0b001:
            switch (funct7)
            {
            case 0b0000000: // SLL
                // rd <- rs1 << (rs2 % XLEN), pc += 4
                TRACE("SLL %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                cpu->xreg[rd] = cpu->xreg[rs1] << (cpu->xreg[rs2] % 32);
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            case 0b00000001: // MULH (RV32M)
                TRACE("MULH %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                int64_t t = (int64_t)(int32_t)cpu->xreg[rs1] * (int64_t)(int32_t)cpu->xreg[rs2];
                cpu->xreg[rd] = t >> 32;
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            default:
                return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
            }
            break;

        case 0b010:
            switch (funct7)
            {
            case 0b0000000: // SLT
                // rd <- (rs1 < rs2) ? 1 : 0, pc += 4
                TRACE("SLT %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                cpu->xreg[rd] = ((int32_t)cpu->xreg[rs1] < (int32_t)cpu->xreg[rs2]) ? 1 : 0;
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            case 0b00000001: // MULHSU (RV32M)
                TRACE("MULHSU %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                int64_t t = (int64_t)(int32_t)cpu->xreg[rs1] * (uint64_t)cpu->xreg[rs2];
                cpu->xreg[rd] = t >> 32;
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            default:
                return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
            }
            break;

        case 0b011:
            switch (funct7)
            {
            case 0b0000000: // SLTU
                // rd <- (rs1 < rs2) ? 1 : 0, pc += 4
                TRACE("SLTU %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                cpu->xreg[rd] = (cpu->xreg[rs1] < cpu->xreg[rs2]) ? 1 : 0;
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                cpu->xreg[0] = 0;
                break;
            case 0b00000001: // MULHU (RV32M)
                TRACE("MULHU %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                uint64_t t = (uint64_t)cpu->xreg[rs1] * (uint64_t)cpu->xreg[rs2];
                cpu->xreg[rd] = t >> 32;
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            default:
                return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
            }
            break;

        case 0b100:
            switch (funct7)
            {
            case 0b0000000: // XOR
                // rd <- rs1 ^ rs2, pc += 4
                TRACE("XOR %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                cpu->xreg[rd] = cpu->xreg[rs1] ^ cpu->xreg[rs2];
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            case 0b00000001: // DIV (RV32M)
                TRACE("DIV %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                const int32_t dividend = (int32_t)cpu->xreg[rs1];
                const int32_t divisor = (int32_t)cpu->xreg[rs2];
                // Check for signed division overflow.
                if (dividend != 0x80000000 || divisor != -1)
                {
                    cpu->xreg[rd] = divisor != 0 // Check for division by zero.
                            ? dividend / divisor
                            : -1;
                }
                else
                {
                    // Signed division overflow occurred.
                    cpu->xreg[rd] = dividend;
                }
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            default:
                return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
            }
            break;

        case 0b101:
            switch (funct7)
            {
            case 0b0000000: // SRL
                // rd <- rs1 >> (rs2 % XLEN), pc += 4
                TRACE("SRL %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                cpu->xreg[rd] = cpu->xreg[rs1] >> (cpu->xreg[rs2] % 32);
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            case 0b0100000: // SRA
                // rd <- rs1 >> (rs2 % XLEN), pc += 4
                TRACE("SRA %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                cpu->xreg[rd] = (int32_t)cpu->xreg[rs1] >> (cpu->xreg[rs2] % 32);
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            case 0b00000001: // DIVU (RV32M)
                TRACE("DIVU %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                uint32_t divisor = cpu->xreg[rs2];
                cpu->xreg[rd] = divisor != 0 ? cpu->xreg[rs1] / divisor : 0xffffffff;
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            default:
                return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
            }
            break;

        case 0b110:
            switch (funct7)
            {
            case 0b0000000: // OR
                // rd <- rs1 | rs2, pc += 4
                TRACE("OR %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                cpu->xreg[rd] = cpu->xreg[rs1] | cpu->xreg[rs2];
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            case 0b00000001: // REM
                TRACE("REM %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                const int32_t dividend = (int32_t)cpu->xreg[rs1];
                const int32_t divisor = (int32_t)cpu->xreg[rs2];
                // Check for signed division overflow.
                if (dividend != 0x80000000 || divisor != -1)
                {
                    cpu->xreg[rd] = divisor != 0 // Check for division by zero.
                            ? dividend % divisor
                            : dividend;
                }
                else
                {
                    // Signed division overflow occurred.
                    cpu->xreg[rd] = 0;
                }
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            default:
                return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
            }
            break;

        case 0b111:
            switch (funct7)
            {
            case 0b0000000: // AND
                // rd <- rs1 & rs2, pc += 4
                TRACE("AND %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                cpu->xreg[rd] = cpu->xreg[rs1] & cpu->xreg[rs2];
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            case 0b00000001: // REMU
                TRACE("REMU %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                const uint32_t dividend = cpu->xreg[rs1];
                const uint32_t divisor = cpu->xreg[rs2];
                cpu->xreg[rd] = divisor != 0 ? cpu->xreg[rs1] % divisor : dividend;
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            default:
                return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
            }
            break;

        default:
            return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
        }
    }
    break;

    case OP_MISCMEM: {
        uint32_t funct3 = Funct3(instruction);
        if (funct3 == 0b000)
        {
            TRACE("FENCE\n");
            return MakeTrap(trNOT_IMPLEMENTED_YET, 0);
        }
        else
        {
            return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
        }
    }

    case OP_SYSTEM:
        if ((instruction & 0b00000000000011111111111110000000) == 0)
        {
            uint32_t funct12 = Funct12(instruction);
            switch (funct12)
            {
            case 0b000000000000: // ECALL
                TRACE("ECALL\n");
                return MakeTrap(trNOT_IMPLEMENTED_YET, 0);

            case 0b000000000001: // EBREAK
                TRACE("EBREAK\n");
                return MakeTrap(trBREAKPOINT, 0); // TODO: what should value be here?

            case 0b000000000010: // URET
                TRACE("URET\n");
                // TODO: Only provide this if user mode traps are supported, otherwise raise an illegal instruction exception.
                return MakeTrap(trNOT_IMPLEMENTED_YET, 0);

            case 0b000100000010: // SRET
                TRACE("SRET\n");
                // TODO: Only provide this if supervisor mode is supported, otherwise raise an illegal instruction exception.
                return MakeTrap(trNOT_IMPLEMENTED_YET, 0);

            case 0b001100000010: // MRET
                // pc <- mepc, pc += 4
                TRACE("MRET\n");
                cpu->pc = cpu->mepc; // Restore the program counter from the machine exception program counter.
                cpu->pc += 4;        // ...and increment it as normal.
                break;

            default:
                return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
            }
        }
        else
        {
            return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
        }

    case OP_LOADFP: { // Floating point load (RV32F)
        uint32_t funct3 = Funct3(instruction);
        if (funct3 == 0b010) // FLW
        {
            int32_t imm = IImmediate(instruction);
            TRACE("FLW f%d, %d(%s)", rd, imm, abiNames[rs1]);
            // TODO: implement.
            return MakeTrap(trNOT_IMPLEMENTED_YET, 0);
        }
        else
        {
            return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
        }
    }

    case OP_STOREFP: { // Floating point store (RV32F)
        uint32_t funct3 = Funct3(instruction);
        if (funct3 == 0b010) // FSW
        {
            uint32_t rs2 = Rs2(instruction);
            int32_t imm = SImmediate(instruction);
            TRACE("FSW f%d, %d(%s)\n", rs2, imm, abiNames[rs1]);
            // TODO: implement.
            return MakeTrap(trNOT_IMPLEMENTED_YET, 0);
        }
        else
        {
            return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
        }
    }

    case OP_MADD: { // Floating point fused multiply-add (RV32F)
        uint32_t rm = Rm(instruction);
        uint32_t rs2 = Rs2(instruction);
        uint32_t rs3 = Rs3(instruction);
        if (((instruction >> 25) & 0b11) == 0) // FMADD.S
        {
            // rd <- (rs1 x rs2) + rs3
            TRACE("FMADD.S f%d, f%d, f%d, f%d, %s", rd, rs1, rs2, rs3, roundingModes[rm]);
            cpu->freg[rd] = (cpu->freg[rs1] * cpu->freg[rs2]) + cpu->freg[rs3];
            cpu->pc += 4;
            // TODO: rounding.
            break;
        }
        else
        {
            return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
        }
    }

    case OP_MSUB: { // Floating point fused multiply-sub (RV32F)
        uint32_t rm = Rm(instruction);
        uint32_t rs2 = Rs2(instruction);
        uint32_t rs3 = Rs3(instruction);
        if (((instruction >> 25) & 0b11) == 0) // FMSUB.S
        {
            // rd <- (rs1 x rs2) - rs3
            TRACE("FMSUB.S f%d, f%d, f%d, f%d, %s", rd, rs1, rs2, rs3, roundingModes[rm]);
            cpu->freg[rd] = (cpu->freg[rs1] * cpu->freg[rs2]) - cpu->freg[rs3];
            cpu->pc += 4;
            // TODO: rounding.
            break;
        }
        else
        {
            return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
        }
    }

    case OP_NMSUB: { // Floating point negated fused multiply-sub (RV32F)
        uint32_t rm = Rm(instruction);
        uint32_t rs2 = Rs2(instruction);
        uint32_t rs3 = Rs3(instruction);
        if (((instruction >> 25) & 0b11) == 0) // FNMSUB.S
        {
            // rd <- -(rs1 x rs2) + rs3
            TRACE("FNMSUB.S f%d, f%d, f%d, f%d, %s", rd, rs1, rs2, rs3, roundingModes[rm]);
            cpu->freg[rd] = -(cpu->freg[rs1] * cpu->freg[rs2]) + cpu->freg[rs3];
            cpu->pc += 4;
            // TODO: rounding.
            break;
        }
        else
        {
            return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
        }
    }

    case OP_NMADD: { // Floating point negated fused multiple-add (RV32F)
        uint32_t rm = Rm(instruction);
        uint32_t rs2 = Rs2(instruction);
        uint32_t rs3 = Rs3(instruction);
        if (((instruction >> 25) & 0b11) == 0) // FNMADD.S
        {
            // rd <- -(rs1 x rs2) - rs3
            cpu->freg[rd] = -(cpu->freg[rs1] * cpu->freg[rs2]) - cpu->freg[rs3];
            cpu->pc += 4;
            // TODO: rounding.
            break;
        }
        else
        {
            return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
        }
    }

    case OP_OPFP: { // Floating point operations (RV32F)
        uint32_t funct7 = Funct7(instruction);
        uint32_t funct3 = Funct3(instruction);
        uint32_t rm = funct3;
        uint32_t rs2 = Rs2(instruction);
        switch (funct7)
        {
        case 0b0000000: // FADD.S
            // rd <- rs1 + rs2
            TRACE("FADD.S f%d, f%d, f%d, %s", rd, rs1, rs2, roundingModes[rm]);
            cpu->freg[rd] = cpu->freg[rs1] + cpu->freg[rs2];
            cpu->pc += 4;
            // TODO: rounding.
            break;

        case 0b0000100: // FSUB.S
            // rd <- rs1 - rs2
            TRACE("FSUB.S f%d, f%d, f%d, %s", rd, rs1, rs2, roundingModes[rm]);
            cpu->freg[rd] = cpu->freg[rs1] - cpu->freg[rs2];
            cpu->pc += 4;
            // TODO: rounding.
            break;

        case 0b0001000: // FMUL.S
            // rd <- rs1 * rs2
            TRACE("FMUL.S f%d, f%d, f%d, %s", rd, rs1, rs2, roundingModes[rm]);
            cpu->freg[rd] = cpu->freg[rs1] * cpu->freg[rs2];
            cpu->pc += 4;
            // TODO: rounding.
            break;

        case 0b0001100: // FDIV.S
            // rd <- rs1 / rs2
            TRACE("FDIV.S f%d, f%d, f%d, %s", rd, rs1, rs2, roundingModes[rm]);
            cpu->freg[rd] = cpu->freg[rs1] / cpu->freg[rs2];
            cpu->pc += 4;
            // TODO: rounding.
            break;

        case 0b0101100:
            if (rs2 == 0b00000) // FSQRT.S
            {
                // rd <- sqrt(rs1)
                TRACE("FSQRT.S f%d, f%d, %s", rd, rs1, roundingModes[rm]);
                cpu->freg[rd] = sqrtf(cpu->freg[rs1]);
                cpu->pc += 4;
                // TODO: rounding.
                break;
            }
            else
            {
                return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
            }

        case 0b0010000: {
            switch (funct3)
            {
            case 0b000: // FSGNJ.S
                TRACE("FSGNJ.S f%d, f%d, f%d", rd, rs1, rs2);
                // TODO: implement.
                return MakeTrap(trNOT_IMPLEMENTED_YET, 0);
            case 0b001: // FSGNJN.S
                TRACE("FSGNJN.S f%d, f%d, f%d", rd, rs1, rs2);
                // TODO: implement.
                return MakeTrap(trNOT_IMPLEMENTED_YET, 0);
            case 0b010: // FSGNJX.S
                TRACE("FSGNJX.S f%d, f%d, f%d", rd, rs1, rs2);
                // TODO: implement.
                return MakeTrap(trNOT_IMPLEMENTED_YET, 0);
            default:
                return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
            }
        }

        case 0b0010100: {
            switch (funct3)
            {
            case 0b000: // FMIN.S
                // rd <- min(rs1, rs2)
                TRACE("FMIN.S f%d, f%d, f%d", rd, rs1, rs2);
                cpu->freg[rd] = fminf(cpu->freg[rs1], cpu->freg[rs2]);
                cpu->pc += 4;
                // TODO: rounding.
                break;

            case 0b001: // FMAX.S
                // rd <- max(rs1, rs2)
                TRACE("FMAX.S f%d, f%d, f%d", rd, rs1, rs2);
                cpu->freg[rd] = fmaxf(cpu->freg[rs1], cpu->freg[rs2]);
                cpu->pc += 4;
                // TODO: rounding.
                break;

            default:
                return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
            }
        }

        case 0b1100000: {
            switch (rs2) // Not actually rs2 - just the same bits.
            {
            case 0b00000:
                TRACE("FCVT.W.S r%d, f%d, %s", rd, rs1, roundingModes[rm]);
                // TODO: implement.
                return MakeTrap(trNOT_IMPLEMENTED_YET, 0);
            case 0b00001:
                TRACE("FCVT.WU.S r%d, f%d, %s", rd, rs1, roundingModes[rm]);
                // TODO: implement.
                return MakeTrap(trNOT_IMPLEMENTED_YET, 0);
            default:
                return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
            }
        }

        case 0b1110000: {
            if (rs2 == 0b00000) // Not actually rs2 - just the same bits.
            {
                switch (funct3)
                {
                case 0b000:
                    TRACE("FMV.X.W r%d, f%d", rd, rs1);
                    // TODO: implement.
                    return MakeTrap(trNOT_IMPLEMENTED_YET, 0);
                case 0b001:
                    TRACE("FCLASS.S r%d, f%d", rd, rs1);
                    // TODO: implement.
                    return MakeTrap(trNOT_IMPLEMENTED_YET, 0);
                default:
                    return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
                }
            }
            else
            {
                return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
            }
        }

        case 0b1010000: {
            switch (funct3)
            {
            case 0b010: // FEQ.S
                TRACE("FEQ.S r%d, f%d, f%d", rd, rs1, rs2);
                // TODO: implement.
                return MakeTrap(trNOT_IMPLEMENTED_YET, 0);
            case 0b001: // FLT.S
                TRACE("FLT.S r%d, f%d, f%d", rd, rs1, rs2);
                // TODO: implement.
                return MakeTrap(trNOT_IMPLEMENTED_YET, 0);
            case 0b000: // FLE.S
                TRACE("FLE.S r%d, f%d, f%d", rd, rs1, rs2);
                // TODO: implement.
                return MakeTrap(trNOT_IMPLEMENTED_YET, 0);
            default:
                return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
            }
        }

        case 0b1101000: {
            switch (rs2) // No actually rs2 - just the same bits,
            {
            case 0b00000: // FCVT.S.W
                TRACE("FCVT.S.W f%d, r%d, %s", rd, rs1, roundingModes[rm]);
                // TODO: implement.
                break;
            case 0b00001: // FVCT.S.WU
                TRACE("FVCT.S.WU f%d, r%d, %s", rd, rs1, roundingModes[rm]);
                // TODO: implement.
                break;
            default:
                return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
            }
        }

        case 0b1111000:
            if (rs2 == 0b00000 && funct3 == 0b000) // FMV.W.X
            {
                TRACE("FMV.W.X f%d, r%d", rd, rs1);
                // TODO: implement.
                return MakeTrap(trNOT_IMPLEMENTED_YET, 0);
            }
            return MakeTrap(trILLEGAL_INSTRUCTION, instruction);

        default:
            return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
        }
    }

    default:
        return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
    }

    return MakeOk();
}

CpuResult Fetch(CPU* cpu)
{
    TRACE("%08x ", cpu->pc);
    return ReadWord(cpu->memory, cpu->pc);
}

CpuResult HandleTrap(CPU* cpu, Trap trap)
{
    cpu->mepc = cpu->pc;       // Save the program counter in the machine exception program counter.
    cpu->mcause = trap.mcause; // mcause <- reason for trap.
    cpu->mtval = trap.mtval;   // mtval <- exception specific information.

    // TODO: do something sensible other than just returning the same trap.
    return MakeTrap(trap.mcause, trap.mtval);
}

CpuResult Run(CPU* cpu, int count)
{
    CpuResult result = MakeOk();
    for (int i = 0; i < count; i++)
    {
        result = Fetch(cpu);
        if (ResultIsWord(result))
        {
            result = Decode(cpu, ResultAsWord(result));
        }

        if (ResultIsTrap(result))
        {
            result = HandleTrap(cpu, ResultAsTrap(result));
            if (ResultIsTrap(result))
            {
                // Stop, as we can no longer proceed.
                break;
            }
        }
    }
    return result;
}
