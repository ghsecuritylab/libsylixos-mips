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
;** 文   件   名: mipsMmuCommonAsm.S
;**
;** 创   建   人: Ryan.Xin (信金龙)
;**
;** 文件创建日期: 2015 年 09 月 01 日
;**
;** 描        述: mips 体系架构 MMU 通用函数支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/vmm_cfg.h>

    FILE_BEGIN()

#if LW_CFG_VMM_EN > 0

    EXPORT_LABEL(mipsMmuAbtFaultStatus)
    EXPORT_LABEL(mipsMmuPreFaultStatus)
    EXPORT_LABEL(mipsMmuAbtFaultAddr)
    EXPORT_LABEL(mipsMmuEnable)
    EXPORT_LABEL(mipsMmuDisable)
    EXPORT_LABEL(mipsMmuInvalidateTLB)
    EXPORT_LABEL(mipsMmuInvalidateTLBMVA)
    EXPORT_LABEL(mipsMmuSetTTBase)

;/*********************************************************************************************************
;  获取错误信息
;*********************************************************************************************************/

FUNC_DEF(mipsMmuAbtFaultStatus)
    JR      RA
    MOV     V0 , ZERO
FUNC_END(mipsMmuAbtFaultStatus)

FUNC_DEF(mipsMmuPreFaultStatus)
    JR      RA
    MOV     V0 , ZERO
FUNC_END(mipsMmuPreFaultStatus)

FUNC_DEF(mipsMmuAbtFaultAddr)
    JR      RA
    MOV     V0 , ZERO
FUNC_END(mipsMmuAbtFaultAddr)

;/*********************************************************************************************************
;  使能/禁能 MMU
;*********************************************************************************************************/

FUNC_DEF(mipsMmuEnable)
    JR      RA
    NOP
FUNC_END(mipsMmuEnable)

FUNC_DEF(mipsMmuDisable)
    JR      RA
    NOP
FUNC_END(mipsMmuDisable)

;/*********************************************************************************************************
;  mipsMmuSetTTBase
;*********************************************************************************************************/

FUNC_DEF(mipsMmuSetTTBase)
    JR      RA
    NOP
FUNC_END(mipsMmuSetTTBase)

;/*********************************************************************************************************
;  TLB 相关操作
;*********************************************************************************************************/

FUNC_DEF(mipsMmuInvalidateTLB)
    JR      RA
    NOP
FUNC_END(mipsMmuInvalidateTLB)

FUNC_DEF(mipsMmuInvalidateTLBMVA)
    JR      RA
    NOP
FUNC_END(mipsMmuInvalidateTLBMVA)

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
