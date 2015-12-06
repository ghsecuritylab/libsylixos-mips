;/**********************************************************************************************************
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
;** 文   件   名: mipsVfp32Asm.S
;**
;** 创   建   人: Ryan.Xin (信金龙)
;**
;** 文件创建日期: 2015 年 11 月 17 日
;**
;** 描        述: MIPS 体系架构 VFP32 支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if LW_CFG_CPU_FPU_EN > 0

    FILE_BEGIN()

    EXPORT_LABEL(mipsVfp32GetFIR)
    EXPORT_LABEL(mipsVfp32Enable)
    EXPORT_LABEL(mipsVfp32Disable)
    EXPORT_LABEL(mipsVfp32IsEnable)
    EXPORT_LABEL(mipsVfp32Save16)
    EXPORT_LABEL(mipsVfp32Restore16)
    EXPORT_LABEL(mipsVfp32Save32)
    EXPORT_LABEL(mipsVfp32Restore32)

;/*********************************************************************************************************
;  浮点运算器基本操作函数(mipsVfp32Sid 函数先保留)
;*********************************************************************************************************/

FUNC_DEF(mipsVfp32Sid)
    LA      V0 , 0
    JR      RA
    NOP
FUNC_END(mipsVfp32Sid)

FUNC_DEF(mipsVfp32GetFIR)
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
FUNC_END(mipsVfp32GetFIR)

FUNC_DEF(mipsVfp32Enable)
    MFC0(T0, CP0_STATUS)
    OR      T0 , M_StatusCU1
    MTC0(T0, CP0_STATUS)
    JR      RA
    NOP
FUNC_END(mipsVfp32Enable)

FUNC_DEF(mipsVfp32Disable)
    MFC0(T0, CP0_STATUS)
    AND     T0 , ~M_StatusCU1
    MTC0(T0, CP0_STATUS)
    JR      RA
    NOP
FUNC_END(mipsVfp32Disable)

FUNC_DEF(mipsVfp32IsEnable)
    MOV     V0 , ZERO                                                   ;/*  OK return value             */
    MFC0(T0, CP0_STATUS)
    AND     T0 , M_StatusCU1                                            ;/*  coprocessor 1 enabled       */
    BEQ     T0 , ZERO, 2f
    NOP
    LI      V0 , 1
2:
    JR      RA
    NOP
FUNC_END(mipsVfp32IsEnable)

;/*********************************************************************************************************
;  VFP-D16
;   +-----------+
;   | freg[31]  |    + 0x54
;   |  ...      |
;   | freg[2]   |    + 0x0C
;   | freg[1]   |    + 0x08
;   | freg[0]   |    + 0x04
;   | fpscr     | <-- cpu_fpu_context ( = a0 )
;   +-----------+
;*********************************************************************************************************/

;/*********************************************************************************************************
;  MIPSVFP 保存/恢复寄存器 32Bit MIPS 使用偶数的寄存器
;*********************************************************************************************************/

FUNC_DEF(mipsVfp32Save16)
    MFC0(T0, CP0_STATUS)
    OR      T0   ,   M_StatusCU1
    MTC0(T0, CP0_STATUS)
    CFC1(T0,  CP1_STATUS)
    SW      T0   ,   0x00(A0)
    SWC1    FP0  ,   0x04(A0)
    SWC1    FP2  ,   0x08(A0)
    SWC1    FP4  ,   0x1c(A0)
    SWC1    FP6  ,   0x20(A0)
    SWC1    FP8  ,   0x24(A0)
    SWC1    FP10 ,   0x28(A0)
    SWC1    FP12 ,   0x2c(A0)
    SWC1    FP14 ,   0x30(A0)
    SWC1    FP16 ,   0x34(A0)
    SWC1    FP18 ,   0x3c(A0)
    SWC1    FP20 ,   0x40(A0)
    SWC1    FP22 ,   0x44(A0)
    SWC1    FP24 ,   0x48(A0)
    SWC1    FP26 ,   0x4c(A0)
    SWC1    FP28 ,   0x50(A0)
    SWC1    FP30 ,   0x54(A0)
    JR      RA
    LI      V0   ,   0
FUNC_END(mipsVfp32Save16)

FUNC_DEF(mipsVfp32Restore16)
    MFC0(T0, CP0_STATUS)
    OR      T0   ,   M_StatusCU1
    MTC0(T0, CP0_STATUS)
    LW      T0   ,   0X00(A0)
    LWC1    FP0  ,   0x04(A0)
    LWC1    FP2  ,   0x08(A0)
    LWC1    FP4  ,   0x1c(A0)
    LWC1    FP6  ,   0x20(A0)
    LWC1    FP8  ,   0x24(A0)
    LWC1    FP10 ,   0x28(A0)
    LWC1    FP12 ,   0x2c(A0)
    LWC1    FP14 ,   0x30(A0)
    LWC1    FP16 ,   0x34(A0)
    LWC1    FP18 ,   0x3c(A0)
    LWC1    FP20 ,   0x40(A0)
    LWC1    FP22 ,   0x44(A0)
    LWC1    FP24 ,   0x48(A0)
    LWC1    FP26 ,   0x4c(A0)
    LWC1    FP28 ,   0x50(A0)
    LWC1    FP30 ,   0x54(A0)
    CTC1(T0,     CP1_STATUS)                                            ;/*  restore fpp status reg      */
    JR      RA
    LI      V0   ,   0
