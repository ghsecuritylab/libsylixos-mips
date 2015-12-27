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
;** 文   件   名: mipsLibAsm.S
;**
;** 创   建   人: Ryan.Xin (信金龙)
;**
;** 文件创建日期: 2015 年 09 月 01 日
;**
;** 描        述: MIPS 体系构架内部库.
;**
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/cpu/cpu_cfg.h>

    FILE_BEGIN()

    IMPORT_LABEL(__setjmpSetup)
    IMPORT_LABEL(__sigsetjmpSetup)
    IMPORT_LABEL(__longjmpSetup)
    IMPORT_LABEL(__siglongjmpSetup)
    IMPORT_LABEL(bspReboot)

    EXPORT_LABEL(setjmp)
    EXPORT_LABEL(sigsetjmp)
    EXPORT_LABEL(longjmp)
    EXPORT_LABEL(siglongjmp)

#if LW_CFG_MIPS_HAS_CLZ_INSTR > 0
    EXPORT_LABEL(archFindLsb)
    EXPORT_LABEL(archFindMsb)
#endif

    EXPORT_LABEL(archIntDisable)
    EXPORT_LABEL(archIntEnable)
    EXPORT_LABEL(archIntEnableForce)
    EXPORT_LABEL(archPageCopy)
    EXPORT_LABEL(archReboot)

    WEAK(archIntDisable)
    WEAK(archIntEnable)
    WEAK(archIntEnableForce)

;/*********************************************************************************************************
;  MIPS 关闭总中断
;*********************************************************************************************************/

FUNC_DEF(archIntDisable)
    MFC0(V0, CP0_STATUS)
    AND     T0 , V0 , ~M_StatusIE
    MTC0(T0, CP0_STATUS)
    JR      RA
    NOP
FUNC_END(archIntDisable)

;/*********************************************************************************************************
;  MIPS 打开总中断
;*********************************************************************************************************/
FUNC_DEF(archIntEnable)
    MTC0(A0, CP0_STATUS)
    JR      RA
    NOP
FUNC_END(archIntEnable)

;/*********************************************************************************************************
;  MIPS 强制打开总中断
;*********************************************************************************************************/

FUNC_DEF(archIntEnableForce)
    MFC0(T0, CP0_STATUS)
    OR      T1 , T0 , M_StatusIE
    MTC0(T1, CP0_STATUS)
    JR      RA
    NOP
FUNC_END(archIntEnableForce)

;/*********************************************************************************************************
;  MIPS 前导零运算
;*********************************************************************************************************/

#if LW_CFG_MIPS_HAS_CLZ_INSTR > 0
FUNC_DEF(archFindLsb)
    NEGU    V0 , A0
    AND     A0 , V0 , A0
    CLZ     A0 , A0
    LI      V0 , 32
    J       RA
    SUBU    V0 , V0 , A0
FUNC_END(archFindLsb)

FUNC_DEF(archFindMsb)
    CLZ     A0 , A0
    LI      V0 , 32
    J       RA
    SUBU    V0 , V0 , A0
FUNC_END(archFindMsb)
#endif

;/*********************************************************************************************************
;  注意: setjmp 与 longjmp 上下文结构与线程上下文结构不同
;
;  +-------+
;  | a0-a3 |   Storage provided by ABI contract from calling function
;  +-------+<---   Original SP
;  |  RA   |   Storage for return address (8 bytes for alignment)
;  +-------+
;  | extra |   Optional additional storage requested by SETFRAME_EXTRA
;  +-------+
;  | r0-rN |   Storage for up to 4 additional local registers
;  +-------+
;  |(a0-a3)|   Storage provided by ABI contract to called function
;  +-------+<---   Adjusted SP
;
;*********************************************************************************************************/

;/*********************************************************************************************************
;  sigsetjmp (参数为 jmp_buf, mask_saved)
;*********************************************************************************************************/

