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
** 文   件   名: mipsCp0.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 10 月 13 日
**
** 描        述: MIPS CP0 函数库.
*********************************************************************************************************/
#ifndef MIPSCP0_H_
#define MIPSCP0_H_

#include "arch/mips/arch_def.h"

#define mipsCp0RegRead(name, reg) \
static LW_INLINE UINT32  mipsCp0##name##Read (VOID) \
{\
    UINT32 uiValue;\
    asm volatile("mfc0   %0, "reg   : "=r"(uiValue));\
    asm volatile("ehb"); \
    return  (uiValue);\
}

#define mipsCp0RegWrite(name, reg) \
static LW_INLINE VOID  mipsCp0##name##Write (UINT32  uiValue) \
{\
    asm volatile("mtc0   %0, "reg   : : "r"(uiValue));\
    asm volatile("ehb"); \
}

#define mipsCp0RegReadWrite(name, reg)\
mipsCp0RegRead(name, reg) \
\
mipsCp0RegWrite(name, reg)

mipsCp0RegReadWrite(Status, CP0_STATUS)
mipsCp0RegReadWrite(Cause, CP0_CAUSE)
mipsCp0RegReadWrite(EPC, CP0_ERRPC)
mipsCp0RegReadWrite(BadVAddr, CP0_BADVADDR)
mipsCp0RegReadWrite(Count, CP0_COUNT)
mipsCp0RegReadWrite(Compare, CP0_COMPARE)
mipsCp0RegReadWrite(PRId, CP0_PRID)
mipsCp0RegReadWrite(Config, CP0_CONFIG)
mipsCp0RegReadWrite(EBase, CP0_EBASE)
mipsCp0RegReadWrite(IntCtl, CP0_INTCTL)
mipsCp0RegReadWrite(LLAddr, CP0_LLADDR)

mipsCp0RegReadWrite(Index, CP0_INX)
mipsCp0RegReadWrite(Wired, CP0_WIRED)
mipsCp0RegReadWrite(Random, CP0_RAND)
mipsCp0RegReadWrite(EntryLo0, CP0_TLBLO0)
mipsCp0RegReadWrite(EntryLo1, CP0_TLBLO1)
mipsCp0RegReadWrite(Context, CP0_CTXT)
mipsCp0RegReadWrite(PageMask, CP0_PAGEMASK)
mipsCp0RegReadWrite(EntryHi, CP0_TLBHI)

#endif                                                                  /*  MIPSCP0_H_                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
