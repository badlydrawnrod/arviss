// Based on Table 24.1 (or whatever it is in the latest spec) we could decode opcodes from inst[6:2] as follows:
struct
{
    void (*Decoder)(CPU*);
} opcodes[] = {
        /* inst[4:2]                                                   //
        //////////////////////////////////////////////////////////////////////////////
        000     001      010    011      100     101    110      111   // inst[6:5] */
        Load,   LoadFp,  NULL,  MiscMem, OpImm,  AuiPc, OpImm32, NULL, // 00
        Store,  StoreFp, NULL,  Amo,     Op,     Lui,   Op32,    NULL, // 01
        Madd,   Msub,    Nmsub, Nmadd,   OpFp,   NULL,  NULL,    NULL, // 10
        Branch, Jalr,    NULL,  Jal,     System, NULL,  NULL,    NULL  // 11
};
