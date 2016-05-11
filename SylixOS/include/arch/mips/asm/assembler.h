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
  MIPS32 / MIPS64
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

#ifndef __MP_CFG_H
#include "../SylixOS/config/mp/mp_cfg.h"
#endif

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
#define ORI             ori
#define XOR             xor
#define XORI            xori
#define JR              jr
#define MOV             move
#define JALR            jalr
#define JAL             jal
#define BNE             bne
#define BAL             bal
#define BLT             blt
#define BLEZ            blez
#define BEQ             beq

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
    mtc0    src , dst;                  \
    ehb

#define MFC0(dst, src)                  \
    mfc0    dst , src;                  \
    ehb

#define CTC1(src, dst)                  \
    ctc1    src , dst;                  \
    ehb

#define CFC1(dst, src)                  \
    cfc1    dst , src;                  \
    ehb

#endif                                                                  /*  __ASSEMBLY__                */

#endif                                                                  /*  __ASMMIPS_ASSEMBLER_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
