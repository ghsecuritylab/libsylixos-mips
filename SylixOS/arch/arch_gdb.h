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
** 文   件   名: arch_gdb.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 05 月 22 日
**
** 描        述: SylixOS 体系构架 GDB 调试接口.
*********************************************************************************************************/

#ifndef __ARCH_GDB_H
#define __ARCH_GDB_H

#include "config/cpu/cpu_cfg.h"

#if (defined LW_CFG_CPU_ARCH_ARM)
#include "./arm/arm_gdb.h"

#elif (defined LW_CFG_CPU_ARCH_X86)
#include "./x86/x86_gdb.h"

#elif (defined LW_CFG_CPU_ARCH_MIPS)
#include "./mips/mips_gdb.h"

#elif (defined LW_CFG_CPU_ARCH_PPC)
#include "./ppc/ppc_gdb.h"
#endif                                                                  /*  LW_CFG_CPU_ARCH_ARM         */

/*********************************************************************************************************
  寄存器集合结构
*********************************************************************************************************/
typedef struct {
    INT         GDBR_iRegCnt;                                           /* 寄存器数量                   */
    struct {
        ULONG   GDBRA_ulValue;                                          /* 寄存器值                     */
    } regArr[GDB_MAX_REG_CNT];                                          /* 寄存器数组                   */
} GDB_REG_SET;

/*********************************************************************************************************
  Xfer:features:read:target.xml 与 Xfer:features:read:arch-core.xml 回应包
*********************************************************************************************************/
CPCHAR  archGdbTargetXml(VOID);

CPCHAR  archGdbCoreXml(VOID);

/*********************************************************************************************************
  gdb 需要的和体系结构相关的功能
*********************************************************************************************************/
INT     archGdbRegsGet(PVOID               pvDtrace,
                       LW_OBJECT_HANDLE    ulThread,
                       GDB_REG_SET        *pregset);                    /*  获取系统寄存器信息          */

INT     archGdbRegsSet(PVOID               pvDtrace,
                       LW_OBJECT_HANDLE    ulThread,
                       GDB_REG_SET        *pregset);                    /*  设置系统寄存器信息          */

INT     archGdbRegSetPc(PVOID              pvDtrace,
                        LW_OBJECT_HANDLE   ulThread,
                        ULONG              uiPc);                       /*  设置 pc 寄存器              */

ULONG   archGdbRegGetPc (GDB_REG_SET *pRegs);                           /*  获取 pc 寄存器值            */

ULONG   archGdbGetNextPc(PVOID pvDtrace,
                         LW_OBJECT_HANDLE ulThread,
                         GDB_REG_SET *pRegs);                           /*  获取下一个 pc 值，含分支预测*/

#endif                                                                  /*  __ARCH_GDB_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
