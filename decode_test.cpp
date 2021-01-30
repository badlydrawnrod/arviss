#include "decode.h"
#include "smallmem.h"

#include "gtest/gtest.h"

class TestDecoder : public ::testing::Test
{
protected:
    void SetUp();

    static uint32_t EncodeRd(uint32_t n);
    static uint32_t EncodeRs1(uint32_t n);
    static uint32_t EncodeRs2(uint32_t n);
    static uint32_t EncodeJ(uint32_t n);
    static uint32_t EncodeB(uint32_t n);
    static uint32_t EncodeS(uint32_t n);
    static uint32_t EncodeI(uint32_t n);

    static constexpr uint32_t rambase = 0x1000; // Deliberately not zero.
    static constexpr uint32_t ramsize = 0x1000; // Deliberately small to keep offsets from getting out of range.

    CPU cpu;

    inline static Memory memory;
};

void TestDecoder::SetUp()
{
    // Clear the RAM.
    for (auto& b : memory.ram)
    {
        b = 0;
    }

    // Reset the CPU.
    Reset(&cpu, rambase + ramsize);
    cpu.memory = smallmem_Init(&memory);
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

        Decode(&cpu, (imm_u << 12) | EncodeRd(rd) | OP_LUI);

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

    Decode(&cpu, (imm_u << 12) | EncodeRd(0) | OP_LUI);

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

        Decode(&cpu, (imm_u << 12) | EncodeRd(rd) | OP_AUIPC);

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

    Decode(&cpu, (imm_u << 12) | EncodeRd(0) | OP_AUIPC);

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

        Decode(&cpu, (EncodeJ(imm_j) | EncodeRd(rd)) | OP_JAL);

        // rd <- pc + 4
        ASSERT_EQ(pc + 4, cpu.xreg[rd]);

        // pc <- pc + imm_j
        ASSERT_EQ(pc + v, cpu.pc);
    }
}

TEST_F(TestDecoder, Jal_x0_Is_Zero)
{
    Decode(&cpu, (EncodeJ(123) | EncodeRd(0)) | OP_JAL);

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
        Decode(&cpu, ins);

        // rd <- pc + 4
        ASSERT_EQ(pc + 4, cpu.xreg[rd]);

        // pc <- (rs1 + imm_i) & ~1
        ASSERT_EQ((rs1Before + imm_i) & ~1, cpu.pc);
    }
}

