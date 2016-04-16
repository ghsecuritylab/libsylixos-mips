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
;** 文   件   名: ppcExcAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2015 年 12 月 15 日
;**
;** 描        述: PowerPC 体系构架异常处理.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include "ppcContextAsm.h"

    FILE_BEGIN()
    
    IMPORT_LABEL(API_ThreadTcbInter)
    IMPORT_LABEL(API_InterStackBaseGet)
    IMPORT_LABEL(API_InterGetNesting)
    IMPORT_LABEL(API_InterEnter)
    IMPORT_LABEL(API_InterExit)
    
    IMPORT_LABEL(bspIntHandle)
    IMPORT_LABEL(archDataStorageExceptionHandle)
    IMPORT_LABEL(archInstructionStorageExceptionHandle)
    IMPORT_LABEL(archAlignmentExceptionHandle)
    IMPORT_LABEL(archProgramExceptionHandle)
    IMPORT_LABEL(archFpuUnavailableExceptionHandle)
    IMPORT_LABEL(archSystemCallHandle)
    IMPORT_LABEL(archDecrementerInterruptHandle)
    IMPORT_LABEL(archTraceHandle)
    
    EXPORT_LABEL(archDataStorageExceptionEntry)
    EXPORT_LABEL(archInstructionStorageExceptionEntry)
    EXPORT_LABEL(archExternalInterruptEntry)
    EXPORT_LABEL(archAlignmentExceptionEntry)
    EXPORT_LABEL(archProgramExceptionEntry)
    EXPORT_LABEL(archFpuUnavailableExceptionEntry)
    EXPORT_LABEL(archSystemCallEntry)
    EXPORT_LABEL(archDecrementerInterruptEntry)
    EXPORT_LABEL(archTraceEntry)
    
;/*********************************************************************************************************
;  中断入口宏
;*********************************************************************************************************/

MACRO_DEF(IRQ_ENTRY, handle)
    EXC_SAVE_REGS                                                       ;/*  所有寄存器入栈              */

    ;/*
    ; *  if (API_InterEnter() == 1) {
    ; *      current stack = SP;
    ; *      SP = interrupt stack;
    ; *  }
    ; */
    BL      API_InterEnter                                              ;/*  获取中断层数放到返回值 V0   */

    CMPWI   CR0, R3 , 1
    BNE     CR0, 1f

    BL      API_ThreadTcbInter                                          ;/*  get current tcb             */
    STW     SP , 0(R3)                                                  ;/*  current stack = SP          */

    BL      API_InterStackBaseGet                                       ;/*  获得当前 CPU 中断堆栈栈顶   */
    OR      SP , R3 , R3

1:
    MFSPR   R3 , SRR0                                                   ;/*  传递返回地址                */
    ISYNC
    BL      \handle                                                     ;/*  调用处理函数                */

    ;/*
    ; *  if (API_InterGetNesting() == 1) {
    ; *      SP = current stack;
    ; *  }
    ; */
    BL      API_InterGetNesting

    CMPWI   CR0, R3 , 1
    BNE     CR0, 2f

    BL      API_ThreadTcbInter                                          ;/*  get current tcb             */

    LWZ     SP , 0(R3)                                                  ;/*  SP = current stack          */

2:
    BL      API_InterExit

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    MACRO_END()

;/*********************************************************************************************************
;  数据存储异常入口(Vector 0x300)
;*********************************************************************************************************/

FUNC_DEF(archDataStorageExceptionEntry)
    IRQ_ENTRY   archDataStorageExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  指令访问异常入口(Vector 0x400)
;*********************************************************************************************************/

FUNC_DEF(archInstructionStorageExceptionEntry)
    IRQ_ENTRY   archInstructionStorageExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  外部中断入口(Vector 0x500)
;*********************************************************************************************************/

FUNC_DEF(archExternalInterruptEntry)
    IRQ_ENTRY   bspIntHandle
    FUNC_END()

;/*********************************************************************************************************
;  非对齐异常入口(Vector 0x600)
;*********************************************************************************************************/

FUNC_DEF(archAlignmentExceptionEntry)
    IRQ_ENTRY   archAlignmentExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  程序异常入口(Vector 0x700)
;*********************************************************************************************************/

FUNC_DEF(archProgramExceptionEntry)
    IRQ_ENTRY   archProgramExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  FPU 不可用入口(Vector 0x800)
;*********************************************************************************************************/

FUNC_DEF(archFpuUnavailableExceptionEntry)
    IRQ_ENTRY   archFpuUnavailableExceptionHandle
    FUNC_END()

;/*********************************************************************************************************
;  Decrementer 异常入口(Vector 0x900)
;*********************************************************************************************************/

FUNC_DEF(archDecrementerInterruptEntry)
    IRQ_ENTRY   archDecrementerInterruptHandle
    FUNC_END()

;/*********************************************************************************************************
;  系统调用入口(Vector 0xC00)
;*********************************************************************************************************/

FUNC_DEF(archSystemCallEntry)
    IRQ_ENTRY   archSystemCallHandle
    FUNC_END()

;/*********************************************************************************************************
;  Trace 入口(Vector 0xD00)
;*********************************************************************************************************/

FUNC_DEF(archTraceEntry)
    IRQ_ENTRY   archTraceHandle
    FUNC_END()

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
