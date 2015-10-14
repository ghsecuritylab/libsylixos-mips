/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: assembler.h
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 09 月 01 日
**
** 描        述: MIPS 汇编相关.
*********************************************************************************************************/

#ifndef __ASMMIPS_ASSEMBLER_H
#define __ASMMIPS_ASSEMBLER_H

/*********************************************************************************************************
  定义 MIPS 架构是大/小端存储
*********************************************************************************************************/

#if defined(MIPSEB) || defined(__MIPSEB__)
#undef __SYLIXOS_MIPS_BYTE_ORDER
#define __SYLIXOS_MIPS_BYTE_ORDER       _BIG_ENDIAN
#elif defined(MIPSEL) || defined(__MIPSEL__)
#undef __SYLIXOS_MIPS_BYTE_ORDER
#define __SYLIXOS_MIPS_BYTE_ORDER       _LITTLE_ENDIAN
#else
#warning "One of MIPSEL or MIPSEB must be defined"
#endif

/*********************************************************************************************************
  定义 MIPS32/MIPS64，暂时先不用
*********************************************************************************************************/

#define __SYLIXOS_CPU_MIPS_32BIT ((__SYLIXOS_CPU==__SYLIXOS_MIPS32) || \
                (__SYLIXOS_CPU==__SYLIXOS_MIPSI2) || \
                (__SYLIXOS_CPU==__SYLIXOS_MIPSI32) || \
                (__SYLIXOS_CPU==__SYLIXOS_MIPSI32R2))

#define __SYLIXOS_CPU_MIPS_64BIT ((__SYLIXOS_CPU==__SYLIXOS_MIPS64) || \
                (__SYLIXOS_CPU==__SYLIXOS_MIPSI3) || \
                (__SYLIXOS_CPU==__SYLIXOS_MIPSI64) || \
                (__SYLIXOS_CPU==__SYLIXOS_MIPSI64R2))

/*********************************************************************************************************
  包含定义
*********************************************************************************************************/

#include "arch/mips/arch_def.h"

/*********************************************************************************************************
  mips architecture assembly special code
*********************************************************************************************************/

#if defined(__ASSEMBLY__) || defined(ASSEMBLY)

/*********************************************************************************************************
  assembler define
*********************************************************************************************************/

#define EXPORT_LABEL(label)       .global label

#define IMPORT_LABEL(label)       .extern label

#define FUNC_LABEL(func)          func:
#define LINE_LABEL(line)          line:

#define FUNC_DEF(name)                  \
        .text;                          \
        .balign     4;                  \
        .type       symbol, @function;  \
        .ent        name;               \
name:

#define FUNC_END(name)                  \
        .size       name,.-name;        \
        .end        name

#define MACRO_DEF(mfunc...)             \
        .macro      mfunc

#define MACRO_DEF(mfunc...)             \
        .macro      mfunc

#define MACRO_END()                     \
        .endm

#define FILE_BEGIN()                    \
        .set        noreorder;          \
        .balign     4;

#define FILE_END()

#define SECTION(sec)                    \
        .section    sec

#define WEAK(name)                      \
        .weakext    name;               \
        .balign     4;

/*********************************************************************************************************
  MIPS Use the following macros in assembler code
*********************************************************************************************************/

#define LA              la
#define LI              li
#define SW              sw
#define LW              lw
#define ADD             add
#define ADDU            addu
#define ADDI            addi
#define ADDIU           addiu
#define SUB             sub
#define SUBU            subu
#define SLL             sll
#define SLLV            sllv
#define SRL             srl
#define SRLV            srlv
#define SRA             sra
#define SRAV            srav
#define AND             and
#define OR              or
#define XOR             xor
#define JR              jr
#define MOV             move
#define JALR            jalr
#define JAL             jal
#define BNE             bne
#define BAL             bal
#define BLT             blt
#define BLEZ            blez

#define LONG            .word
#define LONGSIZE        4
#define LONGMASK        3
#define LONGLOG         2

#define PTR             .word
#define PTR_SCALESHIFT  2
#define PTRSIZE         4
#define PTRLOG          2

#define SSNOP           SLL ZER0, ZER0, 1
#define NOPS            SSNOP; SSNOP; SSNOP; SSNOP
#define NOP             nop
#define ERET            eret

#define MTC0(src, dst)                  \
    mtc0    src, dst;                   \
    ehb

#define MFC0(dst, src)                  \
    mfc0    dst, src;                   \
    ehb

/*********************************************************************************************************
  Operate the general registers: at, v0-v1, a0-a3, t0-t9, s0-s7, gp, fp, ra
*********************************************************************************************************/