FUNC_END(mipsVfp32Restore16)

;/*********************************************************************************************************
;  MIPSVFP32 保存/恢复寄存器 64Bit MIPS 使用32的寄存器
;*********************************************************************************************************/

FUNC_DEF(mipsVfp32Save32)
    MFC0(T0, CP0_STATUS)
    OR      T0   ,   M_StatusCU1
    MTC0(T0, CP0_STATUS)
    CFC1(T0,  CP1_STATUS)
    SW      T0   ,   0x00(A0)
    SWC1    FP0  ,   0x04(A0)
    SWC1    FP1  ,   0x08(A0)
    SWC1    FP2  ,   0x0c(A0)
    SWC1    FP3  ,   0x10(A0)
    SWC1    FP4  ,   0x14(A0)
    SWC1    FP5  ,   0x18(A0)
    SWC1    FP6  ,   0x1c(A0)
    SWC1    FP7  ,   0x20(A0)
    SWC1    FP8  ,   0x24(A0)
    SWC1    FP9  ,   0x28(A0)
    SWC1    FP10 ,   0x2c(A0)
    SWC1    FP11 ,   0x30(A0)
    SWC1    FP12 ,   0x34(A0)
    SWC1    FP13 ,   0x38(A0)
    SWC1    FP14 ,   0x3c(A0)
    SWC1    FP15 ,   0x40(A0)
    SWC1    FP16 ,   0x44(A0)
    SWC1    FP17 ,   0x48(A0)
    SWC1    FP18 ,   0x4c(A0)
    SWC1    FP19 ,   0x50(A0)
    SWC1    FP20 ,   0x54(A0)
    SWC1    FP21 ,   0x58(A0)
    SWC1    FP22 ,   0x5c(A0)
    SWC1    FP23 ,   0x60(A0)
    SWC1    FP24 ,   0x64(A0)
    SWC1    FP25 ,   0x68(A0)
    SWC1    FP26 ,   0x6c(A0)
    SWC1    FP27 ,   0x70(A0)
    SWC1    FP28 ,   0x74(A0)
    SWC1    FP29 ,   0x78(A0)
    SWC1    FP30 ,   0x7c(A0)
    SWC1    FP31 ,   0x80(A0)
    JR      RA
    LI      V0   ,   0
FUNC_END(mipsVfp32Save32)

FUNC_DEF(mipsVfp32Restore32)
    MFC0(T0, CP0_STATUS)
    OR      T0   ,   M_StatusCU1
    MTC0(T0, CP0_STATUS)
    LW      T0   ,   0X00(A0)
    LWC1    FP0  ,   0x04(A0)
    LWC1    FP1  ,   0x08(A0)
    LWC1    FP2  ,   0x0c(A0)
    LWC1    FP3  ,   0x10(A0)
    LWC1    FP4  ,   0x14(A0)
    LWC1    FP5  ,   0x18(A0)
    LWC1    FP6  ,   0x1c(A0)
    LWC1    FP7  ,   0x20(A0)
    LWC1    FP8  ,   0x24(A0)
    LWC1    FP9  ,   0x28(A0)
    LWC1    FP10 ,   0x2c(A0)
    LWC1    FP11 ,   0x30(A0)
    LWC1    FP12 ,   0x34(A0)
    LWC1    FP13 ,   0x38(A0)
    LWC1    FP14 ,   0x3c(A0)
    LWC1    FP15 ,   0x40(A0)
    LWC1    FP16 ,   0x44(A0)
    LWC1    FP17 ,   0x48(A0)
    LWC1    FP18 ,   0x4c(A0)
    LWC1    FP19 ,   0x50(A0)
    LWC1    FP20 ,   0x54(A0)
    LWC1    FP21 ,   0x58(A0)
    LWC1    FP22 ,   0x5c(A0)
    LWC1    FP23 ,   0x60(A0)
    LWC1    FP24 ,   0x64(A0)
    LWC1    FP25 ,   0x68(A0)
    LWC1    FP26 ,   0x6c(A0)
    LWC1    FP27 ,   0x70(A0)
    LWC1    FP28 ,   0x74(A0)
    LWC1    FP29 ,   0x78(A0)
    LWC1    FP30 ,   0x7c(A0)
    LWC1    FP31 ,   0x80(A0)
    CTC1(T0,     CP1_STATUS)                                            ;/*  restore fpp status reg      */
    JR      RA
    LI      V0   ,   0
FUNC_END(mipsVfp32Restore32)

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
