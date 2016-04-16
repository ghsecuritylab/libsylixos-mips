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
;** 文   件   名: ppcContextAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2015 年 12 月 15 日
;**
;** 描        述: PowerPC 体系构架上下文切换.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/kernel_cfg.h>
#include "ppcContextAsm.h"

    FILE_BEGIN()

    IMPORT_LABEL(_SchedSwp)
#if LW_CFG_COROUTINE_EN > 0
    IMPORT_LABEL(_SchedCrSwp)
#endif
    
    EXPORT_LABEL(archTaskCtxStart)
    EXPORT_LABEL(archTaskCtxSwitch)
    EXPORT_LABEL(archCrtCtxSwitch)
    EXPORT_LABEL(archIntCtxLoad)
    EXPORT_LABEL(archSigCtxLoad)

;/*********************************************************************************************************
;  上下文栈帧(调试时打开)
;*********************************************************************************************************/

#undef __CTX_FRAME

#ifdef __CTX_FRAME
MACRO_DEF(PUSH_FRAME)
    SUBI    SP , SP , 8
    STW     FP , 0(SP)
    MFLR    R4
    ISYNC
    STW     R4 , 4(SP)
    SYNC
    MACRO_END()

MACRO_DEF(POP_FRAME)
    LWZ     FP , 0(SP)
    LWZ     R4 , 4(SP)
    SYNC
    MTLR    R4
    ISYNC
    ADDI    SP , SP , 8
    MACRO_END()
#else
MACRO_DEF(PUSH_FRAME)
    MACRO_END()

MACRO_DEF(POP_FRAME)
    MACRO_END()
#endif

;/*********************************************************************************************************
;  当前 CPU 开始运行多任务执行
;  参数为当前 CPU 控制块, 即 R3 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxStart)
    LWZ     R4 , 0(R3)                                                  ;/*  获取该任务堆栈栈顶地址      */
    LWZ     SP , 0(R4)                                                  ;/*  堆栈指针赋值                */

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END()

;/*********************************************************************************************************
;  线程切换 
;  参数为当前 CPU 控制块, 即 R3 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxSwitch)
    SAVE_REGS                                                           ;/*  所有寄存器入栈              */

    LWZ     R4 , 0(R3)                                                  ;/*  stack current = SP          */
    STW     SP , 0(R4)

    SUBI    SP , SP , 8
    STW     R3 , 0(SP)                                                  ;/*  当前 CPU 信息入栈           */

    PUSH_FRAME
    BL      _SchedSwp                                                   ;/*  _SchedSwp();                */
    POP_FRAME

    LWZ     R3 , 0(SP)                                                  ;/*  当前 CPU 信息出栈           */
    ADDI    SP , SP , 8

    LWZ     R4 , 0(R3)                                                  ;/*  SP = stack current          */
    LWZ     SP , 0(R4)

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END()

;/*********************************************************************************************************
;  协程切换 
;  参数为当前 CPU 控制块, 即 R3 为当前 CPU 控制块指针
;*********************************************************************************************************/

#if LW_CFG_COROUTINE_EN > 0
FUNC_DEF(archCrtCtxSwitch)
    SAVE_REGS                                                           ;/*  所有寄存器入栈              */

    LWZ     R4 , 0(R3)                                                  ;/*  stack current = SP          */
    STW     SP , 0(R4)

    SUBI    SP , SP , 8
    STW     R3 , 0(SP)                                                  ;/*  当前 CPU 信息入栈           */

    PUSH_FRAME
    BL      _SchedCrSwp                                                 ;/*  _SchedCrSwp();              */
    POP_FRAME

    LWZ     R3 , 0(SP)                                                  ;/*  当前 CPU 信息出栈           */
    ADDI    SP , SP , 8

    LWZ     R4 , 0(R3)                                                  ;/*  SP = stack current          */
    LWZ     SP , 0(R4)

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END()
#endif

;/*********************************************************************************************************
;  中断返回时, 线程装载 
;  参数为当前 CPU 控制块, 即 R3 为当前 CPU 控制块指针
;*********************************************************************************************************/

FUNC_DEF(archIntCtxLoad)
    SUBI    SP , SP , 8
    STW     R3 , 0(SP)                                                  ;/*  当前 CPU 信息入栈           */

    PUSH_FRAME
    BL      _SchedSwp                                                   ;/*  _SchedSwp();                */
    POP_FRAME

    LWZ     R3 , 0(SP)                                                  ;/*  当前 CPU 信息出栈           */
    ADDI    SP , SP , 8

    LWZ     R4 , 0(R3)                                                  ;/*  SP = stack current          */
    LWZ     SP , 0(R4)

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END()

;/*********************************************************************************************************
;  信号上下文返回
;  参数为需要返回的堆栈栈顶位置
;*********************************************************************************************************/

FUNC_DEF(archSigCtxLoad)
    OR      SP , R3 , R3                                                ;/*  返回的堆栈指针              */

    RESTORE_REGS                                                        ;/*  所有寄存器出栈              */
    FUNC_END()

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
