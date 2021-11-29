# From: https://stackoverflow.com/questions/7204805/how-to-merge-dictionaries-of-dictionaries
def merge(a, b, path=None):
    if path is None: path = []
    for key in b:
        if key in a:
            if isinstance(a[key], dict) and isinstance(b[key], dict):
                merge(a[key], b[key], path + [str(key)])
            elif a[key] == b[key]:
                pass  # same leaf value
            else:
                raise Exception('Conflict at %s' % '.'.join(path + [str(key)]))
        else:
            a[key] = b[key]
    return a


def walk(d, level=0):
    indent = "    " * level
    for k, v in sorted(d.items()):
        hi, lo = k
        print(f"{indent}switch (bits({hi}, {lo}))")
        print(f"{indent}{{")
        for c in sorted(v.keys()):
            print(f"{indent}case 0x{c:02x}:")
            content = v[c]
            if isinstance(content, dict):
                walk(v[c], level + 1)
            else:
                print(f"{indent}    // {content[0]}")
                opcode = "EXEC_" + content[0].upper().replace('.', '_')
                print(f"{indent}    return mk_{'_'.join(content[1])}({opcode}, {', '.join(content[1])});")
        print(f"{indent}default:")
        print(f"{indent}    // Illegal instruction.")
        print(f"{indent}    return mk_trap(EXEC_ILLEGAL_INSTRUCTION, ins);")
        print(f"{indent}}}")


