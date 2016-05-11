;/*********************************************************************************************************
;**
;**                                    中国软件开源组织
;**
;**                                   嵌入式实时操作系统
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------文件信息--------------------------------------------------------------------------------
;**
;** 文   件   名: mipsVfp64Asm.S
;**
;** 创   建   人: Ryan.Xin (信金龙)
;**
;** 文件创建日期: 2015 年 12 月 01 日
;**
;** 描        述: MIPS 体系架构 VFP64 支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if LW_CFG_CPU_FPU_EN > 0

    FILE_BEGIN()

    EXPORT_LABEL(mipsVfp64GetFIR)
    EXPORT_LABEL(mipsVfp64Enable)
    EXPORT_LABEL(mipsVfp64Disable)
    EXPORT_LABEL(mipsVfp64IsEnable)
    EXPORT_LABEL(mipsVfp64Save16)
    EXPORT_LABEL(mipsVfp64Restore16)
    EXPORT_LABEL(mipsVfp64Save32)
    EXPORT_LABEL(mipsVfp64Restore32)

;/*********************************************************************************************************
;  浮点运算器基本操作函数(mipsVfp64Sid 函数先保留)
;*********************************************************************************************************/

FUNC_DEF(mipsVfp64Sid)
    LA      V0 , 0
    JR      RA
    NOP
FUNC_END(mipsVfp64Sid)

FUNC_DEF(mipsVfp64GetFIR)
    MFC0(T1, CP0_STATUS)
    MOV     T0 , T1
    OR      T0 , M_StatusCU1
    MTC0(T0, CP0_STATUS)
    CFC1(T0,  CP1_FIR)
    AND     T0 , M_FIRConfigF64
    SRL     T0 , S_FIRConfigF64
    MOV     V0 , T0
    MTC0(T1, CP0_STATUS)
    JR      RA
    NOP
FUNC_END(mipsVfp64GetFIR)

FUNC_DEF(mipsVfp64Enable)
    MFC0(T0, CP0_STATUS)
    OR      T0 , M_StatusCU1
    MTC0(T0, CP0_STATUS)
    JR      RA
    NOP
FUNC_END(mipsVfp64Enable)

FUNC_DEF(mipsVfp64Disable)
    MFC0(T0, CP0_STATUS)
    AND     T0 , ~M_StatusCU1
    MTC0(T0, CP0_STATUS)
    JR      RA
    NOP
FUNC_END(mipsVfp64Disable)

FUNC_DEF(mipsVfp64IsEnable)
    MOV     V0 , ZERO                                                   ;/*  OK return value             */
    MFC0(T0, CP0_STATUS)
    AND     T0 , M_StatusCU1                                            ;/*  coprocessor 1 enabled       */
    BEQ     T0 , ZERO, 2f
    NOP
    LI      V0 , 1
2:
    JR      RA
    NOP
FUNC_END(mipsVfp64IsEnable)

;/*********************************************************************************************************
;  VFP-D16
;   +-----------+
;   | freg[31]  |    + 0x80
;   |  ...      |
;   | freg[2]   |    + 0x14
;   | freg[1]   |    + 0x0C
;   | freg[0]   |    + 0x04
;   | fpscr     | <-- cpu_fpu_context ( = a0 )
;   +-----------+
;*********************************************************************************************************/

;/*********************************************************************************************************
;  MIPSVFP 保存/恢复寄存器 32Bit MIPS 使用偶数的寄存器
;*********************************************************************************************************/

FUNC_DEF(mipsVfp64Save16)
    MFC0(T0, CP0_STATUS)
    OR      T0   ,   M_StatusCU1
    MTC0(T0, CP0_STATUS)
    CFC1(T0,  CP1_STATUS)
    SW      T0   ,   0x00(A0)
    SWC1    FP0  ,   0x04(A0)
    SWC1    FP2  ,   0x0c(A0)
    SWC1    FP4  ,   0x14(A0)
    SWC1    FP6  ,   0x1c(A0)
    SWC1    FP8  ,   0x24(A0)
    SWC1    FP10 ,   0x2c(A0)
    SWC1    FP12 ,   0x34(A0)
    SWC1    FP14 ,   0x3c(A0)
    SWC1    FP16 ,   0x44(A0)
    SWC1    FP18 ,   0x4c(A0)
    SWC1    FP20 ,   0x54(A0)
    SWC1    FP22 ,   0x5c(A0)
    SWC1    FP24 ,   0x64(A0)
    SWC1    FP26 ,   0x6c(A0)
    SWC1    FP28 ,   0x74(A0)
    SWC1    FP30 ,   0x7c(A0)
    JR      RA
    LI      V0   ,     0
FUNC_END(mipsVfp64Save16)

