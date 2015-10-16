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
;  定义 CACHE NEED MACRO
;*********************************************************************************************************/

MACRO_DEF(SYNC_WB)
    sync
MACRO_END()

MACRO_DEF(CACHE_OP op addr)
    .set    noreorder
    .set    mips32
    cache   \op , 0(\addr)
    .set    mips0
    .set    reorder
MACRO_END()

;/*********************************************************************************************************
;  使能 CACHE
;*********************************************************************************************************/

FUNC_DEF(mipsICacheEnable)
    JR      RA
    NOP
FUNC_END(mipsICacheEnable)

FUNC_DEF(mipsDCacheEnable)
    MFC0(T0, CP0_CONFIG)
    ORI     T0 , M_ConfigK0
    XORI    T0 , M_ConfigK0
    ORI     T0 , MIPS_CACHABLE_NONCOHERENT
    MTC0(T0, CP0_CONFIG)
    JR     RA
    NOP
FUNC_END(mipsDCacheEnable)

;/*********************************************************************************************************
;  禁能 CACHE
;*********************************************************************************************************/

FUNC_DEF(mipsICacheDisable)
    JR     RA
    NOP
FUNC_END(mipsICacheDisable)

FUNC_DEF(mipsDCacheDisable)
    MFC0(T0, CP0_CONFIG)
    LI      T1 , -8
    AND     T0 , T0 , T1
    ORI     T0 , T0 , MIPS_UNCACHED
    MTC0(T0, CP0_CONFIG)
    JR     RA
    NOP
FUNC_END(mipsDCacheDisable)

;/*********************************************************************************************************
;  无效 CACHE
;*********************************************************************************************************/

FUNC_DEF(mipsICacheInvalidateAll)
    MTC0(ZERO, CP0_TAGLO)
    MTC0(ZERO, CP0_TAGHI)
    JR      RA
    NOP
FUNC_END(mipsICacheInvalidateAll)

;/*********************************************************************************************************
;  无效 CACHE A0=void *start; A1=void *end; A2=cache line size
;*********************************************************************************************************/

FUNC_DEF(mipsICacheInvalidate)
1:
    CACHE_OP    INDEX_INVALIDATE_I A0
    ADD     A0 , A0 , A2
    BLT     A0 , A1 , 1b
    JR      RA
    NOP
FUNC_END(mipsICacheInvalidate)

;/*********************************************************************************************************
;  将指定虚拟地址的 DCACHE 无效  A0=void *start; A1=void *end; A2=cache line size
;*********************************************************************************************************/

FUNC_DEF(mipsDCacheInvalidate)
1:
    CACHE_OP    HIT_INVALIDATE_D A0
    ADD     A0 , A0 , A2
    BLT     A0 , A1 , 1b
    JR      RA
    NOP
FUNC_END(mipsDCacheInvalidate)

;/*********************************************************************************************************
;  将指定虚拟地址的 DCACHE 回写  A0=void *start; A1=void *end; A2=cache line size
;*********************************************************************************************************/

FUNC_DEF(mipsDCacheFlush)
1:
    CACHE_OP    INDEX_WRITEBACK_INV_D A0
    ADD     A0 , A0 , A2
    BLT     A0 , A1 , 1b
    SYNC_WB
    JR      RA
    NOP
FUNC_END(mipsDCacheFlush)

;/*********************************************************************************************************
;  将指定虚拟地址的 DCACHE 回写并无效  A0=void *start; A1=void *end; A2=cache line size
;*********************************************************************************************************/

FUNC_DEF(mipsDCacheClear)
1:
    CACHE_OP    HIT_WRITEBACK_INV_D A0
    ADD     A0 , A0 , A2
    BLT     A0 , A1 , 1b
    SYNC_WB
    JR      RA
    NOP
FUNC_END(mipsDCacheClear)

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
