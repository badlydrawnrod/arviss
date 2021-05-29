#include "arviss.h"
#include "conversions.h"
#include "opcodes.h"
#include "rounding_modes.h"
#include "smallmem.h"

#include "gtest/gtest.h"
#include <math.h>

class TestDecoder : public ::testing::Test
{
protected:
    void SetUp() override;

    static uint32_t EncodeRd(uint32_t n);
    static uint32_t EncodeRs1(uint32_t n);
    static uint32_t EncodeRs2(uint32_t n);
    static uint32_t EncodeRs3(uint32_t n);
    static uint32_t EncodeRm(uint32_t n);
    static uint32_t EncodeJ(uint32_t n);
    static uint32_t EncodeB(uint32_t n);
    static uint32_t EncodeS(uint32_t n);
    static uint32_t EncodeI(uint32_t n);

    static constexpr uint32_t rambase = 0x1000; // Deliberately not zero.
    static constexpr uint32_t ramsize = 0x1000; // Deliberately small to keep offsets from getting out of range.

    ArvissCpu cpu;

    inline static ArvissMemory memory;
};

void TestDecoder::SetUp()
{
    // Clear the RAM.
    for (auto& b : memory.ram)
    {
        b = 0;
    }

    // Reset the CPU.
    ArvissReset(&cpu, rambase + ramsize);
    cpu.memory = SmallMemInit(&memory);
    cpu.pc = rambase;
}

uint32_t TestDecoder::EncodeRd(uint32_t n)
{
    return n << 7;
}

uint32_t TestDecoder::EncodeRs1(uint32_t n)
{
    return n << 15;
}

uint32_t TestDecoder::EncodeRs2(uint32_t n)
{
    return n << 20;
}

uint32_t TestDecoder::EncodeRs3(uint32_t n)
{
    return n << 27;
}

uint32_t TestDecoder::EncodeRm(uint32_t n)
{
    return n << 12;
}

uint32_t TestDecoder::EncodeJ(uint32_t n)
{
    return ((n & 0x100000) << 11) // imm[20]    -> j[31]
            | ((n & 0x7fe) << 20) // imm[10:1]  -> j[30:21]
            | ((n & 0x800) << 9)  // imm[11]    -> j[20]
            | (n & 0x000ff000)    // imm[19:12] -> j[19:12]
            ;
}

uint32_t TestDecoder::EncodeB(uint32_t n)
{
    return ((n & 0x1000) << 19)   // imm[12]   -> b[31]
            | ((n & 0x7e0) << 20) // imm[10:5] -> b[30:25]
            | ((n & 0x1e) << 7)   // imm[4:1]  -> b[11:4]
            | ((n & 0x800) >> 4)  // imm[11]   -> b[7]
            ;
}

uint32_t TestDecoder::EncodeS(uint32_t n)
{
    return ((n & 0xfe0) << 20)  // imm[11:5] -> s[31:25]
            | ((n & 0x1f) << 7) // imm[4:0]  -> s[11:7]
            ;
}

uint32_t TestDecoder::EncodeI(uint32_t n)
{
    return (n & 0xfff) << 20; // imm[11:0] -> s[31:20]
}

TEST_F(TestDecoder, Lui)
{
    // rd <- imm_u, pc <- pc + 4
    for (int32_t v : {0, 1, -1, 1234, -1234, -(1 << 19), (1 << 19) - 1})
    {
        int32_t imm_u = v;
        uint32_t rd = 2;
        uint32_t pc = cpu.pc;

        ArvissExecute(&cpu, (imm_u << 12) | EncodeRd(rd) | OP_LUI);

        // rd <- imm_u
        ASSERT_EQ(imm_u, (int32_t)cpu.xreg[rd] >> 12);

        // pc <- pc + 4
        ASSERT_EQ(pc + 4, cpu.pc);
    }
}

TEST_F(TestDecoder, Lui_x0_Is_Zero)
{
    // x0 is unchanged when it's the target of a LUI.
    int32_t imm_u = 123;

    ArvissExecute(&cpu, (imm_u << 12) | EncodeRd(0) | OP_LUI);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);
}

TEST_F(TestDecoder, Auipc)
{
    // rd <- pc + imm_u, pc <- pc + 4
    for (int32_t v : {0, 1, -1, 1234, -1234, -(1 << 19), (1 << 19) - 1})
    {
        int32_t imm_u = v;
        uint32_t rd = 9;
        uint32_t pc = cpu.pc;

        ArvissExecute(&cpu, (imm_u << 12) | EncodeRd(rd) | OP_AUIPC);

        // rd <- pc + imm_u
        uint32_t expected = pc + (imm_u << 12);
        ASSERT_EQ(expected, cpu.xreg[rd]);

        // pc <- pc + 4
        ASSERT_EQ(pc + 4, cpu.pc);
    }
}

TEST_F(TestDecoder, Auipc_x0_Is_Zero)
{
    // x0 is unchanged when it's the target of an AUIPC.
    int32_t imm_u = 123;

    ArvissExecute(&cpu, (imm_u << 12) | EncodeRd(0) | OP_AUIPC);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);
}

TEST_F(TestDecoder, Jal)
{
    // rd <- pc + 4, pc <- pc + imm_j
    for (int32_t v : {0, -2, 2, -(1 << 20), (1 << 20) - 2})
    {
        uint32_t pc = cpu.pc;
        int32_t imm_j = v;
        uint32_t rd = 3;

        ArvissExecute(&cpu, (EncodeJ(imm_j) | EncodeRd(rd)) | OP_JAL);

        // rd <- pc + 4
        ASSERT_EQ(pc + 4, cpu.xreg[rd]);

        // pc <- pc + imm_j
        ASSERT_EQ(pc + v, cpu.pc);
    }
}

TEST_F(TestDecoder, Jal_x0_Is_Zero)
{
    ArvissExecute(&cpu, (EncodeJ(123) | EncodeRd(0)) | OP_JAL);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);
}

TEST_F(TestDecoder, Jalr)
{
    // rd <- pc + 4, pc <- (rs1 + imm_i) & ~1
    for (int32_t v : {0, -1, 1, -(1 << 1), (1 << 11) - 1})
    {
        cpu.pc = 0x1000;

        uint32_t pc = cpu.pc;
        int32_t imm_i = v;
        uint32_t rs1 = 10;
        uint32_t rd = 10;
        uint32_t rs1Before = 12345;
        cpu.xreg[rs1] = rs1Before;

        uint32_t ins = EncodeI(imm_i) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_JALR;
        ArvissExecute(&cpu, ins);

        // rd <- pc + 4
        ASSERT_EQ(pc + 4, cpu.xreg[rd]);

        // pc <- (rs1 + imm_i) & ~1
        ASSERT_EQ((rs1Before + imm_i) & ~1, cpu.pc);
    }
}

TEST_F(TestDecoder, Jalr_x0_Is_Zero)
{
    ArvissExecute(&cpu, EncodeI(123) | EncodeRs1(1) | (0b000 << 12) | EncodeRd(0) | OP_JALR);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);
}

