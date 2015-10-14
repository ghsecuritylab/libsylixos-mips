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
    IMPORT_LABEL(archCacheErrorHandle)
    IMPORT_LABEL(archExceptionHandle)

    EXPORT_LABEL(archInterruptEntry)
    EXPORT_LABEL(archCacheErrorEntry)
    EXPORT_LABEL(archExceptionEntry)

;/*********************************************************************************************************
;  异常入口宏
;*********************************************************************************************************/

MACRO_DEF(EXCEPTION_ENTRY handle)
    MFC0(K0, CP0_EPC)                                                   ;/*  获得异常的返回地址          */

    STORE_REG_RET(K0)                                                   ;/*  保存任务控制块              */

    ;/*
    ; *  if (API_InterEnter() == 1) {
    ; *      current stack = SP;
    ; *      SP = interrupt stack;
    ; *  }
    ; */
    JAL     API_InterEnter                                              ;/*  获取中断层数放到返回值V0    */
    NOP

    BNE     V0 , 1 , 1f
    NOP

    JAL     API_ThreadTcbInter                                          ;/*  get current tcb             */
    NOP
    SW      SP , 0(V0)                                                  ;/*  current stack = SP          */

    JAL     API_InterStackBaseGet                                       ;/*  获得当前 CPU 中断堆栈栈顶   */
    NOP
    MOV     SP , V0

1:
    MFC0(A0, CP0_EPC)                                                   ;/*  获得异常的返回地址          */
    JAL     \handle
    NOP

    ;/*
    ; *  if (API_InterGetNesting() == 1) {
    ; *      SP = current stack;
    ; *  }
    ; */
    JAL     API_InterGetNesting
    NOP

    BNE     V0 , 1 , 2f
    NOP

    JAL     API_ThreadTcbInter                                          ;/*  get current tcb             */
    NOP

    LW      SP , 0(V0)                                                  ;/*  SP = current stack          */

2:
    JAL     API_InterExit
    NOP

    RESTORE_REG_ERET()
MACRO_END()

;/*********************************************************************************************************
;  中断入口
;*********************************************************************************************************/

FUNC_DEF(archInterruptEntry)
    EXCEPTION_ENTRY     bspIntHandle
FUNC_END(archInterruptEntry)

;/*********************************************************************************************************
;  Cache Error 入口
;*********************************************************************************************************/

FUNC_DEF(archCacheErrorEntry)
    EXCEPTION_ENTRY     archCacheErrorHandle
FUNC_END(archCacheErrorEntry)

;/*********************************************************************************************************
;  Exception 入口
;*********************************************************************************************************/

FUNC_DEF(archExceptionEntry)
    EXCEPTION_ENTRY     archExceptionHandle
FUNC_END(archExceptionEntry)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
