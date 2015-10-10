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
;** 文   件   名: mipsExcAsm.S
;**
;** 创   建   人: Ryan.Xin (信金龙)
;**
;** 文件创建日期: 2015 年 09 月 01 日
;**
;** 描        述: MIPS 体系构架异常处理.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

    FILE_BEGIN()

    IMPORT_LABEL(API_ThreadTcbInter)
    IMPORT_LABEL(API_InterStackBaseGet)
    IMPORT_LABEL(API_InterGetNesting)
    IMPORT_LABEL(API_InterEnter)
    IMPORT_LABEL(API_InterExit)

    IMPORT_LABEL(bspIntHandle)
    EXPORT_LABEL(archInterruptEntry)

    EXPORT_LABEL(CP0_wGetSR)
    EXPORT_LABEL(CP0_wSetSR)
    EXPORT_LABEL(CP0_wGetCAUSE)
    EXPORT_LABEL(CP0_wGet_COUNT)
    EXPORT_LABEL(CP0_wSet_COUNT)
    EXPORT_LABEL(CP0_wGet_COMPARE)
    EXPORT_LABEL(CP0_wSet_COMPARE)
    EXPORT_LABEL(CP0_wGet_INTCTL)

;/*********************************************************************************************************
;  Exception 入口
;*********************************************************************************************************/

MACRO_DEF(EXCEPTION_ENTRY handle)
    MFC0(K0, CP0_EPC)                                                   ;/*  Get异常的入口地址           */
    STORE_REG_RET(K0)                                                   ;/*  保存任务控制块              */

    JAL     API_InterEnter                                              ;/*  获取中断层数放到返回值V0    */
    NOP

    BNE     V0 , 1 , BSP_INT_HANDLE
    NOP

    JAL     API_ThreadTcbInter                                          ;/*  get current tcb             */
    NOP
    SW      SP , 0(V0)                                                  ;/*  current stack = SP          */

    JAL     API_InterStackBaseGet                                       ;/*  获得当前 CPU 中断堆栈栈顶   */
    NOP
    MOV     SP , V0

BSP_INT_HANDLE:
    JAL     \handle
    NOP

    JAL     API_InterGetNesting
    NOP

    BNE     V0 , 1 , BSP_INT_EXIT
    NOP

    JAL     API_ThreadTcbInter
    NOP

    LW      SP , 0(V0)

BSP_INT_EXIT:
    JAL     API_InterExit
    NOP
    RESTORE_REG_ERET()
    JR      RA
    NOP
MACRO_END()

FUNC_DEF(archInterruptEntry)
    EXCEPTION_ENTRY bspIntHandle
FUNC_END(archInterruptEntry)

;/********************************************************************************************************
;  Get CP0 Status register Value
;********************************************************************************************************/
FUNC_DEF(CP0_wGetSR)
    MFC0(V0, CP0_STATUS)
    JR      RA
    NOP
FUNC_END(CP0_wGetSR)

FUNC_DEF(CP0_wSetSR)
    MTC0(A0, CP0_STATUS)
    JR      RA
    NOP
FUNC_END(CP0_wSetSR)

;/********************************************************************************************************
;  Get CP0 Cause register Value
;********************************************************************************************************/
FUNC_DEF(CP0_wGetCAUSE)
    MFC0(V0, CP0_CAUSE)
    JR      RA
    NOP
FUNC_END(CP0_wGetCAUSE)

;/********************************************************************************************************
;  Get CP0 Count register Value
;********************************************************************************************************/
FUNC_DEF(CP0_wGet_COUNT)
    MFC0(V0, CP0_COUNT)
    JR      RA
    NOP
FUNC_END(CP0_wGet_COUNT)

;/********************************************************************************************************
;  Set CP0 Count register Value
;********************************************************************************************************/
FUNC_DEF(CP0_wSet_COUNT)
    MTC0(A0, CP0_COUNT)
    JR      RA
    NOP
FUNC_END(CP0_wSet_COUNT)

;/********************************************************************************************************
;  Get CP0 Compare register Value
;********************************************************************************************************/
FUNC_DEF(CP0_wGet_COMPARE)
    MFC0(V0, CP0_COMPARE)
    JR      RA
    NOP
FUNC_END(CP0_wGet_COMPARE)

;/********************************************************************************************************
;  Set CP0 Compare register Value
;********************************************************************************************************/
FUNC_DEF(CP0_wSet_COMPARE)
    MTC0(A0, CP0_COMPARE)
    JR      RA
    NOP
FUNC_END(CP0_wSet_COMPARE)

;/********************************************************************************************************
;  Get interrupts vector
;********************************************************************************************************/
FUNC_DEF(CP0_wGet_INTCTL)
    MFC0(V0, CP0_INTCTL)
    JR      RA
    NOP
FUNC_END(CP0_wGet_INTCTL)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
