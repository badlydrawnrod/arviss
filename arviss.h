/**
 * Arviss - A Risc-V Instruction Set Simulator.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define CACHE_LINES 64
#define CACHE_LINE_LENGTH 32

// Opcodes.
typedef enum
{
    opLUI = 0b0110111,
    opAUIPC = 0b0010111,
    opJAL = 0b1101111,
    opJALR = 0b1100111,
    opBRANCH = 0b1100011,
    opLOAD = 0b0000011,
    opSTORE = 0b0100011,
    opOPIMM = 0b0010011,
    opOP = 0b0110011,
    opMISCMEM = 0b0001111,
    opSYSTEM = 0b1110011,
    opLOADFP = 0b0000111,  // RV32F
    opSTOREFP = 0b0100111, // RV32F
    opOPFP = 0b1010011,    // RV32F
    opMADD = 0b1000011,    // RV32F
    opMSUB = 0b1000111,    // RV32F
    opNMSUB = 0b1001011,   // RV32F
    opNMADD = 0b1001111,   // RV32F
} ArvissOpcode;

// Types of machine mode traps. See privileged spec, table 3.6: machine cause register (mcause) values after trap.
typedef enum
{
    // Non-interrupt traps.
    trINSTRUCTION_MISALIGNED = 0,
    trINSTRUCTION_ACCESS_FAULT = 1,
    trILLEGAL_INSTRUCTION = 2,
    trBREAKPOINT = 3,
    trLOAD_ADDRESS_MISALIGNED = 4,
    trLOAD_ACCESS_FAULT = 5,
    trSTORE_ADDRESS_MISALIGNED = 6,
    trSTORE_ACCESS_FAULT = 7,
    trENVIRONMENT_CALL_FROM_U_MODE = 8,
    trENVIRONMENT_CALL_FROM_S_MODE = 9,
    trRESERVED_10 = 10,
    trENVIRONMENT_CALL_FROM_M_MODE = 11,
    trINSTRUCTION_PAGE_FAULT = 12,
    trRESERVERD_14 = 14,
    trSTORE_PAGE_FAULT = 15,
    trNOT_IMPLEMENTED_YET = 24, // Technically this is the first item reserved for custom use.

    // Interrupts (bit 31 is set).
    trUSER_SOFTWARE_INTERRUPT = (int)(0x80000000 + 0),
    trSUPERVISOR_SOFTWARE_INTERRUPT = 0x80000000 + 1,
    trRESERVED_INT_2 = 0x80000000 + 2,
    trMACHINE_SOFTWARE_INTERRUPT = 0x80000000 + 3,
    trUSER_TIMER_INTERRUPT = 0x80000000 + 4,
    trSUPERVISOR_TIMER_INTERRUPT = 0x80000000 + 5,
    trRESERVED_INT_6 = 0x80000000 + 6,
    trMACHINE_TIMER_INTERRUPT = 0x80000000 + 7
} ArvissTrapType;

typedef struct
{
    ArvissTrapType mcause; // TODO: interrupts.
    uint32_t mtval;
} ArvissTrap;

// The result of an Arviss operation.
typedef enum
{
    rtOK,
    rtTRAP
} ArvissResultType;

typedef struct
{
    ArvissResultType type;
    ArvissTrap trap;
} ArvissResult;

// ABI names for integer registers.
// See: https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-cc.adoc#register-convention
typedef enum
{
    abiZERO = 0,
    abiRA = 1,
    abiSP = 2,
    abiGP = 3,
    abiTP = 4,
    abiT0 = 5,
    abiT1 = 6,
    abiT2 = 7,
    abiS0 = 8,
    abiS1 = 9,
    abiA0 = 10,
    abiA1 = 11,
    abiA2 = 12,
    abiA3 = 13,
    abiA4 = 14,
    abiA5 = 15,
    abiA6 = 16,
    abiA7 = 17,
    abiS2 = 18,
    abiS3 = 19,
    abiS4 = 20,
    abiS5 = 21,
    abiS6 = 22,
    abiS7 = 23,
    abiS8 = 24,
    abiS9 = 25,
    abiS10 = 26,
    abiS11 = 27,
    abiT3 = 28,
    abiT4 = 29,
    abiT5 = 30,
    abiT6 = 31
} ArvissIntReg;

// ABI names for floating point registers.
// See: https://github.com/riscv-non-isa/riscv-elf-psabi-doc/blob/master/riscv-cc.adoc#register-convention
typedef enum
{
    abiFT0 = 0,
    abiFT1 = 1,
    abiFT2 = 2,
    abiFT3 = 3,
    abiFT4 = 4,
    abiFT5 = 5,
    abiFT6 = 6,
    abiFT7 = 7,
    abiFS0 = 8,
    abiFS1 = 9,
    abiFA0 = 10,
    abiFA1 = 11,
    abiFA2 = 12,
    abiFA3 = 13,
    abiFA4 = 14,
    abiFA5 = 15,
    abiFA6 = 16,
    abiFA7 = 17,
    abiFS2 = 18,
    abiFS3 = 19,
    abiFS4 = 20,
    abiFS5 = 21,
    abiFS6 = 22,
    abiFS7 = 23,
    abiFS8 = 24,
    abiFS9 = 25,
    abiFS10 = 26,
    abiFS11 = 27,
    abiFT8 = 28,
    abiFT9 = 29,
    abiFT10 = 30,
    abiFT11 = 31
} ArvissFloatReg;

typedef enum
{
    rmRNE = 0b000,
    rmRTZ = 0b001,
    rmRDN = 0b010,
    rmRUP = 0b011,
    rmRMM = 0b100,
    rmRSVD5 = 0b101,
    rmRSVD6 = 0b110,
    rmDYN = 0b111
} ArvissRoundingMode;

/**
 * A handle to an Arviss CPU.
 */
typedef struct ArvissCpu ArvissCpu;

/**
 * An arbitrary, caller supplied token that an Arviss CPU passes to the bus callbacks.
 */
typedef struct
{
    void* t;
} BusToken;

// Codes returned by bus operations.
typedef enum
{
    bcOK,
    bcLOAD_ACCESS_FAULT,
    bcSTORE_ACCESS_FAULT
} BusCode;

/**
 * Signatures of bus callbacks.
 */
typedef uint8_t (*BusRead8Fn)(BusToken token, uint32_t addr, BusCode* busCode);
typedef uint16_t (*BusRead16Fn)(BusToken token, uint32_t addr, BusCode* busCode);
typedef uint32_t (*BusRead32Fn)(BusToken token, uint32_t addr, BusCode* busCode);
typedef void (*BusWrite8Fn)(BusToken token, uint32_t addr, uint8_t byte, BusCode* busCode);
typedef void (*BusWrite16Fn)(BusToken token, uint32_t addr, uint16_t halfword, BusCode* busCode);
typedef void (*BusWrite32Fn)(BusToken token, uint32_t addr, uint32_t word, BusCode* busCode);

/**
 * The bus is how an Arviss CPU interacts with the rest of the system. It has a number of callbacks, and a caller-supplied token
 * that is passed to them on invocation.
 */
typedef struct
{
    BusToken token;
    BusRead8Fn Read8;
    BusRead16Fn Read16;
    BusRead32Fn Read32;
    BusWrite8Fn Write8;
    BusWrite16Fn Write16;
    BusWrite32Fn Write32;
} Bus;

typedef enum
{
    execIllegalInstruction,
    execFetchDecodeReplace,
    execLui,
    execAuipc,
    execJal,
    execJalr,
    execBeq,
    execBne,
    execBlt,
    execBge,
    execBltu,
    execBgeu,
    execLb,
    execLh,
    execLw,
    execLbu,
    execLhu,
    execSb,
    execSh,
    execSw,
    execAddi,
    execSlti,
    execSltiu,
    execXori,
    execOri,
    execAndi,
    execSlli,
    execSrli,
    execSrai,
    execAdd,
    execSub,
    execMul,
    execSll,
    execMulh,
    execSlt,
    execMulhsu,
    execSltu,
    execMulhu,
    execXor,
    execDiv,
    execSrl,
    execSra,
    execDivu,
    execOr,
    execRem,
    execAnd,
    execRemu,
    execFence,
    execEcall,
    execEbreak,
    execUret,
    execSret,
    execMret,
    execFlw,
    execFsw,
    execFmadd_s,
    execFmsub_s,
    execFnmsub_s,
    execFnmadd_s,
    execFadd_s,
    execFsub_s,
    execFmul_s,
    execFdiv_s,
    execFsqrt_s,
    execFsgnj_s,
    execFsgnjn_s,
    execFsgnjx_s,
    execFmin_s,
    execFmax_s,
    execFcvt_w_s,
    execFcvt_wu_s,
    execFmv_x_w,
    execFclass_s,
    execFeq_s,
    execFlt_s,
    execFle_s,
    execFcvt_s_w,
    execFcvt_s_wu,
    execFmv_w_x,
} ExecFn;

typedef struct DecodedInstruction DecodedInstruction;

struct DecodedInstruction
{
    ExecFn opcode; // The instruction that this function executes. TODO: need to call it something other than opcode?
    union
    {
        struct
        {
            uint32_t cacheLine; // The instruction's cache line.
            uint32_t index;     // The instruction's index in the cache line.
        } fdr;

        struct
        {
            uint8_t rd;  // Destination register.
            int32_t imm; // Immediate operand.
        } rd_imm;

        struct
        {
            uint8_t rd;  // Destination register.
            uint8_t rs1; // Source register.
        } rd_rs1;

        struct
        {
            uint8_t rd;  // Destination register.
            uint8_t rs1; // Source register.
            int32_t imm; // Immediate operand.
        } rd_rs1_imm;

        struct
        {
            uint8_t rd;  // Destination register.
            uint8_t rs1; // First source register.
            uint8_t rs2; // Second source register.
        } rd_rs1_rs2;

        struct
        {
            uint8_t rs1; // First source register.
            uint8_t rs2; // Second source register.
            int32_t imm; // Immediate operand.
        } rs1_rs2_imm;

        struct
        {
            uint8_t rd;  // Destination register.
            uint8_t rs1; // First source register.
            uint8_t rs2; // Second source register.
            uint8_t rs3; // Third source register.
            uint8_t rm;  // Rounding mode.
        } rd_rs1_rs2_rs3_rm;

        struct
        {
            uint8_t rd;  // Destination register.
            uint8_t rs1; // Source register.
            uint8_t rm;  // Rounding mode.
        } rd_rs1_rm;

        struct
        {
            uint8_t rd;  // Destination register.
            uint8_t rs1; // First source register.
            uint8_t rs2; // Second source register.
            uint8_t rm;  // Rounding mode.
        } rd_rs1_rs2_rm;

        uint32_t ins; // Instruction.
    };
};