#define OPERATE_REG(op)                 \
    op      RA, STK_OFFSET_RA(SP);      \
    op      FP, STK_OFFSET_FP(SP);      \
    op      GP, STK_OFFSET_GP(SP);      \
    op      S7, STK_OFFSET_S7(SP);      \
    op      S6, STK_OFFSET_S6(SP);      \
    op      S5, STK_OFFSET_S5(SP);      \
    op      S4, STK_OFFSET_S4(SP);      \
    op      S3, STK_OFFSET_S3(SP);      \
    op      S2, STK_OFFSET_S2(SP);      \
    op      S1, STK_OFFSET_S1(SP);      \
    op      S0, STK_OFFSET_S0(SP);      \
    op      T9, STK_OFFSET_T9(SP);      \
    op      T8, STK_OFFSET_T8(SP);      \
    op      T7, STK_OFFSET_T7(SP);      \
    op      T6, STK_OFFSET_T6(SP);      \
    op      T5, STK_OFFSET_T5(SP);      \
    op      T4, STK_OFFSET_T4(SP);      \
    op      T3, STK_OFFSET_T3(SP);      \
    op      T2, STK_OFFSET_T2(SP);      \
    op      T1, STK_OFFSET_T1(SP);      \
    op      T0, STK_OFFSET_T0(SP);      \
    op      A3, STK_OFFSET_A3(SP);      \
    op      A2, STK_OFFSET_A2(SP);      \
    op      A1, STK_OFFSET_A1(SP);      \
    op      A0, STK_OFFSET_A0(SP);      \
    op      V1, STK_OFFSET_V1(SP);      \
    op      V0, STK_OFFSET_V0(SP);      \
    op      AT, STK_OFFSET_AT(SP);

/*********************************************************************************************************
  Pop the context (normal execution): at, v0-v1, a0-a3, t0-t9, s0-s7, gp, fp, ra, pc
*********************************************************************************************************/

#define RESTORE_REG_RET()               \
    .set    push;                       \
    .set    noat;                       \
    .set    noreorder;                  \
                                        \
    OPERATE_REG(LW);                    \
                                        \
    LW      K0, STK_OFFSET_SR(SP);      \
    MTC0(K0, CP0_STATUS);               \
                                        \
    LW      K0, STK_OFFSET_EPC(SP);     \
                                        \
    ADDU    SP, STK_CTX_SIZE;           \
                                        \
    JR      K0;                         \
    NOP;                                \
                                        \
    .set    pop

/*********************************************************************************************************
  Pop the context: at, v0-v1, a0-a3, t0-t9, s0-s7, gp, fp, ra, pc
*********************************************************************************************************/

#define RESTORE_REG_ERET()              \
    .set    push;                       \
    .set    noat;                       \
    .set    noreorder;                  \
                                        \
    OPERATE_REG(LW);                    \
                                        \
    LW      K0, STK_OFFSET_SR(SP);      \
    MTC0(K0, CP0_STATUS);               \
                                        \
    LW      K0, STK_OFFSET_EPC(SP);     \
    MTC0(K0, CP0_EPC);                  \
                                        \
    ADDU    SP, STK_CTX_SIZE;           \
                                        \
    ERET;                               \
    NOP;                                \
                                        \
    .set    pop

/*********************************************************************************************************
  Push the context: at, v0-v1, a0-a3, t0-t9, s0-s7, gp, fp, ra, pc
*********************************************************************************************************/

#define STORE_REG_RET(RetAddr)          \
    .set    push;                       \
    .set    noat;                       \
    .set    noreorder;                  \
                                        \
    SUBU    SP, STK_CTX_SIZE;           \
                                        \
    OPERATE_REG(SW);                    \
                                        \
    MFC0(T0, CP0_STATUS);               \
    SW      T0, STK_OFFSET_SR(SP);      \
                                        \
    SW      RetAddr, STK_OFFSET_EPC(SP);\
                                        \
    .set    pop

/*********************************************************************************************************
  TLB refill, 32 bit task
*********************************************************************************************************/

#define MIPS32_TLB_REFILL_HANDLE()      \
    .set    push;                       \
    .set    noat;                       \
    .set    noreorder;                  \
                                        \
    MFC0    K1, CP0_CTXT;               \
    LW      K0, 0(K1);                  \
    LW      K1, 8(K1);                  \
    MTC0    K0, CP0_TLBLO0;             \
    MTC0    K1, CP0_TLBLO1;             \
    EHB;                                \
    TLBWR;                              \
    ERET;                               \
                                        \
    .set    pop

/*********************************************************************************************************
  Cache error exception
*********************************************************************************************************/

#define MIPS_CACHE_ERROR_HANDLE()       \
    .set    push;                       \
    .set    noat;                       \
    .set    noreorder;                  \
                                        \
    LA      K0 , archCacheErrorEntry;   \
    JR      K0;                         \
    NOP;                                \
                                        \
    .set    pop

/*********************************************************************************************************
  General exception
*********************************************************************************************************/

#define MIPS_EXCEPTION_HANDLE()         \
    .set    push;                       \
    .set    noat;                       \
    .set    noreorder;                  \
                                        \
    LA      K0 , archExceptionEntry;    \
    JR      K0;                         \
    NOP;                                \
                                        \
    .set    pop

/*********************************************************************************************************
  Catch interrupt exceptions
*********************************************************************************************************/

#define MIPS_INTERRUPT_HANDLE()         \
    .set    push;                       \
    .set    noat;                       \
    .set    noreorder;                  \
                                        \
    LA      K0 , archInterruptEntry;    \
    JR      K0;                         \
    NOP;                                \
                                        \
    .set    pop

#endif                                                                  /*  __ASSEMBLY__                */

#endif                                                                  /*  __ASMMIPS_ASSEMBLER_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/