TEST_F(TestDecoder, Branch_Beq)
{
    // pc <- pc + ((rs1 == rs2) ? imm_b : 4)
    uint32_t pc = cpu.pc;
    int32_t imm_b = 1234;
    uint32_t rs1 = 19;
    uint32_t rs2 = 27;

    // Branch taken.
    cpu.xreg[rs1] = 5678;
    cpu.xreg[rs2] = cpu.xreg[rs1];
    ArvissExecute(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b000 << 12) | OP_BRANCH);

    // pc <- pc + imm_b
    ASSERT_EQ(pc + imm_b, cpu.pc);

    // Branch not taken.
    pc = cpu.pc;
    cpu.xreg[rs1] = 5678;
    cpu.xreg[rs2] = 8765;
    ArvissExecute(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b000 << 12) | OP_BRANCH);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Branch_Bne)
{
    // pc <- pc + ((rs1 != rs2) ? imm_b : 4)
    uint32_t pc = cpu.pc;
    int32_t imm_b = 1234;
    uint32_t rs1 = 19;
    uint32_t rs2 = 27;

    // Branch taken.
    cpu.xreg[rs1] = 5678;
    cpu.xreg[rs2] = 8765;
    ArvissExecute(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b001 << 12) | OP_BRANCH);

    // pc <- pc + imm_b
    ASSERT_EQ(pc + imm_b, cpu.pc);

    // Branch not taken.
    pc = cpu.pc;
    cpu.xreg[rs2] = cpu.xreg[rs1];
    ArvissExecute(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b001 << 12) | OP_BRANCH);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Branch_Blt)
{
    // pc <- pc + ((rs1 < rs2) ? imm_b : 4)
    uint32_t pc = cpu.pc;
    int32_t imm_b = 1234;
    uint32_t rs1 = 19;
    uint32_t rs2 = 27;

    // Branch taken.
    cpu.xreg[rs1] = -1;
    cpu.xreg[rs2] = 0;
    ArvissExecute(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b100 << 12) | OP_BRANCH);

    // pc <- pc + imm_b
    ASSERT_EQ(pc + imm_b, cpu.pc);

    // Branch not taken.
    pc = cpu.pc;
    cpu.xreg[rs1] = 456;
    cpu.xreg[rs2] = 123;
    ArvissExecute(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b100 << 12) | OP_BRANCH);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Branch_Bge)
{
    // pc <- pc + ((rs1 >= rs2) ? imm_b : 4)
    uint32_t pc = cpu.pc;
    int32_t imm_b = 1234;
    uint32_t rs1 = 19;
    uint32_t rs2 = 27;

    // Branch taken (greater)
    cpu.xreg[rs1] = 0;
    cpu.xreg[rs2] = -1;
    ArvissExecute(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b101 << 12) | OP_BRANCH);

    // pc <- pc + imm_b
    ASSERT_EQ(pc + imm_b, cpu.pc);

    // Branch taken (equal)
    pc = cpu.pc;
    cpu.xreg[rs1] = -1;
    cpu.xreg[rs2] = -1;
    ArvissExecute(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b101 << 12) | OP_BRANCH);

    // pc <- pc + imm_b
    ASSERT_EQ(pc + imm_b, cpu.pc);

    // Branch not taken.
    pc = cpu.pc;
    cpu.xreg[rs1] = -1;
    cpu.xreg[rs2] = 0;
    ArvissExecute(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b101 << 12) | OP_BRANCH);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Branch_Bltu)
{
    // pc <- pc + ((rs1 < rs2) ? imm_b : 4)
    uint32_t pc = cpu.pc;
    int32_t imm_b = 1234;
    uint32_t rs1 = 19;
    uint32_t rs2 = 27;

    // Branch taken.
    cpu.xreg[rs1] = 0;
    cpu.xreg[rs2] = 0xffffffff;
    ArvissExecute(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b110 << 12) | OP_BRANCH);

    // pc <- pc + imm_b
    ASSERT_EQ(pc + imm_b, cpu.pc);

    // Branch not taken.
    pc = cpu.pc;
    cpu.xreg[rs1] = 0xffffffff;
    cpu.xreg[rs2] = 0;
    ArvissExecute(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b110 << 12) | OP_BRANCH);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Branch_Bgeu)
{
    // pc <- pc + ((rs1 >= rs2) ? imm_b : 4)
    uint32_t pc = cpu.pc;
    int32_t imm_b = 1234;
    uint32_t rs1 = 19;
    uint32_t rs2 = 27;

    // Branch taken (greater)
    cpu.xreg[rs1] = 0xffffffff;
    cpu.xreg[rs2] = 0;
    ArvissExecute(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b111 << 12) | OP_BRANCH);

    // pc <- pc + imm_b
    ASSERT_EQ(pc + imm_b, cpu.pc);

    // Branch taken (equal)
    pc = cpu.pc;
    cpu.xreg[rs1] = 1;
    cpu.xreg[rs2] = 1;
    ArvissExecute(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b111 << 12) | OP_BRANCH);

    // pc <- pc + imm_b
    ASSERT_EQ(pc + imm_b, cpu.pc);

    // Branch not taken.
    pc = cpu.pc;
    cpu.xreg[rs1] = 0;
    cpu.xreg[rs2] = 0xffffffff;
    ArvissExecute(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b111 << 12) | OP_BRANCH);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Load_Lb)
{
    // rd <- sx(m8(rs1 + imm_i)), pc += 4
    uint32_t pc = cpu.pc;
    int32_t imm_i = 23;
    uint32_t rd = 31;
    uint32_t rs1 = 13;
    cpu.xreg[rs1] = rambase;

    // Sign extend when bit 7 is zero.
    ArvissWriteByte(cpu.memory, cpu.xreg[rs1] + imm_i, 123);
    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_LOAD);

    // rd <- sx(m8(rs1 + imm_i))
    ASSERT_EQ(123, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Sign extend when bit 7 is one.
    pc = cpu.pc;
    ArvissWriteByte(cpu.memory, cpu.xreg[rs1] + imm_i, 0xff);
    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_LOAD);

    // rd <- sx(m8(rs1 + imm_i))
    ASSERT_EQ(-1, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Load_Lh)
{
    // rd <- sx(m16(rs1 + imm_i)), pc += 4
    uint32_t pc = cpu.pc;
    int32_t imm_i = 2000;
    uint32_t rd = 31;
    uint32_t rs1 = 6;
    cpu.xreg[rs1] = rambase;

    // Sign extend when bit 15 is zero.
    ArvissWriteHalfword(cpu.memory, cpu.xreg[rs1] + imm_i, 0x7fff);
    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_LOAD);

    // rd <- sx(m16(rs1 + imm_i))
    ASSERT_EQ(0x7fff, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Sign extend when bit 15 is one.
    pc = cpu.pc;
    ArvissWriteHalfword(cpu.memory, cpu.xreg[rs1] + imm_i, 0xffff);
    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_LOAD);

    // rd <- sx(m16(rs1 + imm_i))
    ASSERT_EQ(-1, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Load_Lw)
{
    // rd <- sx(m32(rs1 + imm_i)), pc += 4
    uint32_t pc = cpu.pc;
    int32_t imm_i = 274;
    uint32_t rd = 14;
    uint32_t rs1 = 15;
    cpu.xreg[rs1] = rambase;

    // Sign extend when bit 31 is zero.
    ArvissWriteWord(cpu.memory, cpu.xreg[rs1] + imm_i, 0x7fffffff);
    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_LOAD);

    // rd <- sx(m32(rs1 + imm_i))
    ASSERT_EQ(0x7fffffff, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Sign extend when bit 31 is one.
    pc = cpu.pc;
    ArvissWriteWord(cpu.memory, cpu.xreg[rs1] + imm_i, 0xffffffff);
    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_LOAD);

    // rd <- sx(m32(rs1 + imm_i))
    ASSERT_EQ(-1, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Load_Lbu)
{
    // rd <- zx(m8(rs1 + imm_i)), pc += 4
    uint32_t pc = cpu.pc;
    int32_t imm_i = -5;
    uint32_t rd = 23;
    uint32_t rs1 = 18;
    cpu.xreg[rs1] = rambase + ramsize / 2;

    // Zero extend when bit 7 is zero.
    ArvissWriteByte(cpu.memory, cpu.xreg[rs1] + imm_i, 123);
    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b100 << 12) | EncodeRd(rd) | OP_LOAD);

    // rd <- zx(m8(rs1 + imm_i))
    ASSERT_EQ(123, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Zero extend when bit 7 is zero.
    pc = cpu.pc;
    ArvissWriteByte(cpu.memory, cpu.xreg[rs1] + imm_i, 0xff);
    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b100 << 12) | EncodeRd(rd) | OP_LOAD);

    // rd <- zx(m8(rs1 + imm_i))
    ASSERT_EQ(0xff, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Load_Lhu)
{
    // rd <- zx(m16(rs1 + imm_i)), pc += 4
    uint32_t pc = cpu.pc;
    int32_t imm_i = -((int32_t)ramsize / 4);
    uint32_t rd = 13;
    uint32_t rs1 = 16;
    cpu.xreg[rs1] = rambase + ramsize / 2;

    // Zero extend when bit 15 is zero.
    ArvissWriteHalfword(cpu.memory, cpu.xreg[rs1] + imm_i, 0x7fff);
    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b101 << 12) | EncodeRd(rd) | OP_LOAD);

    // rd <- zx(m16(rs1 + imm_i))
    ASSERT_EQ(0x7fff, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Zero extend when bit 15 is one.
    pc = cpu.pc;
    ArvissWriteHalfword(cpu.memory, cpu.xreg[rs1] + imm_i, 0xffff);
    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b101 << 12) | EncodeRd(rd) | OP_LOAD);

    // rd <- zx(m16(rs1 + imm_i))
    ASSERT_EQ(0xffff, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Load_x0_Is_Zero)
{
    ArvissWriteWord(cpu.memory, rambase, 0x12345678);

    // LB
    uint32_t rs1 = 13;
    cpu.xreg[rs1] = rambase;
    ArvissWriteByte(cpu.memory, rambase, 0xff);
    ArvissExecute(&cpu, EncodeI(0) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(0) | OP_LOAD);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // LH
    ArvissExecute(&cpu, EncodeI(0) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(0) | OP_LOAD);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // LW
    ArvissExecute(&cpu, EncodeI(0) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(0) | OP_LOAD);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // LBU
    ArvissExecute(&cpu, EncodeI(0) | EncodeRs1(rs1) | (0b100 << 12) | EncodeRd(0) | OP_LOAD);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // LHU
    ArvissExecute(&cpu, EncodeI(0) | EncodeRs1(rs1) | (0b101 << 12) | EncodeRd(0) | OP_LOAD);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);
}