// Decoded instructions are written to cache lines in the decoded instruction cache. Arviss then executes these decoded
// instructions.
typedef struct
{
    struct CacheLine
    {
        uint32_t owner;                                     // The address that owns this cache line.
        DecodedInstruction instructions[CACHE_LINE_LENGTH]; // The cache line itself.
        bool isValid;                                       // True if the cache line is valid.
    } line[CACHE_LINES];
} DecodedInstructionCache;

// An Arviss CPU.
struct ArvissCpu
{
    ArvissResult result;           // The result of the last operation.
    BusCode busCode;               // The result of the last bus operation.
    uint32_t pc;                   // The program counter.
    uint32_t xreg[32];             // Regular registers, x0-x31.
    uint32_t mepc;                 // The machine exception program counter.
    uint32_t mcause;               // The machine cause register.
    uint32_t mtval;                // The machine trap value register.
    float freg[32];                // Floating point registers, f0-f31.
    uint32_t fcsr;                 // Floating point control and status register.
    Bus bus;                       // The address bus.
    DecodedInstructionCache cache; // The decoded instruction cache.
    int retired;                   // Instructions retired in the most recent call to ArvissRun().
};

#ifdef __cplusplus
extern "C" {
#endif

static void RunOne(ArvissCpu* cpu, DecodedInstruction* ins);

static inline ArvissResult ArvissMakeOk(void)
{
    // Not using designated initializers as...
    // a) C++ < 20 doesn't support them
    // b) C++ 20 doesn't like the cast
    // return (ArvissResult){ .type = rtOK };
    ArvissResult r;
    r.type = rtOK;
    return r;
}

static inline ArvissResult ArvissMakeTrap(ArvissTrapType trap, uint32_t value)
{
    ArvissResult r;
    r.type = rtTRAP;
    r.trap.mcause = trap;
    r.trap.mtval = value;
    return r;
}

static inline bool ArvissResultIsTrap(ArvissResult result)
{
    return result.type == rtTRAP;
}

static inline ArvissTrap ArvissResultAsTrap(ArvissResult result)
{
    return result.trap;
}

/**
 * Resets the given Arviss CPU.
 * @param cpu the CPU to reset.
 */
void ArvissReset(ArvissCpu* cpu);

/**
 * Initialises the given Arviss CPU and provides it with its bus.
 * @param cpu the CPU.
 * @param bus the bus that the CPU should use to interact with the rest of the system.
 */
static inline void ArvissInit(ArvissCpu* cpu, Bus* bus)
{
    ArvissReset(cpu);
    cpu->bus = *bus;
}

/**
 * Decodes and executes a single Arviss instruction.
 * @param cpu the CPU.
 * @param instruction the instruction to execute.
 * @return an ArvissResult indicating the state of the CPU after executing the instruction.
 */
ArvissResult ArvissExecute(ArvissCpu* cpu, uint32_t instruction);

/**
 * Runs count instructions on the CPU.
 * @param cpu the CPU.
 * @param count how many instructions to run.
 * @return an ArvissResult indicating the state of the CPU after attempting to run count instructions.
 */
ArvissResult ArvissRun(ArvissCpu* cpu, int count);

/**
 * Reads the given X register.
 * @param cpu the CPU.
 * @param reg which X register to read (0 - 31).
 * @return the content of the X register.
 */
static inline uint32_t ArvissReadXReg(ArvissCpu* cpu, int reg)
{
    return cpu->xreg[reg];
}

/**
 * Writes to the given X register.
 * @param cpu the CPU.
 * @param reg which X register to write to (0 - 31).
 * @param value the value to write.
 */
static inline void ArvissWriteXReg(ArvissCpu* cpu, int reg, uint32_t value)
{
    cpu->xreg[reg] = value;
}

/**
 * Reads the given F register.
 * @param cpu the CPU.
 * @param reg which F register to read (0 - 31).
 * @return the content of the F register.
 */
static inline float ArvissReadFReg(ArvissCpu* cpu, int reg)
{
    return cpu->freg[reg];
}

/**
 * Writes to the given F register.
 * @param cpu the CPU.
 * @param reg which F register to write to (0 - 31).
 * @param value the value to write.
 */
static inline void ArvissWriteFReg(ArvissCpu* cpu, int reg, float value)
{
    cpu->freg[reg] = value;
}

/**
 * Performs an MRET instruction on the CPU. Use this when returning from a machine-mode trap.
 * @param cpu the CPU.
 */
void ArvissMret(ArvissCpu* cpu);

#ifdef __cplusplus
}
#endif

#ifdef ARVISS_IMPLEMENTATION

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline float U32AsFloat(const uint32_t a)
{
    union
    {
        uint32_t a;
        float b;
    } u;
    u.a = a;
    return u.b;
}

static inline uint32_t FloatAsU32(const float a)
{
    union
    {
        float a;
        uint32_t b;
    } u;
    u.a = a;
    return u.b;
}

static inline uint32_t BoolAsU32(const bool b)
{
    return b ? 1 : 0;
}

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

static DecodedInstruction ArvissDecode(uint32_t instruction);

// --- Bus access ------------------------------------------------------------------------------------------------------------------

static inline uint8_t Read8(Bus* bus, uint32_t addr, BusCode* busCode)
{
    return bus->Read8(bus->token, addr, busCode);
}

static inline uint16_t Read16(Bus* bus, uint32_t addr, BusCode* busCode)
{
    return bus->Read16(bus->token, addr, busCode);
}

static inline uint32_t Read32(Bus* bus, uint32_t addr, BusCode* busCode)
{
    return bus->Read32(bus->token, addr, busCode);
}

static inline void Write8(Bus* bus, uint32_t addr, uint8_t byte, BusCode* busCode)
{
    bus->Write8(bus->token, addr, byte, busCode);
}

static inline void Write16(Bus* bus, uint32_t addr, uint16_t halfword, BusCode* busCode)
{
    bus->Write16(bus->token, addr, halfword, busCode);
}

static inline void Write32(Bus* bus, uint32_t addr, uint32_t word, BusCode* busCode)
{
    bus->Write32(bus->token, addr, word, busCode);
}

// --- Execution -------------------------------------------------------------------------------------------------------------------
//
// Functions in this section execute decoded instructions. Instruction execution is separate from decoding, as this allows an
// instruction to be fetched and decoded once, then placed in the decoded instruction cache where it can be executed several times.
// This mitigates the cost of decoding, as decoded instructions are already in a form that is easy to execute.

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

inline static void Exec_IllegalInstruction(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    cpu->result = CreateTrap(cpu, trILLEGAL_INSTRUCTION, ins->ins);
}

inline static void Exec_FetchDecodeReplace(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // Reconstitute the address given the cache line and index.
    const uint32_t cacheLine = ins->fdr.cacheLine;
    const uint32_t index = ins->fdr.index;
    struct CacheLine* line = &cpu->cache.line[cacheLine];
    const uint32_t owner = line->owner;
    const uint32_t addr = owner * 4 * CACHE_LINE_LENGTH + index * 4;

    // Fetch a word from memory at the address.
    uint32_t instruction = Read32(&cpu->bus, addr, &cpu->busCode);

    // Decode it, save the result in the cache, then execute it.
    if (cpu->busCode == bcOK)
    {
        // Decode the instruction and save it in the cache. All instructions are decodable into something executable, because
        // all illegal instructions become Exec_IllegalInstruction, which is itself executable.
        DecodedInstruction decoded = ArvissDecode(instruction);
        line->instructions[index] = decoded;

        // Execute the decoded instruction.
        RunOne(cpu, &decoded);
    }
    else
    {
        cpu->result = ArvissMakeTrap(trINSTRUCTION_ACCESS_FAULT, addr);
    }
}

