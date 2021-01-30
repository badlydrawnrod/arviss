#include "decode.h"

#include <stdint.h>
#include <stdio.h>

static char* abiNames[] = {"zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
                           "a6",   "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

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
    uint32_t opcode = instruction & 0x7f;
    uint32_t rd = (instruction >> 7) & 0x1f;
    uint32_t rs1 = (instruction >> 15) & 0x1f;

    switch (opcode)
    {
    case OP_LUI: {
        // rd <- imm_u, pc += 4
        int32_t upper = UImmediate(instruction);
        printf("LUI %s, %d\n", abiNames[rd], upper >> 12);
        cpu->xreg[rd] = upper;
        cpu->pc += 4;
        cpu->xreg[0] = 0;
    }
    break;

    case OP_AUIPC: {
        // rd <- pc + imm_u, pc += 4
        int32_t upper = UImmediate(instruction);
        printf("AUIPC %s, %d\n", abiNames[rd], upper >> 12);
        cpu->xreg[rd] = cpu->pc + upper;
        cpu->pc += 4;
        cpu->xreg[0] = 0;
    }
    break;

    case OP_JAL: {
        // rd <- pc + 4, pc <- pc + imm_j
        int32_t imm = JImmediate(instruction);
        printf("JAL %s, %d\n", abiNames[rd], imm);
        cpu->xreg[rd] = cpu->pc + 4;
        cpu->pc += imm;
        cpu->xreg[0] = 0;
    }
    break;

    case OP_JALR: {
        // rd <- pc + 4, pc <- (rs1 + imm_i) & ~1
        int32_t imm = IImmediate(instruction);
        uint32_t funct3 = (instruction >> 12) & 7;
        if (funct3 == 0b000)
        {
            printf("JALR %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
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
        uint32_t funct3 = (instruction >> 12) & 7;
        uint32_t rs2 = (instruction >> 20) & 0x1f;
        int32_t imm = BImmediate(instruction);
        switch (funct3)
        {
        case 0b000: // BEQ
            // pc <- pc + ((rs1 == rs2) ? imm_b : 4)
            printf("BEQ %s, %s, %d\n", abiNames[rs1], abiNames[rs2], imm);
            cpu->pc += ((cpu->xreg[rs1] == cpu->xreg[rs2]) ? imm : 4);
            break;
        case 0b001: // BNE
            // pc <- pc + ((rs1 != rs2) ? imm_b : 4)
            printf("BNE %s, %s, %d\n", abiNames[rs1], abiNames[rs2], imm);
            cpu->pc += ((cpu->xreg[rs1] != cpu->xreg[rs2]) ? imm : 4);
            break;
        case 0b100: // BLT
            // pc <- pc + ((rs1 < rs2) ? imm_b : 4)
            printf("BLT %s, %s, %d\n", abiNames[rs1], abiNames[rs2], imm);
            cpu->pc += (((int32_t)cpu->xreg[rs1] < (int32_t)cpu->xreg[rs2]) ? imm : 4);
            break;
        case 0b101: // BGE
            // pc <- pc + ((rs1 >= rs2) ? imm_b : 4)
            printf("BGE %s, %s, %d\n", abiNames[rs1], abiNames[rs2], imm);
            cpu->pc += (((int32_t)cpu->xreg[rs1] >= (int32_t)cpu->xreg[rs2]) ? imm : 4);
            break;
        case 0b110: // BLTU
            // pc <- pc + ((rs1 < rs2) ? imm_b : 4)
            printf("BLTU %s, %s, %d\n", abiNames[rs1], abiNames[rs2], imm);
            cpu->pc += ((cpu->xreg[rs1] < cpu->xreg[rs2]) ? imm : 4);
            break;
        case 0b111: // BGEU
            // pc <- pc + ((rs1 >= rs2) ? imm_b : 4)
            printf("BGEU %s, %s, %d\n", abiNames[rs1], abiNames[rs2], imm);
            cpu->pc += ((cpu->xreg[rs1] >= cpu->xreg[rs2]) ? imm : 4);
            break;
        default:
            return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
        }
    }
    break;

    case OP_LOAD: {
        uint32_t funct3 = (instruction >> 12) & 7;
        int32_t imm = IImmediate(instruction);
        switch (funct3)
        {
        case 0b000: { // LB
            // rd <- sx(m8(rs1 + imm_i)), pc += 4
            printf("LB %s, %d(%s)\n", abiNames[rd], imm, abiNames[rs1]);
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
            printf("LH %s, %d(%s)\n", abiNames[rd], imm, abiNames[rs1]);
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
            printf("LW %s, %d(%s)\n", abiNames[rd], imm, abiNames[rs1]);
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
            printf("LBU x%d, %d(x%d)\n", rd, imm, rs1);
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
            printf("LHU %s, %d(%s)\n", abiNames[rd], imm, abiNames[rs1]);
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
        uint32_t funct3 = (instruction >> 12) & 7;
        uint32_t rs2 = (instruction >> 20) & 0x1f;
        int32_t imm = SImmediate(instruction);
        switch (funct3)
        {
        case 0b000: // SB
            // m8(rs1 + imm_s) <- rs2[7:0], pc += 4
            printf("SB %s, %d(%s)\n", abiNames[rs2], imm, abiNames[rs1]);
            WriteByte(cpu->memory, cpu->xreg[rs1] + imm, cpu->xreg[rs2] & 0xff);
            cpu->pc += 4;
            break;
        case 0b001: // SH
            // m16(rs1 + imm_s) <- rs2[15:0], pc += 4
            printf("SH %s, %d(%s)\n", abiNames[rs2], imm, abiNames[rs1]);
            WriteHalfword(cpu->memory, cpu->xreg[rs1] + imm, cpu->xreg[rs2] & 0xffff);
            cpu->pc += 4;
            break;
        case 0b010: // SW
            // m32(rs1 + imm_s) <- rs2[31:0], pc += 4
            printf("SW %s, %d(%s)\n", abiNames[rs2], imm, abiNames[rs1]);
            WriteWord(cpu->memory, cpu->xreg[rs1] + imm, cpu->xreg[rs2]);
            cpu->pc += 4;
            break;
        default:
            return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
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
            printf("ADDI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
            cpu->xreg[rd] = cpu->xreg[rs1] + imm;
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            break;
        case 0b010: // SLTI
            // rd <- (rs1 < imm_i) ? 1 : 0, pc += 4
            printf("SLTI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
            cpu->xreg[rd] = ((int32_t)cpu->xreg[rs1] < imm) ? 1 : 0;
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            break;
        case 0b011: // SLTIU
            // rd <- (rs1 < imm_i) ? 1 : 0, pc += 4
            printf("SLTIU %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
            cpu->xreg[rd] = (cpu->xreg[rs1] < (uint32_t)imm) ? 1 : 0;
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            break;
        case 0b100: // XORI
            // rd <- rs1 ^ imm_i, pc += 4
            printf("XORI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
            cpu->xreg[rd] = cpu->xreg[rs1] ^ imm;
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            break;
        case 0b110: // ORI
            // rd <- rs1 | imm_i, pc += 4
            printf("ORI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
            cpu->xreg[rd] = cpu->xreg[rs1] | imm;
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            break;
        case 0b111: // ANDI
            // rd <- rs1 & imm_i, pc += 4
            printf("ANDI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
            cpu->xreg[rd] = cpu->xreg[rs1] & imm;
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            break;
        case 0b001: // SLLI
            // rd <- rs1 << shamt_i, pc += 4
            printf("SLLI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
            cpu->xreg[rd] = cpu->xreg[rs1] << shamt;
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            break;
        case 0b101:
            switch (funct7)
            {
            case 0b0000000: // SRLI
                // rd <- rs1 >> shamt_i, pc += 4
                printf("SRLI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
                cpu->xreg[rd] = cpu->xreg[rs1] >> shamt;
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            case 0b0100000: // SRAI
                // rd <- rs1 >> shamt_i, pc += 4
                printf("SRAI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
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
                printf("ADD %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                cpu->xreg[rd] = cpu->xreg[rs1] + cpu->xreg[rs2];
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            case 0b0000001: // SUB
                // rd <- rs1 - rs2, pc += 4
                printf("SUB %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                cpu->xreg[rd] = cpu->xreg[rs1] - cpu->xreg[rs2];
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            default:
                return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
            }
            break;
        case 0b001: // SLL
            // rd <- rs1 << (rs2 % XLEN), pc += 4
            printf("SLL %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
            cpu->xreg[rd] = cpu->xreg[rs1] << (cpu->xreg[rs2] % 32);
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            break;
        case 0b010: // SLT
            // rd <- (rs1 < rs2) ? 1 : 0, pc += 4
            printf("SLT %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
            cpu->xreg[rd] = ((int32_t)cpu->xreg[rs1] < (int32_t)cpu->xreg[rs2]) ? 1 : 0;
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            break;
        case 0b011: // SLTU
            // rd <- (rs1 < rs2) ? 1 : 0, pc += 4
            printf("SLTU %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
            cpu->xreg[rd] = (cpu->xreg[rs1] < cpu->xreg[rs2]) ? 1 : 0;
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            cpu->xreg[0] = 0;
            break;
        case 0b100: // XOR
            // rd <- rs1 ^ rs2, pc += 4
            printf("XOR %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
            cpu->xreg[rd] = cpu->xreg[rs1] ^ cpu->xreg[rs2];
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            break;
        case 0b101: // SRL / SRA
            switch (funct7)
            {
            case 0b0000000: // SRL
                // rd <- rs1 >> (rs2 % XLEN), pc += 4
                printf("SRL %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                cpu->xreg[rd] = cpu->xreg[rs1] >> (cpu->xreg[rs2] % 32);
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            case 0b0100000: // SRA
                // rd <- rs1 >> (rs2 % XLEN), pc += 4
                printf("SRA %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
                cpu->xreg[rd] = (int32_t)cpu->xreg[rs1] >> (cpu->xreg[rs2] % 32);
                cpu->pc += 4;
                cpu->xreg[0] = 0;
                break;
            default:
                return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
            }
            break;
        case 0b110: // OR
            // rd <- rs1 | rs2, pc += 4
            printf("OR %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
            cpu->xreg[rd] = cpu->xreg[rs1] | cpu->xreg[rs2];
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            break;
        case 0b111: // AND
            // rd <- rs1 & rs2, pc += 4
            printf("AND %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
            cpu->xreg[rd] = cpu->xreg[rs1] & cpu->xreg[rs2];
            cpu->pc += 4;
            cpu->xreg[0] = 0;
            break;
        default:
            return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
        }
    }
    break;

    case OP_MISCMEM: {
        uint32_t funct3 = (instruction >> 12) & 7;
        if (funct3 == 0b000)
        {
            printf("FENCE\n");
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
            uint32_t funct12 = instruction >> 20;
            switch (funct12)
            {
            case 0b000000000000: // ECALL
                printf("ECALL\n");
                return MakeTrap(trNOT_IMPLEMENTED_YET, 0);

            case 0b000000000001: // EBREAK
                printf("EBREAK\n");
                return MakeTrap(trBREAKPOINT, 0); // TODO: what should value be here?

            case 0b000000000010: // URET
                printf("URET\n");
                // TODO: Only provide this if user mode traps are supported, otherwise raise an illegal instruction exception.
                return MakeTrap(trNOT_IMPLEMENTED_YET, 0);

            case 0b000100000010: // SRET
                printf("SRET\n");
                // TODO: Only provide this if supervisor mode is supported, otherwise raise an illegal instruction exception.
                return MakeTrap(trNOT_IMPLEMENTED_YET, 0);

            case 0b001100000010: // MRET
                // pc <- mepc, pc += 4
                printf("MRET\n");
                cpu->pc = cpu->mepc; // Restore the program counter from the machine exception program counter.
                cpu->pc += 4;        // ...and increment it as normal.
                break;

            default:
                break; // Fall through to illegal instruction.
            }
        }
        return MakeTrap(trILLEGAL_INSTRUCTION, instruction);

    default:
        return MakeTrap(trILLEGAL_INSTRUCTION, instruction);
    }

    return MakeOk();
}

CpuResult Fetch(CPU* cpu)
{
    printf("%08x ", cpu->pc);
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
