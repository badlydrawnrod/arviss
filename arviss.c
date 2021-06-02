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

DecodedInstruction ArvissDecode(uint32_t instruction);

static inline ArvissResult TakeTrap(ArvissCpu* cpu, ArvissResult result);
static inline ArvissResult CreateTrap(ArvissCpu* cpu, ArvissTrapType trap, uint32_t value);

static ArvissResult Exec_IllegalInstruction(ArvissCpu* cpu, DecodedInstruction ins)
{
    return CreateTrap(cpu, trILLEGAL_INSTRUCTION, ins.ins);
}

static ArvissResult Exec_FetchDecodeReplace(ArvissCpu* cpu, DecodedInstruction ins)
{
    // Reconstitute the address given the cache line and index.
    const uint32_t cacheLine = ins.fdr.cacheLine;
    const uint32_t index = ins.fdr.index;
    struct CacheLine* line = &cpu->cache.line[cacheLine];
    const uint32_t owner = line->owner;
    const uint32_t addr = owner * 4 * CACHE_LINE_LENGTH + index * 4;

    // Fetch a word from memory at the address.
    ArvissResult result = ArvissReadWord(cpu->memory, addr);

    // Decode it, save the result in the cache, then execute it.
    if (ArvissResultIsWord(result))
    {
        uint32_t instruction = ArvissResultAsWord(result);
        DecodedInstruction decoded = ArvissDecode(instruction);
        line->instructions[index] = decoded;

        // Execute the decoded instruction.
        result = decoded.opcode(cpu, decoded);
    }

    return result;
}

