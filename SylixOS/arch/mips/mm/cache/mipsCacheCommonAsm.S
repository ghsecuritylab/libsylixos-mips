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
;** 文   件   名: mipsCacheCommonAsm.S
;**
;** 创   建   人: Ryan.Xin (信金龙)
;**
;** 文件创建日期: 2015 年 09 月 01 日
;**
;** 描        述: mips 体系架构 CACHE 通用函数支持.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/cache_cfg.h>

FILE_BEGIN()

#if LW_CFG_CACHE_EN > 0

    EXPORT_LABEL(mipsICacheEnable)
    EXPORT_LABEL(mipsDCacheEnable)

    EXPORT_LABEL(mipsICacheDisable)
    EXPORT_LABEL(mipsDCacheDisable)

    EXPORT_LABEL(mipsICacheInvalidate)
    EXPORT_LABEL(mipsICacheInvalidateAll)

    EXPORT_LABEL(mipsDCacheInvalidate)
    EXPORT_LABEL(mipsDCacheFlush)
    EXPORT_LABEL(mipsDCacheClear)

;/*********************************************************************************************************
;  使能 CACHE
;*********************************************************************************************************/

FUNC_DEF(mipsICacheEnable)
    jr  ra
    nop
FUNC_END(mipsICacheEnable)

FUNC_DEF(mipsDCacheEnable)
    jr  ra
    nop
FUNC_END(mipsDCacheEnable)

;/*********************************************************************************************************
;  禁能 CACHE
;*********************************************************************************************************/

FUNC_DEF(mipsICacheDisable)
    jr  ra
    nop
FUNC_END(mipsICacheDisable)

FUNC_DEF(mipsDCacheDisable)
    jr  ra
    nop
FUNC_END(mipsDCacheDisable)

;/*********************************************************************************************************
;  无效 CACHE
;*********************************************************************************************************/

FUNC_DEF(mipsICacheInvalidateAll)
    jr  ra
    nop
FUNC_END(mipsICacheInvalidateAll)

;/*********************************************************************************************************
;  无效 CACHE R0=void *start; R1=void *end; R2=cache line size
;*********************************************************************************************************/

FUNC_DEF(mipsICacheInvalidate)
    jr  ra
    nop
FUNC_END(mipsICacheInvalidate)

;/*********************************************************************************************************
;  将指定虚拟地址的 DCACHE 无效  R0=void *start; R1=void *end; R2=cache line size
;*********************************************************************************************************/

FUNC_DEF(mipsDCacheInvalidate)
    jr  ra
    nop
FUNC_END(mipsDCacheInvalidate)

;/*********************************************************************************************************
;  将指定虚拟地址的 DCACHE 回写  R0=void *start; R1=void *end; R2=cache line size
;*********************************************************************************************************/

FUNC_DEF(mipsDCacheFlush)
    jr  ra
    nop
FUNC_END(mipsDCacheFlush)

;/*********************************************************************************************************
;  将指定虚拟地址的 DCACHE 回写并无效  R0=void *start; R1=void *end; R2=cache line size
;*********************************************************************************************************/

FUNC_DEF(mipsDCacheClear)
    jr  ra
    nop
FUNC_END(mipsDCacheClear)

FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