inline static void Exec_Lui(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- imm_u, pc += 4
    TRACE("LUI %s, %d\n", abiNames[ins->rd_imm.rd], ins->rd_imm.imm >> 12);
    cpu->xreg[ins->rd_imm.rd] = ins->rd_imm.imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Auipc(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- pc + imm_u, pc += 4
    TRACE("AUIPC %s, %d\n", abiNames[ins->rd_imm.rd], ins->rd_imm.imm >> 12);
    cpu->xreg[ins->rd_imm.rd] = cpu->pc + ins->rd_imm.imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Jal(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- pc + 4, pc <- pc + imm_j
    TRACE("JAL %s, %d\n", abiNames[ins->rd_imm.rd], ins->rd_imm.imm);
    cpu->xreg[ins->rd_imm.rd] = cpu->pc + 4;
    cpu->pc += ins->rd_imm.imm;
    cpu->xreg[0] = 0;
}

inline static void Exec_Jalr(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- pc + 4, pc <- (rs1 + imm_i) & ~1
    TRACE("JALR %s, %s, %d\n", abiNames[ins->rd_rs1_imm.rd], abiNames[ins->rd_rs1_imm.rs1], ins->rd_rs1_imm.imm);
    uint32_t rs1Before = cpu->xreg[ins->rd_rs1_imm.rs1]; // Because rd and rs1 might be the same register.
    cpu->xreg[ins->rd_rs1_imm.rd] = cpu->pc + 4;
    cpu->pc = (rs1Before + ins->rd_rs1_imm.imm) & ~1;
    cpu->xreg[0] = 0;
}

inline static void Exec_Beq(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // pc <- pc + ((rs1 == rs2) ? imm_b : 4)
    TRACE("BEQ %s, %s, %d\n", abiNames[ins->rs1_rs2_imm.rs1], abiNames[ins->rs1_rs2_imm.rs2], ins->rs1_rs2_imm.imm);
    cpu->pc += ((cpu->xreg[ins->rs1_rs2_imm.rs1] == cpu->xreg[ins->rs1_rs2_imm.rs2]) ? ins->rs1_rs2_imm.imm : 4);
}

inline static void Exec_Bne(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // pc <- pc + ((rs1 != rs2) ? imm_b : 4)
    TRACE("BNE %s, %s, %d\n", abiNames[ins->rs1_rs2_imm.rs1], abiNames[ins->rs1_rs2_imm.rs2], ins->rs1_rs2_imm.imm);
    cpu->pc += ((cpu->xreg[ins->rs1_rs2_imm.rs1] != cpu->xreg[ins->rs1_rs2_imm.rs2]) ? ins->rs1_rs2_imm.imm : 4);
}

inline static void Exec_Blt(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // pc <- pc + ((rs1 < rs2) ? imm_b : 4)
    TRACE("BLT %s, %s, %d\n", abiNames[ins->rs1_rs2_imm.rs1], abiNames[ins->rs1_rs2_imm.rs2], ins->rs1_rs2_imm.imm);
    cpu->pc += (((int32_t)cpu->xreg[ins->rs1_rs2_imm.rs1] < (int32_t)cpu->xreg[ins->rs1_rs2_imm.rs2]) ? ins->rs1_rs2_imm.imm : 4);
}

inline static void Exec_Bge(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // pc <- pc + ((rs1 >= rs2) ? imm_b : 4)
    TRACE("BGE %s, %s, %d\n", abiNames[ins->rs1_rs2_imm.rs1], abiNames[ins->rs1_rs2_imm.rs2], ins->rs1_rs2_imm.imm);
    cpu->pc += (((int32_t)cpu->xreg[ins->rs1_rs2_imm.rs1] >= (int32_t)cpu->xreg[ins->rs1_rs2_imm.rs2]) ? ins->rs1_rs2_imm.imm : 4);
}

inline static void Exec_Bltu(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // pc <- pc + ((rs1 < rs2) ? imm_b : 4)
    TRACE("BLTU %s, %s, %d\n", abiNames[ins->rs1_rs2_imm.rs1], abiNames[ins->rs1_rs2_imm.rs2], ins->rs1_rs2_imm.imm);
    cpu->pc += ((cpu->xreg[ins->rs1_rs2_imm.rs1] < cpu->xreg[ins->rs1_rs2_imm.rs2]) ? ins->rs1_rs2_imm.imm : 4);
}

inline static void Exec_Bgeu(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // pc <- pc + ((rs1 >= rs2) ? imm_b : 4)
    TRACE("BGEU %s, %s, %d\n", abiNames[ins->rs1_rs2_imm.rs1], abiNames[ins->rs1_rs2_imm.rs2], ins->rs1_rs2_imm.imm);
    cpu->pc += ((cpu->xreg[ins->rs1_rs2_imm.rs1] >= cpu->xreg[ins->rs1_rs2_imm.rs2]) ? ins->rs1_rs2_imm.imm : 4);
}

inline static void Exec_Lb(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- sx(m8(rs1 + imm_i)), pc += 4
    TRACE("LB %s, %d(%s)\n", abiNames[ins->rd_rs1_imm.rd], ins->rd_rs1_imm.imm, abiNames[ins->rd_rs1_imm.rs1]);
    uint8_t byte = Read8(&cpu->bus, cpu->xreg[ins->rd_rs1_imm.rs1] + ins->rd_rs1_imm.imm, &cpu->busCode);
    if (cpu->busCode != bcOK)
    {
        cpu->result = TakeTrap(cpu, ArvissMakeTrap(trLOAD_ACCESS_FAULT, cpu->xreg[ins->rd_rs1_imm.rs1] + ins->rd_rs1_imm.imm));
        return;
    }
    cpu->xreg[ins->rd_rs1_imm.rd] = (int32_t)(int16_t)(int8_t)byte;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Lh(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- sx(m16(rs1 + imm_i)), pc += 4
    TRACE("LH %s, %d(%s)\n", abiNames[ins->rd_rs1_imm.rd], ins->rd_rs1_imm.imm, abiNames[ins->rd_rs1_imm.rs1]);
    uint16_t halfword = Read16(&cpu->bus, cpu->xreg[ins->rd_rs1_imm.rs1] + ins->rd_rs1_imm.imm, &cpu->busCode);
    if (cpu->busCode != bcOK)
    {
        cpu->result = TakeTrap(cpu, ArvissMakeTrap(trLOAD_ACCESS_FAULT, cpu->xreg[ins->rd_rs1_imm.rs1] + ins->rd_rs1_imm.imm));
        return;
    }
    cpu->xreg[ins->rd_rs1_imm.rd] = (int32_t)(int16_t)halfword;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Lw(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- sx(m32(rs1 + imm_i)), pc += 4
    TRACE("LW %s, %d(%s)\n", abiNames[ins->rd_rs1_imm.rd], ins->rd_rs1_imm.imm, abiNames[ins->rd_rs1_imm.rs1]);
    uint32_t word = Read32(&cpu->bus, cpu->xreg[ins->rd_rs1_imm.rs1] + ins->rd_rs1_imm.imm, &cpu->busCode);
    if (cpu->busCode != bcOK)
    {
        cpu->result = TakeTrap(cpu, ArvissMakeTrap(trLOAD_ACCESS_FAULT, cpu->xreg[ins->rd_rs1_imm.rs1] + ins->rd_rs1_imm.imm));
        return;
    }
    cpu->xreg[ins->rd_rs1_imm.rd] = (int32_t)word;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Lbu(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- zx(m8(rs1 + imm_i)), pc += 4
    TRACE("LBU x%d, %d(x%d)\n", ins->rd_rs1_imm.rd, ins->rd_rs1_imm.imm, ins->rd_rs1_imm.rs1);
    uint8_t byte = Read8(&cpu->bus, cpu->xreg[ins->rd_rs1_imm.rs1] + ins->rd_rs1_imm.imm, &cpu->busCode);
    if (cpu->busCode != bcOK)
    {
        cpu->result = TakeTrap(cpu, ArvissMakeTrap(trLOAD_ACCESS_FAULT, cpu->xreg[ins->rd_rs1_imm.rs1] + ins->rd_rs1_imm.imm));
        return;
    }
    cpu->xreg[ins->rd_rs1_imm.rd] = byte;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Lhu(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- zx(m16(rs1 + imm_i)), pc += 4
    TRACE("LHU %s, %d(%s)\n", abiNames[ins->rd_rs1_imm.rd], ins->rd_rs1_imm.imm, abiNames[ins->rd_rs1_imm.rs1]);
    uint16_t halfword = Read16(&cpu->bus, cpu->xreg[ins->rd_rs1_imm.rs1] + ins->rd_rs1_imm.imm, &cpu->busCode);
    if (cpu->busCode != bcOK)
    {
        cpu->result = TakeTrap(cpu, ArvissMakeTrap(trLOAD_ACCESS_FAULT, cpu->xreg[ins->rd_rs1_imm.rs1] + ins->rd_rs1_imm.imm));
        return;
    }
    cpu->xreg[ins->rd_rs1_imm.rd] = halfword;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Sb(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // m8(rs1 + imm_s) <- rs2[7:0], pc += 4
    TRACE("SB %s, %d(%s)\n", abiNames[ins->rs1_rs2_imm.rs2], ins->rs1_rs2_imm.imm, abiNames[ins->rs1_rs2_imm.rs1]);
    Write8(&cpu->bus, cpu->xreg[ins->rs1_rs2_imm.rs1] + ins->rs1_rs2_imm.imm, cpu->xreg[ins->rs1_rs2_imm.rs2] & 0xff,
           &cpu->busCode);
    if (cpu->busCode != bcOK)
    {
        cpu->result = TakeTrap(cpu, ArvissMakeTrap(trSTORE_ACCESS_FAULT, cpu->xreg[ins->rs1_rs2_imm.rs1] + ins->rs1_rs2_imm.imm));
        return;
    }
    cpu->pc += 4;
}

inline static void Exec_Sh(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // m16(rs1 + imm_s) <- rs2[15:0], pc += 4
    TRACE("SH %s, %d(%s)\n", abiNames[ins->rs1_rs2_imm.rs2], ins->rs1_rs2_imm.imm, abiNames[ins->rs1_rs2_imm.rs1]);
    Write16(&cpu->bus, cpu->xreg[ins->rs1_rs2_imm.rs1] + ins->rs1_rs2_imm.imm, cpu->xreg[ins->rs1_rs2_imm.rs2] & 0xffff,
            &cpu->busCode);
    if (cpu->busCode != bcOK)
    {
        cpu->result = TakeTrap(cpu, ArvissMakeTrap(trSTORE_ACCESS_FAULT, cpu->xreg[ins->rs1_rs2_imm.rs1] + ins->rs1_rs2_imm.imm));
        return;
    }
    cpu->pc += 4;
}

inline static void Exec_Sw(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // m32(rs1 + imm_s) <- rs2[31:0], pc += 4
    TRACE("SW %s, %d(%s)\n", abiNames[ins->rs1_rs2_imm.rs2], ins->rs1_rs2_imm.imm, abiNames[ins->rs1_rs2_imm.rs1]);
    Write32(&cpu->bus, cpu->xreg[ins->rs1_rs2_imm.rs1] + ins->rs1_rs2_imm.imm, cpu->xreg[ins->rs1_rs2_imm.rs2], &cpu->busCode);
    if (cpu->busCode != bcOK)
    {
        cpu->result = TakeTrap(cpu, ArvissMakeTrap(trSTORE_ACCESS_FAULT, cpu->xreg[ins->rs1_rs2_imm.rs1] + ins->rs1_rs2_imm.imm));
        return;
    }
    cpu->pc += 4;
}

inline static void Exec_Addi(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- rs1 + imm_i, pc += 4
    TRACE("ADDI %s, %s, %d\n", abiNames[ins->rd_rs1_imm.rd], abiNames[ins->rd_rs1_imm.rs1], ins->rd_rs1_imm.imm);
    cpu->xreg[ins->rd_rs1_imm.rd] = cpu->xreg[ins->rd_rs1_imm.rs1] + ins->rd_rs1_imm.imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Slti(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- (rs1 < imm_i) ? 1 : 0, pc += 4
    TRACE("SLTI %s, %s, %d\n", abiNames[ins->rd_rs1_imm.rd], abiNames[ins->rd_rs1_imm.rs1], ins->rd_rs1_imm.imm);
    cpu->xreg[ins->rd_rs1_imm.rd] = ((int32_t)cpu->xreg[ins->rd_rs1_imm.rs1] < ins->rd_rs1_imm.imm) ? 1 : 0;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Sltiu(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- (rs1 < imm_i) ? 1 : 0, pc += 4
    TRACE("SLTIU %s, %s, %d\n", abiNames[ins->rd_rs1_imm.rd], abiNames[ins->rd_rs1_imm.rs1], ins->rd_rs1_imm.imm);
    cpu->xreg[ins->rd_rs1_imm.rd] = (cpu->xreg[ins->rd_rs1_imm.rs1] < (uint32_t)ins->rd_rs1_imm.imm) ? 1 : 0;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Xori(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- rs1 ^ imm_i, pc += 4
    TRACE("XORI %s, %s, %d\n", abiNames[ins->rd_rs1_imm.rd], abiNames[ins->rd_rs1_imm.rs1], ins->rd_rs1_imm.imm);
    cpu->xreg[ins->rd_rs1_imm.rd] = cpu->xreg[ins->rd_rs1_imm.rs1] ^ ins->rd_rs1_imm.imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Ori(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- rs1 | imm_i, pc += 4
    TRACE("ORI %s, %s, %d\n", abiNames[ins->rd_rs1_imm.rd], abiNames[ins->rd_rs1_imm.rs1], ins->rd_rs1_imm.imm);
    cpu->xreg[ins->rd_rs1_imm.rd] = cpu->xreg[ins->rd_rs1_imm.rs1] | ins->rd_rs1_imm.imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Andi(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- rs1 & imm_i, pc += 4
    TRACE("ANDI %s, %s, %d\n", abiNames[ins->rd_rs1_imm.rd], abiNames[ins->rd_rs1_imm.rs1], ins->rd_rs1_imm.imm);
    cpu->xreg[ins->rd_rs1_imm.rd] = cpu->xreg[ins->rd_rs1_imm.rs1] & ins->rd_rs1_imm.imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Slli(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    TRACE("SLLI %s, %s, %d\n", abiNames[ins->rd_rs1_imm.rd], abiNames[ins->rd_rs1_imm.rs1], ins->rd_rs1_imm.imm);
    cpu->xreg[ins->rd_rs1_imm.rd] = cpu->xreg[ins->rd_rs1_imm.rs1] << ins->rd_rs1_imm.imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Srli(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- rs1 >> shamt_i, pc += 4
    TRACE("SRLI %s, %s, %d\n", abiNames[ins->rd_rs1_imm.rd], abiNames[ins->rd_rs1_imm.rs1], ins->rd_rs1_imm.imm);
    cpu->xreg[ins->rd_rs1_imm.rd] = cpu->xreg[ins->rd_rs1_imm.rs1] >> ins->rd_rs1_imm.imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Srai(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- sx(rs1) >> shamt_i, pc += 4
    TRACE("SRAI %s, %s, %d\n", abiNames[ins->rd_rs1_imm.rd], abiNames[ins->rd_rs1_imm.rs1], ins->rd_rs1_imm.imm);
    cpu->xreg[ins->rd_rs1_imm.rd] = (int32_t)cpu->xreg[ins->rd_rs1_imm.rs1] >> ins->rd_rs1_imm.imm;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Add(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- rs1 + rs2, pc += 4
    TRACE("ADD %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], abiNames[ins->rd_rs1_rs2.rs1], abiNames[ins->rd_rs1_rs2.rs2]);
    cpu->xreg[ins->rd_rs1_rs2.rd] = cpu->xreg[ins->rd_rs1_rs2.rs1] + cpu->xreg[ins->rd_rs1_rs2.rs2];
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Sub(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- rs1 - rs2, pc += 4
    TRACE("SUB %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], abiNames[ins->rd_rs1_rs2.rs1], abiNames[ins->rd_rs1_rs2.rs2]);
    cpu->xreg[ins->rd_rs1_rs2.rd] = cpu->xreg[ins->rd_rs1_rs2.rs1] - cpu->xreg[ins->rd_rs1_rs2.rs2];
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Mul(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    TRACE("MUL %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], abiNames[ins->rd_rs1_rs2.rs1], abiNames[ins->rd_rs1_rs2.rs2]);
    cpu->xreg[ins->rd_rs1_rs2.rd] = cpu->xreg[ins->rd_rs1_rs2.rs1] * cpu->xreg[ins->rd_rs1_rs2.rs2];
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Sll(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- rs1 << (rs2 % XLEN), pc += 4
    TRACE("SLL %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], abiNames[ins->rd_rs1_rs2.rs1], abiNames[ins->rd_rs1_rs2.rs2]);
    cpu->xreg[ins->rd_rs1_rs2.rd] = cpu->xreg[ins->rd_rs1_rs2.rs1] << (cpu->xreg[ins->rd_rs1_rs2.rs2] % 32);
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Mulh(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    TRACE("MULH %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], abiNames[ins->rd_rs1_rs2.rs1], abiNames[ins->rd_rs1_rs2.rs2]);
    int64_t t = (int64_t)(int32_t)cpu->xreg[ins->rd_rs1_rs2.rs1] * (int64_t)(int32_t)cpu->xreg[ins->rd_rs1_rs2.rs2];
    cpu->xreg[ins->rd_rs1_rs2.rd] = t >> 32;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Slt(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- (rs1 < rs2) ? 1 : 0, pc += 4
    TRACE("SLT %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], abiNames[ins->rd_rs1_rs2.rs1], abiNames[ins->rd_rs1_rs2.rs2]);
    cpu->xreg[ins->rd_rs1_rs2.rd] = ((int32_t)cpu->xreg[ins->rd_rs1_rs2.rs1] < (int32_t)cpu->xreg[ins->rd_rs1_rs2.rs2]) ? 1 : 0;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Mulhsu(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    TRACE("MULHSU %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], abiNames[ins->rd_rs1_rs2.rs1], abiNames[ins->rd_rs1_rs2.rs2]);
    int64_t t = (int64_t)(int32_t)cpu->xreg[ins->rd_rs1_rs2.rs1] * (uint64_t)cpu->xreg[ins->rd_rs1_rs2.rs2];
    cpu->xreg[ins->rd_rs1_rs2.rd] = t >> 32;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Sltu(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- (rs1 < rs2) ? 1 : 0, pc += 4
    TRACE("SLTU %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], abiNames[ins->rd_rs1_rs2.rs1], abiNames[ins->rd_rs1_rs2.rs2]);
    cpu->xreg[ins->rd_rs1_rs2.rd] = (cpu->xreg[ins->rd_rs1_rs2.rs1] < cpu->xreg[ins->rd_rs1_rs2.rs2]) ? 1 : 0;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
    cpu->xreg[0] = 0;
}

inline static void Exec_Mulhu(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    TRACE("MULHU %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], abiNames[ins->rd_rs1_rs2.rs1], abiNames[ins->rd_rs1_rs2.rs2]);
    uint64_t t = (uint64_t)cpu->xreg[ins->rd_rs1_rs2.rs1] * (uint64_t)cpu->xreg[ins->rd_rs1_rs2.rs2];
    cpu->xreg[ins->rd_rs1_rs2.rd] = t >> 32;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Xor(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- rs1 ^ rs2, pc += 4
    TRACE("XOR %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], abiNames[ins->rd_rs1_rs2.rs1], abiNames[ins->rd_rs1_rs2.rs2]);
    cpu->xreg[ins->rd_rs1_rs2.rd] = cpu->xreg[ins->rd_rs1_rs2.rs1] ^ cpu->xreg[ins->rd_rs1_rs2.rs2];
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Div(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    TRACE("DIV %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], abiNames[ins->rd_rs1_rs2.rs1], abiNames[ins->rd_rs1_rs2.rs2]);
    const int32_t dividend = (int32_t)cpu->xreg[ins->rd_rs1_rs2.rs1];
    const int32_t divisor = (int32_t)cpu->xreg[ins->rd_rs1_rs2.rs2];
    // Check for signed division overflow.
    if (dividend != 0x80000000 || divisor != -1)
    {
        cpu->xreg[ins->rd_rs1_rs2.rd] = divisor != 0 // Check for division by zero.
                ? dividend / divisor
                : -1;
    }
    else
    {
        // Signed division overflow occurred.
        cpu->xreg[ins->rd_rs1_rs2.rd] = dividend;
    }
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Srl(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- rs1 >> (rs2 % XLEN), pc += 4
    TRACE("SRL %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], abiNames[ins->rd_rs1_rs2.rs1], abiNames[ins->rd_rs1_rs2.rs2]);
    cpu->xreg[ins->rd_rs1_rs2.rd] = cpu->xreg[ins->rd_rs1_rs2.rs1] >> (cpu->xreg[ins->rd_rs1_rs2.rs2] % 32);
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Sra(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- rs1 >> (rs2 % XLEN), pc += 4
    TRACE("SRA %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], abiNames[ins->rd_rs1_rs2.rs1], abiNames[ins->rd_rs1_rs2.rs2]);
    cpu->xreg[ins->rd_rs1_rs2.rd] = (int32_t)cpu->xreg[ins->rd_rs1_rs2.rs1] >> (cpu->xreg[ins->rd_rs1_rs2.rs2] % 32);
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Divu(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    TRACE("DIVU %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], abiNames[ins->rd_rs1_rs2.rs1], abiNames[ins->rd_rs1_rs2.rs2]);
    uint32_t divisor = cpu->xreg[ins->rd_rs1_rs2.rs2];
    cpu->xreg[ins->rd_rs1_rs2.rd] = divisor != 0 ? cpu->xreg[ins->rd_rs1_rs2.rs1] / divisor : 0xffffffff;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Or(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- rs1 | rs2, pc += 4
    TRACE("OR %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], abiNames[ins->rd_rs1_rs2.rs1], abiNames[ins->rd_rs1_rs2.rs2]);
    cpu->xreg[ins->rd_rs1_rs2.rd] = cpu->xreg[ins->rd_rs1_rs2.rs1] | cpu->xreg[ins->rd_rs1_rs2.rs2];
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Rem(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    TRACE("REM %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], abiNames[ins->rd_rs1_rs2.rs1], abiNames[ins->rd_rs1_rs2.rs2]);
    const int32_t dividend = (int32_t)cpu->xreg[ins->rd_rs1_rs2.rs1];
    const int32_t divisor = (int32_t)cpu->xreg[ins->rd_rs1_rs2.rs2];
    // Check for signed division overflow.
    if (dividend != 0x80000000 || divisor != -1)
    {
        cpu->xreg[ins->rd_rs1_rs2.rd] = divisor != 0 // Check for division by zero.
                ? dividend % divisor
                : dividend;
    }
    else
    {
        // Signed division overflow occurred.
        cpu->xreg[ins->rd_rs1_rs2.rd] = 0;
    }
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_And(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- rs1 & rs2, pc += 4
    TRACE("AND %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], abiNames[ins->rd_rs1_rs2.rs1], abiNames[ins->rd_rs1_rs2.rs2]);
    cpu->xreg[ins->rd_rs1_rs2.rd] = cpu->xreg[ins->rd_rs1_rs2.rs1] & cpu->xreg[ins->rd_rs1_rs2.rs2];
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Remu(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    TRACE("REMU %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], abiNames[ins->rd_rs1_rs2.rs1], abiNames[ins->rd_rs1_rs2.rs2]);
    const uint32_t dividend = cpu->xreg[ins->rd_rs1_rs2.rs1];
    const uint32_t divisor = cpu->xreg[ins->rd_rs1_rs2.rs2];
    cpu->xreg[ins->rd_rs1_rs2.rd] = divisor != 0 ? cpu->xreg[ins->rd_rs1_rs2.rs1] % divisor : dividend;
    cpu->pc += 4;
    cpu->xreg[0] = 0;
}

inline static void Exec_Fence(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    TRACE("FENCE\n");
    cpu->result = CreateTrap(cpu, trNOT_IMPLEMENTED_YET, 0);
}

inline static void Exec_Ecall(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    TRACE("ECALL\n");
    cpu->result = CreateTrap(cpu, trENVIRONMENT_CALL_FROM_M_MODE, 0);
}

inline static void Exec_Ebreak(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    TRACE("EBREAK\n");
    cpu->result = CreateTrap(cpu, trBREAKPOINT, 0); // TODO: what should value be here?
}

inline static void Exec_Uret(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    TRACE("URET\n");
    // TODO: Only provide this if user mode traps are supported, otherwise raise an illegal instruction exception.
    cpu->result = CreateTrap(cpu, trNOT_IMPLEMENTED_YET, 0);
}

inline static void Exec_Sret(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    TRACE("SRET\n");
    // TODO: Only provide this if supervisor mode is supported, otherwise raise an illegal instruction exception.
    cpu->result = CreateTrap(cpu, trNOT_IMPLEMENTED_YET, 0);
}

inline static void Exec_Mret(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // pc <- mepc, pc += 4
    TRACE("MRET\n");
    cpu->pc = cpu->mepc; // Restore the program counter from the machine exception program counter.
    cpu->pc += 4;        // ...and increment it as normal.
}

inline static void Exec_Flw(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- f32(rs1 + imm_i)
    TRACE("FLW %s, %d(%s)\n", fabiNames[ins->rd_rs1_imm.rd], ins->rd_rs1_imm.imm, abiNames[ins->rd_rs1_imm.rs1]);
    if (cpu->busCode != bcOK)
    {
        return;
    }
    uint32_t word = Read32(&cpu->bus, cpu->xreg[ins->rd_rs1_imm.rs1] + ins->rd_rs1_imm.imm, &cpu->busCode);
    if (cpu->busCode != bcOK)
    {
        cpu->result = TakeTrap(cpu, ArvissMakeTrap(trLOAD_ACCESS_FAULT, cpu->xreg[ins->rd_rs1_imm.rs1] + ins->rd_rs1_imm.imm));
        return;
    }
    const float resultAsFloat = U32AsFloat(word);
    cpu->freg[ins->rd_rs1_imm.rd] = resultAsFloat;
    cpu->pc += 4;
}

inline static void Exec_Fsw(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // f32(rs1 + imm_s) = rs2
    TRACE("FSW %s, %d(%s)\n", fabiNames[ins->rs1_rs2_imm.rs2], ins->rs1_rs2_imm.imm, abiNames[ins->rs1_rs2_imm.rs1]);
    uint32_t t = FloatAsU32(cpu->freg[ins->rs1_rs2_imm.rs2]);
    Write32(&cpu->bus, cpu->xreg[ins->rs1_rs2_imm.rs1] + ins->rs1_rs2_imm.imm, t, &cpu->busCode);
    if (cpu->busCode != bcOK)
    {
        cpu->result = TakeTrap(cpu, ArvissMakeTrap(trSTORE_ACCESS_FAULT, cpu->xreg[ins->rs1_rs2_imm.rs1] + ins->rs1_rs2_imm.imm));
        return;
    }
    cpu->pc += 4;
}

inline static void Exec_Fmadd_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- (rs1 * rs2) + rs3
    TRACE("FMADD.S %s, %s, %s, %s, %s\n", fabiNames[ins->rd_rs1_rs2_rs3_rm.rd], fabiNames[ins->rd_rs1_rs2_rs3_rm.rs1],
          fabiNames[ins->rd_rs1_rs2_rs3_rm.rs2], fabiNames[ins->rd_rs1_rs2_rs3_rm.rs3], roundingModes[ins->rd_rs1_rs2_rs3_rm.rm]);
    cpu->freg[ins->rd_rs1_rs2_rs3_rm.rd] =
            (cpu->freg[ins->rd_rs1_rs2_rs3_rm.rs1] * cpu->freg[ins->rd_rs1_rs2_rs3_rm.rs2]) + cpu->freg[ins->rd_rs1_rs2_rs3_rm.rs3];
    cpu->pc += 4;
    // TODO: rounding.
}

inline static void Exec_Fmsub_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- (rs1 x rs2) - rs3
    TRACE("FMSUB.S %s, %s, %s, %s, %s\n", fabiNames[ins->rd_rs1_rs2_rs3_rm.rd], fabiNames[ins->rd_rs1_rs2_rs3_rm.rs1],
          fabiNames[ins->rd_rs1_rs2_rs3_rm.rs2], fabiNames[ins->rd_rs1_rs2_rs3_rm.rs3], roundingModes[ins->rd_rs1_rs2_rs3_rm.rm]);
    cpu->freg[ins->rd_rs1_rs2_rs3_rm.rd] =
            (cpu->freg[ins->rd_rs1_rs2_rs3_rm.rs1] * cpu->freg[ins->rd_rs1_rs2_rs3_rm.rs2]) - cpu->freg[ins->rd_rs1_rs2_rs3_rm.rs3];
    cpu->pc += 4;
    // TODO: rounding.
}

inline static void Exec_Fnmsub_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- -(rs1 x rs2) + rs3
    TRACE("FNMSUB.S %s, %s, %s, %s, %s\n", fabiNames[ins->rd_rs1_rs2_rs3_rm.rd], fabiNames[ins->rd_rs1_rs2_rs3_rm.rs1],
          fabiNames[ins->rd_rs1_rs2_rs3_rm.rs2], fabiNames[ins->rd_rs1_rs2_rs3_rm.rs3], roundingModes[ins->rd_rs1_rs2_rs3_rm.rm]);
    cpu->freg[ins->rd_rs1_rs2_rs3_rm.rd] = -(cpu->freg[ins->rd_rs1_rs2_rs3_rm.rs1] * cpu->freg[ins->rd_rs1_rs2_rs3_rm.rs2])
            + cpu->freg[ins->rd_rs1_rs2_rs3_rm.rs3];
    cpu->pc += 4;
    // TODO: rounding.
}

inline static void Exec_Fnmadd_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- -(rs1 x rs2) - rs3
    TRACE("FNMADD.S %s, %s, %s, %s, %s\n", fabiNames[ins->rd_rs1_rs2_rs3_rm.rd], fabiNames[ins->rd_rs1_rs2_rs3_rm.rs1],
          fabiNames[ins->rd_rs1_rs2_rs3_rm.rs2], fabiNames[ins->rd_rs1_rs2_rs3_rm.rs3], roundingModes[ins->rd_rs1_rs2_rs3_rm.rm]);
    cpu->freg[ins->rd_rs1_rs2_rs3_rm.rd] = -(cpu->freg[ins->rd_rs1_rs2_rs3_rm.rs1] * cpu->freg[ins->rd_rs1_rs2_rs3_rm.rs2])
            - cpu->freg[ins->rd_rs1_rs2_rs3_rm.rs3];
    cpu->pc += 4;
    // TODO: rounding.
}

inline static void Exec_Fadd_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- rs1 + rs2
    TRACE("FADD.S %s, %s, %s, %s\n", fabiNames[ins->rd_rs1_rs2_rm.rd], fabiNames[ins->rd_rs1_rs2_rm.rs1],
          fabiNames[ins->rd_rs1_rs2_rm.rs2], roundingModes[ins->rd_rs1_rs2_rm.rm]);
    cpu->freg[ins->rd_rs1_rs2_rm.rd] = cpu->freg[ins->rd_rs1_rs2_rm.rs1] + cpu->freg[ins->rd_rs1_rs2_rm.rs2];
    cpu->pc += 4;
    // TODO: rounding.
}

inline static void Exec_Fsub_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- rs1 - rs2
    TRACE("FSUB.S %s, %s, %s, %s\n", fabiNames[ins->rd_rs1_rs2_rm.rd], fabiNames[ins->rd_rs1_rs2_rm.rs1],
          fabiNames[ins->rd_rs1_rs2_rm.rs2], roundingModes[ins->rd_rs1_rs2_rm.rm]);
    cpu->freg[ins->rd_rs1_rs2_rm.rd] = cpu->freg[ins->rd_rs1_rs2_rm.rs1] - cpu->freg[ins->rd_rs1_rs2_rm.rs2];
    cpu->pc += 4;
    // TODO: rounding.
}

inline static void Exec_Fmul_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- rs1 * rs2
    TRACE("FMUL.S %s, %s, %s, %s\n", fabiNames[ins->rd_rs1_rs2_rm.rd], fabiNames[ins->rd_rs1_rs2_rm.rs1],
          fabiNames[ins->rd_rs1_rs2_rm.rs2], roundingModes[ins->rd_rs1_rs2_rm.rm]);
    cpu->freg[ins->rd_rs1_rs2_rm.rd] = cpu->freg[ins->rd_rs1_rs2_rm.rs1] * cpu->freg[ins->rd_rs1_rs2_rm.rs2];
    cpu->pc += 4;
    // TODO: rounding.
}

inline static void Exec_Fdiv_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- rs1 / rs2
    TRACE("FDIV.S %s, %s, %s, %s\n", fabiNames[ins->rd_rs1_rs2_rm.rd], fabiNames[ins->rd_rs1_rs2_rm.rs1],
          fabiNames[ins->rd_rs1_rs2_rm.rs2], roundingModes[ins->rd_rs1_rs2_rm.rm]);
    cpu->freg[ins->rd_rs1_rs2_rm.rd] = cpu->freg[ins->rd_rs1_rs2_rm.rs1] / cpu->freg[ins->rd_rs1_rs2_rm.rs2];
    cpu->pc += 4;
    // TODO: rounding.
}

inline static void Exec_Fsqrt_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- sqrt(rs1)
    TRACE("FSQRT.S %s, %s, %s\n", fabiNames[ins->rd_rs1_rm.rd], fabiNames[ins->rd_rs1_rm.rs1], roundingModes[ins->rd_rs1_rm.rm]);
    cpu->freg[ins->rd_rs1_rm.rd] = sqrtf(cpu->freg[ins->rd_rs1_rm.rs1]);
    cpu->pc += 4;
    // TODO: rounding.
}

inline static void Exec_Fsgnj_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- abs(rs1) * sgn(rs2)
    TRACE("FSGNJ.S %s, %s, %s\n", fabiNames[ins->rd_rs1_rs2.rd], fabiNames[ins->rd_rs1_rs2.rs1], fabiNames[ins->rd_rs1_rs2.rs2]);
    cpu->freg[ins->rd_rs1_rs2.rd] = fabsf(cpu->freg[ins->rd_rs1_rs2.rs1]) * (cpu->freg[ins->rd_rs1_rs2.rs2] < 0.0f ? -1.0f : 1.0f);
    cpu->pc += 4;
}

inline static void Exec_Fsgnjn_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- abs(rs1) * -sgn(rs2)
    TRACE("FSGNJN.S %s, %s, %s\n", fabiNames[ins->rd_rs1_rs2.rd], fabiNames[ins->rd_rs1_rs2.rs1], fabiNames[ins->rd_rs1_rs2.rs2]);
    cpu->freg[ins->rd_rs1_rs2.rd] = fabsf(cpu->freg[ins->rd_rs1_rs2.rs1]) * (cpu->freg[ins->rd_rs1_rs2.rs2] < 0.0f ? 1.0f : -1.0f);
    cpu->pc += 4;
}

inline static void Exec_Fsgnjx_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    float m; // The sign bit is the XOR of the sign bits of rs1 and rs2.
    if ((cpu->freg[ins->rd_rs1_rs2.rs1] < 0.0f && cpu->freg[ins->rd_rs1_rs2.rs2] >= 0.0f)
        || (cpu->freg[ins->rd_rs1_rs2.rs1] >= 0.0f && cpu->freg[ins->rd_rs1_rs2.rs2] < 0.0f))
    {
        m = -1.0f;
    }
    else
    {
        m = 1.0f;
    }
    // rd <- abs(rs1) * (sgn(rs1) == sgn(rs2)) ? 1 : -1
    TRACE("FSGNJX.S %s, %s, %s\n", fabiNames[ins->rd_rs1_rs2.rd], fabiNames[ins->rd_rs1_rs2.rs1], fabiNames[ins->rd_rs1_rs2.rs2]);
    cpu->freg[ins->rd_rs1_rs2.rd] = fabsf(cpu->freg[ins->rd_rs1_rs2.rs1]) * m;
    cpu->pc += 4;
}

inline static void Exec_Fmin_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- min(rs1, rs2)
    TRACE("FMIN.S %s, %s, %s\n", fabiNames[ins->rd_rs1_rs2.rd], fabiNames[ins->rd_rs1_rs2.rs1], fabiNames[ins->rd_rs1_rs2.rs2]);
    cpu->freg[ins->rd_rs1_rs2.rd] = fminf(cpu->freg[ins->rd_rs1_rs2.rs1], cpu->freg[ins->rd_rs1_rs2.rs2]);
    cpu->pc += 4;
}

inline static void Exec_Fmax_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- max(rs1, rs2)
    TRACE("FMAX.S %s, %s, %s\n", fabiNames[ins->rd_rs1_rs2.rd], fabiNames[ins->rd_rs1_rs2.rs1], fabiNames[ins->rd_rs1_rs2.rs2]);
    cpu->freg[ins->rd_rs1_rs2.rd] = fmaxf(cpu->freg[ins->rd_rs1_rs2.rs1], cpu->freg[ins->rd_rs1_rs2.rs2]);
    cpu->pc += 4;
}

inline static void Exec_Fcvt_w_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- int32_t(rs1)
    TRACE("FCVT.W.S %s, %s, %s\n", abiNames[ins->rd_rs1_rm.rd], fabiNames[ins->rd_rs1_rm.rs1], roundingModes[ins->rd_rs1_rm.rm]);
    cpu->xreg[ins->rd_rs1_rm.rd] = (int32_t)cpu->freg[ins->rd_rs1_rm.rs1];
    cpu->pc += 4;
    // TODO: rounding.
}

inline static void Exec_Fcvt_wu_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- uint32_t(rs1)
    TRACE("FCVT.WU.S %s, %s, %s\n", abiNames[ins->rd_rs1_rm.rd], fabiNames[ins->rd_rs1_rm.rs1], roundingModes[ins->rd_rs1_rm.rm]);
    cpu->xreg[ins->rd_rs1_rm.rd] = (uint32_t)(int32_t)cpu->freg[ins->rd_rs1_rm.rs1];
    cpu->pc += 4;
    // TODO: rounding.
}

inline static void Exec_Fmv_x_w(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // bits(rd) <- bits(rs1)
    TRACE("FMV.X.W %s, %s\n", abiNames[ins->rd_rs1.rd], fabiNames[ins->rd_rs1.rs1]);
    cpu->xreg[ins->rd_rs1.rd] = FloatAsU32(cpu->freg[ins->rd_rs1.rs1]);
    cpu->pc += 4;
}

inline static void Exec_Fclass_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    TRACE("FCLASS.S %s, %s\n", abiNames[ins->rd_rs1.rd], fabiNames[ins->rd_rs1.rs1]);
    const float v = cpu->freg[ins->rd_rs1.rs1];
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
    cpu->xreg[ins->rd_rs1.rd] = result;
    cpu->pc += 4;
}

inline static void Exec_Feq_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- (rs1 == rs2) ? 1 : 0;
    TRACE("FEQ.S %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], fabiNames[ins->rd_rs1_rs2.rs1], fabiNames[ins->rd_rs1_rs2.rs2]);
    cpu->xreg[ins->rd_rs1_rs2.rd] = cpu->freg[ins->rd_rs1_rs2.rs1] == cpu->freg[ins->rd_rs1_rs2.rs2] ? 1 : 0;
    cpu->pc += 4;
}

inline static void Exec_Flt_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- (rs1 < rs2) ? 1 : 0;
    TRACE("FLT.S %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], fabiNames[ins->rd_rs1_rs2.rs1], fabiNames[ins->rd_rs1_rs2.rs2]);
    cpu->xreg[ins->rd_rs1_rs2.rd] = cpu->freg[ins->rd_rs1_rs2.rs1] < cpu->freg[ins->rd_rs1_rs2.rs2] ? 1 : 0;
    cpu->pc += 4;
}

inline static void Exec_Fle_s(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- (rs1 <= rs2) ? 1 : 0;
    TRACE("FLE.S %s, %s, %s\n", abiNames[ins->rd_rs1_rs2.rd], fabiNames[ins->rd_rs1_rs2.rs1], fabiNames[ins->rd_rs1_rs2.rs2]);
    cpu->xreg[ins->rd_rs1_rs2.rd] = cpu->freg[ins->rd_rs1_rs2.rs1] <= cpu->freg[ins->rd_rs1_rs2.rs2] ? 1 : 0;
    cpu->pc += 4;
}

inline static void Exec_Fcvt_s_w(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- float(int32_t((rs1))
    TRACE("FCVT.S.W %s, %s, %s\n", fabiNames[ins->rd_rs1_rm.rd], abiNames[ins->rd_rs1_rm.rs1], roundingModes[ins->rd_rs1_rm.rm]);
    cpu->freg[ins->rd_rs1_rm.rd] = (float)(int32_t)cpu->xreg[ins->rd_rs1_rm.rs1];
    cpu->pc += 4;
    // TODO: rounding.
}

inline static void Exec_Fcvt_s_wu(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // rd <- float(rs1)
    TRACE("FVCT.S.WU %s, %s, %s\n", fabiNames[ins->rd_rs1_rm.rd], abiNames[ins->rd_rs1_rm.rs1], roundingModes[ins->rd_rs1_rm.rm]);
    cpu->freg[ins->rd_rs1_rm.rd] = (float)cpu->xreg[ins->rd_rs1_rm.rs1];
    cpu->pc += 4;
    // TODO: rounding.
}

inline static void Exec_Fmv_w_x(ArvissCpu* cpu, const DecodedInstruction* ins)
{
    // bits(rd) <- bits(rs1)
    TRACE("FMV.W.X %s, %s\n", fabiNames[ins->rd_rs1.rd], abiNames[ins->rd_rs1.rs1]);
    cpu->freg[ins->rd_rs1.rd] = U32AsFloat(cpu->xreg[ins->rd_rs1.rs1]);
    cpu->pc += 4;
}

// --- Decoding --------------------------------------------------------------------------------------------------------------------
//
// Functions in this section decode instructions into their executable form.

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

// See: http://www.five-embeddev.com/riscv-isa-manual/latest/gmaps.html#rv3264g-instruction-set-listings
// or riscv-spec-209191213.pdf.
static DecodedInstruction ArvissDecode(uint32_t instruction)
{
    const uint32_t opcode = Opcode(instruction);
    const uint32_t rd = Rd(instruction);
    const uint32_t rs1 = Rs1(instruction);

    switch (opcode)
    {
    case opLUI: {
        const int32_t upper = UImmediate(instruction);
        return MkRdImm(execLui, rd, upper);
    }

    case opAUIPC: {
        const int32_t upper = UImmediate(instruction);
        return MkRdImm(execAuipc, rd, upper);
    }

    case opJAL: {
        const int32_t imm = JImmediate(instruction);
        return MkRdImm(execJal, rd, imm);
    }

    case opJALR: {
        const uint32_t funct3 = Funct3(instruction);
        if (funct3 == 0b000)
        {
            const int32_t imm = IImmediate(instruction);
            return MkRdRs1Imm(execJalr, rd, rs1, imm);
        }
        return MkTrap(execIllegalInstruction, instruction);
    }

    case opBRANCH: {
        const uint32_t funct3 = Funct3(instruction);
        const uint32_t rs2 = Rs2(instruction);
        const int32_t imm = BImmediate(instruction);
        switch (funct3)
        {
        case 0b000: // BEQ
            return MkRs1Rs2Imm(execBeq, rs1, rs2, imm);
        case 0b001: // BNE
            return MkRs1Rs2Imm(execBne, rs1, rs2, imm);
        case 0b100: // BLT
            return MkRs1Rs2Imm(execBlt, rs1, rs2, imm);
        case 0b101: // BGE
            return MkRs1Rs2Imm(execBge, rs1, rs2, imm);
        case 0b110: // BLTU
            return MkRs1Rs2Imm(execBltu, rs1, rs2, imm);
        case 0b111: // BGEU
            return MkRs1Rs2Imm(execBgeu, rs1, rs2, imm);
        default:
            return MkTrap(execIllegalInstruction, instruction);
        }
    }

    case opLOAD: {
        const uint32_t funct3 = Funct3(instruction);
        const int32_t imm = IImmediate(instruction);
        switch (funct3)
        {
        case 0b000: // LB
            return MkRdRs1Imm(execLb, rd, rs1, imm);
        case 0b001: // LH
            return MkRdRs1Imm(execLh, rd, rs1, imm);
        case 0b010: // LW
            return MkRdRs1Imm(execLw, rd, rs1, imm);
        case 0b100: // LBU
            return MkRdRs1Imm(execLbu, rd, rs1, imm);
        case 0b101: // LHU
            return MkRdRs1Imm(execLhu, rd, rs1, imm);
        default:
            return MkTrap(execIllegalInstruction, instruction);
        }
    }

    case opSTORE: {
        const uint32_t funct3 = Funct3(instruction);
        const uint32_t rs2 = Rs2(instruction);
        const int32_t imm = SImmediate(instruction);
        switch (funct3)
        {
        case 0b000: // SB
            return MkRs1Rs2Imm(execSb, rs1, rs2, imm);
        case 0b001: // SH
            return MkRs1Rs2Imm(execSh, rs1, rs2, imm);
        case 0b010: // SW
            return MkRs1Rs2Imm(execSw, rs1, rs2, imm);
        default:
            return MkTrap(execIllegalInstruction, instruction);
        }
    }

    case opOPIMM: {
        const uint32_t funct3 = Funct3(instruction);
        const int32_t imm = IImmediate(instruction);
        const uint32_t funct7 = Funct7(instruction);
        switch (funct3)
        {
        case 0b000: // ADDI
            return MkRdRs1Imm(execAddi, rd, rs1, imm);
        case 0b010: // SLTI
            return MkRdRs1Imm(execSlti, rd, rs1, imm);
        case 0b011: // SLTIU
            return MkRdRs1Imm(execSltiu, rd, rs1, imm & 0x1f);
        case 0b100: // XORI
            return MkRdRs1Imm(execXori, rd, rs1, imm);
        case 0b110: // ORI
            return MkRdRs1Imm(execOri, rd, rs1, imm);
        case 0b111: // ANDI
            return MkRdRs1Imm(execAndi, rd, rs1, imm);
        case 0b001: // SLLI
            return MkRdRs1Imm(execSlli, rd, rs1, imm & 0x1f);
        case 0b101:
            switch (funct7)
            {
            case 0b0000000: // SRLI
                return MkRdRs1Imm(execSrli, rd, rs1, imm & 0x1f);
            case 0b0100000: // SRAI
                return MkRdRs1Imm(execSrai, rd, rs1, imm & 0x1f);
            default:
                return MkTrap(execIllegalInstruction, instruction);
            }
        default:
            return MkTrap(execIllegalInstruction, instruction);
        }
    }

    case opOP: {
        const uint32_t funct3 = Funct3(instruction);
        const uint32_t rs2 = Rs2(instruction);
        const uint32_t funct7 = Funct7(instruction);
        switch (funct3)
        {
        case 0b000:
            switch (funct7)
            {
            case 0b0000000: // ADD
                return MkRdRs1Rs2(execAdd, rd, rs1, rs2);
            case 0b0100000: // SUB
                return MkRdRs1Rs2(execSub, rd, rs1, rs2);
            case 0b00000001: // MUL (RV32M)
                return MkRdRs1Rs2(execMul, rd, rs1, rs2);
            default:
                return MkTrap(execIllegalInstruction, instruction);
            }
        case 0b001:
            switch (funct7)
            {
            case 0b0000000: // SLL
                return MkRdRs1Rs2(execSll, rd, rs1, rs2);
            case 0b00000001: // MULH (RV32M)
                return MkRdRs1Rs2(execMulh, rd, rs1, rs2);
            default:
                return MkTrap(execIllegalInstruction, instruction);
            }
        case 0b010:
            switch (funct7)
            {
            case 0b0000000: // SLT
                return MkRdRs1Rs2(execSlt, rd, rs1, rs2);
            case 0b00000001: // MULHSU (RV32M)
                return MkRdRs1Rs2(execMulhsu, rd, rs1, rs2);
            default:
                return MkTrap(execIllegalInstruction, instruction);
            }
        case 0b011:
            switch (funct7)
            {
            case 0b0000000: // SLTU
                return MkRdRs1Rs2(execSltu, rd, rs1, rs2);
            case 0b00000001: // MULHU (RV32M)
                return MkRdRs1Rs2(execMulhu, rd, rs1, rs2);
            default:
                return MkTrap(execIllegalInstruction, instruction);
            }
        case 0b100:
            switch (funct7)
            {
            case 0b0000000: // XOR
                return MkRdRs1Rs2(execXor, rd, rs1, rs2);
            case 0b00000001: // DIV (RV32M)
                return MkRdRs1Rs2(execDiv, rd, rs1, rs2);
            default:
                return MkTrap(execIllegalInstruction, instruction);
            }
        case 0b101:
            switch (funct7)
            {
            case 0b0000000: // SRL
                return MkRdRs1Rs2(execSrl, rd, rs1, rs2);
            case 0b0100000: // SRA
                return MkRdRs1Rs2(execSra, rd, rs1, rs2);
            case 0b00000001: // DIVU (RV32M)
                return MkRdRs1Rs2(execDivu, rd, rs1, rs2);
            default:
                return MkTrap(execIllegalInstruction, instruction);
            }
        case 0b110:
            switch (funct7)
            {
            case 0b0000000: // OR
                return MkRdRs1Rs2(execOr, rd, rs1, rs2);
            case 0b00000001: // REM
                return MkRdRs1Rs2(execRem, rd, rs1, rs2);
            default:
                return MkTrap(execIllegalInstruction, instruction);
            }
        case 0b111:
            switch (funct7)
            {
            case 0b0000000: // AND
                return MkRdRs1Rs2(execAnd, rd, rs1, rs2);
            case 0b00000001: // REMU
                return MkRdRs1Rs2(execRemu, rd, rs1, rs2);
            default:
                return MkTrap(execIllegalInstruction, instruction);
            }
        default:
            return MkTrap(execIllegalInstruction, instruction);
        }
    }

    case opMISCMEM: {
        const uint32_t funct3 = Funct3(instruction);
        if (funct3 == 0b000)
        {
            return MkNoArg(execFence);
        }
        else
        {
            return MkTrap(execIllegalInstruction, instruction);
        }
    }

    case opSYSTEM:
        if ((instruction & 0b00000000000011111111111110000000) == 0)
        {
            const uint32_t funct12 = Funct12(instruction);
            switch (funct12)
            {
            case 0b000000000000: // ECALL
                return MkNoArg(execEcall);
            case 0b000000000001: // EBREAK
                return MkNoArg(execEbreak);
            case 0b000000000010: // URET
                return MkNoArg(execUret);
            case 0b000100000010: // SRET
                return MkNoArg(execSret);
            case 0b001100000010: // MRET
                return MkNoArg(execMret);
            default:
                return MkTrap(execIllegalInstruction, instruction);
            }
        }
        else
        {
            return MkTrap(execIllegalInstruction, instruction);
        }

    case opLOADFP: { // Floating point load (RV32F)
        const uint32_t funct3 = Funct3(instruction);
        if (funct3 == 0b010) // FLW
        {
            const int32_t imm = IImmediate(instruction);
            return MkRdRs1Imm(execFlw, rd, rs1, imm);
        }
        else
        {
            return MkTrap(execIllegalInstruction, instruction);
        }
    }

    case opSTOREFP: { // Floating point store (RV32F)
        const uint32_t funct3 = Funct3(instruction);
        if (funct3 == 0b010) // FSW
        {
            // f32(rs1 + imm_s) = rs2
            const uint32_t rs2 = Rs2(instruction);
            const int32_t imm = SImmediate(instruction);
            return MkRs1Rs2Imm(execFsw, rs1, rs2, imm);
        }
        else
        {
            return MkTrap(execIllegalInstruction, instruction);
        }
    }

    case opMADD: {                             // Floating point fused multiply-add (RV32F)
        if (((instruction >> 25) & 0b11) == 0) // FMADD.S
        {
            const uint32_t rm = Rm(instruction);
            const uint32_t rs2 = Rs2(instruction);
            const uint32_t rs3 = Rs3(instruction);
            return MkRdRs1Rs2Rs3Rm(execFmadd_s, rd, rs1, rs2, rs3, rm);
        }
        else
        {
            return MkTrap(execIllegalInstruction, instruction);
        }
    }

    case opMSUB: {                             // Floating point fused multiply-sub (RV32F)
        if (((instruction >> 25) & 0b11) == 0) // FMSUB.S
        {
            const uint32_t rm = Rm(instruction);
            const uint32_t rs2 = Rs2(instruction);
            const uint32_t rs3 = Rs3(instruction);
            return MkRdRs1Rs2Rs3Rm(execFmsub_s, rd, rs1, rs2, rs3, rm);
        }
        else
        {
            return MkTrap(execIllegalInstruction, instruction);
        }
    }

    case opNMSUB: {                            // Floating point negated fused multiply-sub (RV32F)
        if (((instruction >> 25) & 0b11) == 0) // FNMSUB.S
        {
            const uint32_t rm = Rm(instruction);
            const uint32_t rs2 = Rs2(instruction);
            const uint32_t rs3 = Rs3(instruction);
            return MkRdRs1Rs2Rs3Rm(execFnmsub_s, rd, rs1, rs2, rs3, rm);
        }
        else
        {
            return MkTrap(execIllegalInstruction, instruction);
        }
    }

    case opNMADD: {                            // Floating point negated fused multiply-add (RV32F)
        if (((instruction >> 25) & 0b11) == 0) // FNMADD.S
        {
            const uint32_t rm = Rm(instruction);
            const uint32_t rs2 = Rs2(instruction);
            const uint32_t rs3 = Rs3(instruction);
            return MkRdRs1Rs2Rs3Rm(execFnmadd_s, rd, rs1, rs2, rs3, rm);
        }
        else
        {
            return MkTrap(execIllegalInstruction, instruction);
        }
    }

    case opOPFP: { // Floating point operations (RV32F)
        const uint32_t funct7 = Funct7(instruction);
        const uint32_t funct3 = Funct3(instruction);
        const uint32_t rm = funct3;
        const uint32_t rs2 = Rs2(instruction);
        switch (funct7)
        {
        case 0b0000000: // FADD.S
            return MkRdRs1Rs2Rm(execFadd_s, rd, rs1, rs2, rm);
        case 0b0000100: // FSUB.S
            return MkRdRs1Rs2Rm(execFsub_s, rd, rs1, rs2, rm);
        case 0b0001000: // FMUL.S
            return MkRdRs1Rs2Rm(execFmul_s, rd, rs1, rs2, rm);
        case 0b0001100: // FDIV.S
            return MkRdRs1Rs2Rm(execFdiv_s, rd, rs1, rs2, rm);
        case 0b0101100:
            if (rs2 == 0b00000) // FSQRT.S
            {
                return MkRdRs1Rm(execFsqrt_s, rd, rs1, rm);
            }
            else
            {
                return MkTrap(execIllegalInstruction, instruction);
            }
        case 0b0010000: {
            switch (funct3)
            {
            case 0b000: // FSGNJ.S
                return MkRdRs1Rs2Rm(execFsgnj_s, rd, rs1, rs2, rm);
            case 0b001: // FSGNJN.S
                return MkRdRs1Rs2Rm(execFsgnjn_s, rd, rs1, rs2, rm);
            case 0b010: // FSGNJX.S
                return MkRdRs1Rs2Rm(execFsgnjx_s, rd, rs1, rs2, rm);
            default:
                return MkTrap(execIllegalInstruction, instruction);
            }
        }
        case 0b0010100: {
            switch (funct3)
            {
            case 0b000: // FMIN.S
                return MkRdRs1Rs2(execFmin_s, rd, rs1, rs2);
            case 0b001: // FMAX.S
                return MkRdRs1Rs2(execFmax_s, rd, rs1, rs2);
            default:
                return MkTrap(execIllegalInstruction, instruction);
            }
        }
        case 0b1100000: {
            switch (rs2) // Not actually rs2 - just the same bits.
            {
            case 0b00000: // FCVT.W.S
                return MkRdRs1Rm(execFcvt_w_s, rd, rs1, rm);
            case 0b00001: // FVCT.WU.S
                return MkRdRs1Rm(execFcvt_wu_s, rd, rs1, rm);
            default:
                return MkTrap(execIllegalInstruction, instruction);
            }
        }
        case 0b1110000: {
            if (rs2 == 0b00000) // Not actually rs2 - just the same bits.
            {
                switch (funct3)
                {
                case 0b000: // FMV.X.W
                    return MkRdRs1(execFmv_x_w, rd, rs1);
                case 0b001: // FCLASS.S
                    return MkRdRs1(execFclass_s, rd, rs1);
                default:
                    return MkTrap(execIllegalInstruction, instruction);
                }
            }
            return MkTrap(execIllegalInstruction, instruction);
        }
        case 0b1010000: {
            switch (funct3)
            {
            case 0b010: // FEQ.S
                return MkRdRs1Rs2(execFeq_s, rd, rs1, rs2);
            case 0b001: // FLT.S
                return MkRdRs1Rs2(execFlt_s, rd, rs1, rs2);
            case 0b000: // FLE.S
                return MkRdRs1Rs2(execFle_s, rd, rs1, rs2);
            default:
                return MkTrap(execIllegalInstruction, instruction);
            }
        }
        case 0b1101000: {
            switch (rs2) // No actually rs2 - just the same bits,
            {
            case 0b00000: // FCVT.S.W
                return MkRdRs1(execFcvt_s_w, rd, rs1);
            case 0b00001: // FVCT.S.WU
                return MkRdRs1(execFcvt_s_wu, rd, rs1);
            default:
                return MkTrap(execIllegalInstruction, instruction);
            }
        }
        case 0b1111000:
            if (rs2 == 0b00000 && funct3 == 0b000) // FMV.W.X
            {
                return MkRdRs1(execFmv_w_x, rd, rs1);
            }
            else
            {
                return MkTrap(execIllegalInstruction, instruction);
            }
        default:
            return MkTrap(execIllegalInstruction, instruction);
        }
    }

    default:
        return MkTrap(execIllegalInstruction, instruction);
    }
}

static inline DecodedInstruction* FetchFromCache(ArvissCpu* cpu)
{
    // Use the PC to figure out which cache line we need and where we are in it (the line index).
    const uint32_t addr = cpu->pc;
    const uint32_t owner = ((addr / 4) / CACHE_LINE_LENGTH);
    const uint32_t cacheLine = owner % CACHE_LINES;
    const uint32_t lineIndex = (addr / 4) % CACHE_LINE_LENGTH;
    struct CacheLine* line = &cpu->cache.line[cacheLine];

    // If we don't own the cache line, or it's invalid, then populate it.
    if (owner != line->owner || !line->isValid)
    {
        // Populate this cache line with fetch/decode/replace operations which, when called, replace themselves with a decoded
        // version of the instruction at the corresponding address. This way we don't incur an overhead for decoding instructions
        // that are never run.
        for (uint32_t i = 0u; i < CACHE_LINE_LENGTH; i++)
        {
            line->instructions[i] = MkFetchDecodeReplace(execFetchDecodeReplace, cacheLine, i);
        }
        line->isValid = true;
        line->owner = owner;
    }

    return &line->instructions[lineIndex];
}

// --- The Arviss API --------------------------------------------------------------------------------------------------------------

static void RunOne(ArvissCpu* cpu, DecodedInstruction* ins)
{
    switch (ins->opcode)
    {
    case execFetchDecodeReplace:
        Exec_FetchDecodeReplace(cpu, ins);
        break;
    case execLui:
        Exec_Lui(cpu, ins);
        break;
    case execAuipc:
        Exec_Auipc(cpu, ins);
        break;
    case execJal:
        Exec_Jal(cpu, ins);
        break;
    case execJalr:
        Exec_Jalr(cpu, ins);
        break;
    case execBeq:
        Exec_Beq(cpu, ins);
        break;
    case execBne:
        Exec_Bne(cpu, ins);
        break;
    case execBlt:
        Exec_Blt(cpu, ins);
        break;
    case execBge:
        Exec_Bge(cpu, ins);
        break;
    case execBltu:
        Exec_Bltu(cpu, ins);
        break;
    case execBgeu:
        Exec_Bgeu(cpu, ins);
        break;
    case execLb:
        Exec_Lb(cpu, ins);
        break;
    case execLh:
        Exec_Lh(cpu, ins);
        break;
    case execLw:
        Exec_Lw(cpu, ins);
        break;
    case execLbu:
        Exec_Lbu(cpu, ins);
        break;
    case execLhu:
        Exec_Lhu(cpu, ins);
        break;
    case execSb:
        Exec_Sb(cpu, ins);
        break;
    case execSh:
        Exec_Sh(cpu, ins);
        break;
    case execSw:
        Exec_Sw(cpu, ins);
        break;
    case execAddi:
        Exec_Addi(cpu, ins);
        break;
    case execSlti:
        Exec_Slti(cpu, ins);
        break;
    case execSltiu:
        Exec_Sltiu(cpu, ins);
        break;
    case execXori:
        Exec_Xori(cpu, ins);
        break;
    case execOri:
        Exec_Ori(cpu, ins);
        break;
    case execAndi:
        Exec_Andi(cpu, ins);
        break;
    case execSlli:
        Exec_Slli(cpu, ins);
        break;
    case execSrli:
        Exec_Srli(cpu, ins);
        break;
    case execSrai:
        Exec_Srai(cpu, ins);
        break;
    case execAdd:
        Exec_Add(cpu, ins);
        break;
    case execSub:
        Exec_Sub(cpu, ins);
        break;
    case execMul:
        Exec_Mul(cpu, ins);
        break;
    case execSll:
        Exec_Sll(cpu, ins);
        break;
    case execMulh:
        Exec_Mulh(cpu, ins);
        break;
    case execSlt:
        Exec_Slt(cpu, ins);
        break;
    case execMulhsu:
        Exec_Mulhsu(cpu, ins);
        break;
    case execSltu:
        Exec_Sltu(cpu, ins);
        break;
    case execMulhu:
        Exec_Mulhu(cpu, ins);
        break;
    case execXor:
        Exec_Xor(cpu, ins);
        break;
    case execDiv:
        Exec_Div(cpu, ins);
        break;
    case execSrl:
        Exec_Srl(cpu, ins);
        break;
    case execSra:
        Exec_Sra(cpu, ins);
        break;
    case execDivu:
        Exec_Divu(cpu, ins);
        break;
    case execOr:
        Exec_Or(cpu, ins);
        break;
    case execRem:
        Exec_Rem(cpu, ins);
        break;
    case execAnd:
        Exec_And(cpu, ins);
        break;
    case execRemu:
        Exec_Remu(cpu, ins);
        break;
    case execFence:
        Exec_Fence(cpu, ins);
        break;
    case execEcall:
        Exec_Ecall(cpu, ins);
        break;
    case execEbreak:
        Exec_Ebreak(cpu, ins);
        break;
    case execUret:
        Exec_Uret(cpu, ins);
        break;
    case execSret:
        Exec_Sret(cpu, ins);
        break;
    case execMret:
        Exec_Mret(cpu, ins);
        break;
    case execFlw:
        Exec_Flw(cpu, ins);
        break;
    case execFsw:
        Exec_Fsw(cpu, ins);
        break;
    case execFmadd_s:
        Exec_Fmadd_s(cpu, ins);
        break;
    case execFmsub_s:
        Exec_Fmsub_s(cpu, ins);
        break;
    case execFnmsub_s:
        Exec_Fnmsub_s(cpu, ins);
        break;
    case execFnmadd_s:
        Exec_Fnmadd_s(cpu, ins);
        break;
    case execFadd_s:
        Exec_Fadd_s(cpu, ins);
        break;
    case execFsub_s:
        Exec_Fsub_s(cpu, ins);
        break;
    case execFmul_s:
        Exec_Fmul_s(cpu, ins);
        break;
    case execFdiv_s:
        Exec_Fdiv_s(cpu, ins);
        break;
    case execFsqrt_s:
        Exec_Fsqrt_s(cpu, ins);
        break;
    case execFsgnj_s:
        Exec_Fsgnj_s(cpu, ins);
        break;
    case execFsgnjn_s:
        Exec_Fsgnjn_s(cpu, ins);
        break;
    case execFsgnjx_s:
        Exec_Fsgnjx_s(cpu, ins);
        break;
    case execFmin_s:
        Exec_Fmin_s(cpu, ins);
        break;
    case execFmax_s:
        Exec_Fmax_s(cpu, ins);
        break;
    case execFcvt_w_s:
        Exec_Fcvt_w_s(cpu, ins);
        break;
    case execFcvt_wu_s:
        Exec_Fcvt_wu_s(cpu, ins);
        break;
    case execFmv_x_w:
        Exec_Fmv_x_w(cpu, ins);
        break;
    case execFclass_s:
        Exec_Fclass_s(cpu, ins);
        break;
    case execFeq_s:
        Exec_Feq_s(cpu, ins);
        break;
    case execFlt_s:
        Exec_Flt_s(cpu, ins);
        break;
    case execFle_s:
        Exec_Fle_s(cpu, ins);
        break;
    case execFcvt_s_w:
        Exec_Fcvt_s_w(cpu, ins);
        break;
    case execFcvt_s_wu:
        Exec_Fcvt_s_wu(cpu, ins);
        break;
    case execFmv_w_x:
        Exec_Fmv_w_x(cpu, ins);
        break;
    case execIllegalInstruction:
    default:
        Exec_IllegalInstruction(cpu, ins);
        break;
    }
}

ArvissResult ArvissRun(ArvissCpu* cpu, int count)
{
    cpu->result = ArvissMakeOk();
    int retired = 0;
    for (; retired < count; retired++)
    {
        DecodedInstruction* decoded = FetchFromCache(cpu); // Fetch a decoded instruction from the decoded instruction cache.
        RunOne(cpu, decoded);

        if (ArvissResultIsTrap(cpu->result))
        {
            // Stop, as we can no longer proceeed.
            cpu->busCode = bcOK; // Reset any memory fault.
            break;
        }
    }
    cpu->retired = retired;
    return cpu->result;
}

ArvissResult ArvissExecute(ArvissCpu* cpu, uint32_t instruction)
{
    DecodedInstruction decoded = ArvissDecode(instruction);
    RunOne(cpu, &decoded);
    return cpu->result;
}

void ArvissMret(ArvissCpu* cpu)
{
    Exec_Mret(cpu, NULL);
}

void ArvissReset(ArvissCpu* cpu)
{
    cpu->result = ArvissMakeOk();
    cpu->busCode = bcOK;
    cpu->pc = 0;
    for (int i = 0; i < 32; i++)
    {
        cpu->xreg[i] = 0;
    }
    for (int i = 0; i < 32; i++)
    {
        cpu->freg[i] = 0;
    }
    cpu->mepc = 0;
    cpu->mcause = 0;
    cpu->mtval = 0;

    // Invalidate the decoded instruction cache.
    for (int i = 0; i < CACHE_LINES; i++)
    {
        cpu->cache.line[i].isValid = false;
    }
}

#ifdef __cplusplus
}
#endif

#endif
