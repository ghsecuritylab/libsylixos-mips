;/**********************************************************************************************************
;**
;**                                    �й�������Դ��֯
;**
;**                                   Ƕ��ʽʵʱ����ϵͳ
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------�ļ���Ϣ--------------------------------------------------------------------------------
;**
;** ��   ��   ��: mipsExcAsm.S
;**
;** ��   ��   ��: Ryan.Xin (�Ž���)
;**
;** �ļ���������: 2015 �� 09 �� 01 ��
;**
;** ��        ��: MIPS ��ϵ�����쳣����.
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
;  �ж����
;*********************************************************************************************************/

MACRO_DEF(IRQ_ENTRY)
    jr  ra
    nop
    MACRO_END()

FUNC_DEF(archIntEntry)
    IRQ_ENTRY
FUNC_END(archIntEntry)

;/*********************************************************************************************************
;  abort ���
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
;  undefine ���
;*********************************************************************************************************/

MACRO_DEF(UND_ENTRY)
    jr  ra
    nop
    MACRO_END()

FUNC_DEF(archUndEntry)
    UND_ENTRY
FUNC_END(archUndEntry)

;/*********************************************************************************************************
;  swi ���
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