FUNC_DEF(sigsetjmp)
    SUBU    SP , 16
    SW      RA , 12(SP)
    SW      A0 , 8(SP)
    SW      A1 , 4(SP)
    JAL     __sigsetjmpSetup
    NOP
    LW      A1 , 4(SP)
    LW      A0 , 8(SP)
    LW      RA , 12(SP)
    ADDU    SP , 16

    SW      S0 ,  0(A0)                                                 ;/*  Save registers in jump buf  */
    SW      S1 ,  4(A0)
    SW      S2 ,  8(A0)
    SW      S3 , 12(A0)
    SW      S4 , 16(A0)
    SW      S5 , 20(A0)
    SW      S6 , 24(A0)
    SW      S7 , 28(A0)
    SW      GP , 32(A0)
    SW      SP , 36(A0)
    SW      FP , 40(A0)
    SW      RA , 44(A0)

    MOV     V0 , ZERO                                                   ;/*  Return zero.                */
    JR      RA
    NOP
FUNC_END(sigsetjmp)

;/*********************************************************************************************************
;  setjmp (参数为 jmp_buf)，暂时不考虑FPU
;*********************************************************************************************************/

FUNC_DEF(setjmp)
    SUBU    SP , 12
    SW      RA , 8(SP)
    SW      A0 , 4(SP)
    JAL     __setjmpSetup
    NOP
    LW      A0 , 4(SP)
    LW      RA , 8(SP)
    ADDU    SP , 12

    SW      S0 ,  0(A0)                                                 ;/*  Save registers in jump buf  */
    SW      S1 ,  4(A0)
    SW      S2 ,  8(A0)
    SW      S3 , 12(A0)
    SW      S4 , 16(A0)
    SW      S5 , 20(A0)
    SW      S6 , 24(A0)
    SW      S7 , 28(A0)
    SW      GP , 32(A0)
    SW      SP , 36(A0)
    SW      FP , 40(A0)
    SW      RA , 44(A0)

    MOV     V0 , ZERO                                                   ;/*  Return zero.                */
    JR      RA
    NOP
FUNC_END(setjmp)

;/*********************************************************************************************************
;  siglongjmp (参数为 jmp_buf, retval)
;*********************************************************************************************************/

FUNC_DEF(siglongjmp)
    SUBU    SP , 16
    SW      RA , 12(SP)
    SW      A0 , 8(SP)
    SW      A1 , 4(SP)
    JAL     __siglongjmpSetup
    NOP
    LW      A1 , 4(SP)
    LW      A0 , 8(SP)
    LW      RA , 12(SP)
    ADDU    SP , 16

    LW      S0 ,  0(A0)
    LW      S1 ,  4(A0)
    LW      S2 ,  8(A0)
    LW      S3 , 12(A0)
    LW      S4 , 16(A0)
    LW      S5 , 20(A0)
    LW      S6 , 24(A0)
    LW      S7 , 28(A0)
    LW      GP , 32(A0)
    LW      SP , 36(A0)
    LW      FP , 40(A0)
    LW      RA , 44(A0)

    MOV     V0 , A1                                                     ;/*  Return the result argument, */
    BNE     V0 , 1, 2f                                                  ;/*  or 1 if it is zero.         */
    NOP
    LI      V0 , 1

2:
    JR      RA
    NOP
FUNC_END(siglongjmp)

;/*********************************************************************************************************
;  longjmp (参数为 jmp_buf, retval)暂时不考虑FPU
;*********************************************************************************************************/

FUNC_DEF(longjmp)
    SUBU    SP , 16
    SW      RA , 12(SP)
    SW      A0 , 8(SP)
    SW      A1 , 4(SP)
    JAL     __longjmpSetup
    NOP
    LW      A1 , 4(SP)
    LW      A0 , 8(SP)
    LW      RA , 12(SP)
    ADDU    SP , 16

    LW      S0 ,  0(A0)
    LW      S1 ,  4(A0)
    LW      S2 ,  8(A0)
    LW      S3 , 12(A0)
    LW      S4 , 16(A0)
    LW      S5 , 20(A0)
    LW      S6 , 24(A0)
    LW      S7 , 28(A0)
    LW      GP , 32(A0)
    LW      SP , 36(A0)
    LW      FP , 40(A0)
    LW      RA , 44(A0)

    MOV     V0 , A1                                                     ;/*  Return the result argument, */
    BNE     V0 , 1, 2f                                                  ;/*  or 1 if it is zero.         */
    NOP
    LI      V0 , 1

2:
    JR      RA
    NOP
FUNC_END(longjmp)

;/*********************************************************************************************************
;  系统重启
;*********************************************************************************************************/

FUNC_DEF(archReboot)
    JAL     bspReboot                                                   ;/*  由BSP进行复位操作, 不再返回 */
    NOP
FUNC_END(archReboot)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
