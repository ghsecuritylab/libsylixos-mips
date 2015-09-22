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

    IMPORT_LABEL(archAbtHandle)
    IMPORT_LABEL(archUndHandle)
    IMPORT_LABEL(archSwiHandle)
    IMPORT_LABEL(bspIntHandle)

    EXPORT_LABEL(archIntEntry)
    EXPORT_LABEL(archAbtEntry)
    EXPORT_LABEL(archPreEntry)
    EXPORT_LABEL(archUndEntry)
    EXPORT_LABEL(archSwiEntry)

;/*********************************************************************************************************
;  中断入口
;*********************************************************************************************************/

MACRO_DEF(IRQ_ENTRY)
    jr  ra
    nop
    MACRO_END()

FUNC_DEF(archIntEntry)
    IRQ_ENTRY
FUNC_END(archIntEntry)

;/*********************************************************************************************************
;  abort 入口
;  data_abort     $retoff == 8
;  prefetch_abort $retoff == 4
;*********************************************************************************************************/

MACRO_DEF(ABT_ENTRY retoff)
    jr  ra
    nop
    MACRO_END()

FUNC_DEF(archAbtEntry)
    ABT_ENTRY 8
FUNC_END(archAbtEntry)

FUNC_DEF(archPreEntry)
    ABT_ENTRY 4
FUNC_END(archPreEntry)

;/*********************************************************************************************************
;  undefine 入口
;*********************************************************************************************************/

MACRO_DEF(UND_ENTRY)
    jr  ra
    nop
    MACRO_END()

FUNC_DEF(archUndEntry)
    UND_ENTRY
FUNC_END(archUndEntry)

;/*********************************************************************************************************
;  swi 入口
;*********************************************************************************************************/

MACRO_DEF(SWI_ENTRY)
    jr  ra
    nop
    MACRO_END()

FUNC_DEF(archSwiEntry)
    SWI_ENTRY
FUNC_END(archSwiEntry)

FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