TEST_F(TestDecoder, Jalr_x0_Is_Zero)
{
    Decode(&cpu, EncodeI(123) | EncodeRs1(1) | (0b000 << 12) | EncodeRd(0) | OP_JALR);

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
    Decode(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b000 << 12) | OP_BRANCH);

    // pc <- pc + imm_b
    ASSERT_EQ(pc + imm_b, cpu.pc);

    // Branch not taken.
    pc = cpu.pc;
    cpu.xreg[rs1] = 5678;
    cpu.xreg[rs2] = 8765;
    Decode(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b000 << 12) | OP_BRANCH);

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
    Decode(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b001 << 12) | OP_BRANCH);

    // pc <- pc + imm_b
    ASSERT_EQ(pc + imm_b, cpu.pc);

    // Branch not taken.
    pc = cpu.pc;
    cpu.xreg[rs2] = cpu.xreg[rs1];
    Decode(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b001 << 12) | OP_BRANCH);

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
    Decode(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b100 << 12) | OP_BRANCH);

    // pc <- pc + imm_b
    ASSERT_EQ(pc + imm_b, cpu.pc);

    // Branch not taken.
    pc = cpu.pc;
    cpu.xreg[rs1] = 456;
    cpu.xreg[rs2] = 123;
    Decode(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b100 << 12) | OP_BRANCH);

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
    Decode(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b101 << 12) | OP_BRANCH);

    // pc <- pc + imm_b
    ASSERT_EQ(pc + imm_b, cpu.pc);

    // Branch taken (equal)
    pc = cpu.pc;
    cpu.xreg[rs1] = -1;
    cpu.xreg[rs2] = -1;
    Decode(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b101 << 12) | OP_BRANCH);

    // pc <- pc + imm_b
    ASSERT_EQ(pc + imm_b, cpu.pc);

    // Branch not taken.
    pc = cpu.pc;
    cpu.xreg[rs1] = -1;
    cpu.xreg[rs2] = 0;
    Decode(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b101 << 12) | OP_BRANCH);

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
    Decode(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b110 << 12) | OP_BRANCH);

    // pc <- pc + imm_b
    ASSERT_EQ(pc + imm_b, cpu.pc);

    // Branch not taken.
    pc = cpu.pc;
    cpu.xreg[rs1] = 0xffffffff;
    cpu.xreg[rs2] = 0;
    Decode(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b110 << 12) | OP_BRANCH);

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
    Decode(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b111 << 12) | OP_BRANCH);

    // pc <- pc + imm_b
    ASSERT_EQ(pc + imm_b, cpu.pc);

    // Branch taken (equal)
    pc = cpu.pc;
    cpu.xreg[rs1] = 1;
    cpu.xreg[rs2] = 1;
    Decode(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b111 << 12) | OP_BRANCH);

    // pc <- pc + imm_b
    ASSERT_EQ(pc + imm_b, cpu.pc);

    // Branch not taken.
    pc = cpu.pc;
    cpu.xreg[rs1] = 0;
    cpu.xreg[rs2] = 0xffffffff;
    Decode(&cpu, EncodeB(imm_b) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b111 << 12) | OP_BRANCH);

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
    ::WriteByte(cpu.memory, cpu.xreg[rs1] + imm_i, 123);
    Decode(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_LOAD);

    // rd <- sx(m8(rs1 + imm_i))
    ASSERT_EQ(123, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Sign extend when bit 7 is one.
    pc = cpu.pc;
    ::WriteByte(cpu.memory, cpu.xreg[rs1] + imm_i, 0xff);
    Decode(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_LOAD);

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
    ::WriteHalfword(cpu.memory, cpu.xreg[rs1] + imm_i, 0x7fff);
    Decode(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_LOAD);

    // rd <- sx(m16(rs1 + imm_i))
    ASSERT_EQ(0x7fff, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Sign extend when bit 15 is one.
    pc = cpu.pc;
    ::WriteHalfword(cpu.memory, cpu.xreg[rs1] + imm_i, 0xffff);
    Decode(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_LOAD);

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
    ::WriteWord(cpu.memory, cpu.xreg[rs1] + imm_i, 0x7fffffff);
    Decode(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_LOAD);

    // rd <- sx(m32(rs1 + imm_i))
    ASSERT_EQ(0x7fffffff, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Sign extend when bit 31 is one.
    pc = cpu.pc;
    ::WriteWord(cpu.memory, cpu.xreg[rs1] + imm_i, 0xffffffff);
    Decode(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_LOAD);

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
    ::WriteByte(cpu.memory, cpu.xreg[rs1] + imm_i, 123);
    Decode(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b100 << 12) | EncodeRd(rd) | OP_LOAD);

    // rd <- zx(m8(rs1 + imm_i))
    ASSERT_EQ(123, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Zero extend when bit 7 is zero.
    pc = cpu.pc;
    ::WriteByte(cpu.memory, cpu.xreg[rs1] + imm_i, 0xff);
    Decode(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b100 << 12) | EncodeRd(rd) | OP_LOAD);

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
    ::WriteHalfword(cpu.memory, cpu.xreg[rs1] + imm_i, 0x7fff);
    Decode(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b101 << 12) | EncodeRd(rd) | OP_LOAD);

    // rd <- zx(m16(rs1 + imm_i))
    ASSERT_EQ(0x7fff, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Zero extend when bit 15 is one.
    pc = cpu.pc;
    ::WriteHalfword(cpu.memory, cpu.xreg[rs1] + imm_i, 0xffff);
    Decode(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b101 << 12) | EncodeRd(rd) | OP_LOAD);

    // rd <- zx(m16(rs1 + imm_i))
    ASSERT_EQ(0xffff, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Load_x0_Is_Zero)
{
    ::WriteWord(cpu.memory, rambase, 0x12345678);

    // LB
    uint32_t rs1 = 13;
    cpu.xreg[rs1] = rambase;
    ::WriteByte(cpu.memory, rambase, 0xff);
    Decode(&cpu, EncodeI(0) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(0) | OP_LOAD);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // LH
    Decode(&cpu, EncodeI(0) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(0) | OP_LOAD);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // LW
    Decode(&cpu, EncodeI(0) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(0) | OP_LOAD);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // LBU
    Decode(&cpu, EncodeI(0) | EncodeRs1(rs1) | (0b100 << 12) | EncodeRd(0) | OP_LOAD);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // LHU
    Decode(&cpu, EncodeI(0) | EncodeRs1(rs1) | (0b101 << 12) | EncodeRd(0) | OP_LOAD);

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

    Decode(&cpu, EncodeS(imm_s) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b000 << 12) | OP_STORE);

    // m8(rs1 + imm_s) <- rs2[7:0]
    CpuResult byteResult = ::ReadByte(cpu.memory, cpu.xreg[rs1] + imm_s);
    ASSERT_TRUE(ResultIsByte(byteResult));
    ASSERT_EQ(ResultAsByte(byteResult), cpu.xreg[rs2] & 0xff);

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

    Decode(&cpu, EncodeS(imm_s) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b001 << 12) | OP_STORE);

    // m16(rs1 + imm_s) <- rs2[15:0]
    CpuResult halfwordResult = ::ReadHalfword(cpu.memory, cpu.xreg[rs1] + imm_s);
    ASSERT_TRUE(ResultIsHalfword(halfwordResult));
    ASSERT_EQ(ResultAsHalfword(halfwordResult), cpu.xreg[rs2] & 0xffff);

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

    Decode(&cpu, EncodeS(imm_s) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b010 << 12) | OP_STORE);

    // m32(rs1 + imm_s) <- rs2[31:0]
    CpuResult wordResult = ::ReadWord(cpu.memory, cpu.xreg[rs1] + imm_s);
    ASSERT_TRUE(ResultIsWord(wordResult));
    ASSERT_EQ(ResultAsWord(wordResult), cpu.xreg[rs2] & 0xffff);

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
    Decode(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_OPIMM);

    // rd <- rs1 + imm_i
    ASSERT_EQ(cpu.xreg[rs1] + imm_i, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Add negative number.
    pc = cpu.pc;
    imm_i = -123;
    Decode(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_OPIMM);

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
    Decode(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_OPIMM);

    // rd <- (rs1 < imm_i) ? 1 : 0
    ASSERT_EQ(1, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Condition false.
    pc = cpu.pc;
    cpu.xreg[rs1] = 123;
    Decode(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_OPIMM);

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
    Decode(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b011 << 12) | EncodeRd(rd) | OP_OPIMM);

    // rd <- (rs1 < imm_i) ? 1 : 0
    ASSERT_EQ(1, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Condition false.
    pc = cpu.pc;
    cpu.xreg[rs1] = 0xffffffff;
    Decode(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b011 << 12) | EncodeRd(rd) | OP_OPIMM);

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

    Decode(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b100 << 12) | EncodeRd(rd) | OP_OPIMM);

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

    Decode(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b110 << 12) | EncodeRd(rd) | OP_OPIMM);

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

    Decode(&cpu, EncodeI(imm_i) | EncodeRs1(rs1) | (0b111 << 12) | EncodeRd(rd) | OP_OPIMM);

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

    Decode(&cpu, EncodeI(shamt) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_OPIMM);

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

    Decode(&cpu, EncodeI(shamt) | EncodeRs1(rs1) | (0b101 << 12) | EncodeRd(rd) | OP_OPIMM);

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

    Decode(&cpu, (1 << 30) | EncodeI(shamt) | EncodeRs1(rs1) | (0b101 << 12) | EncodeRd(rd) | OP_OPIMM);

    // rd <- sx(rs1) >> shamt_i
    ASSERT_EQ(((int32_t)cpu.xreg[rs1]) >> shamt, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, OpImm_x0_Is_Zero)
{
    // ADDI
    Decode(&cpu, EncodeI(123) | EncodeRs1(1) | (0b000 << 12) | EncodeRd(0) | OP_OPIMM);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SLTI
    Decode(&cpu, EncodeI(123) | EncodeRs1(1) | (0b010 << 12) | EncodeRd(0) | OP_OPIMM);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SLTIU
    Decode(&cpu, EncodeI(123) | EncodeRs1(1) | (0b011 << 12) | EncodeRd(0) | OP_OPIMM);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // XORI
    Decode(&cpu, EncodeI(123) | EncodeRs1(1) | (0b100 << 12) | EncodeRd(0) | OP_OPIMM);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // ORI
    Decode(&cpu, EncodeI(123) | EncodeRs1(1) | (0b110 << 12) | EncodeRd(0) | OP_OPIMM);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // ANDI
    cpu.xreg[1] = 0xffffffff;
    Decode(&cpu, EncodeI(123) | EncodeRs1(1) | (0b111 << 12) | EncodeRd(0) | OP_OPIMM);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SLLI
    cpu.xreg[1] = 0xffffffff;
    Decode(&cpu, EncodeI(0xff) | EncodeRs1(1) | (0b001 << 12) | EncodeRd(0) | OP_OPIMM);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SRLI
    cpu.xreg[1] = 0xffffffff;
    Decode(&cpu, EncodeI(3) | EncodeRs1(1) | (0b101 << 12) | EncodeRd(0) | OP_OPIMM);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SRAI
    cpu.xreg[1] = 0xffffffff;
    Decode(&cpu, (1 << 30) | EncodeI(3) | EncodeRs1(1) | (0b101 << 12) | EncodeRd(0) | OP_OPIMM);

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

    Decode(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_OP);

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

    Decode(&cpu, (1 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b000 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- rs1 - rs2
    ASSERT_EQ(cpu.xreg[rs1] - cpu.xreg[rs2], cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
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

    Decode(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b001 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- rs1 << (rs2 % XLEN)
    ASSERT_EQ(cpu.xreg[rs1] << (cpu.xreg[rs2] % 32), cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
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
    Decode(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- (rs1 < imm_i) ? 1 : 0
    ASSERT_EQ(1, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Condition false.
    pc = cpu.pc;
    cpu.xreg[rs1] = 123;
    Decode(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b010 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- (rs1 < imm_i) ? 1 : 0
    ASSERT_EQ(0, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
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
    Decode(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b011 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- (rs1 < imm_i) ? 1 : 0
    ASSERT_EQ(1, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);

    // Condition false.
    pc = cpu.pc;
    cpu.xreg[rs1] = 0xffffffff;
    Decode(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b011 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- (rs1 < imm_i) ? 1 : 0
    ASSERT_EQ(0, cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
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

    Decode(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b100 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- rs1 ^ rs2
    ASSERT_EQ(cpu.xreg[rs1] ^ cpu.xreg[rs2], cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
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

    Decode(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b101 << 12) | EncodeRd(rd) | OP_OP);

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

    Decode(&cpu, (0b0100000 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b101 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- sx(rs1) >> (rs2 % XLEN)
    ASSERT_EQ((int32_t)cpu.xreg[rs1] >> (cpu.xreg[rs2] % 32), cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
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

    Decode(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b110 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- rs1 | rs2
    ASSERT_EQ(cpu.xreg[rs1] | cpu.xreg[rs2], cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
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

    Decode(&cpu, (0 << 25) | EncodeRs2(rs2) | EncodeRs1(rs1) | (0b111 << 12) | EncodeRd(rd) | OP_OP);

    // rd <- rs1 & rs2
    ASSERT_EQ(cpu.xreg[rs1] & cpu.xreg[rs2], cpu.xreg[rd]);

    // pc <- pc + 4
    ASSERT_EQ(pc + 4, cpu.pc);
}

TEST_F(TestDecoder, Op_x0_Is_Zero)
{
    // ADD
    cpu.xreg[1] = 123;
    cpu.xreg[2] = 456;
    Decode(&cpu, (0 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b000 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SUB
    cpu.xreg[1] = 123;
    cpu.xreg[2] = 456;
    Decode(&cpu, (1 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b000 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SLL
    cpu.xreg[1] = 0xff;
    cpu.xreg[2] = 3;
    Decode(&cpu, (0 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b001 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SLT
    cpu.xreg[1] = -1;
    cpu.xreg[2] = 1;
    Decode(&cpu, (0 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b010 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SLTU
    cpu.xreg[1] = 0;
    cpu.xreg[2] = 0xffffffff;
    Decode(&cpu, (0 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b011 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // XOR
    cpu.xreg[1] = 0x00ff00;
    cpu.xreg[2] = 0xffff00;
    Decode(&cpu, (0 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b100 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SRL
    cpu.xreg[1] = 0x80000000;
    cpu.xreg[2] = 3;
    Decode(&cpu, (0 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b101 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // SRA
    cpu.xreg[1] = 0x80000000;
    cpu.xreg[2] = 3;
    Decode(&cpu, (0b0100000 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b101 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // OR
    cpu.xreg[1] = 0x000055;
    cpu.xreg[2] = 0xffffaa;
    Decode(&cpu, (0 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b110 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);

    // AND
    cpu.xreg[1] = 0x555555;
    cpu.xreg[2] = 0xffffff;
    Decode(&cpu, (0 << 25) | EncodeRs2(2) | EncodeRs1(1) | (0b111 << 12) | EncodeRd(0) | OP_OP);

    // x0 <- 0
    ASSERT_EQ(0, cpu.xreg[0]);
}

TEST_F(TestDecoder, Op_Mret)
{
    // pc <- mepc + 4
    cpu.mepc = 0x4000;
    cpu.pc = 0x8080;

    Decode(&cpu, (0b001100000010 << 20) | OP_SYSTEM);

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
    Trap trap = {trBREAKPOINT, 0x0};
    HandleTrap(&cpu, trap);

    // mepc <- pc
    ASSERT_EQ(savedPc, cpu.mepc);
}

TEST_F(TestDecoder, Traps_Set_Mcause)
{
    // mcause <- reason for trap
    cpu.pc = 0x8086;
    cpu.mepc = 0;

    // Take a breakpoint.
    Trap trap = {trBREAKPOINT, 0x0};
    HandleTrap(&cpu, trap);

    // mcause <- reason for trap
    ASSERT_EQ(trBREAKPOINT, cpu.mcause);
}

TEST_F(TestDecoder, Traps_Set_Mtval)
{
    // mtval <- exception specific information
    cpu.pc = 0x8086;
    cpu.mepc = 0;
    uint32_t address = 0x1234;

    // Load fault.
    Trap trap = {trLOAD_ACCESS_FAULT, address};
    HandleTrap(&cpu, trap);

    // mtval <- exception specific information
    ASSERT_EQ(address, cpu.mtval);
}