TEST_F(TestDecoder, Store_Sb)
{
    // m8(rs1 + imm_s) <- rs2[7:0], pc += 4
    uint32_t pc = cpu.pc;
    int32_t imm_s = -123;
    uint32_t rs1 = 12;
    uint32_t rs2 = 3;
    cpu.xreg[rs1] = rambase + ramsize / 2;

    ArvissExecute(&cpu, EncodeS(imm_s) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b000 << 12) | OP_STORE);

    // m8(rs1 + imm_s) <- rs2[7:0]
    ArvissResult byteResult = ::ArvissReadByte(cpu.memory, cpu.xreg[rs1] + imm_s);
    ASSERT_TRUE(ArvissResultIsByte(byteResult));
    ASSERT_EQ(ArvissResultAsByte(byteResult), cpu.xreg[rs2] & 0xff);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Store_Sh)
{
    // m16(rs1 + imm_s) <- rs2[15:0], pc += 4
    uint32_t pc = cpu.pc;
    int32_t imm_s = 222;
    uint32_t rs1 = 2;
    uint32_t rs2 = 29;
    cpu.xreg[rs1] = rambase + ramsize / 2;

    ArvissExecute(&cpu, EncodeS(imm_s) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b001 << 12) | OP_STORE);

    // m16(rs1 + imm_s) <- rs2[15:0]
    ArvissResult halfwordResult = ::ArvissReadHalfword(cpu.memory, cpu.xreg[rs1] + imm_s);
    ASSERT_TRUE(ArvissResultIsHalfword(halfwordResult));
    ASSERT_EQ(ArvissResultAsHalfword(halfwordResult), cpu.xreg[rs2] & 0xffff);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Store_Sw)
{
    // m32(rs1 + imm_s) <- rs2[31:0], pc += 4
    uint32_t pc = cpu.pc;
    int32_t imm_s = 222;
    uint32_t rs1 = 2;
    uint32_t rs2 = 29;
    cpu.xreg[rs1] = rambase + ramsize / 2;

    ArvissExecute(&cpu, EncodeS(imm_s) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b010 << 12) | OP_STORE);

    // m32(rs1 + imm_s) <- rs2[31:0]
    ArvissResult wordResult = ::ArvissReadWord(cpu.memory, cpu.xreg[rs1] + imm_s);
    ASSERT_TRUE(ArvissResultIsWord(wordResult));
    ASSERT_EQ(ArvissResultAsWord(wordResult), cpu.xreg[rs2] & 0xffff);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpImm_Addi)
{
    // rd <- rs1 + imm_i, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 15;
    uint32_t rs1 = 12;
    int32_t imm_i = 64;
    cpu.xreg[rs1] = 128;

    // Add immediate.
    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_OPIMM);

    // rd <- rs1 + imm_i
    ASSERT_EQ(cpu.xreg[rs1] + imm_i, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Add negative number.
    pc = cpu.pc;
    imm_i = -123;
    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_OPIMM);

    // rd <- rs1 + imm_i
    ASSERT_EQ(cpu.xreg[rs1] + imm_i, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpImm_Slti)
{
    // rd <- (rs1 < imm_i) ? 1 : 0, pc += 4
    uint32_t pc = cpu.pc;

    int32_t imm_i = 0;
    uint32_t rd = 19;
    uint32_t rs1 = 27;

    // Condition true.
    cpu.xreg[rs1] = -1;
    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_OPIMM);

    // rd <- (rs1 < imm_i) ? 1 : 0
    ASSERT_EQ(1, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Condition false.
    pc = cpu.pc;
    cpu.xreg[rs1] = 123;
    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_OPIMM);

    // rd <- (rs1 < imm_i) ? 1 : 0
    ASSERT_EQ(0, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpImm_Sltiu)
{
    // rd <- (rs1 < imm_i) ? 1 : 0, pc += 4
    uint32_t pc = cpu.pc;

    int32_t imm_i = 0xffff;
    uint32_t rd = 9;
    uint32_t rs1 = 1;

    // Condition true.
    cpu.xreg[rs1] = 0;
    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b011 << 12) | EncodeRd(rd) | OP_OPIMM);

    // rd <- (rs1 < imm_i) ? 1 : 0
    ASSERT_EQ(1, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Condition false.
    pc = cpu.pc;
    cpu.xreg[rs1] = 0xffffffff;
    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b011 << 12) | EncodeRd(rd) | OP_OPIMM);

    // rd <- (rs1 < imm_i) ? 1 : 0
    ASSERT_EQ(0, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpImm_Xori)
{
    // rd <- rs1 ^ imm_i, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 5;
    uint32_t rs1 = 3;
    int32_t imm_i = -1;
    cpu.xreg[rs1] = 123456;

    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b100 << 12) | EncodeRd(rd) | OP_OPIMM);

    // rd <- rs1 ^ imm_i
    ASSERT_EQ(cpu.xreg[rs1] ^ imm_i, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpImm_Ori)
{
    // rd <- rs1 | imm_i, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 25;
    uint32_t rs1 = 13;
    int32_t imm_i = 0x00ff;
    cpu.xreg[rs1] = 0xff00;

    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b110 << 12) | EncodeRd(rd) | OP_OPIMM);

    // rd <- rs1 | imm_i
    ASSERT_EQ(cpu.xreg[rs1] | imm_i, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpImm_Andi)
{
    // rd <- rs1 & imm_i, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 5;
    uint32_t rs1 = 3;
    int32_t imm_i = 0xfff0;
    cpu.xreg[rs1] = 0xffff;

    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b111 << 12) | EncodeRd(rd) | OP_OPIMM);

    // rd <- rs1 & imm_i
    ASSERT_EQ(cpu.xreg[rs1] & imm_i, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpImm_Slli)
{
    // rd <- rs1 << shamt_i, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 5;
    uint32_t rs1 = 3;
    int32_t shamt = 4;
    cpu.xreg[rs1] = 0x0010;

    ArvissExecute(&cpu, EncodeI(shamt) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_OPIMM);

    // rd <- rs1 << shamt_i
    ASSERT_EQ(cpu.xreg[rs1] << shamt, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpImm_Srli)
{
    // rd <- rs1 >> shamt_i, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 15;
    uint32_t rs1 = 23;
    int32_t shamt = 4;
    cpu.xreg[rs1] = 0x1000;

    ArvissExecute(&cpu, EncodeI(shamt) | EncodeRs1(rs1) | (0b101 << 12) | EncodeRd(rd) | OP_OPIMM);

    // rd <- rs1 >> shamt_i
    ASSERT_EQ(cpu.xreg[rs1] >> shamt, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpImm_Srai)
{
    // rd <- rs1 >> shamt_i, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 15;
    uint32_t rs1 = 23;
    int32_t shamt = 4;
    cpu.xreg[rs1] = 0x80000000;

    ArvissExecute(&cpu, (1 << 30) | EncodeI(shamt) | EncodeRs1(rs1) | (0b101 << 12) | EncodeRd(rd) | OP_OPIMM);

    // rd <- sx(rs1) >> shamt_i
    ASSERT_EQ(((int32_t)cpu.xreg[rs1]) >> shamt, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpImm_x0_Is_Zero)
{
    // ADDI
    ArvissExecute(&cpu, EncodeI(123) | EncodeRs1(1) | (0b000 << 12) | EncodeRd(0) | OP_OPIMM);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SLTI
    ArvissExecute(&cpu, EncodeI(123) | EncodeRs1(1) | (0b010 << 12) | EncodeRd(0) | OP_OPIMM);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SLTIU
    ArvissExecute(&cpu, EncodeI(123) | EncodeRs1(1) | (0b011 << 12) | EncodeRd(0) | OP_OPIMM);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // XORI
    ArvissExecute(&cpu, EncodeI(123) | EncodeRs1(1) | (0b100 << 12) | EncodeRd(0) | OP_OPIMM);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // ORI
    ArvissExecute(&cpu, EncodeI(123) | EncodeRs1(1) | (0b110 << 12) | EncodeRd(0) | OP_OPIMM);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // ANDI
    cpu.xreg[1] = 0xffffffff;
    ArvissExecute(&cpu, EncodeI(123) | EncodeRs1(1) | (0b111 << 12) | EncodeRd(0) | OP_OPIMM);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SLLI
    cpu.xreg[1] = 0xffffffff;
    ArvissExecute(&cpu, EncodeI(0xff) | EncodeRs1(1) | (0b001 << 12) | EncodeRd(0) | OP_OPIMM);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SRLI
    cpu.xreg[1] = 0xffffffff;
    ArvissExecute(&cpu, EncodeI(3) | EncodeRs1(1) | (0b101 << 12) | EncodeRd(0) | OP_OPIMM);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SRAI
    cpu.xreg[1] = 0xffffffff;
    ArvissExecute(&cpu, (1 << 30) | EncodeI(3) | EncodeRs1(1) | (0b101 << 12) | EncodeRd(0) | OP_OPIMM);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);
}

TEST_F(TestDecoder, Op_Add)
{
    // rd <- rs1 + rs2, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 1;
    uint32_t rs1 = 2;
    uint32_t rs2 = 3;
    cpu.xreg[rs1] = 128;
    cpu.xreg[rs2] = 64;

    ArvissExecute(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- rs1 + rs2
    ASSERT_EQ(cpu.xreg[rs1] + cpu.xreg[rs2], cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Op_Sub)
{
    // rd <- rs1 - rs2, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 1;
    uint32_t rs1 = 2;
    uint32_t rs2 = 3;
    cpu.xreg[rs1] = 192;
    cpu.xreg[rs2] = 64;

    ArvissExecute(&cpu, (0b0100000 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- rs1 - rs2
    ASSERT_EQ(cpu.xreg[rs1] - cpu.xreg[rs2], cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Op_Mul) // RV32M
{
    // MUL performs a 32-bit x 32-bit multiplication of rs1 by rs2 and places the lower 32 bits in the destination register.

    // rd <- lower32(rs1 * rs2), pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 5;
    uint32_t rs1 = 13;
    uint32_t rs2 = 14;
    cpu.xreg[rs1] = 333;
    cpu.xreg[rs2] = 3;
    int32_t expected = 999;

    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- lower32(rs1 * rs2)
    ASSERT_EQ(expected, (int32_t)cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // x0 is immutable.
    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs1) | EncodeRs1(rs2) | (0b000 << 12) | EncodeRd(0) | OP_OP);
    ASSERT_EQ(0, cpu.xreg[0]);
}

TEST_F(TestDecoder, Op_Sll)
{
    // rd <- rs1 << (rs2 % XLEN), pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 1;
    uint32_t rs1 = 2;
    uint32_t rs2 = 3;
    cpu.xreg[rs1] = 1;
    cpu.xreg[rs2] = 10;

    ArvissExecute(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- rs1 << (rs2 % XLEN)
    ASSERT_EQ(cpu.xreg[rs1] << (cpu.xreg[rs2] % 32), cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Op_Mulh) // RV32M
{
    // MULH performs a 32-bit x 32-bit (signed x signed) multiplication of rs1 by rs2 and places the upper 32 bits of the 64 bit
    // product in the destination register.

    // rd <- upper32(rs1 * rs2), pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 5;
    uint32_t rs1 = 13;
    uint32_t rs2 = 14;
    cpu.xreg[rs1] = 16777216;  // 2 ** 24
    cpu.xreg[rs2] = -16777216; // -(2 ** 24)

    int64_t product = (int64_t)(int32_t)cpu.xreg[rs1] * (int64_t)(int32_t)cpu.xreg[rs2];
    int32_t expected = product >> 32;

    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- upper32(rs1 * rs2)
    ASSERT_EQ(expected, (int32_t)cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // x0 is immutable.
    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs1) | EncodeRs1(rs2) | (0b001 << 12) | EncodeRd(0) | OP_OP);
    ASSERT_EQ(0, cpu.xreg[0]);
}

TEST_F(TestDecoder, Op_Slt)
{
    // rd <- (rs1 < rs2) ? 1 : 0, pc += 4
    uint32_t pc = cpu.pc;

    uint32_t rd = 19;
    uint32_t rs1 = 7;
    uint32_t rs2 = 4;
    cpu.xreg[rs2] = 0;

    // Condition true.
    cpu.xreg[rs1] = -1;
    ArvissExecute(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- (rs1 < imm_i) ? 1 : 0
    ASSERT_EQ(1, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Condition false.
    pc = cpu.pc;
    cpu.xreg[rs1] = 123;
    ArvissExecute(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- (rs1 < imm_i) ? 1 : 0
    ASSERT_EQ(0, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Op_Mulhsu) // RV32M
{
    // MULHSU performs a 32-bit x 32-bit (signed x unsigned) multiplication of rs1 by rs2 and places the upper 32 bits of the 64 bit
    // product in the destination register.

    // rd <- upper32(rs1 * rs2), pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 5;
    uint32_t rs1 = 13;
    uint32_t rs2 = 14;
    cpu.xreg[rs1] = 16777216;   // 2 ** 24
    cpu.xreg[rs2] = 0xffffc000; // -16384 signed, 4294950912 unsigned

    int64_t product = (int64_t)(int32_t)cpu.xreg[rs1] * (uint64_t)cpu.xreg[rs2];
    int32_t expected = product >> 32;

    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- upper32(rs1 * rs2)
    ASSERT_EQ(expected, (int32_t)cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // x0 is immutable.
    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs1) | EncodeRs1(rs2) | (0b010 << 12) | EncodeRd(0) | OP_OP);
    ASSERT_EQ(0, cpu.xreg[0]);
}

TEST_F(TestDecoder, Op_Sltu)
{
    // rd <- (rs1 < rs2) ? 1 : 0, pc += 4
    uint32_t pc = cpu.pc;

    uint32_t rd = 19;
    uint32_t rs1 = 7;
    uint32_t rs2 = 4;
    cpu.xreg[rs2] = 0xffffffff;

    // Condition true.
    cpu.xreg[rs1] = 0;
    ArvissExecute(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b011 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- (rs1 < imm_i) ? 1 : 0
    ASSERT_EQ(1, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Condition false.
    pc = cpu.pc;
    cpu.xreg[rs1] = 0xffffffff;
    ArvissExecute(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b011 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- (rs1 < imm_i) ? 1 : 0
    ASSERT_EQ(0, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Op_Mulhu) // RV32M
{
    // MULHSU performs a 32-bit x 32-bit (signed x unsigned) multiplication of rs1 by rs2 and places the upper 32 bits of the 64 bit
    // product in the destination register.

    // rd <- upper32(rs1 * rs2), pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 5;
    uint32_t rs1 = 13;
    uint32_t rs2 = 14;
    cpu.xreg[rs1] = 0xffffc000; // 4294950912 unsigned
    cpu.xreg[rs2] = 0xffffc000; // 4294950912 unsigned

    uint64_t product = (uint64_t)cpu.xreg[rs1] * (uint64_t)cpu.xreg[rs2];
    uint32_t expected = product >> 32;

    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b011 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- upper32(rs1 * rs2)
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // x0 is immutable.
    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs1) | EncodeRs1(rs2) | (0b011 << 12) | EncodeRd(0) | OP_OP);
    ASSERT_EQ(0, cpu.xreg[0]);
}

TEST_F(TestDecoder, Op_Xor)
{
    // rd <- rs1 ^ rs2, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 9;
    uint32_t rs1 = 10;
    uint32_t rs2 = 11;
    cpu.xreg[rs1] = 0xff;
    cpu.xreg[rs2] = 0xfe;

    ArvissExecute(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b100 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- rs1 ^ rs2
    ASSERT_EQ(cpu.xreg[rs1] ^ cpu.xreg[rs2], cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Op_Div) // RV32M
{
    // DIV performs a 32-bit x 32-bit (signed / signed) integer division of rs1 by rs2, rounding towards zero.

    // rd <- rs / rs2, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 5;
    uint32_t rs1 = 13;
    uint32_t rs2 = 14;
    cpu.xreg[rs1] = 262144;
    cpu.xreg[rs2] = -1024;

    int32_t expected = (int32_t)cpu.xreg[rs1] / (int32_t)cpu.xreg[rs2];

    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b100 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- rs1 / rs2
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Division by zero set the result to -1.
    cpu.xreg[rs2] = 0;
    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b100 << 12) | EncodeRd(rd) | OP_OP);
    ASSERT_EQ(-1, (int32_t)cpu.xreg[rd]);

    // Division of the most negative integer by -1 results in overflow.
    cpu.xreg[rs1] = 0x80000000;
    cpu.xreg[rs2] = -1;
    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b100 << 12) | EncodeRd(rd) | OP_OP);
    ASSERT_EQ(0x80000000, cpu.xreg[rd]);

    // x0 is immutable.
    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs1) | EncodeRs1(rs2) | (0b100 << 12) | EncodeRd(0) | OP_OP);
    ASSERT_EQ(0, cpu.xreg[0]);
}

TEST_F(TestDecoder, Op_Srl)
{
    // rd <- rs1 >> (rs2 % XLEN), pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 9;
    uint32_t rs1 = 10;
    uint32_t rs2 = 11;
    cpu.xreg[rs1] = 0x80000000;
    cpu.xreg[rs2] = 4;

    ArvissExecute(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b101 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- rs1 >> (rs2 % XLEN)
    ASSERT_EQ(cpu.xreg[rs1] >> (cpu.xreg[rs2] % 32), cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Op_Sra)
{
    // rd <- rs1 >> (rs2 % XLEN), pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 10;
    uint32_t rs1 = 11;
    uint32_t rs2 = 12;
    cpu.xreg[rs1] = 0x80000000;
    cpu.xreg[rs2] = 4;

    ArvissExecute(&cpu, (0b0100000 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b101 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- sx(rs1) >> (rs2 % XLEN)
    ASSERT_EQ((int32_t)cpu.xreg[rs1] >> (cpu.xreg[rs2] % 32), cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Op_Divu) // RV32M
{
    // DIVU performs a 32-bit x 32-bit (unsigned / unsigned) integer division of rs1 by rs2, rounding towards zero.

    // rd <- rs / rs2, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 5;
    uint32_t rs1 = 13;
    uint32_t rs2 = 14;
    cpu.xreg[rs1] = 262144;
    cpu.xreg[rs2] = 1024;

    uint32_t expected = cpu.xreg[rs1] / cpu.xreg[rs2];

    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b101 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- rs1 / rs2
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Division by zero set the result to 0xffffffff.
    cpu.xreg[rs2] = 0;
    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b101 << 12) | EncodeRd(rd) | OP_OP);
    ASSERT_EQ(0xffffffff, (int32_t)cpu.xreg[rd]);

    // x0 is immutable.
    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs1) | EncodeRs1(rs2) | (0b101 << 12) | EncodeRd(0) | OP_OP);
    ASSERT_EQ(0, cpu.xreg[0]);
}

TEST_F(TestDecoder, Op_Or)
{
    // rd <- rs1 | rs2, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 11;
    uint32_t rs1 = 12;
    uint32_t rs2 = 13;
    cpu.xreg[rs1] = 0x00ff00ff;
    cpu.xreg[rs2] = 0xff00ffff;

    ArvissExecute(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b110 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- rs1 | rs2
    ASSERT_EQ(cpu.xreg[rs1] | cpu.xreg[rs2], cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Op_Rem) // RV32M
{
    // REM performs a 32-bit x 32-bit (signed / signed) integer division of rs1 by rs2, rounding towards zero, and returns the
    // remainder. The sign of the result is the sign of the dividend.

    // rd <- rs % rs2, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 5;
    uint32_t rs1 = 13;
    uint32_t rs2 = 14;
    int32_t dividend = -65535;
    cpu.xreg[rs1] = dividend;
    cpu.xreg[rs2] = 4096;

    int32_t expected = (int32_t)cpu.xreg[rs1] % (int32_t)cpu.xreg[rs2];

    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b110 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- rs1 % rs2
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Division by zero set the result to the dividend.
    cpu.xreg[rs2] = 0;
    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b110 << 12) | EncodeRd(rd) | OP_OP);
    ASSERT_EQ(dividend, (int32_t)cpu.xreg[rd]);

    // Division of the most negative integer by -1 results in overflow which sets the result to zero.
    cpu.xreg[rs1] = 0x80000000;
    cpu.xreg[rs2] = -1;
    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b110 << 12) | EncodeRd(rd) | OP_OP);
    ASSERT_EQ(0, cpu.xreg[rd]);

    // x0 is immutable.
    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs1) | EncodeRs1(rs2) | (0b110 << 12) | EncodeRd(0) | OP_OP);
    ASSERT_EQ(0, cpu.xreg[0]);
}

TEST_F(TestDecoder, Op_And)
{
    // rd <- rs1 & rs2, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 12;
    uint32_t rs1 = 13;
    uint32_t rs2 = 14;
    cpu.xreg[rs1] = 0xff00ff00;
    cpu.xreg[rs2] = 0xffffffff;

    ArvissExecute(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b111 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- rs1 & rs2
    ASSERT_EQ(cpu.xreg[rs1] & cpu.xreg[rs2], cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Op_Remu) // RV32M
{
    // REMU performs a 32-bit x 32-bit (unsigned / unsigned) integer division of rs1 by rs2, rounding towards zero, and returns the
    // remainder.

    // rd <- rs % rs2, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 5;
    uint32_t rs1 = 13;
    uint32_t rs2 = 14;
    uint32_t dividend = 65535;
    cpu.xreg[rs1] = dividend;
    cpu.xreg[rs2] = 16384;

    uint32_t expected = cpu.xreg[rs1] % cpu.xreg[rs2];

    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b111 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- rs1 % rs2
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Division by zero set the result to the dividend.
    cpu.xreg[rs2] = 0;
    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b111 << 12) | EncodeRd(rd) | OP_OP);
    ASSERT_EQ(dividend, (int32_t)cpu.xreg[rd]);

    // x0 is immutable.
    ArvissExecute(&cpu, (0b0000001 << 25) | EncodeRs2(rs1) | EncodeRs1(rs2) | (0b111 << 12) | EncodeRd(0) | OP_OP);
    ASSERT_EQ(0, cpu.xreg[0]);
}

TEST_F(TestDecoder, Op_x0_Is_Zero)
{
    // ADD
    cpu.xreg[1] = 123;
    cpu.xreg[2] = 456;
    ArvissExecute(&cpu, (0 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b000 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SUB
    cpu.xreg[1] = 123;
    cpu.xreg[2] = 456;
    ArvissExecute(&cpu, (1 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b000 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SLL
    cpu.xreg[1] = 0xff;
    cpu.xreg[2] = 3;
    ArvissExecute(&cpu, (0 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b001 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SLT
    cpu.xreg[1] = -1;
    cpu.xreg[2] = 1;
    ArvissExecute(&cpu, (0 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b010 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SLTU
    cpu.xreg[1] = 0;
    cpu.xreg[2] = 0xffffffff;
    ArvissExecute(&cpu, (0 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b011 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // XOR
    cpu.xreg[1] = 0x00ff00;
    cpu.xreg[2] = 0xffff00;
    ArvissExecute(&cpu, (0 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b100 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SRL
    cpu.xreg[1] = 0x80000000;
    cpu.xreg[2] = 3;
    ArvissExecute(&cpu, (0 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b101 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SRA
    cpu.xreg[1] = 0x80000000;
    cpu.xreg[2] = 3;
    ArvissExecute(&cpu, (0b0100000 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b101 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // OR
    cpu.xreg[1] = 0x000055;
    cpu.xreg[2] = 0xffffaa;
    ArvissExecute(&cpu, (0 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b110 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // AND
    cpu.xreg[1] = 0x555555;
    cpu.xreg[2] = 0xffffff;
    ArvissExecute(&cpu, (0 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b111 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);
}

TEST_F(TestDecoder, Op_Mret)
{
    // pc <- mepc + 4
    cpu.mepc = 0x4000;
    cpu.pc = 0x8080;

    ArvissExecute(&cpu, (0b001100000010 << 20) | OP_SYSTEM);

    // pc <- mepc + 4
    ASSERT_EQ(cpu.mepc + 4, cpu.pc);
}

TEST_F(TestDecoder, Traps_Set_Mepc)
{
    // mepc <- pc
    cpu.pc = 0x8086;
    cpu.mepc = 0;
    uint32_t savedPc = cpu.pc;

    // Take a breakpoint.
    ArvissExecute(&cpu, (0b000000000001 << 20) | OP_SYSTEM);

    // mepc <- pc
    ASSERT_EQ(savedPc, cpu.mepc);
}

TEST_F(TestDecoder, Traps_Set_Mcause)
{
    // mcause <- reason for trap
    cpu.pc = 0x8086;
    cpu.mepc = 0;

    // Take a breakpoint.
    ArvissExecute(&cpu, (0b000000000001 << 20) | OP_SYSTEM);

    // mcause <- reason for trap
    ASSERT_EQ(trBREAKPOINT, cpu.mcause);
}

TEST_F(TestDecoder, Traps_Set_Mtval)
{
    // mtval <- exception specific information
    uint32_t address = 0x80000000;
    cpu.mepc = 0;

    // Attempt to read from invalid memory.
    uint32_t pc = cpu.pc;
    int32_t imm_i = 0;
    uint32_t rd = 14;
    uint32_t rs1 = 15;
    cpu.xreg[rs1] = address;
    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_LOAD);

    // mtval <- exception specific information
    ASSERT_EQ(address, cpu.mtval);
}

TEST_F(TestDecoder, LoadFp_Flw)
{
    // rd <- f32(rs1 + imm_i), pc += 4
    uint32_t pc = cpu.pc;
    int32_t imm_i = 274;
    uint32_t rd = 14;
    uint32_t rs1 = 15;
    cpu.xreg[rs1] = rambase;

    // Write a float.
    float expected = -1234e-6f;
    uint32_t expectedAsU32 = FloatAsU32(expected);
    ArvissWriteWord(cpu.memory, cpu.xreg[rs1] + imm_i, expectedAsU32);

    ArvissExecute(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_LOADFP);

    // rd <- f32(rs1 + imm_i)
    ASSERT_EQ(expected, cpu.freg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, StoreFp_Fsw)
{
    // f32(rs1 + imm_s) = rs2, pc += 4
    uint32_t pc = cpu.pc;
    int32_t imm_s = 222;
    uint32_t rs1 = 2;
    uint32_t rs2 = 29;
    cpu.xreg[rs1] = rambase + ramsize / 2;
    float expected = 12345.99f;
    cpu.freg[rs2] = expected;

    ArvissExecute(&cpu, EncodeS(imm_s) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b010 << 12) | OP_STOREFP);

    // m32(rs1 + imm_s) <- rs2
    ArvissResult wordResult = ArvissReadWord(cpu.memory, cpu.xreg[rs1] + imm_s);
    ASSERT_TRUE(ArvissResultIsWord(wordResult));

    uint32_t resultAsWord = ArvissResultAsWord(wordResult);
    float resultAsFloat = U32AsFloat(resultAsWord);
    ASSERT_EQ(expected, resultAsFloat);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Madd_Fmadd_s)
{
    // rd <- (rs1 * rs2) + rs3, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 5;
    uint32_t rs1 = 2;
    uint32_t rs2 = 29;
    uint32_t rs3 = 3;
    uint32_t rm = RM_DYN;
    cpu.freg[rs1] = 12.34f;
    cpu.freg[rs2] = 56.78f;
    cpu.freg[rs3] = 100.0f;
    float expected = cpu.freg[rs1] * cpu.freg[rs2] + cpu.freg[rs3];

    ArvissExecute(&cpu, EncodeRs3(rs3) | (0b00 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | EncodeRm(rm) | EncodeRd(rd) | OP_MADD);

    // rd <- (rs1 * rs2) + rs3
    ASSERT_EQ(expected, cpu.freg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Msub_Fmsub_s)
{
    // rd <- (rs1 * rs2) - rs3, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 5;
    uint32_t rs1 = 2;
    uint32_t rs2 = 29;
    uint32_t rs3 = 3;
    uint32_t rm = RM_DYN;
    cpu.freg[rs1] = 1244.5f;
    cpu.freg[rs2] = 10.0f;
    cpu.freg[rs3] = 100.0f;
    float expected = cpu.freg[rs1] * cpu.freg[rs2] - cpu.freg[rs3];

    ArvissExecute(&cpu, EncodeRs3(rs3) | (0b00 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | EncodeRm(rm) | EncodeRd(rd) | OP_MSUB);

    // rd <- (rs1 * rs2) - rs3
    ASSERT_EQ(expected, cpu.freg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Nmsub_Fnmsub_s)
{
    // rd <- -(rs1 * rs2) + rs3, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 5;
    uint32_t rs1 = 2;
    uint32_t rs2 = 29;
    uint32_t rs3 = 3;
    uint32_t rm = RM_DYN;
    cpu.freg[rs1] = 1244.5f;
    cpu.freg[rs2] = 10.0f;
    cpu.freg[rs3] = 100.0f;
    float expected = -(cpu.freg[rs1] * cpu.freg[rs2]) + cpu.freg[rs3];

    ArvissExecute(&cpu, EncodeRs3(rs3) | (0b00 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | EncodeRm(rm) | EncodeRd(rd) | OP_NMSUB);

    // rd <- -(rs1 * rs2) + rs3
    ASSERT_EQ(expected, cpu.freg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Nmadd_Fnmadd_s)
{
    // rd <- -(rs1 * rs2) - rs3, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 5;
    uint32_t rs1 = 2;
    uint32_t rs2 = 29;
    uint32_t rs3 = 3;
    uint32_t rm = RM_DYN;
    cpu.freg[rs1] = 1244.5f;
    cpu.freg[rs2] = 10.0f;
    cpu.freg[rs3] = 100.0f;
    float expected = -(cpu.freg[rs1] * cpu.freg[rs2]) - cpu.freg[rs3];

    ArvissExecute(&cpu, EncodeRs3(rs3) | (0b00 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | EncodeRm(rm) | EncodeRd(rd) | OP_NMADD);

    // rd <- -(rs1 * rs2) - rs3
    ASSERT_EQ(expected, cpu.freg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpFp_Fadd_s)
{
    // rd <- rs1 + rs2, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 15;
    uint32_t rs1 = 4;
    uint32_t rs2 = 7;
    uint32_t rm = RM_DYN;
    cpu.freg[rs1] = 1024.0f;
    cpu.freg[rs2] = 512.0f;
    float expected = cpu.freg[rs1] + cpu.freg[rs2];

    ArvissExecute(&cpu, (0b0000000 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | EncodeRm(rm) | EncodeRd(rd) | OP_OPFP);

    // rd <- rs1 + rs2
    ASSERT_EQ(expected, cpu.freg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpFp_Fsub_s)
{
    // rd <- rs1 + rs2, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 5;
    uint32_t rs1 = 14;
    uint32_t rs2 = 17;
    uint32_t rm = RM_DYN;
    cpu.freg[rs1] = 16384.0f;
    cpu.freg[rs2] = 1024.0f;
    float expected = cpu.freg[rs1] - cpu.freg[rs2];

    ArvissExecute(&cpu, (0b0000100 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | EncodeRm(rm) | EncodeRd(rd) | OP_OPFP);

    // rd <- rs1 - rs2
    ASSERT_EQ(expected, cpu.freg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpFp_Fmul_s)
{
    // rd <- rs1 + rs2, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 2;
    uint32_t rs1 = 3;
    uint32_t rs2 = 7;
    uint32_t rm = RM_DYN;
    cpu.freg[rs1] = 2560.0f;
    cpu.freg[rs2] = -1440.0f;
    float expected = cpu.freg[rs1] * cpu.freg[rs2];

    ArvissExecute(&cpu, (0b0001000 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | EncodeRm(rm) | EncodeRd(rd) | OP_OPFP);

    // rd <- rs1 * rs2
    ASSERT_EQ(expected, cpu.freg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpFp_Fdiv_s)
{
    // rd <- rs1 + rs2, pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 12;
    uint32_t rs1 = 13;
    uint32_t rs2 = 6;
    uint32_t rm = RM_DYN;
    cpu.freg[rs1] = -327680.0f;
    cpu.freg[rs2] = 1024.0f;
    float expected = cpu.freg[rs1] / cpu.freg[rs2];

    ArvissExecute(&cpu, (0b0001100 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | EncodeRm(rm) | EncodeRd(rd) | OP_OPFP);

    // rd <- rs1 / rs2
    ASSERT_EQ(expected, cpu.freg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpFp_Fsqrt_s)
{
    // rd <- sqrt(ts1), pc += 4
    uint32_t pc = cpu.pc;
    uint32_t rd = 5;
    uint32_t rs1 = 3;
    uint32_t rm = RM_DYN;
    cpu.freg[rs1] = 65536.0;
    float expected = sqrtf(cpu.freg[rs1]);

    ArvissExecute(&cpu, (0b0101100 << 25) | EncodeRs2(0) | EncodeRs1(rs1) | EncodeRm(rm) | EncodeRd(rd) | OP_OPFP);

    // rd <- sqrt(rs1)
    ASSERT_EQ(expected, cpu.freg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

static inline float Sgn(float n)
{
    return n < 0.0f ? -1.0f : 1.0f;
}

TEST_F(TestDecoder, OpFp_Fsgnj_s)
{
    // rd <- abs(rs1) * sgn(rs2), pc += 4
    uint32_t pc = cpu.pc;

    uint32_t rd = 0;
    uint32_t rs1 = 4;
    uint32_t rs2 = 4;
    cpu.freg[rs1] = -32.0f;
    cpu.freg[rs2] = -21.0f;

    float expected = fabsf(cpu.freg[rs1]) * Sgn(cpu.freg[rs2]);

    ArvissExecute(&cpu, (0b0010000 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_OPFP);

    // rd <- abs(rs1) * sgn(rs2)
    ASSERT_EQ(expected, cpu.freg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpFp_Fsgnjn_s)
{
    // rd <- abs(rs1) * -sgn(rs2), pc += 4
    uint32_t pc = cpu.pc;

    uint32_t rd = 3;
    uint32_t rs1 = 2;
    uint32_t rs2 = 1;
    cpu.freg[rs1] = -53623.0;
    cpu.freg[rs2] = 75.0f;

    float expected = fabsf(cpu.freg[rs1]) * -Sgn(cpu.freg[rs2]);

    ArvissExecute(&cpu, (0b0010000 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_OPFP);

    // rd <- abs(rs1) * -sgn(rs2)
    ASSERT_EQ(expected, cpu.freg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpFp_Fsgnjx_s)
{
    // rd <- abs(rs1) * (sgn(rs1) == sgn(rs2)) ? 1 : -1
    uint32_t pc = cpu.pc;

    uint32_t rd = 3;
    uint32_t rs1 = 2;
    uint32_t rs2 = 1;

    // Both positive.
    cpu.freg[rs1] = 4623.0;
    cpu.freg[rs2] = 75.0f;

    float expected = fabsf(cpu.freg[rs1]);

    ArvissExecute(&cpu, (0b0010000 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_OPFP);

    // rd <- rs1 * 1
    ASSERT_EQ(expected, cpu.freg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Both negative.
    cpu.freg[rs1] = -234.0;
    cpu.freg[rs2] = -984.0f;

    expected = fabsf(cpu.freg[rs1]);

    ArvissExecute(&cpu, (0b0010000 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_OPFP);

    // rd <- rs1 * 1
    ASSERT_EQ(expected, cpu.freg[rd]);

    // Positive and negative.
    cpu.freg[rs1] = 249.0;
    cpu.freg[rs2] = -194.0f;

    expected = -fabsf(cpu.freg[rs1]);

    ArvissExecute(&cpu, (0b0010000 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_OPFP);

    // rd <- rs1 * -1
    ASSERT_EQ(expected, cpu.freg[rd]);

    // Negative and positive.
    cpu.freg[rs1] = -1337.0;
    cpu.freg[rs2] = 1943.0f;

    expected = -fabsf(cpu.freg[rs1]);

    ArvissExecute(&cpu, (0b0010000 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_OPFP);

    // rd <- rs1 * -1
    ASSERT_EQ(expected, cpu.freg[rd]);
}

TEST_F(TestDecoder, OpFp_Fmin_s)
{
    // rd <- min(rs1, rs2), pc += 4
    uint32_t pc = cpu.pc;

    uint32_t rd = 15;
    uint32_t rs1 = 13;
    uint32_t rs2 = 31;
    cpu.freg[rs1] = 456.7f;
    cpu.freg[rs2] = 89.10f;

    float expected = fminf(cpu.freg[rs1], cpu.freg[rs2]);

    ArvissExecute(&cpu, (0b0010100 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_OPFP);

    // rd <- min(rs1, rs2)
    ASSERT_EQ(expected, cpu.freg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpFp_Fmax_s)
{
    // rd <- max(rs1, rs2), pc += 4
    uint32_t pc = cpu.pc;

    uint32_t rd = 15;
    uint32_t rs1 = 13;
    uint32_t rs2 = 31;
    cpu.freg[rs1] = 456.7f;
    cpu.freg[rs2] = 89.10f;

    float expected = fmaxf(cpu.freg[rs1], cpu.freg[rs2]);

    ArvissExecute(&cpu, (0b0010100 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_OPFP);

    // rd <- max(rs1, rs2)
    ASSERT_EQ(expected, cpu.freg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpFp_Fcvt_w_s)
{
    // rd <- int32_t(rs1), pc += 4
    uint32_t pc = cpu.pc;

    uint32_t rd = 15;
    uint32_t rs1 = 13;
    uint32_t op = 0b00000;
    uint32_t rm = RM_DYN;
    cpu.freg[rs1] = -12345678.910f; // Actually -12345679.0f because of rounding.

    int32_t expected = -12345679;

    ArvissExecute(&cpu, (0b1100000 << 25) | EncodeRs2(op) | EncodeRs1(rs1) | EncodeRm(rm) | EncodeRd(rd) | OP_OPFP);

    // rd <- int32_t(rs1)
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpFp_Fcvt_wu_s)
{
    // rd <- uint32_t(rs1), pc += 4
    uint32_t pc = cpu.pc;

    uint32_t rd = 15;
    uint32_t rs1 = 13;
    uint32_t op = 0b00001;
    uint32_t rm = RM_DYN;
    cpu.freg[rs1] = -12345678.910f; // Actually -12345679.0f because of rounding.

    uint32_t expected = (uint32_t)-12345679;

    ArvissExecute(&cpu, (0b1100000 << 25) | EncodeRs2(op) | EncodeRs1(rs1) | EncodeRm(rm) | EncodeRd(rd) | OP_OPFP);

    // rd <- uint32_t(rs1)
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpFp_Fmv_x_w)
{
    // bits(rd) <- bits(rs1), pc += 4
    uint32_t pc = cpu.pc;

    uint32_t rd = 15;
    uint32_t rs1 = 13;
    cpu.freg[rs1] = 12345678;

    uint32_t expected = FloatAsU32(cpu.freg[rs1]);

    ArvissExecute(&cpu, (0b1110000 << 25) | EncodeRs2(0b00000) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_OPFP);

    // bits(rd) <- bits(rs1)
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpFp_Fclass_s)
{
    uint32_t pc = cpu.pc;

    uint32_t rd = 12;
    uint32_t rs1 = 1;

    // rs1 is -infinity
    cpu.freg[rs1] = -INFINITY;
    uint32_t expected = (1 << 0);

    ArvissExecute(&cpu, (0b1110000 << 25) | EncodeRs2(0b00000) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_OPFP);

    // rs1 is -infinity.
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // rs1 is infinity.
    cpu.freg[rs1] = INFINITY;
    expected = (1 << 7);

    ArvissExecute(&cpu, (0b1110000 << 25) | EncodeRs2(0b00000) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_OPFP);

    // rs1 is infinity.
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // rs1 is -0
    cpu.freg[rs1] = U32AsFloat(0x80000000);
    expected = (1 << 3);

    ArvissExecute(&cpu, (0b1110000 << 25) | EncodeRs2(0b00000) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_OPFP);

    // rs1 is -0.
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // rs1 is 0
    cpu.freg[rs1] = 0.0f;
    expected = (1 << 4);

    ArvissExecute(&cpu, (0b1110000 << 25) | EncodeRs2(0b00000) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_OPFP);

    // rs1 is 0.
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // rs1 is a negative normal number.
    cpu.freg[rs1] = -123.45f;
    expected = (1 << 1);

    ArvissExecute(&cpu, (0b1110000 << 25) | EncodeRs2(0b00000) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_OPFP);

    // rs1 is a negative normal number.
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // rs1 is a positive normal number.
    cpu.freg[rs1] = 123.45f;
    expected = (1 << 6);

    ArvissExecute(&cpu, (0b1110000 << 25) | EncodeRs2(0b00000) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_OPFP);

    // rs1 is a positive normal number.
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // rs1 is a negative subnormal number (sign bit is set and exponent is zero ... significand is not zero)
    cpu.freg[rs1] = U32AsFloat(0x80000001);
    expected = (1 << 2);

    ArvissExecute(&cpu, (0b1110000 << 25) | EncodeRs2(0b00000) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_OPFP);

    // rs1 is a negative subnormal number.
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // rs1 is a positive subnormal number (sign bit is clear and exponent is zero ... significand is not zero)
    cpu.freg[rs1] = U32AsFloat(0x00000001);
    expected = (1 << 5);

    ArvissExecute(&cpu, (0b1110000 << 25) | EncodeRs2(0b00000) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_OPFP);

    // rs1 is a positive subnormal number.
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // rs1 is a signalling NaN. We're poking the value in because otherwise it is converted to a quiet NaN, which has a different
    // bit representation.
    auto* preg = reinterpret_cast<uint32_t*>(&cpu.freg[rs1]);
    *preg = 0x7f800001;
    expected = (1 << 8);

    ArvissExecute(&cpu, (0b1110000 << 25) | EncodeRs2(0b00000) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_OPFP);

    // rs1 is a signalling NaN.
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // rs1 is a quiet NaN
    cpu.freg[rs1] = U32AsFloat(0x7fc00000);
    expected = (1 << 9);

    ArvissExecute(&cpu, (0b1110000 << 25) | EncodeRs2(0b00000) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_OPFP);

    // rs1 is a quiet NaN.
    ASSERT_EQ(expected, cpu.xreg[rd]);
}

TEST_F(TestDecoder, OpFp_Feq_s)
{
    // rd <- (rs == rs2) ? 1 : 0, pc += 4
    uint32_t pc = cpu.pc;

    uint32_t rd = 3;
    uint32_t rs1 = 2;
    uint32_t rs2 = 1;

    // rs1 == rs2
    cpu.freg[rs1] = 75.0f;
    cpu.freg[rs2] = 75.0f;
    uint32_t expected = 1;

    ArvissExecute(&cpu, (0b1010000 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_OPFP);

    // rd <- 1
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // rs1 != rs2
    cpu.freg[rs1] = 75.1f;
    cpu.freg[rs2] = 75.0f;
    expected = 0;

    ArvissExecute(&cpu, (0b1010000 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_OPFP);

    // rd <- 0
    ASSERT_EQ(expected, cpu.xreg[rd]);
}

TEST_F(TestDecoder, OpFp_Flt_s)
{
    // rd <- (rs < rs2) ? 1 : 0, pc += 4
    uint32_t pc = cpu.pc;

    uint32_t rd = 3;
    uint32_t rs1 = 2;
    uint32_t rs2 = 1;

    // rs1 < rs2
    cpu.freg[rs1] = 75.0f;
    cpu.freg[rs2] = 75.1f;
    uint32_t expected = 1;

    ArvissExecute(&cpu, (0b1010000 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_OPFP);

    // rd <- 1
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // rs1 >= rs2
    cpu.freg[rs1] = 75.1f;
    cpu.freg[rs2] = 75.0f;
    expected = 0;

    ArvissExecute(&cpu, (0b1010000 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_OPFP);

    // rd <- 0
    ASSERT_EQ(expected, cpu.xreg[rd]);
}

TEST_F(TestDecoder, OpFp_Fle_s)
{
    // rd <- (rs <= rs2) ? 1 : 0, pc += 4
    uint32_t pc = cpu.pc;

    uint32_t rd = 3;
    uint32_t rs1 = 2;
    uint32_t rs2 = 1;

    // rs1 == rs2
    cpu.freg[rs1] = 75.0f;
    cpu.freg[rs2] = 75.0f;
    uint32_t expected = 1;

    ArvissExecute(&cpu, (0b1010000 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_OPFP);

    // rd <- 1
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // rs1 < rs2
    cpu.freg[rs1] = 75.0f;
    cpu.freg[rs2] = 75.1f;
    expected = 1;

    ArvissExecute(&cpu, (0b1010000 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_OPFP);

    // rd <- 0
    ASSERT_EQ(expected, cpu.xreg[rd]);

    // rs1 > rs2
    cpu.freg[rs1] = 75.1f;
    cpu.freg[rs2] = 75.0f;
    expected = 0;

    ArvissExecute(&cpu, (0b1010000 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_OPFP);

    // rd <- 0
    ASSERT_EQ(expected, cpu.xreg[rd]);
}

TEST_F(TestDecoder, OpFp_Fcvt_s_w)
{
    // rd <- float(int32_t(rs1)), pc += 4
    uint32_t pc = cpu.pc;

    uint32_t rd = 15;
    uint32_t rs1 = 13;
    uint32_t op = 0b00000;
    uint32_t rm = RM_DYN;
    cpu.xreg[rs1] = -456;

    float expected = (float)(int32_t)(cpu.xreg[rs1]); // TODO: is this true?

    ArvissExecute(&cpu, (0b1101000 << 25) | EncodeRs2(op) | EncodeRs1(rs1) | EncodeRm(rm) | EncodeRd(rd) | OP_OPFP);

    // rd <- float(int32_t(rs1))
    ASSERT_EQ(expected, cpu.freg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpFp_Fcvt_s_wu)
{
    // rd <- float(rs1), pc += 4
    uint32_t pc = cpu.pc;

    uint32_t rd = 15;
    uint32_t rs1 = 13;
    uint32_t op = 0b00001;
    uint32_t rm = RM_DYN;
    cpu.xreg[rs1] = -456;

    float expected = (float)(cpu.xreg[rs1]); // TODO: is this true?

    ArvissExecute(&cpu, (0b1101000 << 25) | EncodeRs2(op) | EncodeRs1(rs1) | EncodeRm(rm) | EncodeRd(rd) | OP_OPFP);

    // rd <- float(rs1)
    ASSERT_EQ(expected, cpu.freg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpFp_Fmv_w_x)
{
    // bits(rd) <- bits(rs1), pc += 4
    uint32_t pc = cpu.pc;

    uint32_t rd = 5;
    uint32_t rs1 = 1;
    cpu.xreg[rs1] = 17135;

    float expected = U32AsFloat(cpu.xreg[rs1]);

    ArvissExecute(&cpu, (0b1111000 << 25) | EncodeRs2(0b00000) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_OPFP);

    // bits(rd) <- bits(rs1)
    ASSERT_EQ(expected, cpu.freg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpSystem_ECall)
{
    // As Arviss currently supports a machine mode only CPU, executing an ECALL is essentially a request from the CPU to Arviss
    // itself, so we don't do anything to update the program counter.

    // mepc <- pc
    uint32_t pc = cpu.pc;

    ArvissResult result = ArvissExecute(&cpu, (0b000000000000 << 20) | OP_SYSTEM);

    // mepc <- pc
    ASSERT_EQ(pc, cpu.mepc);

    // Executing an ECALL will always generate an environment call from machine mode as Arviss currently supports machine mode only.
    ASSERT_TRUE(ArvissResultIsTrap(result));
    ArvissTrap trap = ArvissResultAsTrap(result);
    ASSERT_EQ(trENVIRONMENT_CALL_FROM_M_MODE, trap.mcause);
    ASSERT_EQ(0, trap.mtval);
}

TEST_F(TestDecoder, OpSystem_Mret)
{
    // pc <- mepc, pc += 4
    uint32_t mepc = cpu.mepc;

    ArvissExecute(&cpu, (0b001100000010 << 20) | OP_SYSTEM);

    // pc <- mepc + 4
    ASSERT_EQ(mepc + 4, cpu.pc);
}
