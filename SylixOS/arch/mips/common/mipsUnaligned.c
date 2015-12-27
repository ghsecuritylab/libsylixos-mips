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
** 文   件   名: mipsUnaligned.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 12 月 25 日
**
** 描        述: MIPS 非对齐处理.
*********************************************************************************************************/
/*
 * Handle unaligned accesses by emulation.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 1998, 1999, 2002 by Ralf Baechle
 * Copyright (C) 1999 Silicon Graphics, Inc.
 *
 * This file contains exception handler for address error exception with the
 * special capability to execute faulting instructions in software.  The
 * handler does not try to handle the case when the program counter points
 * to an address not aligned to a word boundary.
 *
 * Putting data to unaligned addresses is a bad practice even on Intel where
 * only the performance is affected.  Much worse is that such code is non-
 * portable.  Due to several programs that die on MIPS due to alignment
 * problems I decided to implement this handler anyway though I originally
 * didn't intend to do this at all for user code.
 *
 * For now I enable fixing of address errors by default to make life easier.
 * I however intend to disable this somewhen in the future when the alignment
 * problems with user programs have been fixed.  For programmers this is the
 * right way to go.
 *
 * Fixing address errors is a per process option.  The option is inherited
 * across fork(2) and execve(2) calls.  If you really want to use the
 * option in your user programs - I discourage the use of the software
 * emulation strongly - use the following code in your userland stuff:
 *
 * #include <sys/sysmips.h>
 *
 * ...
 * sysmips(MIPS_FIXADE, x);
 * ...
 *
 * The argument x is 0 for disabling software emulation, enabled otherwise.
 *
 * Below a little program to play around with this feature.
 *
 * #include <stdio.h>
 * #include <sys/sysmips.h>
 *
 * struct foo {
 *         unsigned char bar[8];
 * };
 *
 * main(int argc, char *argv[])
 * {
 *         struct foo x = {0, 1, 2, 3, 4, 5, 6, 7};
 *         unsigned int *p = (unsigned int *) (x.bar + 3);
 *         int i;
 *
 *         if (argc > 1)
 *                 sysmips(MIPS_FIXADE, atoi(argv[1]));
 *
 *         printf("*p = %08lx\n", *p);
 *
 *         *p = 0xdeadface;
 *
 *         for(i = 0; i <= 7; i++)
 *         printf("%02x ", x.bar[i]);
 *         printf("\n");
 * }
 *
 * Coprocessor loads are not supported; I think this case is unimportant
 * in the practice.
 *
 * TODO: Handle ndc (attempted store to doubleword in uncached memory)
 *       exception for the R6000.
 *       A store crossing a page boundary might be executed only partially.
 *       Undo the partial store in this case.
 */
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "mipsInst.h"
#include "mipsBranch.h"

#define STR(x)      __STR(x)
#define __STR(x)    #x

#ifdef __GNUC__
#define PTR         .word
#else
#define PTR         TODO
#endif

