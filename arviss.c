#include "arviss.h"

#include "conversions.h"
#include "opcodes.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#if defined(ARVISS_TRACE_ENABLED)
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

#if defined(ARVISS_TRACE_ENABLED)
// The ABI names of the integer registers x0-x31.
static char* abiNames[] = {"zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
                           "a6",   "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

// The ABI names of the floating point registers f0-f31.
static char* fabiNames[] = {"ft0", "ft1", "ft2", "ft3", "ft4",  "ft5",  "ft6", "ft7", "fs0",  "fs1", "fa0",
                            "fa1", "fa2", "fa3", "fa4", "fa5",  "fa6",  "fa7", "fs2", "fs3",  "fs4", "fs5",
                            "fs6", "fs7", "fs8", "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11"};

static char* roundingModes[] = {"rne", "rtz", "rdn", "rup", "rmm", "reserved5", "reserved6", "dyn"};
#endif

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

static inline ArvissResult TakeTrap(ArvissCpu* cpu, ArvissResult result)
{
    ArvissTrap trap = ArvissResultAsTrap(result);

    cpu->mepc = cpu->pc;       // Save the program counter in the machine exception program counter.
    cpu->mcause = trap.mcause; // mcause <- reason for trap.
    cpu->mtval = trap.mtval;   // mtval <- exception specific information.

    return result;
}

static inline ArvissResult CreateTrap(ArvissCpu* cpu, ArvissTrapType trap, uint32_t value)
{
    ArvissResult result = ArvissMakeTrap(trap, value);
    return TakeTrap(cpu, result);
}

void ArvissReset(ArvissCpu* cpu, uint32_t sp)
{
    cpu->pc = 0;
    for (int i = 0; i < 32; i++)
    {
        cpu->xreg[i] = 0;
    }
    for (int i = 0; i < 32; i++)
    {
        cpu->freg[i] = 0;
    }
    cpu->xreg[2] = sp;
    cpu->mepc = 0;
    cpu->mcause = 0;
    cpu->mtval = 0;
}

static inline ArvissResult OpLui(ArvissCpu* cpu, size_t rd, int32_t upper)
{
    // rd <- imm_u, pc += 4
    TRACE("LUI %s, %d\n", abiNames[rd], upper >> 12);
    cpu->xreg[rd] = upper;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpAuiPc(ArvissCpu* cpu, size_t rd, int32_t upper)
{
    // rd <- pc + imm_u, pc += 4
    TRACE("AUIPC %s, %d\n", abiNames[rd], upper >> 12);
    cpu->xreg[rd] = cpu->pc + upper;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpJal(ArvissCpu* cpu, size_t rd, int32_t imm)
{
    // rd <- pc + 4, pc <- pc + imm_j
    TRACE("JAL %s, %d\n", abiNames[rd], imm);
    cpu->xreg[rd] = cpu->pc + 4;
    cpu->pc += imm;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpJalr(ArvissCpu* cpu, size_t rd, size_t rs1, int32_t imm)
{
    // rd <- pc + 4, pc <- (rs1 + imm_i) & ~1
    TRACE("JALR %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
    uint32_t rs1Before = cpu->xreg[rs1]; // Because rd and rs1 might be the same register.
    cpu->xreg[rd] = cpu->pc + 4;
    cpu->pc = (rs1Before + imm) & ~1;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpBranch_Beq(ArvissCpu* cpu, size_t rs1, size_t rs2, int32_t imm)
{
    // pc <- pc + ((rs1 == rs2) ? imm_b : 4)
    TRACE("BEQ %s, %s, %d\n", abiNames[rs1], abiNames[rs2], imm);
    cpu->pc += ((cpu->xreg[rs1] == cpu->xreg[rs2]) ? imm : 4);
    return ArvissMakeOk();
}

static inline ArvissResult OpBranch_Bne(ArvissCpu* cpu, size_t rs1, size_t rs2, int32_t imm)
{
    // pc <- pc + ((rs1 != rs2) ? imm_b : 4)
    TRACE("BNE %s, %s, %d\n", abiNames[rs1], abiNames[rs2], imm);
    cpu->pc += ((cpu->xreg[rs1] != cpu->xreg[rs2]) ? imm : 4);
    return ArvissMakeOk();
}

static inline ArvissResult OpBranch_Blt(ArvissCpu* cpu, size_t rs1, size_t rs2, int32_t imm)
{
    // pc <- pc + ((rs1 < rs2) ? imm_b : 4)
    TRACE("BLT %s, %s, %d\n", abiNames[rs1], abiNames[rs2], imm);
    cpu->pc += (((int32_t)cpu->xreg[rs1] < (int32_t)cpu->xreg[rs2]) ? imm : 4);
    return ArvissMakeOk();
}

static inline ArvissResult OpBranch_Bge(ArvissCpu* cpu, size_t rs1, size_t rs2, int32_t imm)
{
    // pc <- pc + ((rs1 >= rs2) ? imm_b : 4)
    TRACE("BGE %s, %s, %d\n", abiNames[rs1], abiNames[rs2], imm);
    cpu->pc += (((int32_t)cpu->xreg[rs1] >= (int32_t)cpu->xreg[rs2]) ? imm : 4);
    return ArvissMakeOk();
}

static inline ArvissResult OpBranch_Bltu(ArvissCpu* cpu, size_t rs1, size_t rs2, int32_t imm)
{
    // pc <- pc + ((rs1 < rs2) ? imm_b : 4)
    TRACE("BLTU %s, %s, %d\n", abiNames[rs1], abiNames[rs2], imm);
    cpu->pc += ((cpu->xreg[rs1] < cpu->xreg[rs2]) ? imm : 4);
    return ArvissMakeOk();
}

static inline ArvissResult OpBranch_Bgeu(ArvissCpu* cpu, size_t rs1, size_t rs2, int32_t imm)
{
    // pc <- pc + ((rs1 >= rs2) ? imm_b : 4)
    TRACE("BGEU %s, %s, %d\n", abiNames[rs1], abiNames[rs2], imm);
    cpu->pc += ((cpu->xreg[rs1] >= cpu->xreg[rs2]) ? imm : 4);
    return ArvissMakeOk();
}

static inline ArvissResult OpLoad_Lb(ArvissCpu* cpu, size_t rd, size_t rs1, int32_t imm)
{
    // rd <- sx(m8(rs1 + imm_i)), pc += 4
    TRACE("LB %s, %d(%s)\n", abiNames[rd], imm, abiNames[rs1]);
    ArvissResult byteResult = ArvissReadByte(cpu->memory, cpu->xreg[rs1] + imm);
    if (!ArvissResultIsByte(byteResult))
    {
        return TakeTrap(cpu, byteResult);
    }
    cpu->xreg[rd] = (int32_t)(int16_t)(int8_t)ArvissResultAsByte(byteResult);
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpLoad_Lh(ArvissCpu* cpu, size_t rd, size_t rs1, int32_t imm)
{
    // rd <- sx(m16(rs1 + imm_i)), pc += 4
    TRACE("LH %s, %d(%s)\n", abiNames[rd], imm, abiNames[rs1]);
    ArvissResult halfwordResult = ArvissReadHalfword(cpu->memory, cpu->xreg[rs1] + imm);
    if (!ArvissResultIsHalfword(halfwordResult))
    {
        TakeTrap(cpu, halfwordResult);
        return halfwordResult;
    }
    cpu->xreg[rd] = (int32_t)(int16_t)ArvissResultAsHalfword(halfwordResult);
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpLoad_Lw(ArvissCpu* cpu, size_t rd, size_t rs1, int32_t imm)
{
    // rd <- sx(m32(rs1 + imm_i)), pc += 4
    TRACE("LW %s, %d(%s)\n", abiNames[rd], imm, abiNames[rs1]);
    ArvissResult wordResult = ArvissReadWord(cpu->memory, cpu->xreg[rs1] + imm);
    if (!ArvissResultIsWord(wordResult))
    {
        return TakeTrap(cpu, wordResult);
    }
    cpu->xreg[rd] = (int32_t)ArvissResultAsWord(wordResult);
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpLoad_Lbu(ArvissCpu* cpu, size_t rd, size_t rs1, int32_t imm)
{
    // rd <- zx(m8(rs1 + imm_i)), pc += 4
    TRACE("LBU x%d, %d(x%d)\n", rd, imm, rs1);
    ArvissResult byteResult = ArvissReadByte(cpu->memory, cpu->xreg[rs1] + imm);
    if (!ArvissResultIsByte(byteResult))
    {
        return TakeTrap(cpu, byteResult);
    }
    cpu->xreg[rd] = ArvissResultAsByte(byteResult);
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpLoad_Lhu(ArvissCpu* cpu, size_t rd, size_t rs1, int32_t imm)
{
    // rd <- zx(m16(rs1 + imm_i)), pc += 4
    TRACE("LHU %s, %d(%s)\n", abiNames[rd], imm, abiNames[rs1]);
    ArvissResult halfwordResult = ArvissReadHalfword(cpu->memory, cpu->xreg[rs1] + imm);
    if (!ArvissResultIsHalfword(halfwordResult))
    {
        return TakeTrap(cpu, halfwordResult);
    }
    cpu->xreg[rd] = ArvissResultAsHalfword(halfwordResult);
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpStore_Sb(ArvissCpu* cpu, size_t rs1, size_t rs2, int32_t imm)
{
    // m8(rs1 + imm_s) <- rs2[7:0], pc += 4
    TRACE("SB %s, %d(%s)\n", abiNames[rs2], imm, abiNames[rs1]);
    ArvissResult byteResult = ArvissWriteByte(cpu->memory, cpu->xreg[rs1] + imm, cpu->xreg[rs2] & 0xff);
    if (ArvissResultIsTrap(byteResult))
    {
        return TakeTrap(cpu, byteResult);
    }
    cpu->pc += 4;
    return ArvissMakeOk();
}

static inline ArvissResult OpStore_Sh(ArvissCpu* cpu, size_t rs1, size_t rs2, int32_t imm)
{
    // m16(rs1 + imm_s) <- rs2[15:0], pc += 4
    TRACE("SH %s, %d(%s)\n", abiNames[rs2], imm, abiNames[rs1]);
    ArvissResult halfwordResult = ArvissWriteHalfword(cpu->memory, cpu->xreg[rs1] + imm, cpu->xreg[rs2] & 0xffff);
    if (ArvissResultIsTrap(halfwordResult))
    {
        return TakeTrap(cpu, halfwordResult);
    }
    cpu->pc += 4;
    return ArvissMakeOk();
}

static inline ArvissResult OpStore_Sw(ArvissCpu* cpu, size_t rs1, size_t rs2, int32_t imm)
{
    // m32(rs1 + imm_s) <- rs2[31:0], pc += 4
    TRACE("SW %s, %d(%s)\n", abiNames[rs2], imm, abiNames[rs1]);
    ArvissResult wordResult = ArvissWriteWord(cpu->memory, cpu->xreg[rs1] + imm, cpu->xreg[rs2]);
    if (ArvissResultIsTrap(wordResult))
    {
        return TakeTrap(cpu, wordResult);
    }
    cpu->pc += 4;
    return ArvissMakeOk();
}

static inline ArvissResult OpImm_Addi(ArvissCpu* cpu, size_t rd, size_t rs1, int32_t imm)
{
    // rd <- rs1 + imm_i, pc += 4
    TRACE("ADDI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
    cpu->xreg[rd] = cpu->xreg[rs1] + imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpImm_Slti(ArvissCpu* cpu, size_t rd, size_t rs1, int32_t imm)
{
    // rd <- (rs1 < imm_i) ? 1 : 0, pc += 4
    TRACE("SLTI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
    cpu->xreg[rd] = ((int32_t)cpu->xreg[rs1] < imm) ? 1 : 0;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpImm_Sltiu(ArvissCpu* cpu, size_t rd, size_t rs1, int32_t imm)
{
    // rd <- (rs1 < imm_i) ? 1 : 0, pc += 4
    TRACE("SLTIU %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
    cpu->xreg[rd] = (cpu->xreg[rs1] < (uint32_t)imm) ? 1 : 0;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpImm_Xori(ArvissCpu* cpu, size_t rd, size_t rs1, int32_t imm)
{
    // rd <- rs1 ^ imm_i, pc += 4
    TRACE("XORI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
    cpu->xreg[rd] = cpu->xreg[rs1] ^ imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpImm_Ori(ArvissCpu* cpu, size_t rd, size_t rs1, int32_t imm)
{
    // rd <- rs1 | imm_i, pc += 4
    TRACE("ORI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
    cpu->xreg[rd] = cpu->xreg[rs1] | imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpImm_Andi(ArvissCpu* cpu, size_t rd, size_t rs1, int32_t imm)
{
    // rd <- rs1 & imm_i, pc += 4
    TRACE("ANDI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
    cpu->xreg[rd] = cpu->xreg[rs1] & imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpImm_Slli(ArvissCpu* cpu, size_t rd, size_t rs1, uint32_t shamt)
{
    TRACE("SLLI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
    cpu->xreg[rd] = cpu->xreg[rs1] << shamt;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpImm_Srli(ArvissCpu* cpu, size_t rd, size_t rs1, uint32_t shamt)
{
    // rd <- rs1 >> shamt_i, pc += 4
    TRACE("SRLI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
    cpu->xreg[rd] = cpu->xreg[rs1] >> shamt;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpImm_Srai(ArvissCpu* cpu, size_t rd, size_t rs1, uint32_t shamt)
{
    // rd <- rs1 >> shamt_i, pc += 4
    TRACE("SRAI %s, %s, %d\n", abiNames[rd], abiNames[rs1], imm);
    cpu->xreg[rd] = (int32_t)cpu->xreg[rs1] >> shamt;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpOp_Add(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
    // rd <- rs1 + rs2, pc += 4
    TRACE("ADD %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
    cpu->xreg[rd] = cpu->xreg[rs1] + cpu->xreg[rs2];
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpOp_Sub(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
    // rd <- rs1 - rs2, pc += 4
    TRACE("SUB %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
    cpu->xreg[rd] = cpu->xreg[rs1] - cpu->xreg[rs2];
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpOp_Mul(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
    TRACE("MUL %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
    cpu->xreg[rd] = cpu->xreg[rs1] * cpu->xreg[rs2];
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpOp_Sll(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
    // rd <- rs1 << (rs2 % XLEN), pc += 4
    TRACE("SLL %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
    cpu->xreg[rd] = cpu->xreg[rs1] << (cpu->xreg[rs2] % 32);
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpOp_Mulh(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
    TRACE("MULH %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
    int64_t t = (int64_t)(int32_t)cpu->xreg[rs1] * (int64_t)(int32_t)cpu->xreg[rs2];
    cpu->xreg[rd] = t >> 32;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpOp_Slt(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
    // rd <- (rs1 < rs2) ? 1 : 0, pc += 4
    TRACE("SLT %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
    cpu->xreg[rd] = ((int32_t)cpu->xreg[rs1] < (int32_t)cpu->xreg[rs2]) ? 1 : 0;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpOp_Mulhsu(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
    TRACE("MULHSU %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
    int64_t t = (int64_t)(int32_t)cpu->xreg[rs1] * (uint64_t)cpu->xreg[rs2];
    cpu->xreg[rd] = t >> 32;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpOp_Sltu(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
    // rd <- (rs1 < rs2) ? 1 : 0, pc += 4
    TRACE("SLTU %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
    cpu->xreg[rd] = (cpu->xreg[rs1] < cpu->xreg[rs2]) ? 1 : 0;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpOp_Mulhu(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
    TRACE("MULHU %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
    uint64_t t = (uint64_t)cpu->xreg[rs1] * (uint64_t)cpu->xreg[rs2];
    cpu->xreg[rd] = t >> 32;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpOp_Xor(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
    // rd <- rs1 ^ rs2, pc += 4
    TRACE("XOR %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
    cpu->xreg[rd] = cpu->xreg[rs1] ^ cpu->xreg[rs2];
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpOp_Div(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
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
    return ArvissMakeOk();
}

static inline ArvissResult OpOp_Srl(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
    // rd <- rs1 >> (rs2 % XLEN), pc += 4
    TRACE("SRL %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
    cpu->xreg[rd] = cpu->xreg[rs1] >> (cpu->xreg[rs2] % 32);
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpOp_Sra(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
    // rd <- rs1 >> (rs2 % XLEN), pc += 4
    TRACE("SRA %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
    cpu->xreg[rd] = (int32_t)cpu->xreg[rs1] >> (cpu->xreg[rs2] % 32);
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpOp_Divu(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
    TRACE("DIVU %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
    uint32_t divisor = cpu->xreg[rs2];
    cpu->xreg[rd] = divisor != 0 ? cpu->xreg[rs1] / divisor : 0xffffffff;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpOp_Or(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
    // rd <- rs1 | rs2, pc += 4
    TRACE("OR %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
    cpu->xreg[rd] = cpu->xreg[rs1] | cpu->xreg[rs2];
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpOp_Rem(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
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
    return ArvissMakeOk();
}

static inline ArvissResult OpOp_And(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
    // rd <- rs1 & rs2, pc += 4
    TRACE("AND %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
    cpu->xreg[rd] = cpu->xreg[rs1] & cpu->xreg[rs2];
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpOp_Remu(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
    TRACE("REMU %s, %s, %s\n", abiNames[rd], abiNames[rs1], abiNames[rs2]);
    const uint32_t dividend = cpu->xreg[rs1];
    const uint32_t divisor = cpu->xreg[rs2];
    cpu->xreg[rd] = divisor != 0 ? cpu->xreg[rs1] % divisor : dividend;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static inline ArvissResult OpMiscmem_Fence(ArvissCpu* cpu)
{
    TRACE("FENCE\n");
    return CreateTrap(cpu, trNOT_IMPLEMENTED_YET, 0);
}

static inline ArvissResult OpSystem_Ecall(ArvissCpu* cpu)
{
    TRACE("ECALL\n");
    return CreateTrap(cpu, trENVIRONMENT_CALL_FROM_M_MODE, 0);
}

static inline ArvissResult OpSystem_Ebreak(ArvissCpu* cpu)
{
    TRACE("EBREAK\n");
    return CreateTrap(cpu, trBREAKPOINT, 0); // TODO: what should value be here?
}

static inline ArvissResult OpSystem_Uret(ArvissCpu* cpu)
{
    TRACE("URET\n");
    // TODO: Only provide this if user mode traps are supported, otherwise raise an illegal instruction exception.
    return CreateTrap(cpu, trNOT_IMPLEMENTED_YET, 0);
}

static inline ArvissResult OpSystem_Sret(ArvissCpu* cpu)
{
    TRACE("SRET\n");
    // TODO: Only provide this if supervisor mode is supported, otherwise raise an illegal instruction exception.
    return CreateTrap(cpu, trNOT_IMPLEMENTED_YET, 0);
}

static inline ArvissResult OpSystem_Mret(ArvissCpu* cpu)
{
    // pc <- mepc, pc += 4
    TRACE("MRET\n");
    cpu->pc = cpu->mepc; // Restore the program counter from the machine exception program counter.
    cpu->pc += 4;        // ...and increment it as normal.
    return ArvissMakeOk();
}

static inline ArvissResult OpLoadFp_Flw(ArvissCpu* cpu, size_t rd, size_t rs1, int32_t imm)
{
    // rd <- f32(rs1 + imm_i)
    TRACE("FLW %s, %d(%s)\n", fabiNames[rd], imm, abiNames[rs1]);
    ArvissResult wordResult = ArvissReadWord(cpu->memory, cpu->xreg[rs1] + imm);
    if (!ArvissResultIsWord(wordResult))
    {
        return TakeTrap(cpu, wordResult);
    }
    const uint32_t resultAsWord = ArvissResultAsWord(wordResult);
    const float resultAsFloat = U32AsFloat(resultAsWord);
    cpu->freg[rd] = resultAsFloat;
    cpu->pc += 4;
    return ArvissMakeOk();
}

static inline ArvissResult OpStoreFp_Fsw(ArvissCpu* cpu, size_t rs1, size_t rs2, int32_t imm)
{
    // f32(rs1 + imm_s) = rs2
    TRACE("FSW %s, %d(%s)\n", fabiNames[rs2], imm, abiNames[rs1]);
    uint32_t t = FloatAsU32(cpu->freg[rs2]);
    ArvissResult wordResult = ArvissWriteWord(cpu->memory, cpu->xreg[rs1] + imm, t);
    if (ArvissResultIsTrap(wordResult))
    {
        return TakeTrap(cpu, wordResult);
    }
    cpu->pc += 4;
    return ArvissMakeOk();
}

static inline ArvissResult OpMadd_Fmadd_s(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2, size_t rs3, uint32_t rm)
{
    // rd <- (rs1 * rs2) + rs3
    TRACE("FMADD.S %s, %s, %s, %s, %s\n", fabiNames[rd], fabiNames[rs1], fabiNames[rs2], fabiNames[rs3], roundingModes[rm]);
    cpu->freg[rd] = (cpu->freg[rs1] * cpu->freg[rs2]) + cpu->freg[rs3];
    cpu->pc += 4;
    (void)rm; // TODO: rounding.
    return ArvissMakeOk();
}

static inline ArvissResult OpMsub_Fmsub_s(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2, size_t rs3, uint32_t rm)
{
    // rd <- (rs1 x rs2) - rs3
    TRACE("FMSUB.S %s, %s, %s, %s, %s\n", fabiNames[rd], fabiNames[rs1], fabiNames[rs2], fabiNames[rs3], roundingModes[rm]);
    cpu->freg[rd] = (cpu->freg[rs1] * cpu->freg[rs2]) - cpu->freg[rs3];
    cpu->pc += 4;
    (void)rm; // TODO: rounding.
    return ArvissMakeOk();
}

static inline ArvissResult OpNmsub_Fnmsub_s(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2, size_t rs3, uint32_t rm)
{
    // rd <- -(rs1 x rs2) + rs3
    TRACE("FNMSUB.S %s, %s, %s, %s, %s\n", fabiNames[rd], fabiNames[rs1], fabiNames[rs2], fabiNames[rs3], roundingModes[rm]);
    cpu->freg[rd] = -(cpu->freg[rs1] * cpu->freg[rs2]) + cpu->freg[rs3];
    cpu->pc += 4;
    (void)rm; // TODO: rounding.
    return ArvissMakeOk();
}

static inline ArvissResult OpNmadd_Fnmadd_s(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2, size_t rs3, uint32_t rm)
{
    // rd <- -(rs1 x rs2) - rs3
    TRACE("FNMADD.S %s, %s, %s, %s, %s\n", fabiNames[rd], fabiNames[rs1], fabiNames[rs2], fabiNames[rs3], roundingModes[rm]);
    cpu->freg[rd] = -(cpu->freg[rs1] * cpu->freg[rs2]) - cpu->freg[rs3];
    cpu->pc += 4;
    (void)rm; // TODO: rounding.
    return ArvissMakeOk();
}

static inline ArvissResult OpOpFp_Fadd_s(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2, uint32_t rm)
{
    // rd <- rs1 + rs2
    TRACE("FADD.S %s, %s, %s, %s\n", fabiNames[rd], fabiNames[rs1], fabiNames[rs2], roundingModes[rm]);
    cpu->freg[rd] = cpu->freg[rs1] + cpu->freg[rs2];
    cpu->pc += 4;
    (void)rm; // TODO: rounding.
    return ArvissMakeOk();
}

static inline ArvissResult OpOpFp_Fsub_s(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2, uint32_t rm)
{
    // rd <- rs1 - rs2
    TRACE("FSUB.S %s, %s, %s, %s\n", fabiNames[rd], fabiNames[rs1], fabiNames[rs2], roundingModes[rm]);
    cpu->freg[rd] = cpu->freg[rs1] - cpu->freg[rs2];
    cpu->pc += 4;
    (void)rm; // TODO: rounding.
    return ArvissMakeOk();
}

static inline ArvissResult OpOpFp_Fmul_s(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2, uint32_t rm)
{
    // rd <- rs1 * rs2
    TRACE("FMUL.S %s, %s, %s, %s\n", fabiNames[rd], fabiNames[rs1], fabiNames[rs2], roundingModes[rm]);
    cpu->freg[rd] = cpu->freg[rs1] * cpu->freg[rs2];
    cpu->pc += 4;
    (void)rm; // TODO: rounding.
    return ArvissMakeOk();
}

static inline ArvissResult OpOpFp_Fdiv_s(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2, uint32_t rm)
{
    // rd <- rs1 / rs2
    TRACE("FDIV.S %s, %s, %s, %s\n", fabiNames[rd], fabiNames[rs1], fabiNames[rs2], roundingModes[rm]);
    cpu->freg[rd] = cpu->freg[rs1] / cpu->freg[rs2];
    cpu->pc += 4;
    (void)rm; // TODO: rounding.
    return ArvissMakeOk();
}

static inline ArvissResult OpOpFp_Fsqrt_s(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2, uint32_t rm)
{
    // rd <- sqrt(rs1)
    TRACE("FSQRT.S %s, %s, %s\n", fabiNames[rd], fabiNames[rs1], roundingModes[rm]);
    cpu->freg[rd] = sqrtf(cpu->freg[rs1]);
    cpu->pc += 4;
    (void)rm; // TODO: rounding.
    return ArvissMakeOk();
}

static inline ArvissResult OpOpFp_Fsgnj_s(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2, uint32_t rm)
{
    // rd <- abs(rs1) * sgn(rs2)
    TRACE("FSGNJ.S %s, %s, %s\n", fabiNames[rd], fabiNames[rs1], fabiNames[rs2]);
    cpu->freg[rd] = fabsf(cpu->freg[rs1]) * (cpu->freg[rs2] < 0.0f ? -1.0f : 1.0f);
    cpu->pc += 4;
    return ArvissMakeOk();
}

static inline ArvissResult OpOpFp_Fsgnjn_s(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2, uint32_t rm)
{
    // rd <- abs(rs1) * -sgn(rs2)
    TRACE("FSGNJN.S %s, %s, %s\n", fabiNames[rd], fabiNames[rs1], fabiNames[rs2]);
    cpu->freg[rd] = fabsf(cpu->freg[rs1]) * (cpu->freg[rs2] < 0.0f ? 1.0f : -1.0f);
    cpu->pc += 4;
    return ArvissMakeOk();
}

static inline ArvissResult OpOpFp_Fsgnjx_s(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2, uint32_t rm)
{
    float m; // The sign bit is the XOR of the sign bits of rs1 and rs2.
    if ((cpu->freg[rs1] < 0.0f && cpu->freg[rs2] >= 0.0f) || (cpu->freg[rs1] >= 0.0f && cpu->freg[rs2] < 0.0f))
    {
        m = -1.0f;
    }
    else
    {
        m = 1.0f;
    }
    // rd <- abs(rs1) * (sgn(rs1) == sgn(rs2)) ? 1 : -1
    TRACE("FSGNJX.S %s, %s, %s\n", fabiNames[rd], fabiNames[rs1], fabiNames[rs2]);
    cpu->freg[rd] = fabsf(cpu->freg[rs1]) * m;
    cpu->pc += 4;
    return ArvissMakeOk();
}

static inline ArvissResult OpOpFp_Fmin_s(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2, uint32_t rm)
{
    // rd <- min(rs1, rs2)
    TRACE("FMIN.S %s, %s, %s\n", fabiNames[rd], fabiNames[rs1], fabiNames[rs2]);
    cpu->freg[rd] = fminf(cpu->freg[rs1], cpu->freg[rs2]);
    cpu->pc += 4;
    (void)rm; // TODO: rounding.
    return ArvissMakeOk();
}

static inline ArvissResult OpOpFp_Fmax_s(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2, uint32_t rm)
{
    // rd <- max(rs1, rs2)
    TRACE("FMAX.S %s, %s, %s\n", fabiNames[rd], fabiNames[rs1], fabiNames[rs2]);
    cpu->freg[rd] = fmaxf(cpu->freg[rs1], cpu->freg[rs2]);
    cpu->pc += 4;
    (void)rm; // TODO: rounding.
    return ArvissMakeOk();
}

static inline ArvissResult OpOpFp_Fcvt_w_s(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2, uint32_t rm)
{
    // rd <- int32_t(rs1)
    TRACE("FCVT.W.S %s, %s, %s\n", abiNames[rd], fabiNames[rs1], roundingModes[rm]);
    cpu->xreg[rd] = (int32_t)cpu->freg[rs1];
    cpu->pc += 4;
    (void)rm; // TODO: rounding.
    return ArvissMakeOk();
}

static inline ArvissResult OpOpFp_Fcvt_wu_s(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2, uint32_t rm)
{
    // rd <- uint32_t(rs1)
    TRACE("FCVT.WU.S %s, %s, %s\n", abiNames[rd], fabiNames[rs1], roundingModes[rm]);
    cpu->xreg[rd] = (uint32_t)(int32_t)cpu->freg[rs1];
    cpu->pc += 4;
    (void)rm; // TODO: rounding.
    return ArvissMakeOk();
}

static inline ArvissResult OpOpFp_Fmv_x_w(ArvissCpu* cpu, size_t rd, size_t rs1)
{
    // bits(rd) <- bits(rs1)
    TRACE("FMV.X.W %s, %s\n", abiNames[rd], fabiNames[rs1]);
    cpu->xreg[rd] = FloatAsU32(cpu->freg[rs1]);
    cpu->pc += 4;
    return ArvissMakeOk();
}

static ArvissResult OpOpFp_Fclass_s(ArvissCpu* cpu, size_t rd, size_t rs1)
{
    TRACE("FCLASS.S %s, %s\n", abiNames[rd], fabiNames[rs1]);
    const float v = cpu->freg[rs1];
    const uint32_t bits = FloatAsU32(v);
    uint32_t result = 0;
    if (v == -INFINITY)
    {
        result = (1 << 0);
    }
    else if (v == INFINITY)
    {
        result = (1 << 7);
    }
    else if (bits == 0x80000000) // Negative zero.
    {
        result = (1 << 3);
    }
    else if (v == 0.0f)
    {
        result = (1 << 4);
    }
    else if ((bits & 0x7f800000) == 0) // Is the exponent zero?
    {
        if (bits & 0x80000000)
        {
            result = (1 << 2); // Negative subnormal number
        }
        else
        {
            result = (1 << 5); // Positive subnormal number.
        }
    }
    else if ((bits & 0x7f800000) == 0x7f800000) // Is the exponent as large as possible?
    {
        if (bits & 0x00400000)
        {
            result = (1 << 9); // Quiet NaN.
        }
        else if (bits & 0x003fffff)
        {
            result = (1 << 8); // Signaling NaN.
        }
    }
    else if (v < 0.0f)
    {
        result = (1 << 1);
    }
    else if (v > 0.0f)
    {
        result = (1 << 6);
    }
    cpu->xreg[rd] = result;
    cpu->pc += 4;
    return ArvissMakeOk();
}

static inline ArvissResult OpOpFp_Feq_s(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
    // rd <- (rs1 == rs2) ? 1 : 0;
    TRACE("FEQ.S %s, %s, %s\n", abiNames[rd], fabiNames[rs1], fabiNames[rs2]);
    cpu->xreg[rd] = cpu->freg[rs1] == cpu->freg[rs2] ? 1 : 0;
    cpu->pc += 4;
    return ArvissMakeOk();
}

static inline ArvissResult OpOpFp_Flt_s(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
    // rd <- (rs1 < rs2) ? 1 : 0;
    TRACE("FLT.S %s, %s, %s\n", abiNames[rd], fabiNames[rs1], fabiNames[rs2]);
    cpu->xreg[rd] = cpu->freg[rs1] < cpu->freg[rs2] ? 1 : 0;
    cpu->pc += 4;
    return ArvissMakeOk();
}

static inline ArvissResult OpOpFp_Fle_s(ArvissCpu* cpu, size_t rd, size_t rs1, size_t rs2)
{
    // rd <- (rs1 <= rs2) ? 1 : 0;
    TRACE("FLE.S %s, %s, %s\n", abiNames[rd], fabiNames[rs1], fabiNames[rs2]);
    cpu->xreg[rd] = cpu->freg[rs1] <= cpu->freg[rs2] ? 1 : 0;
    cpu->pc += 4;
    return ArvissMakeOk();
}

static inline ArvissResult OpOpFp_Fcvt_s_w(ArvissCpu* cpu, size_t rd, size_t rs1, uint32_t rm)
{
    // rd <- float(int32_t((rs1))
    TRACE("FCVT.S.W %s, %s, %s\n", fabiNames[rd], abiNames[rs1], roundingModes[rm]);
    cpu->freg[rd] = (float)(int32_t)cpu->xreg[rs1];
    cpu->pc += 4;
    (void)rm; // TODO: rounding.
    return ArvissMakeOk();
}

static inline ArvissResult OpOpFp_Fcvt_s_wu(ArvissCpu* cpu, size_t rd, size_t rs1, uint32_t rm)
{
    // rd <- float(rs1)
    TRACE("FVCT.S.WU %s, %s, %s\n", fabiNames[rd], abiNames[rs1], roundingModes[rm]);
    cpu->freg[rd] = (float)cpu->xreg[rs1];
    cpu->pc += 4;
    (void)rm; // TODO: rounding.
    return ArvissMakeOk();
}

static inline ArvissResult OpOpFp_Fmv_w_x(ArvissCpu* cpu, size_t rd, size_t rs1)
{
    // bits(rd) <- bits(rs1)
    TRACE("FMV.W.X %s, %s\n", fabiNames[rd], abiNames[rs1]);
    cpu->freg[rd] = U32AsFloat(cpu->xreg[rs1]);
    cpu->pc += 4;
    return ArvissMakeOk();
}

// See: http://www.five-embeddev.com/riscv-isa-manual/latest/gmaps.html#rv3264g-instruction-set-listings
// or riscv-spec-209191213.pdf.
ArvissResult ArvissExecute(ArvissCpu* cpu, uint32_t instruction)
{
    uint32_t opcode = Opcode(instruction);
    uint32_t rd = Rd(instruction);
    uint32_t rs1 = Rs1(instruction);

    switch (opcode)
    {
    case OP_LUI: {
        int32_t upper = UImmediate(instruction);
        return OpLui(cpu, rd, upper);
    }

    case OP_AUIPC: {
        int32_t upper = UImmediate(instruction);
        return OpAuiPc(cpu, rd, upper);
    }

    case OP_JAL: {
        int32_t imm = JImmediate(instruction);
        return OpJal(cpu, rd, imm);
    }

    case OP_JALR: {
        uint32_t funct3 = Funct3(instruction);
        if (funct3 == 0b000)
        {
            int32_t imm = IImmediate(instruction);
            return OpJalr(cpu, rd, rs1, imm);
        }
        return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
    }

    case OP_BRANCH: {
        uint32_t funct3 = Funct3(instruction);
        uint32_t rs2 = Rs2(instruction);
        int32_t imm = BImmediate(instruction);
        switch (funct3)
        {
        case 0b000: // BEQ
            return OpBranch_Beq(cpu, rs1, rs2, imm);
        case 0b001: // BNE
            return OpBranch_Bne(cpu, rs1, rs2, imm);
        case 0b100: // BLT
            return OpBranch_Blt(cpu, rs1, rs2, imm);
        case 0b101: // BGE
            return OpBranch_Bge(cpu, rs1, rs2, imm);
        case 0b110: // BLTU
            return OpBranch_Bltu(cpu, rs1, rs2, imm);
        case 0b111: // BGEU
            return OpBranch_Bgeu(cpu, rs1, rs2, imm);
        default:
            return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
        }
    }

    case OP_LOAD: {
        uint32_t funct3 = Funct3(instruction);
        int32_t imm = IImmediate(instruction);
        switch (funct3)
        {
        case 0b000: // LB
            return OpLoad_Lb(cpu, rd, rs1, imm);
        case 0b001: // LH
            return OpLoad_Lh(cpu, rd, rs1, imm);
        case 0b010: // LW
            return OpLoad_Lw(cpu, rd, rs1, imm);
        case 0b100: // LBU
            return OpLoad_Lbu(cpu, rd, rs1, imm);
        case 0b101: // LHU
            return OpLoad_Lhu(cpu, rd, rs1, imm);
        default:
            return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
        }
    }

    case OP_STORE: {
        uint32_t funct3 = Funct3(instruction);
        uint32_t rs2 = Rs2(instruction);
        int32_t imm = SImmediate(instruction);
        switch (funct3)
        {
        case 0b000: // SB
            return OpStore_Sb(cpu, rs1, rs2, imm);
        case 0b001: // SH
            return OpStore_Sh(cpu, rs1, rs2, imm);
        case 0b010: // SW
            return OpStore_Sw(cpu, rs1, rs2, imm);
        default:
            return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
        }
    }

    case OP_OPIMM: {
        uint32_t funct3 = Funct3(instruction);
        int32_t imm = IImmediate(instruction);
        uint32_t funct7 = Funct7(instruction);
        switch (funct3)
        {
        case 0b000: // ADDI
            return OpImm_Addi(cpu, rd, rs1, imm);
        case 0b010: // SLTI
            return OpImm_Slti(cpu, rd, rs1, imm);
        case 0b011: // SLTIU
            return OpImm_Sltiu(cpu, rd, rs1, imm);
        case 0b100: // XORI
            return OpImm_Xori(cpu, rd, rs1, imm);
        case 0b110: // ORI
            return OpImm_Ori(cpu, rd, rs1, imm);
        case 0b111: // ANDI
            return OpImm_Andi(cpu, rd, rs1, imm);
        case 0b001: // SLLI
            return OpImm_Slli(cpu, rd, rs1, imm & 0x1f);
        case 0b101:
            switch (funct7)
            {
            case 0b0000000: // SRLI
                return OpImm_Srli(cpu, rd, rs1, imm & 0x1f);
            case 0b0100000: // SRAI
                return OpImm_Srai(cpu, rd, rs1, imm & 0x1f);
            default:
                return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
            }
        default:
            return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
        }
    }

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
                return OpOp_Add(cpu, rd, rs1, rs2);
            case 0b0100000: // SUB
                return OpOp_Sub(cpu, rd, rs1, rs2);
            case 0b00000001: // MUL (RV32M)
                return OpOp_Mul(cpu, rd, rs1, rs2);
            default:
                return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
            }
        case 0b001:
            switch (funct7)
            {
            case 0b0000000: // SLL
                return OpOp_Sll(cpu, rd, rs1, rs2);
            case 0b00000001: // MULH (RV32M)
                return OpOp_Mulh(cpu, rd, rs1, rs2);
            default:
                return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
            }
        case 0b010:
            switch (funct7)
            {
            case 0b0000000: // SLT
                return OpOp_Slt(cpu, rd, rs1, rs2);
            case 0b00000001: // MULHSU (RV32M)
                return OpOp_Mulhsu(cpu, rd, rs1, rs2);
            default:
                return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
            }
        case 0b011:
            switch (funct7)
            {
            case 0b0000000: // SLTU
                return OpOp_Sltu(cpu, rd, rs1, rs2);
            case 0b00000001: // MULHU (RV32M)
                return OpOp_Mulhu(cpu, rd, rs1, rs2);
            default:
                return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
            }
        case 0b100:
            switch (funct7)
            {
            case 0b0000000: // XOR
                return OpOp_Xor(cpu, rd, rs1, rs2);
            case 0b00000001: // DIV (RV32M)
                return OpOp_Div(cpu, rd, rs1, rs2);
            default:
                return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
            }
        case 0b101:
            switch (funct7)
            {
            case 0b0000000: // SRL
                return OpOp_Srl(cpu, rd, rs1, rs2);
            case 0b0100000: // SRA
                return OpOp_Sra(cpu, rd, rs1, rs2);
            case 0b00000001: // DIVU (RV32M)
                return OpOp_Divu(cpu, rd, rs1, rs2);
            default:
                return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
            }
        case 0b110:
            switch (funct7)
            {
            case 0b0000000: // OR
                return OpOp_Or(cpu, rd, rs1, rs2);
            case 0b00000001: // REM
                return OpOp_Rem(cpu, rd, rs1, rs2);
            default:
                return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
            }
        case 0b111:
            switch (funct7)
            {
            case 0b0000000: // AND
                return OpOp_And(cpu, rd, rs1, rs2);
            case 0b00000001: // REMU
                return OpOp_Remu(cpu, rd, rs1, rs2);
            default:
                return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
            }
        default:
            return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
        }
    }

    case OP_MISCMEM: {
        uint32_t funct3 = Funct3(instruction);
        if (funct3 == 0b000)
        {
            return OpMiscmem_Fence(cpu);
        }
        else
        {
            return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
        }
    }

    case OP_SYSTEM:
        if ((instruction & 0b00000000000011111111111110000000) == 0)
        {
            uint32_t funct12 = Funct12(instruction);
            switch (funct12)
            {
            case 0b000000000000: // ECALL
                return OpSystem_Ecall(cpu);
            case 0b000000000001: // EBREAK
                return OpSystem_Ebreak(cpu);
            case 0b000000000010: // URET
                return OpSystem_Uret(cpu);
            case 0b000100000010: // SRET
                return OpSystem_Sret(cpu);
            case 0b001100000010: // MRET
                return OpSystem_Mret(cpu);
            default:
                return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
            }
        }
        else
        {
            return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
        }

    case OP_LOADFP: { // Floating point load (RV32F)
        uint32_t funct3 = Funct3(instruction);
        if (funct3 == 0b010) // FLW
        {
            int32_t imm = IImmediate(instruction);
            return OpLoadFp_Flw(cpu, rd, rs1, imm);
        }
        else
        {
            return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
        }
    }

    case OP_STOREFP: { // Floating point store (RV32F)
        uint32_t funct3 = Funct3(instruction);
        if (funct3 == 0b010) // FSW
        {
            // f32(rs1 + imm_s) = rs2
            uint32_t rs2 = Rs2(instruction);
            int32_t imm = SImmediate(instruction);
            return OpStoreFp_Fsw(cpu, rs1, rs2, imm);
        }
        else
        {
            return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
        }
    }

    case OP_MADD: {                            // Floating point fused multiply-add (RV32F)
        if (((instruction >> 25) & 0b11) == 0) // FMADD.S
        {
            uint32_t rm = Rm(instruction);
            uint32_t rs2 = Rs2(instruction);
            uint32_t rs3 = Rs3(instruction);
            return OpMadd_Fmadd_s(cpu, rd, rs1, rs2, rs3, rm);
        }
        else
        {
            return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
        }
    }

    case OP_MSUB: {                            // Floating point fused multiply-sub (RV32F)
        if (((instruction >> 25) & 0b11) == 0) // FMSUB.S
        {
            uint32_t rm = Rm(instruction);
            uint32_t rs2 = Rs2(instruction);
            uint32_t rs3 = Rs3(instruction);
            return OpMsub_Fmsub_s(cpu, rd, rs1, rs2, rs3, rm);
        }
        else
        {
            return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
        }
    }

    case OP_NMSUB: {                           // Floating point negated fused multiply-sub (RV32F)
        if (((instruction >> 25) & 0b11) == 0) // FNMSUB.S
        {
            uint32_t rm = Rm(instruction);
            uint32_t rs2 = Rs2(instruction);
            uint32_t rs3 = Rs3(instruction);
            return OpNmsub_Fnmsub_s(cpu, rd, rs1, rs2, rs3, rm);
        }
        else
        {
            return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
        }
    }

    case OP_NMADD: {                           // Floating point negated fused multiple-add (RV32F)
        if (((instruction >> 25) & 0b11) == 0) // FNMADD.S
        {
            uint32_t rm = Rm(instruction);
            uint32_t rs2 = Rs2(instruction);
            uint32_t rs3 = Rs3(instruction);
            return OpNmadd_Fnmadd_s(cpu, rd, rs1, rs2, rs3, rm);
        }
        else
        {
            return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
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
            return OpOpFp_Fadd_s(cpu, rd, rs1, rs2, rm);
        case 0b0000100: // FSUB.S
            return OpOpFp_Fsub_s(cpu, rd, rs1, rs2, rm);
        case 0b0001000: // FMUL.S
            return OpOpFp_Fmul_s(cpu, rd, rs1, rs2, rm);
        case 0b0001100: // FDIV.S
            return OpOpFp_Fdiv_s(cpu, rd, rs1, rs2, rm);
        case 0b0101100:
            if (rs2 == 0b00000) // FSQRT.S
            {
                return OpOpFp_Fsqrt_s(cpu, rd, rs1, rs2, rm);
            }
            else
            {
                return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
            }
        case 0b0010000: {
            switch (funct3)
            {
            case 0b000: // FSGNJ.S
                return OpOpFp_Fsgnj_s(cpu, rd, rs1, rs2, rm);
            case 0b001: // FSGNJN.S
                return OpOpFp_Fsgnjn_s(cpu, rd, rs1, rs2, rm);
            case 0b010: // FSGNJX.S
                return OpOpFp_Fsgnjx_s(cpu, rd, rs1, rs2, rm);
            default:
                return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
            }
        }
        case 0b0010100: {
            switch (funct3)
            {
            case 0b000: // FMIN.S
                return OpOpFp_Fmin_s(cpu, rd, rs1, rs2, rm);
            case 0b001: // FMAX.S
                return OpOpFp_Fmax_s(cpu, rd, rs1, rs2, rm);
            default:
                return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
            }
        }
        case 0b1100000: {
            switch (rs2) // Not actually rs2 - just the same bits.
            {
            case 0b00000:
                return OpOpFp_Fcvt_w_s(cpu, rd, rs1, rs2, rm);
            case 0b00001:
                return OpOpFp_Fcvt_wu_s(cpu, rd, rs1, rs2, rm);
            default:
                return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
            }
        }
        case 0b1110000: {
            if (rs2 == 0b00000) // Not actually rs2 - just the same bits.
            {
                switch (funct3)
                {
                case 0b000:
                    return OpOpFp_Fmv_x_w(cpu, rd, rs1);
                case 0b001:
                    return OpOpFp_Fclass_s(cpu, rd, rs1);
                default:
                    return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
                }
            }
            return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
        }
        case 0b1010000: {
            switch (funct3)
            {
            case 0b010: // FEQ.S
                return OpOpFp_Feq_s(cpu, rd, rs1, rs2);
            case 0b001: // FLT.S
                return OpOpFp_Flt_s(cpu, rd, rs1, rs2);
            case 0b000: // FLE.S
                return OpOpFp_Fle_s(cpu, rd, rs1, rs2);
            default:
                return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
            }
        }
        case 0b1101000: {
            switch (rs2) // No actually rs2 - just the same bits,
            {
            case 0b00000: // FCVT.S.W
                return OpOpFp_Fcvt_s_w(cpu, rd, rs1, rm);
            case 0b00001: // FVCT.S.WU
                return OpOpFp_Fcvt_s_wu(cpu, rd, rs1, rm);
            default:
                return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
            }
        }
        case 0b1111000:
            if (rs2 == 0b00000 && funct3 == 0b000) // FMV.W.X
            {
                return OpOpFp_Fmv_w_x(cpu, rd, rs1);
            }
            else
            {
                return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
            }
        default:
            return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
        }
    }

    default:
        return CreateTrap(cpu, trILLEGAL_INSTRUCTION, instruction);
    }
}

ArvissResult ArvissFetch(ArvissCpu* cpu)
{
    TRACE("%08x ", cpu->pc);
    ArvissResult result = ArvissReadWord(cpu->memory, cpu->pc);
    if (ArvissResultIsTrap(result))
    {
        result = TakeTrap(cpu, result);
    }
    return result;
}

ArvissResult ArvissRun(ArvissCpu* cpu, int count)
{
    ArvissResult result = ArvissMakeOk();
    for (int i = 0; i < count; i++)
    {
        result = ArvissFetch(cpu);
        if (ArvissResultIsWord(result))
        {
            result = ArvissExecute(cpu, ArvissResultAsWord(result));
        }

        if (ArvissResultIsTrap(result))
        {
            // Stop, as we can no longer proceeed.
            break;
        }
    }
    return result;
}