FUNC_DEF(mipsVfp64Restore16)
    MFC0(T0, CP0_STATUS)
    OR      T0   ,   M_StatusCU1
    MTC0(T0, CP0_STATUS)
    LW      T0   ,   0X00(A0)
    LWC1    FP0  ,   0x04(A0)
    LWC1    FP2  ,   0x0c(A0)
    LWC1    FP4  ,   0x14(A0)
    LWC1    FP6  ,   0x1c(A0)
    LWC1    FP8  ,   0x24(A0)
    LWC1    FP10 ,   0x2c(A0)
    LWC1    FP12 ,   0x34(A0)
    LWC1    FP14 ,   0x3c(A0)
    LWC1    FP16 ,   0x44(A0)
    LWC1    FP18 ,   0x4c(A0)
    LWC1    FP20 ,   0x54(A0)
    LWC1    FP22 ,   0x5c(A0)
    LWC1    FP24 ,   0x64(A0)
    LWC1    FP26 ,   0x6c(A0)
    LWC1    FP28 ,   0x74(A0)
    LWC1    FP30 ,   0x7c(A0)
    CTC1(T0,     CP1_STATUS)                                            ;/*  restore fpp status reg      */
    JR      RA
    LI      V0   ,   0
FUNC_END(mipsVfp64Restore16)

;/*********************************************************************************************************
;  MIPSVFP32 保存/恢复寄存器 64Bit MIPS 使用32的寄存器
;*********************************************************************************************************/

FUNC_DEF(mipsVfp64Save32)
    MFC0(T0, CP0_STATUS)
    OR      T0   ,   M_StatusCU1
    MTC0(T0, CP0_STATUS)
    CFC1(T0,  CP1_STATUS)
    SW      T0   ,   0x00(A0)
    SWC1    FP0  ,   0x04(A0)
    SWC1    FP1  ,   0x0c(A0)
    SWC1    FP2  ,   0x14(A0)
    SWC1    FP3  ,   0x1c(A0)
    SWC1    FP4  ,   0x24(A0)
    SWC1    FP5  ,   0x2c(A0)
    SWC1    FP6  ,   0x34(A0)
    SWC1    FP7  ,   0x3c(A0)
    SWC1    FP8  ,   0x44(A0)
    SWC1    FP9  ,   0x4c(A0)
    SWC1    FP10 ,   0x54(A0)
    SWC1    FP11 ,   0x5c(A0)
    SWC1    FP12 ,   0x64(A0)
    SWC1    FP13 ,   0x6c(A0)
    SWC1    FP14 ,   0x74(A0)
    SWC1    FP15 ,   0x7c(A0)
    SWC1    FP16 ,   0x84(A0)
    SWC1    FP17 ,   0x8c(A0)
    SWC1    FP18 ,   0x94(A0)
    SWC1    FP19 ,   0x9c(A0)
    SWC1    FP20 ,   0xa4(A0)
    SWC1    FP21 ,   0xac(A0)
    SWC1    FP22 ,   0xb4(A0)
    SWC1    FP23 ,   0xbc(A0)
    SWC1    FP24 ,   0xc4(A0)
    SWC1    FP25 ,   0xcc(A0)
    SWC1    FP26 ,   0xd4(A0)
    SWC1    FP27 ,   0xdc(A0)
    SWC1    FP28 ,   0xe4(A0)
    SWC1    FP29 ,   0xec(A0)
    SWC1    FP30 ,   0xf4(A0)
    SWC1    FP31 ,   0xfc(A0)
    JR      RA
    LI      V0   ,   0
FUNC_END(mipsVfp64Save32)

FUNC_DEF(mipsVfp64Restore32)
    MFC0(T0, CP0_STATUS)
    OR      T0   ,   M_StatusCU1
    MTC0(T0, CP0_STATUS)
    LW      T0   ,   0X00(A0)
    LWC1    FP0  ,   0x04(A0)
    LWC1    FP1  ,   0x0c(A0)
    LWC1    FP2  ,   0x14(A0)
    LWC1    FP3  ,   0x1c(A0)
    LWC1    FP4  ,   0x24(A0)
    LWC1    FP5  ,   0x2c(A0)
    LWC1    FP6  ,   0x34(A0)
    LWC1    FP7  ,   0x3c(A0)
    LWC1    FP8  ,   0x44(A0)
    LWC1    FP9  ,   0x4c(A0)
    LWC1    FP10 ,   0x54(A0)
    LWC1    FP11 ,   0x5c(A0)
    LWC1    FP12 ,   0x64(A0)
    LWC1    FP13 ,   0x6c(A0)
    LWC1    FP14 ,   0x74(A0)
    LWC1    FP15 ,   0x7c(A0)
    LWC1    FP16 ,   0x84(A0)
    LWC1    FP17 ,   0x8c(A0)
    LWC1    FP18 ,   0x94(A0)
    LWC1    FP19 ,   0x9c(A0)
    LWC1    FP20 ,   0xa4(A0)
    LWC1    FP21 ,   0xac(A0)
    LWC1    FP22 ,   0xb4(A0)
    LWC1    FP23 ,   0xbc(A0)
    LWC1    FP24 ,   0xc4(A0)
    LWC1    FP25 ,   0xcc(A0)
    LWC1    FP26 ,   0xd4(A0)
    LWC1    FP27 ,   0xdc(A0)
    LWC1    FP28 ,   0xe4(A0)
    LWC1    FP29 ,   0xec(A0)
    LWC1    FP30 ,   0xf4(A0)
    LWC1    FP31 ,   0xfc(A0)
    CTC1(T0,     CP1_STATUS)                                            ;/*  restore fpp status reg      */
    JR      RA
    LI      V0   ,   0
FUNC_END(mipsVfp64Restore32)

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