static int emulate_load_store_insn(ARCH_REG_CTX *regs, void *addr, unsigned int *pc)
{
    union mips_instruction insn;
    unsigned long value;
    unsigned int res;

    /*
     * This load never faults.
     */
    insn.word = *pc;

    switch (insn.i_format.opcode) {
    /*
     * These are instructions that a compiler doesn't generate.  We
     * can assume therefore that the code is MIPS-aware and
     * really buggy.  Emulating these instructions would break the
     * semantics anyway.
     */
    case ll_op:
    case lld_op:
    case sc_op:
    case scd_op:

    /*
     * For these instructions the only way to create an address
     * error is an attempted access to kernel/supervisor address
     * space.
     */
    case ldl_op:
    case ldr_op:
    case lwl_op:
    case lwr_op:
    case sdl_op:
    case sdr_op:
    case swl_op:
    case swr_op:
    case lb_op:
    case lbu_op:
    case sb_op:
        goto sigbus;

    /*
     * The remaining opcodes are the ones that are really of interest.
     */
    case lh_op:
        __asm__ __volatile__ (".set\tnoat\n"
#if BYTE_ORDER == BIG_ENDIAN
            "1:\tlb\t%0, 0(%2)\n"
            "2:\tlbu\t$1, 1(%2)\n\t"
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
            "1:\tlb\t%0, 1(%2)\n"
            "2:\tlbu\t$1, 0(%2)\n\t"
#endif
            "sll\t%0, 0x8\n\t"
            "or\t%0, $1\n\t"
            "li\t%1, 0\n"
            "3:\t.set\tat\n\t"
            ".section\t.fixup,\"ax\"\n\t"
            "4:\tli\t%1, %3\n\t"
            "j\t3b\n\t"
            ".previous\n\t"
            ".section\t__ex_table,\"a\"\n\t"
            STR(PTR)"\t1b, 4b\n\t"
            STR(PTR)"\t2b, 4b\n\t"
            ".previous"
            : "=&r" (value), "=r" (res)
            : "r" (addr), "i" (-EFAULT));
        if (res)
            goto fault;
        compute_return_epc(regs);
        regs->REG_uiReg[insn.i_format.rt] = value;
        break;

    case lw_op:
        __asm__ __volatile__ (
#if BYTE_ORDER == BIG_ENDIAN
            "1:\tlwl\t%0, (%2)\n"
            "2:\tlwr\t%0, 3(%2)\n\t"
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
            "1:\tlwl\t%0, 3(%2)\n"
            "2:\tlwr\t%0, (%2)\n\t"
#endif
            "li\t%1, 0\n"
            "3:\t.section\t.fixup,\"ax\"\n\t"
            "4:\tli\t%1, %3\n\t"
            "j\t3b\n\t"
            ".previous\n\t"
            ".section\t__ex_table,\"a\"\n\t"
            STR(PTR)"\t1b, 4b\n\t"
            STR(PTR)"\t2b, 4b\n\t"
            ".previous"
            : "=&r" (value), "=r" (res)
            : "r" (addr), "i" (-EFAULT));
        if (res)
            goto fault;
        compute_return_epc(regs);
        regs->REG_uiReg[insn.i_format.rt] = value;
        break;

    case lhu_op:
        __asm__ __volatile__ (
            ".set\tnoat\n"
#if BYTE_ORDER == BIG_ENDIAN
            "1:\tlbu\t%0, 0(%2)\n"
            "2:\tlbu\t$1, 1(%2)\n\t"
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
            "1:\tlbu\t%0, 1(%2)\n"
            "2:\tlbu\t$1, 0(%2)\n\t"
#endif
            "sll\t%0, 0x8\n\t"
            "or\t%0, $1\n\t"
            "li\t%1, 0\n"
            "3:\t.set\tat\n\t"
            ".section\t.fixup,\"ax\"\n\t"
            "4:\tli\t%1, %3\n\t"
            "j\t3b\n\t"
            ".previous\n\t"
            ".section\t__ex_table,\"a\"\n\t"
            STR(PTR)"\t1b, 4b\n\t"
            STR(PTR)"\t2b, 4b\n\t"
            ".previous"
            : "=&r" (value), "=r" (res)
            : "r" (addr), "i" (-EFAULT));
        if (res)
            goto fault;
        compute_return_epc(regs);
        regs->REG_uiReg[insn.i_format.rt] = value;
        break;

    case lwu_op:
        /* Cannot handle 64-bit instructions in 32-bit kernel */
        goto sigill;

    case ld_op:
        /* Cannot handle 64-bit instructions in 32-bit kernel */
        goto sigill;

    case sh_op:
        value = regs->REG_uiReg[insn.i_format.rt];
        __asm__ __volatile__ (
#if BYTE_ORDER == BIG_ENDIAN
            ".set\tnoat\n"
            "1:\tsb\t%1, 1(%2)\n\t"
            "srl\t$1, %1, 0x8\n"
            "2:\tsb\t$1, 0(%2)\n\t"
            ".set\tat\n\t"
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
            ".set\tnoat\n"
            "1:\tsb\t%1, 0(%2)\n\t"
            "srl\t$1,%1, 0x8\n"
            "2:\tsb\t$1, 1(%2)\n\t"
            ".set\tat\n\t"
#endif
            "li\t%0, 0\n"
            "3:\n\t"
            ".section\t.fixup,\"ax\"\n\t"
            "4:\tli\t%0, %3\n\t"
            "j\t3b\n\t"
            ".previous\n\t"
            ".section\t__ex_table,\"a\"\n\t"
            STR(PTR)"\t1b, 4b\n\t"
            STR(PTR)"\t2b, 4b\n\t"
            ".previous"
            : "=r" (res)
            : "r" (value), "r" (addr), "i" (-EFAULT));
        if (res)
            goto fault;
        compute_return_epc(regs);
        break;

    case sw_op:
        value = regs->REG_uiReg[insn.i_format.rt];
        __asm__ __volatile__ (
#if BYTE_ORDER == BIG_ENDIAN
            "1:\tswl\t%1,(%2)\n"
            "2:\tswr\t%1, 3(%2)\n\t"
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
            "1:\tswl\t%1, 3(%2)\n"
            "2:\tswr\t%1, (%2)\n\t"
#endif
            "li\t%0, 0\n"
            "3:\n\t"
            ".section\t.fixup,\"ax\"\n\t"
            "4:\tli\t%0, %3\n\t"
            "j\t3b\n\t"
            ".previous\n\t"
            ".section\t__ex_table,\"a\"\n\t"
            STR(PTR)"\t1b, 4b\n\t"
            STR(PTR)"\t2b, 4b\n\t"
            ".previous"
        : "=r" (res)
        : "r" (value), "r" (addr), "i" (-EFAULT));
        if (res)
            goto fault;
        compute_return_epc(regs);
        break;

    case sd_op:
        /* Cannot handle 64-bit instructions in 32-bit kernel */
        goto sigill;

    case lwc1_op:
    case ldc1_op:
    case swc1_op:
    case sdc1_op:
        /*
         * I herewith declare: this does not happen.  So send SIGBUS.
         */
        goto sigbus;

    /*
     * COP2 is available to implementor for application specific use.
     * It's up to applications to register a notifier chain and do
     * whatever they have to do, including possible sending of signals.
     */
    case lwc2_op:
    case ldc2_op:
    case swc2_op:
    case sdc2_op:
    default:
        /*
         * Pheeee...  We encountered an yet unknown instruction or
         * cache coherence problem.  Die sucker, die ...
         */
        goto sigill;
    }

    return 0;

fault:
    return LW_VMM_ABORT_TYPE_TERMINAL;

sigbus:
    return LW_VMM_ABORT_TYPE_BUS;

sigill:
    return LW_VMM_ABORT_TYPE_UNDEF;
}
/*********************************************************************************************************
** 函数名称: mipsUnalignedHandle
** 功能描述: MIPS 非对齐处理
** 输　入  : pregctx           寄存器上下文
**           ulAbortAddr       终止地址
** 输　出  : 终止类型
** 全局变量:
** 调用模块:
*********************************************************************************************************/
ULONG  mipsUnalignedHandle (ARCH_REG_CTX  *pregctx, addr_t  ulAbortAddr)
{
    UINT  *pc;

    /*
     * Did we catch a fault trying to load an instruction?
     * Or are we running in MIPS16 mode?
     */
    if ((ulAbortAddr == pregctx->REG_uiCP0EPC) || (pregctx->REG_uiCP0EPC & 0x1)) {
        goto sigbus;
    }

    pc = (UINT *)exception_epc(pregctx);

    /*
     * Do branch emulation only if we didn't forward the exception.
     * This is all so but ugly ...
     */
    return  (emulate_load_store_insn(pregctx, (PVOID)ulAbortAddr, pc));

sigbus:
    return  (LW_VMM_ABORT_TYPE_BUS);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