static ArvissResult Exec_Lui(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- imm_u, pc += 4
    TRACE("LUI %s, %d\n", abiNames[ins.rd_imm.rd], ins.rd_imm.imm >> 12);
    cpu->xreg[ins.rd_imm.rd] = ins.rd_imm.imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Auipc(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- pc + imm_u, pc += 4
    TRACE("AUIPC %s, %d\n", abiNames[ins.rd_imm.rd], upper >> 12);
    cpu->xreg[ins.rd_imm.rd] = cpu->pc + ins.rd_imm.imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Jal(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- pc + 4, pc <- pc + imm_j
    TRACE("JAL %s, %d\n", abiNames[ins.rd_imm.rd], imm);
    cpu->xreg[ins.rd_imm.rd] = cpu->pc + 4;
    cpu->pc += ins.rd_imm.imm;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Jalr(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- pc + 4, pc <- (rs1 + imm_i) & ~1
    TRACE("JALR %s, %s, %d\n", abiNames[ins.rd_rs1_imm.rd], abiNames[ins.rd_rs1_imm.rs1], ins.rd_rs1_imm.imm);
    uint32_t rs1Before = cpu->xreg[ins.rd_rs1_imm.rs1]; // Because rd and rs1 might be the same register.
    cpu->xreg[ins.rd_rs1_imm.rd] = cpu->pc + 4;
    cpu->pc = (rs1Before + ins.rd_rs1_imm.imm) & ~1;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Beq(ArvissCpu* cpu, DecodedInstruction ins)
{
    // pc <- pc + ((rs1 == rs2) ? imm_b : 4)
    TRACE("BEQ %s, %s, %d\n", abiNames[ins.rs1_rs2_imm.rs1], abiNames[ins.rs1_rs2_imm.rs2], ins.rs1_rs2_imm.imm);
    cpu->pc += ((cpu->xreg[ins.rs1_rs2_imm.rs1] == cpu->xreg[ins.rs1_rs2_imm.rs2]) ? ins.rs1_rs2_imm.imm : 4);
    return ArvissMakeOk();
}

static ArvissResult Exec_Bne(ArvissCpu* cpu, DecodedInstruction ins)
{
    // pc <- pc + ((rs1 != rs2) ? imm_b : 4)
    TRACE("BNE %s, %s, %d\n", abiNames[ins.rs1_rs2_imm.rs1], abiNames[ins.rs1_rs2_imm.rs2], ins.rs1_rs2_imm.imm);
    cpu->pc += ((cpu->xreg[ins.rs1_rs2_imm.rs1] != cpu->xreg[ins.rs1_rs2_imm.rs2]) ? ins.rs1_rs2_imm.imm : 4);
    return ArvissMakeOk();
}

static ArvissResult Exec_Blt(ArvissCpu* cpu, DecodedInstruction ins)
{
    // pc <- pc + ((rs1 < rs2) ? imm_b : 4)
    TRACE("BLT %s, %s, %d\n", abiNames[ins.rs1_rs2_imm.rs1], abiNames[ins.rs1_rs2_imm.rs2], ins.rs1_rs2_imm.imm);
    cpu->pc += (((int32_t)cpu->xreg[ins.rs1_rs2_imm.rs1] < (int32_t)cpu->xreg[ins.rs1_rs2_imm.rs2]) ? ins.rs1_rs2_imm.imm : 4);
    return ArvissMakeOk();
}

static ArvissResult Exec_Bge(ArvissCpu* cpu, DecodedInstruction ins)
{
    // pc <- pc + ((rs1 >= rs2) ? imm_b : 4)
    TRACE("BGE %s, %s, %d\n", abiNames[ins.rs1_rs2_imm.rs1], abiNames[ins.rs1_rs2_imm.rs2], ins.rs1_rs2_imm.imm);
    cpu->pc += (((int32_t)cpu->xreg[ins.rs1_rs2_imm.rs1] >= (int32_t)cpu->xreg[ins.rs1_rs2_imm.rs2]) ? ins.rs1_rs2_imm.imm : 4);
    return ArvissMakeOk();
}

static ArvissResult Exec_Bltu(ArvissCpu* cpu, DecodedInstruction ins)
{
    // pc <- pc + ((rs1 < rs2) ? imm_b : 4)
    TRACE("BLTU %s, %s, %d\n", abiNames[ins.rs1_rs2_imm.rs1], abiNames[ins.rs1_rs2_imm.rs2], ins.rs1_rs2_imm.imm);
    cpu->pc += ((cpu->xreg[ins.rs1_rs2_imm.rs1] < cpu->xreg[ins.rs1_rs2_imm.rs2]) ? ins.rs1_rs2_imm.imm : 4);
    return ArvissMakeOk();
}

static ArvissResult Exec_Bgeu(ArvissCpu* cpu, DecodedInstruction ins)
{
    // pc <- pc + ((rs1 >= rs2) ? imm_b : 4)
    TRACE("BGEU %s, %s, %d\n", abiNames[ins.rs1_rs2_imm.rs1], abiNames[ins.rs1_rs2_imm.rs2], ins.rs1_rs2_imm.imm);
    cpu->pc += ((cpu->xreg[ins.rs1_rs2_imm.rs1] >= cpu->xreg[ins.rs1_rs2_imm.rs2]) ? ins.rs1_rs2_imm.imm : 4);
    return ArvissMakeOk();
}

static ArvissResult Exec_Lb(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- sx(m8(rs1 + imm_i)), pc += 4
    TRACE("LB %s, %d(%s)\n", abiNames[ins.rd_rs1_imm.rd], imm, abiNames[ins.rd_rs1_imm.rs1]);
    ArvissResult byteResult = ArvissReadByte(cpu->memory, cpu->xreg[ins.rd_rs1_imm.rs1] + ins.rd_rs1_imm.imm);
    if (!ArvissResultIsByte(byteResult))
    {
        return TakeTrap(cpu, byteResult);
    }
    cpu->xreg[ins.rd_rs1_imm.rd] = (int32_t)(int16_t)(int8_t)ArvissResultAsByte(byteResult);
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Lh(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- sx(m16(rs1 + imm_i)), pc += 4
    TRACE("LH %s, %d(%s)\n", abiNames[ins.rd_rs1_imm.rd], ins.rd_rs1_imm.imm, abiNames[ins.rd_rs1_imm.rs1]);
    ArvissResult halfwordResult = ArvissReadHalfword(cpu->memory, cpu->xreg[ins.rd_rs1_imm.rs1] + ins.rd_rs1_imm.imm);
    if (!ArvissResultIsHalfword(halfwordResult))
    {
        TakeTrap(cpu, halfwordResult);
        return halfwordResult;
    }
    cpu->xreg[ins.rd_rs1_imm.rd] = (int32_t)(int16_t)ArvissResultAsHalfword(halfwordResult);
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Lw(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- sx(m32(rs1 + imm_i)), pc += 4
    TRACE("LW %s, %d(%s)\n", abiNames[ins.rd_rs1_imm.rd], ins.rd_rs1_imm.imm, abiNames[ins.rd_rs1_imm.rs1]);
    ArvissResult wordResult = ArvissReadWord(cpu->memory, cpu->xreg[ins.rd_rs1_imm.rs1] + ins.rd_rs1_imm.imm);
    if (!ArvissResultIsWord(wordResult))
    {
        return TakeTrap(cpu, wordResult);
    }
    cpu->xreg[ins.rd_rs1_imm.rd] = (int32_t)ArvissResultAsWord(wordResult);
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Lbu(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- zx(m8(rs1 + imm_i)), pc += 4
    TRACE("LBU x%d, %d(x%d)\n", ins.rd_rs1_imm.rd, ins.rd_rs1_imm.imm, ins.rd_rs1_imm.rs1);
    ArvissResult byteResult = ArvissReadByte(cpu->memory, cpu->xreg[ins.rd_rs1_imm.rs1] + ins.rd_rs1_imm.imm);
    if (!ArvissResultIsByte(byteResult))
    {
        return TakeTrap(cpu, byteResult);
    }
    cpu->xreg[ins.rd_rs1_imm.rd] = ArvissResultAsByte(byteResult);
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Lhu(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- zx(m16(rs1 + imm_i)), pc += 4
    TRACE("LHU %s, %d(%s)\n", abiNames[ins.rd_rs1_imm.rd], ins.rd_rs1_imm.imm, abiNames[ins.rd_rs1_imm.rs1]);
    ArvissResult halfwordResult = ArvissReadHalfword(cpu->memory, cpu->xreg[ins.rd_rs1_imm.rs1] + ins.rd_rs1_imm.imm);
    if (!ArvissResultIsHalfword(halfwordResult))
    {
        return TakeTrap(cpu, halfwordResult);
    }
    cpu->xreg[ins.rd_rs1_imm.rd] = ArvissResultAsHalfword(halfwordResult);
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Sb(ArvissCpu* cpu, DecodedInstruction ins)
{
    // m8(rs1 + imm_s) <- rs2[7:0], pc += 4
    TRACE("SB %s, %d(%s)\n", abiNames[ins.rs1_rs2_imm.rs2], ins.rs1_rs2_imm.imm, abiNames[ins.rs1_rs2_imm.rs1]);
    ArvissResult byteResult = ArvissWriteByte(cpu->memory, cpu->xreg[ins.rs1_rs2_imm.rs1] + ins.rs1_rs2_imm.imm,
                                              cpu->xreg[ins.rs1_rs2_imm.rs2] & 0xff);
    if (ArvissResultIsTrap(byteResult))
    {
        return TakeTrap(cpu, byteResult);
    }
    cpu->pc += 4;
    return ArvissMakeOk();
}

static ArvissResult Exec_Sh(ArvissCpu* cpu, DecodedInstruction ins)
{
    // m16(rs1 + imm_s) <- rs2[15:0], pc += 4
    TRACE("SH %s, %d(%s)\n", abiNames[ins.rs1_rs2_imm.rs2], ins.rs1_rs2_imm.imm, abiNames[ins.rs1_rs2_imm.rs1]);
    ArvissResult halfwordResult = ArvissWriteHalfword(cpu->memory, cpu->xreg[ins.rs1_rs2_imm.rs1] + ins.rs1_rs2_imm.imm,
                                                      cpu->xreg[ins.rs1_rs2_imm.rs2] & 0xffff);
    if (ArvissResultIsTrap(halfwordResult))
    {
        return TakeTrap(cpu, halfwordResult);
    }
    cpu->pc += 4;
    return ArvissMakeOk();
}

static ArvissResult Exec_Sw(ArvissCpu* cpu, DecodedInstruction ins)
{
    // m32(rs1 + imm_s) <- rs2[31:0], pc += 4
    TRACE("SW %s, %d(%s)\n", abiNames[ins.rs1_rs2_imm.rs2], ins.rs1_rs2_imm.imm, abiNames[ins.rs1_rs2_imm.rs1]);
    ArvissResult wordResult =
            ArvissWriteWord(cpu->memory, cpu->xreg[ins.rs1_rs2_imm.rs1] + ins.rs1_rs2_imm.imm, cpu->xreg[ins.rs1_rs2_imm.rs2]);
    if (ArvissResultIsTrap(wordResult))
    {
        return TakeTrap(cpu, wordResult);
    }
    cpu->pc += 4;
    return ArvissMakeOk();
}

static ArvissResult Exec_Addi(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- rs1 + imm_i, pc += 4
    TRACE("ADDI %s, %s, %d\n", abiNames[ins.rd_rs1_imm.rd], abiNames[ins.rd_rs1_imm.rs1], ins.rd_rs1_imm.imm);
    cpu->xreg[ins.rd_rs1_imm.rd] = cpu->xreg[ins.rd_rs1_imm.rs1] + ins.rd_rs1_imm.imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Slti(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- (rs1 < imm_i) ? 1 : 0, pc += 4
    TRACE("SLTI %s, %s, %d\n", abiNames[ins.rd_rs1_imm.rd], abiNames[ins.rd_rs1_imm.rs1], ins.rd_rs1_imm.imm);
    cpu->xreg[ins.rd_rs1_imm.rd] = ((int32_t)cpu->xreg[ins.rd_rs1_imm.rs1] < ins.rd_rs1_imm.imm) ? 1 : 0;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Sltiu(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- (rs1 < imm_i) ? 1 : 0, pc += 4
    TRACE("SLTIU %s, %s, %d\n", abiNames[ins.rd_rs1_imm.rd], abiNames[ins.rd_rs1_imm.rs1], ins.rd_rs1_imm.imm);
    cpu->xreg[ins.rd_rs1_imm.rd] = (cpu->xreg[ins.rd_rs1_imm.rs1] < (uint32_t)ins.rd_rs1_imm.imm) ? 1 : 0;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Xori(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- rs1 ^ imm_i, pc += 4
    TRACE("XORI %s, %s, %d\n", abiNames[ins.rd_rs1_imm.rd], abiNames[ins.rd_rs1_imm.rs1], ins.rd_rs1_imm.imm);
    cpu->xreg[ins.rd_rs1_imm.rd] = cpu->xreg[ins.rd_rs1_imm.rs1] ^ ins.rd_rs1_imm.imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Ori(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- rs1 | imm_i, pc += 4
    TRACE("ORI %s, %s, %d\n", abiNames[ins.rd_rs1_imm.rd], abiNames[ins.rd_rs1_imm.rs1], ins.rd_rs1_imm.imm);
    cpu->xreg[ins.rd_rs1_imm.rd] = cpu->xreg[ins.rd_rs1_imm.rs1] | ins.rd_rs1_imm.imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Andi(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- rs1 & imm_i, pc += 4
    TRACE("ANDI %s, %s, %d\n", abiNames[ins.rd_rs1_imm.rd], abiNames[ins.rd_rs1_imm.rs1], ins.rd_rs1_imm.imm);
    cpu->xreg[ins.rd_rs1_imm.rd] = cpu->xreg[ins.rd_rs1_imm.rs1] & ins.rd_rs1_imm.imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Slli(ArvissCpu* cpu, DecodedInstruction ins)
{
    TRACE("SLLI %s, %s, %d\n", abiNames[ins.rd_rs1_imm.rd], abiNames[ins.rd_rs1_imm.rs1], ins.rd_rs1_imm.imm);
    cpu->xreg[ins.rd_rs1_imm.rd] = cpu->xreg[ins.rd_rs1_imm.rs1] << ins.rd_rs1_imm.imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Srli(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- rs1 >> shamt_i, pc += 4
    TRACE("SRLI %s, %s, %d\n", abiNames[ins.rd_rs1_imm.rd], abiNames[ins.rd_rs1_imm.rs1], immins.rd_rs1_imm.);
    cpu->xreg[ins.rd_rs1_imm.rd] = cpu->xreg[ins.rd_rs1_imm.rs1] >> ins.rd_rs1_imm.imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Srai(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- rs1 >> shamt_i, pc += 4
    TRACE("SRAI %s, %s, %d\n", abiNames[ins.rd_rs1_imm.rd], abiNames[ins.rd_rs1_imm.rs1], ins.rd_rs1_imm.imm);
    cpu->xreg[ins.rd_rs1_imm.rd] = (int32_t)cpu->xreg[ins.rd_rs1_imm.rs1] >> ins.rd_rs1_imm.imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Add(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- rs1 + rs2, pc += 4
    TRACE("ADD %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], abiNames[ins.rd_rs1_rs2.rs1], abiNames[ins.rd_rs1_rs2.rs2]);
    cpu->xreg[ins.rd_rs1_rs2.rd] = cpu->xreg[ins.rd_rs1_rs2.rs1] + cpu->xreg[ins.rd_rs1_rs2.rs2];
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Sub(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- rs1 - rs2, pc += 4
    TRACE("SUB %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], abiNames[ins.rd_rs1_rs2.rs1], abiNames[ins.rd_rs1_rs2.rs2]);
    cpu->xreg[ins.rd_rs1_rs2.rd] = cpu->xreg[ins.rd_rs1_rs2.rs1] - cpu->xreg[ins.rd_rs1_rs2.rs2];
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Mul(ArvissCpu* cpu, DecodedInstruction ins)
{
    TRACE("MUL %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], abiNames[ins.rd_rs1_rs2.rs1], abiNames[ins.rd_rs1_rs2.rs2]);
    cpu->xreg[ins.rd_rs1_rs2.rd] = cpu->xreg[ins.rd_rs1_rs2.rs1] * cpu->xreg[ins.rd_rs1_rs2.rs2];
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Sll(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- rs1 << (rs2 % XLEN), pc += 4
    TRACE("SLL %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], abiNames[ins.rd_rs1_rs2.rs1], abiNames[ins.rd_rs1_rs2.rs2]);
    cpu->xreg[ins.rd_rs1_rs2.rd] = cpu->xreg[ins.rd_rs1_rs2.rs1] << (cpu->xreg[ins.rd_rs1_rs2.rs2] % 32);
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Mulh(ArvissCpu* cpu, DecodedInstruction ins)
{
    TRACE("MULH %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], abiNames[ins.rd_rs1_rs2.rs1], abiNames[ins.rd_rs1_rs2.rs2]);
    int64_t t = (int64_t)(int32_t)cpu->xreg[ins.rd_rs1_rs2.rs1] * (int64_t)(int32_t)cpu->xreg[ins.rd_rs1_rs2.rs2];
    cpu->xreg[ins.rd_rs1_rs2.rd] = t >> 32;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Slt(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- (rs1 < rs2) ? 1 : 0, pc += 4
    TRACE("SLT %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], abiNames[ins.rd_rs1_rs2.rs1], abiNames[ins.rd_rs1_rs2.rs2]);
    cpu->xreg[ins.rd_rs1_rs2.rd] = ((int32_t)cpu->xreg[ins.rd_rs1_rs2.rs1] < (int32_t)cpu->xreg[ins.rd_rs1_rs2.rs2]) ? 1 : 0;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Mulhsu(ArvissCpu* cpu, DecodedInstruction ins)
{
    TRACE("MULHSU %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], abiNames[ins.rd_rs1_rs2.rs1], abiNames[ins.rd_rs1_rs2.rs2]);
    int64_t t = (int64_t)(int32_t)cpu->xreg[ins.rd_rs1_rs2.rs1] * (uint64_t)cpu->xreg[ins.rd_rs1_rs2.rs2];
    cpu->xreg[ins.rd_rs1_rs2.rd] = t >> 32;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Sltu(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- (rs1 < rs2) ? 1 : 0, pc += 4
    TRACE("SLTU %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], abiNames[ins.rd_rs1_rs2.rs1], abiNames[ins.rd_rs1_rs2.rs2]);
    cpu->xreg[ins.rd_rs1_rs2.rd] = (cpu->xreg[ins.rd_rs1_rs2.rs1] < cpu->xreg[ins.rd_rs1_rs2.rs2]) ? 1 : 0;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Mulhu(ArvissCpu* cpu, DecodedInstruction ins)
{
    TRACE("MULHU %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], abiNames[ins.rd_rs1_rs2.rs1], abiNames[ins.rd_rs1_rs2.rs2]);
    uint64_t t = (uint64_t)cpu->xreg[ins.rd_rs1_rs2.rs1] * (uint64_t)cpu->xreg[ins.rd_rs1_rs2.rs2];
    cpu->xreg[ins.rd_rs1_rs2.rd] = t >> 32;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Xor(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- rs1 ^ rs2, pc += 4
    TRACE("XOR %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], abiNames[ins.rd_rs1_rs2.rs1], abiNames[ins.rd_rs1_rs2.rs2]);
    cpu->xreg[ins.rd_rs1_rs2.rd] = cpu->xreg[ins.rd_rs1_rs2.rs1] ^ cpu->xreg[ins.rd_rs1_rs2.rs2];
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Div(ArvissCpu* cpu, DecodedInstruction ins)
{
    TRACE("DIV %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], abiNames[ins.rd_rs1_rs2.rs1], abiNames[ins.rd_rs1_rs2.rs2]);
    const int32_t dividend = (int32_t)cpu->xreg[ins.rd_rs1_rs2.rs1];
    const int32_t divisor = (int32_t)cpu->xreg[ins.rd_rs1_rs2.rs2];
    // Check for signed division overflow.
    if (dividend != 0x80000000 || divisor != -1)
    {
        cpu->xreg[ins.rd_rs1_rs2.rd] = divisor != 0 // Check for division by zero.
                ? dividend / divisor
                : -1;
    }
    else
    {
        // Signed division overflow occurred.
        cpu->xreg[ins.rd_rs1_rs2.rd] = dividend;
    }
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Srl(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- rs1 >> (rs2 % XLEN), pc += 4
    TRACE("SRL %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], abiNames[ins.rd_rs1_rs2.rs1], abiNames[ins.rd_rs1_rs2.rs2]);
    cpu->xreg[ins.rd_rs1_rs2.rd] = cpu->xreg[ins.rd_rs1_rs2.rs1] >> (cpu->xreg[ins.rd_rs1_rs2.rs2] % 32);
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Sra(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- rs1 >> (rs2 % XLEN), pc += 4
    TRACE("SRA %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], abiNames[ins.rd_rs1_rs2.rs1], abiNames[ins.rd_rs1_rs2.rs2]);
    cpu->xreg[ins.rd_rs1_rs2.rd] = (int32_t)cpu->xreg[ins.rd_rs1_rs2.rs1] >> (cpu->xreg[ins.rd_rs1_rs2.rs2] % 32);
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Divu(ArvissCpu* cpu, DecodedInstruction ins)
{
    TRACE("DIVU %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], abiNames[ins.rd_rs1_rs2.rs1], abiNames[ins.rd_rs1_rs2.rs2]);
    uint32_t divisor = cpu->xreg[ins.rd_rs1_rs2.rs2];
    cpu->xreg[ins.rd_rs1_rs2.rd] = divisor != 0 ? cpu->xreg[ins.rd_rs1_rs2.rs1] / divisor : 0xffffffff;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Or(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- rs1 | rs2, pc += 4
    TRACE("OR %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], abiNames[ins.rd_rs1_rs2.rs1], abiNames[ins.rd_rs1_rs2.rs2]);
    cpu->xreg[ins.rd_rs1_rs2.rd] = cpu->xreg[ins.rd_rs1_rs2.rs1] | cpu->xreg[ins.rd_rs1_rs2.rs2];
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Rem(ArvissCpu* cpu, DecodedInstruction ins)
{
    TRACE("REM %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], abiNames[ins.rd_rs1_rs2.rs1], abiNames[ins.rd_rs1_rs2.rs2]);
    const int32_t dividend = (int32_t)cpu->xreg[ins.rd_rs1_rs2.rs1];
    const int32_t divisor = (int32_t)cpu->xreg[ins.rd_rs1_rs2.rs2];
    // Check for signed division overflow.
    if (dividend != 0x80000000 || divisor != -1)
    {
        cpu->xreg[ins.rd_rs1_rs2.rd] = divisor != 0 // Check for division by zero.
                ? dividend % divisor
                : dividend;
    }
    else
    {
        // Signed division overflow occurred.
        cpu->xreg[ins.rd_rs1_rs2.rd] = 0;
    }
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_And(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- rs1 & rs2, pc += 4
    TRACE("AND %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], abiNames[ins.rd_rs1_rs2.rs1], abiNames[ins.rd_rs1_rs2.rs2]);
    cpu->xreg[ins.rd_rs1_rs2.rd] = cpu->xreg[ins.rd_rs1_rs2.rs1] & cpu->xreg[ins.rd_rs1_rs2.rs2];
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Remu(ArvissCpu* cpu, DecodedInstruction ins)
{
    TRACE("REMU %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], abiNames[ins.rd_rs1_rs2.rs1], abiNames[ins.rd_rs1_rs2.rs2]);
    const uint32_t dividend = cpu->xreg[ins.rd_rs1_rs2.rs1];
    const uint32_t divisor = cpu->xreg[ins.rd_rs1_rs2.rs2];
    cpu->xreg[ins.rd_rs1_rs2.rd] = divisor != 0 ? cpu->xreg[ins.rd_rs1_rs2.rs1] % divisor : dividend;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    return ArvissMakeOk();
}

static ArvissResult Exec_Fence(ArvissCpu* cpu, DecodedInstruction ins)
{
    TRACE("FENCE\n");
    return CreateTrap(cpu, trNOT_IMPLEMENTED_YET, 0);
}

static ArvissResult Exec_Ecall(ArvissCpu* cpu, DecodedInstruction ins)
{
    TRACE("ECALL\n");
    return CreateTrap(cpu, trENVIRONMENT_CALL_FROM_M_MODE, 0);
}

static ArvissResult Exec_Ebreak(ArvissCpu* cpu, DecodedInstruction ins)
{
    TRACE("EBREAK\n");
    return CreateTrap(cpu, trBREAKPOINT, 0); // TODO: what should value be here?
}

static ArvissResult Exec_Uret(ArvissCpu* cpu, DecodedInstruction ins)
{
    TRACE("URET\n");
    // TODO: Only provide this if user mode traps are supported, otherwise raise an illegal instruction exception.
    return CreateTrap(cpu, trNOT_IMPLEMENTED_YET, 0);
}

static ArvissResult Exec_Sret(ArvissCpu* cpu, DecodedInstruction ins)
{
    TRACE("SRET\n");
    // TODO: Only provide this if supervisor mode is supported, otherwise raise an illegal instruction exception.
    return CreateTrap(cpu, trNOT_IMPLEMENTED_YET, 0);
}

static ArvissResult Exec_Mret(ArvissCpu* cpu, DecodedInstruction ins)
{
    // pc <- mepc, pc += 4
    TRACE("MRET\n");
    cpu->pc = cpu->mepc; // Restore the program counter from the machine exception program counter.
    cpu->pc += 4;        // ...and increment it as normal.
    return ArvissMakeOk();
}

static ArvissResult Exec_Flw(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- f32(rs1 + imm_i)
    TRACE("FLW %s, %d(%s)\n", fabiNames[ins.rd_rs1_imm.rd], imm, abiNames[ins.rd_rs1_imm.rs1]);
    ArvissResult wordResult = ArvissReadWord(cpu->memory, cpu->xreg[ins.rd_rs1_imm.rs1] + ins.rd_rs1_imm.imm);
    if (!ArvissResultIsWord(wordResult))
    {
        return TakeTrap(cpu, wordResult);
    }
    const uint32_t resultAsWord = ArvissResultAsWord(wordResult);
    const float resultAsFloat = U32AsFloat(resultAsWord);
    cpu->freg[ins.rd_rs1_imm.rd] = resultAsFloat;
    cpu->pc += 4;
    return ArvissMakeOk();
}

static ArvissResult Exec_Fsw(ArvissCpu* cpu, DecodedInstruction ins)
{
    // f32(rs1 + imm_s) = rs2
    TRACE("FSW %s, %d(%s)\n", fabiNames[ins.rs1_rs2_imm.rs2], imm, abiNames[ins.rs1_rs2_imm.rs1]);
    uint32_t t = FloatAsU32(cpu->freg[ins.rs1_rs2_imm.rs2]);
    ArvissResult wordResult = ArvissWriteWord(cpu->memory, cpu->xreg[ins.rs1_rs2_imm.rs1] + ins.rs1_rs2_imm.imm, t);
    if (ArvissResultIsTrap(wordResult))
    {
        return TakeTrap(cpu, wordResult);
    }
    cpu->pc += 4;
    return ArvissMakeOk();
}

static ArvissResult Exec_Fmadd_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- (rs1 * rs2) + rs3
    TRACE("FMADD.S %s, %s, %s, %s, %s\n", fabiNames[ins.rd_rs1_rs2_rs3_rm.rd], fabiNames[ins.rd_rs1_rs2_rs3_rm.rs1],
          fabiNames[ins.rd_rs1_rs2_rs3_rm.rs2], fabiNames[ins.rd_rs1_rs2_rs3_rm.rs3], roundingModes[ins.rd_rs1_rs2_rs3_rm.rm]);
    cpu->freg[ins.rd_rs1_rs2_rs3_rm.rd] =
            (cpu->freg[ins.rd_rs1_rs2_rs3_rm.rs1] * cpu->freg[ins.rd_rs1_rs2_rs3_rm.rs2]) + cpu->freg[ins.rd_rs1_rs2_rs3_rm.rs3];
    cpu->pc += 4;
    // TODO: rounding.
    return ArvissMakeOk();
}

static ArvissResult Exec_Fmsub_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- (rs1 x rs2) - rs3
    TRACE("FMSUB.S %s, %s, %s, %s, %s\n", fabiNames[ins.rd_rs1_rs2_rs3_rm.rd], fabiNames[ins.rd_rs1_rs2_rs3_rm.rs1],
          fabiNames[ins.rd_rs1_rs2_rs3_rm.rs2], fabiNames[ins.rd_rs1_rs2_rs3_rm.rs3], roundingModes[ins.rd_rs1_rs2_rs3_rm.rm]);
    cpu->freg[ins.rd_rs1_rs2_rs3_rm.rd] =
            (cpu->freg[ins.rd_rs1_rs2_rs3_rm.rs1] * cpu->freg[ins.rd_rs1_rs2_rs3_rm.rs2]) - cpu->freg[ins.rd_rs1_rs2_rs3_rm.rs3];
    cpu->pc += 4;
    // TODO: rounding.
    return ArvissMakeOk();
}

static ArvissResult Exec_Fnmsub_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- -(rs1 x rs2) + rs3
    TRACE("FNMSUB.S %s, %s, %s, %s, %s\n", fabiNames[ins.rd_rs1_rs2_rs3_rm.rd], fabiNames[ins.rd_rs1_rs2_rs3_rm.rs1],
          fabiNames[ins.rd_rs1_rs2_rs3_rm.rs2], fabiNames[ins.rd_rs1_rs2_rs3_rm.rs3], roundingModes[ins.rd_rs1_rs2_rs3_rm.rm]);
    cpu->freg[ins.rd_rs1_rs2_rs3_rm.rd] =
            -(cpu->freg[ins.rd_rs1_rs2_rs3_rm.rs1] * cpu->freg[ins.rd_rs1_rs2_rs3_rm.rs2]) + cpu->freg[ins.rd_rs1_rs2_rs3_rm.rs3];
    cpu->pc += 4;
    // TODO: rounding.
    return ArvissMakeOk();
}

static ArvissResult Exec_Fnmadd_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- -(rs1 x rs2) - rs3
    TRACE("FNMADD.S %s, %s, %s, %s, %s\n", fabiNames[ins.rd_rs1_rs2_rs3_rm.rd], fabiNames[ins.rd_rs1_rs2_rs3_rm.rs1],
          fabiNames[ins.rd_rs1_rs2_rs3_rm.rs2], fabiNames[ins.rd_rs1_rs2_rs3_rm.rs3], roundingModes[ins.rd_rs1_rs2_rs3_rm.rm]);
    cpu->freg[ins.rd_rs1_rs2_rs3_rm.rd] =
            -(cpu->freg[ins.rd_rs1_rs2_rs3_rm.rs1] * cpu->freg[ins.rd_rs1_rs2_rs3_rm.rs2]) - cpu->freg[ins.rd_rs1_rs2_rs3_rm.rs3];
    cpu->pc += 4;
    // TODO: rounding.
    return ArvissMakeOk();
}

static ArvissResult Exec_Fadd_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- rs1 + rs2
    TRACE("FADD.S %s, %s, %s, %s\n", fabiNames[ins.rd_rs1_rs2_rm.rd], fabiNames[ins.rd_rs1_rs2_rm.rs1],
          fabiNames[ins.rd_rs1_rs2_rm.rs2], roundingModes[ins.rd_rs1_rs2_rm.rm]);
    cpu->freg[ins.rd_rs1_rs2_rm.rd] = cpu->freg[ins.rd_rs1_rs2_rm.rs1] + cpu->freg[ins.rd_rs1_rs2_rm.rs2];
    cpu->pc += 4;
    // TODO: rounding.
    return ArvissMakeOk();
}

static ArvissResult Exec_Fsub_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- rs1 - rs2
    TRACE("FSUB.S %s, %s, %s, %s\n", fabiNames[ins.rd_rs1_rs2_rm.rd], fabiNames[ins.rd_rs1_rs2_rm.rs1],
          fabiNames[ins.rd_rs1_rs2_rm.rs2], roundingModes[ins.rd_rs1_rs2_rm.rm]);
    cpu->freg[ins.rd_rs1_rs2_rm.rd] = cpu->freg[ins.rd_rs1_rs2_rm.rs1] - cpu->freg[ins.rd_rs1_rs2_rm.rs2];
    cpu->pc += 4;
    // TODO: rounding.
    return ArvissMakeOk();
}

static ArvissResult Exec_Fmul_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- rs1 * rs2
    TRACE("FMUL.S %s, %s, %s, %s\n", fabiNames[ins.rd_rs1_rs2_rm.rd], fabiNames[ins.rd_rs1_rs2_rm.rs1],
          fabiNames[ins.rd_rs1_rs2_rm.rs2], roundingModes[ins.rd_rs1_rs2_rm.rm]);
    cpu->freg[ins.rd_rs1_rs2_rm.rd] = cpu->freg[ins.rd_rs1_rs2_rm.rs1] * cpu->freg[ins.rd_rs1_rs2_rm.rs2];
    cpu->pc += 4;
    // TODO: rounding.
    return ArvissMakeOk();
}

static ArvissResult Exec_Fdiv_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- rs1 / rs2
    TRACE("FDIV.S %s, %s, %s, %s\n", fabiNames[ins.rd_rs1_rs2_rm.rd], fabiNames[ins.rd_rs1_rs2_rm.rs1],
          fabiNames[ins.rd_rs1_rs2_rm.rs2], roundingModes[ins.rd_rs1_rs2_rm.rm]);
    cpu->freg[ins.rd_rs1_rs2_rm.rd] = cpu->freg[ins.rd_rs1_rs2_rm.rs1] / cpu->freg[ins.rd_rs1_rs2_rm.rs2];
    cpu->pc += 4;
    // TODO: rounding.
    return ArvissMakeOk();
}

static ArvissResult Exec_Fsqrt_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- sqrt(rs1)
    TRACE("FSQRT.S %s, %s, %s\n", fabiNames[ins.rd_rs1_rm.rd], fabiNames[ins.rd_rs1_rm.rs1], roundingModes[ins.rd_rs1_rm.rm]);
    cpu->freg[ins.rd_rs1_rm.rd] = sqrtf(cpu->freg[ins.rd_rs1_rm.rs1]);
    cpu->pc += 4;
    // TODO: rounding.
    return ArvissMakeOk();
}

static ArvissResult Exec_Fsgnj_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- abs(rs1) * sgn(rs2)
    TRACE("FSGNJ.S %s, %s, %s\n", fabiNames[ins.rd_rs1_rs2.rd], fabiNames[ins.rd_rs1_rs2.rs1], fabiNames[ins.rd_rs1_rs2.rs2]);
    cpu->freg[ins.rd_rs1_rs2.rd] = fabsf(cpu->freg[ins.rd_rs1_rs2.rs1]) * (cpu->freg[ins.rd_rs1_rs2.rs2] < 0.0f ? -1.0f : 1.0f);
    cpu->pc += 4;
    return ArvissMakeOk();
}

static ArvissResult Exec_Fsgnjn_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- abs(rs1) * -sgn(rs2)
    TRACE("FSGNJN.S %s, %s, %s\n", fabiNames[ins.rd_rs1_rs2.rd], fabiNames[ins.rd_rs1_rs2.rs1], fabiNames[ins.rd_rs1_rs2.rs2]);
    cpu->freg[ins.rd_rs1_rs2.rd] = fabsf(cpu->freg[ins.rd_rs1_rs2.rs1]) * (cpu->freg[ins.rd_rs1_rs2.rs2] < 0.0f ? 1.0f : -1.0f);
    cpu->pc += 4;
    return ArvissMakeOk();
}

static ArvissResult Exec_Fsgnjx_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    float m; // The sign bit is the XOR of the sign bits of rs1 and rs2.
    if ((cpu->freg[ins.rd_rs1_rs2.rs1] < 0.0f && cpu->freg[ins.rd_rs1_rs2.rs2] >= 0.0f)
        || (cpu->freg[ins.rd_rs1_rs2.rs1] >= 0.0f && cpu->freg[ins.rd_rs1_rs2.rs2] < 0.0f))
    {
        m = -1.0f;
    }
    else
    {
        m = 1.0f;
    }
    // rd <- abs(rs1) * (sgn(rs1) == sgn(rs2)) ? 1 : -1
    TRACE("FSGNJX.S %s, %s, %s\n", fabiNames[ins.rd_rs1_rs2.rd], fabiNames[ins.rd_rs1_rs2.rs1], fabiNames[ins.rd_rs1_rs2.rs2]);
    cpu->freg[ins.rd_rs1_rs2.rd] = fabsf(cpu->freg[ins.rd_rs1_rs2.rs1]) * m;
    cpu->pc += 4;
    return ArvissMakeOk();
}

static ArvissResult Exec_Fmin_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- min(rs1, rs2)
    TRACE("FMIN.S %s, %s, %s\n", fabiNames[ins.rd_rs1_rs2.rd], fabiNames[ins.rd_rs1_rs2.rs1], fabiNames[ins.rd_rs1_rs2.rs2]);
    cpu->freg[ins.rd_rs1_rs2.rd] = fminf(cpu->freg[ins.rd_rs1_rs2.rs1], cpu->freg[ins.rd_rs1_rs2.rs2]);
    cpu->pc += 4;
    return ArvissMakeOk();
}

static ArvissResult Exec_Fmax_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- max(rs1, rs2)
    TRACE("FMAX.S %s, %s, %s\n", fabiNames[ins.rd_rs1_rs2.rd], fabiNames[ins.rd_rs1_rs2.rs1], fabiNames[ins.rd_rs1_rs2.rs2]);
    cpu->freg[ins.rd_rs1_rs2.rd] = fmaxf(cpu->freg[ins.rd_rs1_rs2.rs1], cpu->freg[ins.rd_rs1_rs2.rs2]);
    cpu->pc += 4;
    return ArvissMakeOk();
}

static ArvissResult Exec_Fcvt_w_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- int32_t(rs1)
    TRACE("FCVT.W.S %s, %s, %s\n", abiNames[ins.rd_rs1_rm.rd], fabiNames[ins.rd_rs1_rm.rs1], roundingModes[ins.rd_rs1_rm.rm]);
    cpu->xreg[ins.rd_rs1_rm.rd] = (int32_t)cpu->freg[ins.rd_rs1_rm.rs1];
    cpu->pc += 4;
    // TODO: rounding.
    return ArvissMakeOk();
}

static ArvissResult Exec_Fcvt_wu_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- uint32_t(rs1)
    TRACE("FCVT.WU.S %s, %s, %s\n", abiNames[ins.rd_rs1_rm.rd], fabiNames[ins.rd_rs1_rm.rs1], roundingModes[ins.rd_rs1_rm.rm]);
    cpu->xreg[ins.rd_rs1_rm.rd] = (uint32_t)(int32_t)cpu->freg[ins.rd_rs1_rm.rs1];
    cpu->pc += 4;
    // TODO: rounding.
    return ArvissMakeOk();
}

static ArvissResult Exec_Fmv_x_w(ArvissCpu* cpu, DecodedInstruction ins)
{
    // bits(rd) <- bits(rs1)
    TRACE("FMV.X.W %s, %s\n", abiNames[ins.rd_rs1.rd], fabiNames[ins.rd_rs1.rs1]);
    cpu->xreg[ins.rd_rs1.rd] = FloatAsU32(cpu->freg[ins.rd_rs1.rs1]);
    cpu->pc += 4;
    return ArvissMakeOk();
}

static ArvissResult Exec_Fclass_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    TRACE("FCLASS.S %s, %s\n", abiNames[ins.rd_rs1.rd], fabiNames[ins.rd_rs1.rs1]);
    const float v = cpu->freg[ins.rd_rs1.rs1];
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
    cpu->xreg[ins.rd_rs1.rd] = result;
    cpu->pc += 4;
    return ArvissMakeOk();
}

static ArvissResult Exec_Feq_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- (rs1 == rs2) ? 1 : 0;
    TRACE("FEQ.S %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], fabiNames[ins.rd_rs1_rs2.rs1], fabiNames[ins.rd_rs1_rs2.rs2]);
    cpu->xreg[ins.rd_rs1_rs2.rd] = cpu->freg[ins.rd_rs1_rs2.rs1] == cpu->freg[ins.rd_rs1_rs2.rs2] ? 1 : 0;
    cpu->pc += 4;
    return ArvissMakeOk();
}

static ArvissResult Exec_Flt_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- (rs1 < rs2) ? 1 : 0;
    TRACE("FLT.S %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], fabiNames[ins.rd_rs1_rs2.rs1], fabiNames[ins.rd_rs1_rs2.rs2]);
    cpu->xreg[ins.rd_rs1_rs2.rd] = cpu->freg[ins.rd_rs1_rs2.rs1] < cpu->freg[ins.rd_rs1_rs2.rs2] ? 1 : 0;
    cpu->pc += 4;
    return ArvissMakeOk();
}

static ArvissResult Exec_Fle_s(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- (rs1 <= rs2) ? 1 : 0;
    TRACE("FLE.S %s, %s, %s\n", abiNames[ins.rd_rs1_rs2.rd], fabiNames[ins.rd_rs1_rs2.rs1], fabiNames[ins.rd_rs1_rs2.rs2]);
    cpu->xreg[ins.rd_rs1_rs2.rd] = cpu->freg[ins.rd_rs1_rs2.rs1] <= cpu->freg[ins.rd_rs1_rs2.rs2] ? 1 : 0;
    cpu->pc += 4;
    return ArvissMakeOk();
}

static ArvissResult Exec_Fcvt_s_w(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- float(int32_t((rs1))
    TRACE("FCVT.S.W %s, %s, %s\n", fabiNames[ins.rd_rs1_rm.rd], abiNames[ins.rd_rs1_rm.rs1], roundingModes[ins.rd_rs1_rm.rm]);
    cpu->freg[ins.rd_rs1_rm.rd] = (float)(int32_t)cpu->xreg[ins.rd_rs1_rm.rs1];
    cpu->pc += 4;
    // TODO: rounding.
    return ArvissMakeOk();
}

static ArvissResult Exec_Fcvt_s_wu(ArvissCpu* cpu, DecodedInstruction ins)
{
    // rd <- float(rs1)
    TRACE("FVCT.S.WU %s, %s, %s\n", fabiNames[ins.rd_rs1_rm.rd], abiNames[ins.rd_rs1_rm.rs1], roundingModes[ins.rd_rs1_rm.rm]);
    cpu->freg[ins.rd_rs1_rm.rd] = (float)cpu->xreg[ins.rd_rs1_rm.rs1];
    cpu->pc += 4;
    // TODO: rounding.
    return ArvissMakeOk();
}

static ArvissResult Exec_Fmv_w_x(ArvissCpu* cpu, DecodedInstruction ins)
{
    // bits(rd) <- bits(rs1)
    TRACE("FMV.W.X %s, %s\n", fabiNames[ins.rd_rs1.rd], abiNames[ins.rd_rs1.rs1]);
    cpu->freg[ins.rd_rs1.rd] = U32AsFloat(cpu->xreg[ins.rd_rs1.rs1]);
    cpu->pc += 4;
    return ArvissMakeOk();
}

static inline DecodedInstruction MkNoArg(ExecFn opcode)
{
    return (DecodedInstruction){.opcode = opcode};
}

static inline DecodedInstruction MkTrap(ExecFn opcode, uint32_t instruction)
{
    return (DecodedInstruction){.opcode = opcode, .ins = instruction};
}

static inline DecodedInstruction MkFetchDecodeReplace(ExecFn opcode, uint32_t cacheLine, uint32_t index)
{
    return (DecodedInstruction){.opcode = opcode, .fdr = {.cacheLine = cacheLine, .index = index}};
}

static inline DecodedInstruction MkRdImm(ExecFn opcode, uint8_t rd, int32_t imm)
{
    return (DecodedInstruction){.opcode = opcode, .rd_imm = {.rd = rd, .imm = imm}};
}

static inline DecodedInstruction MkRdRs1Imm(ExecFn opcode, uint8_t rd, uint8_t rs1, int32_t imm)
{
    return (DecodedInstruction){.opcode = opcode, .rd_rs1_imm = {.rd = rd, .rs1 = rs1, .imm = imm}};
}

static inline DecodedInstruction MkRdRs1(ExecFn opcode, uint8_t rd, uint8_t rs1)
{
    return (DecodedInstruction){.opcode = opcode, .rd_rs1 = {.rd = rd, .rs1 = rs1}};
}

static inline DecodedInstruction MkRdRs1Rs2(ExecFn opcode, uint8_t rd, uint8_t rs1, uint8_t rs2)
{
    return (DecodedInstruction){.opcode = opcode, .rd_rs1_rs2 = {.rd = rd, .rs1 = rs1, .rs2 = rs2}};
}

static inline DecodedInstruction MkRs1Rs2Imm(ExecFn opcode, uint8_t rs1, uint8_t rs2, int32_t imm)
{
    return (DecodedInstruction){.opcode = opcode, .rs1_rs2_imm = {.rs1 = rs1, .rs2 = rs2, .imm = imm}};
}

static inline DecodedInstruction MkRdRs1Rs2Rs3Rm(ExecFn opcode, uint8_t rd, uint8_t rs1, uint8_t rs2, uint8_t rs3, uint8_t rm)
{
    return (DecodedInstruction){.opcode = opcode, .rd_rs1_rs2_rs3_rm = {.rd = rd, .rs1 = rs1, .rs2 = rs2, .rs3 = rs3, .rm = rm}};
}

static inline DecodedInstruction MkRdRs1Rm(ExecFn opcode, uint8_t rd, uint8_t rs1, uint8_t rm)
{
    return (DecodedInstruction){.opcode = opcode, .rd_rs1_rm = {.rd = rd, .rs1 = rs1, .rm = rm}};
}

static inline DecodedInstruction MkRdRs1Rs2Rm(ExecFn opcode, uint8_t rd, uint8_t rs1, uint8_t rs2, uint8_t rm)
{
    return (DecodedInstruction){.opcode = opcode, .rd_rs1_rs2_rm = {.rd = rd, .rs1 = rs1, .rs2 = rs2, .rm = rm}};
}

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
    for (int i = 0; i < CACHE_LINES; i++)
    {
        cpu->cache.line[i].isValid = false;
    }
}

// See: http://www.five-embeddev.com/riscv-isa-manual/latest/gmaps.html#rv3264g-instruction-set-listings
// or riscv-spec-209191213.pdf.
DecodedInstruction ArvissDecode(uint32_t instruction)
{
    const uint32_t opcode = Opcode(instruction);
    const uint32_t rd = Rd(instruction);
    const uint32_t rs1 = Rs1(instruction);

    switch (opcode)
    {
    case OP_LUI: {
        const int32_t upper = UImmediate(instruction);
        return MkRdImm(Exec_Lui, rd, upper);
    }

    case OP_AUIPC: {
        const int32_t upper = UImmediate(instruction);
        return MkRdImm(Exec_Auipc, rd, upper);
    }

    case OP_JAL: {
        const int32_t imm = JImmediate(instruction);
        return MkRdImm(Exec_Jal, rd, imm);
    }

    case OP_JALR: {
        const uint32_t funct3 = Funct3(instruction);
        if (funct3 == 0b000)
        {
            const int32_t imm = IImmediate(instruction);
            return MkRdRs1Imm(Exec_Jalr, rd, rs1, imm);
        }
        return MkTrap(Exec_IllegalInstruction, instruction);
    }

    case OP_BRANCH: {
        const uint32_t funct3 = Funct3(instruction);
        const uint32_t rs2 = Rs2(instruction);
        const int32_t imm = BImmediate(instruction);
        switch (funct3)
        {
        case 0b000: // BEQ
            return MkRs1Rs2Imm(Exec_Beq, rs1, rs2, imm);
        case 0b001: // BNE
            return MkRs1Rs2Imm(Exec_Bne, rs1, rs2, imm);
        case 0b100: // BLT
            return MkRs1Rs2Imm(Exec_Blt, rs1, rs2, imm);
        case 0b101: // BGE
            return MkRs1Rs2Imm(Exec_Bge, rs1, rs2, imm);
        case 0b110: // BLTU
            return MkRs1Rs2Imm(Exec_Bltu, rs1, rs2, imm);
        case 0b111: // BGEU
            return MkRs1Rs2Imm(Exec_Bgeu, rs1, rs2, imm);
        default:
            return MkTrap(Exec_IllegalInstruction, instruction);
        }
    }

    case OP_LOAD: {
        const uint32_t funct3 = Funct3(instruction);
        const int32_t imm = IImmediate(instruction);
        switch (funct3)
        {
        case 0b000: // LB
            return MkRdRs1Imm(Exec_Lb, rd, rs1, imm);
        case 0b001: // LH
            return MkRdRs1Imm(Exec_Lh, rd, rs1, imm);
        case 0b010: // LW
            return MkRdRs1Imm(Exec_Lw, rd, rs1, imm);
        case 0b100: // LBU
            return MkRdRs1Imm(Exec_Lbu, rd, rs1, imm);
        case 0b101: // LHU
            return MkRdRs1Imm(Exec_Lhu, rd, rs1, imm);
        default:
            return MkTrap(Exec_IllegalInstruction, instruction);
        }
    }

    case OP_STORE: {
        const uint32_t funct3 = Funct3(instruction);
        const uint32_t rs2 = Rs2(instruction);
        const int32_t imm = SImmediate(instruction);
        switch (funct3)
        {
        case 0b000: // SB
            return MkRs1Rs2Imm(Exec_Sb, rs1, rs2, imm);
        case 0b001: // SH
            return MkRs1Rs2Imm(Exec_Sh, rs1, rs2, imm);
        case 0b010: // SW
            return MkRs1Rs2Imm(Exec_Sw, rs1, rs2, imm);
        default:
            return MkTrap(Exec_IllegalInstruction, instruction);
        }
    }

    case OP_OPIMM: {
        const uint32_t funct3 = Funct3(instruction);
        const int32_t imm = IImmediate(instruction);
        const uint32_t funct7 = Funct7(instruction);
        switch (funct3)
        {
        case 0b000: // ADDI
            return MkRdRs1Imm(Exec_Addi, rd, rs1, imm);
        case 0b010: // SLTI
            return MkRdRs1Imm(Exec_Slti, rd, rs1, imm);
        case 0b011: // SLTIU
            return MkRdRs1Imm(Exec_Sltiu, rd, rs1, imm);
        case 0b100: // XORI
            return MkRdRs1Imm(Exec_Xori, rd, rs1, imm);
        case 0b110: // ORI
            return MkRdRs1Imm(Exec_Ori, rd, rs1, imm);
        case 0b111: // ANDI
            return MkRdRs1Imm(Exec_Andi, rd, rs1, imm);
        case 0b001: // SLLI
            return MkRdRs1Imm(Exec_Slli, rd, rs1, imm);
        case 0b101:
            switch (funct7)
            {
            case 0b0000000: // SRLI
                return MkRdRs1Imm(Exec_Srli, rd, rs1, imm);
            case 0b0100000: // SRAI
                return MkRdRs1Imm(Exec_Srai, rd, rs1, imm);
            default:
                return MkTrap(Exec_IllegalInstruction, instruction);
            }
        default:
            return MkTrap(Exec_IllegalInstruction, instruction);
        }
    }

    case OP_OP: {
        const uint32_t funct3 = Funct3(instruction);
        const uint32_t rs2 = Rs2(instruction);
        const uint32_t funct7 = Funct7(instruction);
        switch (funct3)
        {
        case 0b000:
            switch (funct7)
            {
            case 0b0000000: // ADD
                return MkRdRs1Rs2(Exec_Add, rd, rs1, rs2);
            case 0b0100000: // SUB
                return MkRdRs1Rs2(Exec_Sub, rd, rs1, rs2);
            case 0b00000001: // MUL (RV32M)
                return MkRdRs1Rs2(Exec_Mul, rd, rs1, rs2);
            default:
                return MkTrap(Exec_IllegalInstruction, instruction);
            }
        case 0b001:
            switch (funct7)
            {
            case 0b0000000: // SLL
                return MkRdRs1Rs2(Exec_Sll, rd, rs1, rs2);
            case 0b00000001: // MULH (RV32M)
                return MkRdRs1Rs2(Exec_Mulh, rd, rs1, rs2);
            default:
                return MkTrap(Exec_IllegalInstruction, instruction);
            }
        case 0b010:
            switch (funct7)
            {
            case 0b0000000: // SLT
                return MkRdRs1Rs2(Exec_Slt, rd, rs1, rs2);
            case 0b00000001: // MULHSU (RV32M)
                return MkRdRs1Rs2(Exec_Mulhsu, rd, rs1, rs2);
            default:
                return MkTrap(Exec_IllegalInstruction, instruction);
            }
        case 0b011:
            switch (funct7)
            {
            case 0b0000000: // SLTU
                return MkRdRs1Rs2(Exec_Sltu, rd, rs1, rs2);
            case 0b00000001: // MULHU (RV32M)
                return MkRdRs1Rs2(Exec_Mulhu, rd, rs1, rs2);
            default:
                return MkTrap(Exec_IllegalInstruction, instruction);
            }
        case 0b100:
            switch (funct7)
            {
            case 0b0000000: // XOR
                return MkRdRs1Rs2(Exec_Xor, rd, rs1, rs2);
            case 0b00000001: // DIV (RV32M)
                return MkRdRs1Rs2(Exec_Div, rd, rs1, rs2);
            default:
                return MkTrap(Exec_IllegalInstruction, instruction);
            }
        case 0b101:
            switch (funct7)
            {
            case 0b0000000: // SRL
                return MkRdRs1Rs2(Exec_Srl, rd, rs1, rs2);
            case 0b0100000: // SRA
                return MkRdRs1Rs2(Exec_Sra, rd, rs1, rs2);
            case 0b00000001: // DIVU (RV32M)
                return MkRdRs1Rs2(Exec_Divu, rd, rs1, rs2);
            default:
                return MkTrap(Exec_IllegalInstruction, instruction);
            }
        case 0b110:
            switch (funct7)
            {
            case 0b0000000: // OR
                return MkRdRs1Rs2(Exec_Or, rd, rs1, rs2);
            case 0b00000001: // REM
                return MkRdRs1Rs2(Exec_Rem, rd, rs1, rs2);
            default:
                return MkTrap(Exec_IllegalInstruction, instruction);
            }
        case 0b111:
            switch (funct7)
            {
            case 0b0000000: // AND
                return MkRdRs1Rs2(Exec_And, rd, rs1, rs2);
            case 0b00000001: // REMU
                return MkRdRs1Rs2(Exec_Remu, rd, rs1, rs2);
            default:
                return MkTrap(Exec_IllegalInstruction, instruction);
            }
        default:
            return MkTrap(Exec_IllegalInstruction, instruction);
        }
    }

    case OP_MISCMEM: {
        const uint32_t funct3 = Funct3(instruction);
        if (funct3 == 0b000)
        {
            return MkNoArg(Exec_Fence);
        }
        else
        {
            return MkTrap(Exec_IllegalInstruction, instruction);
        }
    }

    case OP_SYSTEM:
        if ((instruction & 0b00000000000011111111111110000000) == 0)
        {
            const uint32_t funct12 = Funct12(instruction);
            switch (funct12)
            {
            case 0b000000000000: // ECALL
                return MkNoArg(Exec_Ecall);
            case 0b000000000001: // EBREAK
                return MkNoArg(Exec_Ebreak);
            case 0b000000000010: // URET
                return MkNoArg(Exec_Uret);
            case 0b000100000010: // SRET
                return MkNoArg(Exec_Sret);
            case 0b001100000010: // MRET
                return MkNoArg(Exec_Mret);
            default:
                return MkTrap(Exec_IllegalInstruction, instruction);
            }
        }
        else
        {
            return MkTrap(Exec_IllegalInstruction, instruction);
        }

    case OP_LOADFP: { // Floating point load (RV32F)
        const uint32_t funct3 = Funct3(instruction);
        if (funct3 == 0b010) // FLW
        {
            const int32_t imm = IImmediate(instruction);
            return MkRdRs1Imm(Exec_Flw, rd, rs1, imm);
        }
        else
        {
            return MkTrap(Exec_IllegalInstruction, instruction);
        }
    }

    case OP_STOREFP: { // Floating point store (RV32F)
        const uint32_t funct3 = Funct3(instruction);
        if (funct3 == 0b010) // FSW
        {
            // f32(rs1 + imm_s) = rs2
            const uint32_t rs2 = Rs2(instruction);
            const int32_t imm = SImmediate(instruction);
            return MkRs1Rs2Imm(Exec_Fsw, rs1, rs2, imm);
        }
        else
        {
            return MkTrap(Exec_IllegalInstruction, instruction);
        }
    }

    case OP_MADD: {                            // Floating point fused multiply-add (RV32F)
        if (((instruction >> 25) & 0b11) == 0) // FMADD.S
        {
            const uint32_t rm = Rm(instruction);
            const uint32_t rs2 = Rs2(instruction);
            const uint32_t rs3 = Rs3(instruction);
            return MkRdRs1Rs2Rs3Rm(Exec_Fmadd_s, rd, rs1, rs2, rs3, rm);
        }
        else
        {
            return MkTrap(Exec_IllegalInstruction, instruction);
        }
    }

    case OP_MSUB: {                            // Floating point fused multiply-sub (RV32F)
        if (((instruction >> 25) & 0b11) == 0) // FMSUB.S
        {
            const uint32_t rm = Rm(instruction);
            const uint32_t rs2 = Rs2(instruction);
            const uint32_t rs3 = Rs3(instruction);
            return MkRdRs1Rs2Rs3Rm(Exec_Fmsub_s, rd, rs1, rs2, rs3, rm);
        }
        else
        {
            return MkTrap(Exec_IllegalInstruction, instruction);
        }
    }

    case OP_NMSUB: {                           // Floating point negated fused multiply-sub (RV32F)
        if (((instruction >> 25) & 0b11) == 0) // FNMSUB.S
        {
            const uint32_t rm = Rm(instruction);
            const uint32_t rs2 = Rs2(instruction);
            const uint32_t rs3 = Rs3(instruction);
            return MkRdRs1Rs2Rs3Rm(Exec_Fnmsub_s, rd, rs1, rs2, rs3, rm);
        }
        else
        {
            return MkTrap(Exec_IllegalInstruction, instruction);
        }
    }

    case OP_NMADD: {                           // Floating point negated fused multiple-add (RV32F)
        if (((instruction >> 25) & 0b11) == 0) // FNMADD.S
        {
            const uint32_t rm = Rm(instruction);
            const uint32_t rs2 = Rs2(instruction);
            const uint32_t rs3 = Rs3(instruction);
            return MkRdRs1Rs2Rs3Rm(Exec_Fnmadd_s, rd, rs1, rs2, rs3, rm);
        }
        else
        {
            return MkTrap(Exec_IllegalInstruction, instruction);
        }
    }

    case OP_OPFP: { // Floating point operations (RV32F)
        const uint32_t funct7 = Funct7(instruction);
        const uint32_t funct3 = Funct3(instruction);
        const uint32_t rm = funct3;
        const uint32_t rs2 = Rs2(instruction);
        switch (funct7)
        {
        case 0b0000000: // FADD.S
            return MkRdRs1Rs2Rm(Exec_Fadd_s, rd, rs1, rs2, rm);
        case 0b0000100: // FSUB.S
            return MkRdRs1Rs2Rm(Exec_Fsub_s, rd, rs1, rs2, rm);
        case 0b0001000: // FMUL.S
            return MkRdRs1Rs2Rm(Exec_Fmul_s, rd, rs1, rs2, rm);
        case 0b0001100: // FDIV.S
            return MkRdRs1Rs2Rm(Exec_Fdiv_s, rd, rs1, rs2, rm);
        case 0b0101100:
            if (rs2 == 0b00000) // FSQRT.S
            {
                return MkRdRs1Rm(Exec_Fsqrt_s, rd, rs1, rm);
            }
            else
            {
                return MkTrap(Exec_IllegalInstruction, instruction);
            }
        case 0b0010000: {
            switch (funct3)
            {
            case 0b000: // FSGNJ.S
                return MkRdRs1Rs2Rm(Exec_Fsgnj_s, rd, rs1, rs2, rm);
            case 0b001: // FSGNJN.S
                return MkRdRs1Rs2Rm(Exec_Fsgnjn_s, rd, rs1, rs2, rm);
            case 0b010: // FSGNJX.S
                return MkRdRs1Rs2Rm(Exec_Fsgnjx_s, rd, rs1, rs2, rm);
            default:
                return MkTrap(Exec_IllegalInstruction, instruction);
            }
        }
        case 0b0010100: {
            switch (funct3)
            {
            case 0b000: // FMIN.S
                return MkRdRs1Rs2(Exec_Fmin_s, rd, rs1, rs2);
            case 0b001: // FMAX.S
                return MkRdRs1Rs2(Exec_Fmax_s, rd, rs1, rs2);
            default:
                return MkTrap(Exec_IllegalInstruction, instruction);
            }
        }
        case 0b1100000: {
            switch (rs2) // Not actually rs2 - just the same bits.
            {
            case 0b00000: // FCVT.W.S
                return MkRdRs1Rm(Exec_Fcvt_w_s, rd, rs1, rm);
            case 0b00001: // FVCT.WU.S
                return MkRdRs1Rm(Exec_Fcvt_wu_s, rd, rs1, rm);
            default:
                return MkTrap(Exec_IllegalInstruction, instruction);
            }
        }
        case 0b1110000: {
            if (rs2 == 0b00000) // Not actually rs2 - just the same bits.
            {
                switch (funct3)
                {
                case 0b000: // FMV.X.W
                    return MkRdRs1(Exec_Fmv_x_w, rd, rs1);
                case 0b001: // FCLASS.S
                    return MkRdRs1(Exec_Fclass_s, rd, rs1);
                default:
                    return MkTrap(Exec_IllegalInstruction, instruction);
                }
            }
            return MkTrap(Exec_IllegalInstruction, instruction);
        }
        case 0b1010000: {
            switch (funct3)
            {
            case 0b010: // FEQ.S
                return MkRdRs1Rs2(Exec_Feq_s, rd, rs1, rs2);
            case 0b001: // FLT.S
                return MkRdRs1Rs2(Exec_Flt_s, rd, rs1, rs2);
            case 0b000: // FLE.S
                return MkRdRs1Rs2(Exec_Fle_s, rd, rs1, rs2);
            default:
                return MkTrap(Exec_IllegalInstruction, instruction);
            }
        }
        case 0b1101000: {
            switch (rs2) // No actually rs2 - just the same bits,
            {
            case 0b00000: // FCVT.S.W
                return MkRdRs1(Exec_Fcvt_s_w, rd, rs1);
            case 0b00001: // FVCT.S.WU
                return MkRdRs1(Exec_Fcvt_s_wu, rd, rs1);
            default:
                return MkTrap(Exec_IllegalInstruction, instruction);
            }
        }
        case 0b1111000:
            if (rs2 == 0b00000 && funct3 == 0b000) // FMV.W.X
            {
                return MkRdRs1(Exec_Fmv_w_x, rd, rs1);
            }
            else
            {
                return MkTrap(Exec_IllegalInstruction, instruction);
            }
        default:
            return MkTrap(Exec_IllegalInstruction, instruction);
        }
    }

    default:
        return MkTrap(Exec_IllegalInstruction, instruction);
    }
}

ArvissResult ArvissExecute(ArvissCpu* cpu, uint32_t instruction)
{
    DecodedInstruction decoded = ArvissDecode(instruction);
    return decoded.opcode(cpu, decoded);
}

static DecodedInstruction FetchFromCache(ArvissCpu* cpu)
{
    const uint32_t addr = cpu->pc;
    const uint32_t owner = ((addr / 4) / CACHE_LINE_LENGTH);
    const uint32_t cacheLine = owner % CACHE_LINES;
    const uint32_t lineIndex = (addr / 4) % CACHE_LINE_LENGTH;
    struct CacheLine* line = &cpu->cache.line[cacheLine];
    if (owner != line->owner || !line->isValid)
    {
        // Initialise this cache line with fetch/decode/replace operations which, when called, replace themselves with a decoded
        // version of the instruction at the corresponding address. This way we don't incur an overhead for decoding instructions
        // that are never run.
        for (uint32_t i = 0u; i < CACHE_LINE_LENGTH; i++)
        {
            line->instructions[i] = MkFetchDecodeReplace(Exec_FetchDecodeReplace, cacheLine, i);
        }
        line->isValid = true;
        line->owner = owner;
    }
    return line->instructions[lineIndex];
}

ArvissResult ArvissRun(ArvissCpu* cpu, int count)
{
    ArvissResult result = ArvissMakeOk();
    for (int i = 0; i < count; i++)
    {
        DecodedInstruction decoded = FetchFromCache(cpu);
        result = decoded.opcode(cpu, decoded);

        if (ArvissResultIsTrap(result))
        {
            // Stop, as we can no longer proceeed.
            break;
        }
    }
    return result;
}