# See: https://raw.githubusercontent.com/riscv/riscv-opcodes/master/opcodes-rv32i (etc)
lines = """\
# format of a line in this file:
# <instruction name> <args> <opcode>
#
# <opcode> is given by specifying one or more range/value pairs:
# hi..lo=value or bit=value or arg=value (e.g. 6..2=0x45 10=1 rd=0)
#
# <args> is one of rd, rs1, rs2, rs3, imm20, imm12, imm12lo, imm12hi,
# shamtw, shamt, rm

beq     bimm12hi rs1 rs2 bimm12lo 14..12=0 6..2=0x18 1..0=3
bne     bimm12hi rs1 rs2 bimm12lo 14..12=1 6..2=0x18 1..0=3
blt     bimm12hi rs1 rs2 bimm12lo 14..12=4 6..2=0x18 1..0=3
bge     bimm12hi rs1 rs2 bimm12lo 14..12=5 6..2=0x18 1..0=3
bltu    bimm12hi rs1 rs2 bimm12lo 14..12=6 6..2=0x18 1..0=3
bgeu    bimm12hi rs1 rs2 bimm12lo 14..12=7 6..2=0x18 1..0=3

jalr    rd rs1 imm12              14..12=0 6..2=0x19 1..0=3

jal     rd jimm20                          6..2=0x1b 1..0=3

lui     rd imm20 6..2=0x0D 1..0=3
auipc   rd imm20 6..2=0x05 1..0=3

addi    rd rs1 imm12           14..12=0 6..2=0x04 1..0=3
slti    rd rs1 imm12           14..12=2 6..2=0x04 1..0=3
sltiu   rd rs1 imm12           14..12=3 6..2=0x04 1..0=3
xori    rd rs1 imm12           14..12=4 6..2=0x04 1..0=3
ori     rd rs1 imm12           14..12=6 6..2=0x04 1..0=3
andi    rd rs1 imm12           14..12=7 6..2=0x04 1..0=3

add     rd rs1 rs2 31..25=0  14..12=0 6..2=0x0C 1..0=3
sub     rd rs1 rs2 31..25=32 14..12=0 6..2=0x0C 1..0=3
sll     rd rs1 rs2 31..25=0  14..12=1 6..2=0x0C 1..0=3
slt     rd rs1 rs2 31..25=0  14..12=2 6..2=0x0C 1..0=3
sltu    rd rs1 rs2 31..25=0  14..12=3 6..2=0x0C 1..0=3
xor     rd rs1 rs2 31..25=0  14..12=4 6..2=0x0C 1..0=3
srl     rd rs1 rs2 31..25=0  14..12=5 6..2=0x0C 1..0=3
sra     rd rs1 rs2 31..25=32 14..12=5 6..2=0x0C 1..0=3
or      rd rs1 rs2 31..25=0  14..12=6 6..2=0x0C 1..0=3
and     rd rs1 rs2 31..25=0  14..12=7 6..2=0x0C 1..0=3

lb      rd rs1       imm12 14..12=0 6..2=0x00 1..0=3
lh      rd rs1       imm12 14..12=1 6..2=0x00 1..0=3
lw      rd rs1       imm12 14..12=2 6..2=0x00 1..0=3
lbu     rd rs1       imm12 14..12=4 6..2=0x00 1..0=3
lhu     rd rs1       imm12 14..12=5 6..2=0x00 1..0=3

sb     imm12hi rs1 rs2 imm12lo 14..12=0 6..2=0x08 1..0=3
sh     imm12hi rs1 rs2 imm12lo 14..12=1 6..2=0x08 1..0=3
sw     imm12hi rs1 rs2 imm12lo 14..12=2 6..2=0x08 1..0=3

fence       fm            pred succ     rs1 14..12=0 rd 6..2=0x03 1..0=3
fence.i     imm12                       rs1 14..12=1 rd 6..2=0x03 1..0=3

# shifts
slli rd rs1 31..25=0  shamtw  14..12=1 6..2=0x04 1..0=3
srli rd rs1 31..25=0  shamtw  14..12=5 6..2=0x04 1..0=3
srai rd rs1 31..25=32 shamtw  14..12=5 6..2=0x04 1..0=3

# rv32m
mul     rd rs1 rs2 31..25=1 14..12=0 6..2=0x0C 1..0=3
mulh    rd rs1 rs2 31..25=1 14..12=1 6..2=0x0C 1..0=3
mulhsu  rd rs1 rs2 31..25=1 14..12=2 6..2=0x0C 1..0=3
mulhu   rd rs1 rs2 31..25=1 14..12=3 6..2=0x0C 1..0=3
div     rd rs1 rs2 31..25=1 14..12=4 6..2=0x0C 1..0=3
divu    rd rs1 rs2 31..25=1 14..12=5 6..2=0x0C 1..0=3
rem     rd rs1 rs2 31..25=1 14..12=6 6..2=0x0C 1..0=3
remu    rd rs1 rs2 31..25=1 14..12=7 6..2=0x0C 1..0=3

# rv32f

fadd.s    rd rs1 rs2      31..27=0x00 rm       26..25=0 6..2=0x14 1..0=3
fsub.s    rd rs1 rs2      31..27=0x01 rm       26..25=0 6..2=0x14 1..0=3
fmul.s    rd rs1 rs2      31..27=0x02 rm       26..25=0 6..2=0x14 1..0=3
fdiv.s    rd rs1 rs2      31..27=0x03 rm       26..25=0 6..2=0x14 1..0=3
fsgnj.s   rd rs1 rs2      31..27=0x04 14..12=0 26..25=0 6..2=0x14 1..0=3
fsgnjn.s  rd rs1 rs2      31..27=0x04 14..12=1 26..25=0 6..2=0x14 1..0=3
fsgnjx.s  rd rs1 rs2      31..27=0x04 14..12=2 26..25=0 6..2=0x14 1..0=3
fmin.s    rd rs1 rs2      31..27=0x05 14..12=0 26..25=0 6..2=0x14 1..0=3
fmax.s    rd rs1 rs2      31..27=0x05 14..12=1 26..25=0 6..2=0x14 1..0=3
fsqrt.s   rd rs1 24..20=0 31..27=0x0B rm       26..25=0 6..2=0x14 1..0=3

fle.s     rd rs1 rs2      31..27=0x14 14..12=0 26..25=0 6..2=0x14 1..0=3
flt.s     rd rs1 rs2      31..27=0x14 14..12=1 26..25=0 6..2=0x14 1..0=3
feq.s     rd rs1 rs2      31..27=0x14 14..12=2 26..25=0 6..2=0x14 1..0=3

fcvt.w.s  rd rs1 24..20=0 31..27=0x18 rm       26..25=0 6..2=0x14 1..0=3
fcvt.wu.s rd rs1 24..20=1 31..27=0x18 rm       26..25=0 6..2=0x14 1..0=3
fmv.x.w   rd rs1 24..20=0 31..27=0x1C 14..12=0 26..25=0 6..2=0x14 1..0=3
fclass.s  rd rs1 24..20=0 31..27=0x1C 14..12=1 26..25=0 6..2=0x14 1..0=3

fcvt.s.w  rd rs1 24..20=0 31..27=0x1A rm       26..25=0 6..2=0x14 1..0=3
fcvt.s.wu rd rs1 24..20=1 31..27=0x1A rm       26..25=0 6..2=0x14 1..0=3
fmv.w.x   rd rs1 24..20=0 31..27=0x1E 14..12=0 26..25=0 6..2=0x14 1..0=3

flw       rd rs1 imm12 14..12=2 6..2=0x01 1..0=3

fsw       imm12hi rs1 rs2 imm12lo 14..12=2 6..2=0x09 1..0=3

fmadd.s   rd rs1 rs2 rs3 rm 26..25=0 6..2=0x10 1..0=3
fmsub.s   rd rs1 rs2 rs3 rm 26..25=0 6..2=0x11 1..0=3
fnmsub.s  rd rs1 rs2 rs3 rm 26..25=0 6..2=0x12 1..0=3
fnmadd.s  rd rs1 rs2 rs3 rm 26..25=0 6..2=0x13 1..0=3
"""

operands = {"rd", "rs1", "rs2", "rs3", "bimm12hi", "bimm12lo", "imm12hi", "imm12lo", "imm12", "jimm20", "imm20", "fm",
            "pred", "succ", "rm", "shamtw", "shamt"}

if __name__ == "__main__":
    dispatch = {}

    for line in lines.split('\n'):
        line = line.strip()
        if len(line) > 0 and line[0] != "#":
            ins, *rest = line.split()
            t_ops = []
            t_operands = []
            for arg in rest:
                if arg in operands:
                    operand = arg
                    t_operands.append(operand)
                else:
                    left, right = arg.split("=")
                    right = int(right, 0)
                    if ".." in left:
                        hi, lo = left.split("..")
                    else:
                        hi, lo = left, left
                    hi, lo = int(hi), int(lo)
                    t_ops.append((hi, lo, right))
            t_ops.sort()
            result = (ins, sorted(t_operands))
            for hi, lo, left in reversed(t_ops):
                d = {}
                d[(hi, lo)] = {left: result}
                result = d
            merge(dispatch, result)

    walk(dispatch)
