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
;** 文   件   名: armCacheV7Asm.S
;**
;** 创   建   人: Han.Hui (韩辉)
;**
;** 文件创建日期: 2013 年 12 月 09 日
;**
;** 描        述: ARMv7 体系构架 CACHE 驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/cache_cfg.h>

#if LW_CFG_CACHE_EN > 0

    FILE_BEGIN()
    
    EXPORT_LABEL(armCacheV7CCSIDR)
    EXPORT_LABEL(armDCacheV7Disable)
    EXPORT_LABEL(armDCacheV7FlushPoU)
    EXPORT_LABEL(armDCacheV7FlushAll)
    EXPORT_LABEL(armDCacheV7ClearAll)
    EXPORT_LABEL(armDCacheV7InvalidateAll)
    
;/*********************************************************************************************************
;  ARMv7 获得 CCSIDR 寄存器
;*********************************************************************************************************/

FUNC_DEF(armCacheV7CCSIDR)
    MRC     p15, 1, R0, c0, c0, 0
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  ARMv7 关闭 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV7Disable)
    MRC     p15, 0, R0, c1, c0, 0
    AND     R0, R0, #P15_R1_C
    CMP     R0, #0
    BEQ     dcache_not_en

    STMFD   SP!, {LR}
    BL      armDCacheV7ClearAll
    LDMFD   SP!, {LR}
    
    ARM_DSB()
    ARM_ISB()
    
    MRC     p15, 0, R0, c1, c0, 0
    BIC     R0, R0, #P15_R1_C
    MCR     p15, 0, R0, c1, c0, 0
    BX      LR
	
LINE_LABEL(dcache_not_en)
    STMFD   SP!, {LR}
    BL      armDCacheV7InvalidateAll
    LDMFD   SP!, {LR}
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  将指定虚拟地址的 DCACHE 回写 (point of unification (PoU))
;  R0=void *start; R1=void *end; R2=cache line size
;*********************************************************************************************************/

FUNC_DEF(armDCacheV7FlushPoU)
    ARM_DSB()
    ARM_ISB()
1:
    MCR     p15, 0, R0, c7, c11, 1
    ADD     R0, R0, R2
    CMP     R0, R1
    BCC     1b
    ARM_DSB()
    ARM_ISB()
    BX      LR
    FUNC_END()

;/*********************************************************************************************************
;  ARMv7 DCACHE 操作宏
;*********************************************************************************************************/

#define _FLD_MAX_WAY    0x3ff
#define _FLD_MAX_IDX    0x7fff

MACRO_DEF(armDCacheV7Op Op CRm)
    STMFD   SP!, {R4-R11}
    ARM_DMB()
    MRC     p15, 1, R0, c0, c0, 1                                       ;/* Read CLID register           */
    ANDS    R3, R0, #0x7000000                                          ;/* Get Level of Coherency       */
    MOV     R3, R3, LSR #23
    BEQ     finished\Op
    MOV     R10, #0

LINE_LABEL(loop1\Op)
    ADD     R2, R10, R10, LSR #1
    MOV     R1, R0, LSR R2
    AND     R1, R1, #7
    CMP     R1, #2
    BLT     skip\Op
    MCR     p15, 2, R10, c0, c0, 0
    ARM_ISB()
    MRC     p15, 1, R1, c0, c0, 0
    AND     R2, R1, #7
    ADD     R2, R2, #4
    LDR     R4, =_FLD_MAX_WAY
    ANDS    R4, R4, R1, LSR #3
#if __SYLIXOS_ARM_ARCH__ >= 5
    CLZ     R5, R4
#else
    .long   0xe16f5f14
#endif
    LDR     R7, =_FLD_MAX_IDX
    ANDS    R7, R7, R1, LSR #13

LINE_LABEL(loop2\Op)
    MOV     R9, R4

LINE_LABEL(loop3\Op)
    ORR     R11, R10, R9, LSL R5
    ORR     R11, R11, R7, LSL R2
    MCR     p15, 0, R11, c7, \CRm, 2
    SUBS    R9, R9, #1
    BGE     loop3\Op
    SUBS    R7, R7, #1
    BGE     loop2\Op

LINE_LABEL(skip\Op)
    ADD     R10, R10, #2
    CMP     R3, R10
    BGT     loop1\Op

LINE_LABEL(finished\Op)
    MOV     R10, #0                                                     ;/*  swith back to cache lvl 0   */
    MCR     p15, 2, R10, c0, c0, 0                                      ;/*  select cache lvl in csselr  */

    ARM_DSB()
    ARM_ISB()
    LDMFD   SP!, {R4-R11}
    BX      LR
    MACRO_END()

;/*********************************************************************************************************
;  ARMv7 回写 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV7FlushAll)
    armDCacheV7Op   FlushAll        c10
    FUNC_END()
    
;/*********************************************************************************************************
;  ARMv7 回写并无效 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV7ClearAll)
	armDCacheV7Op   ClearAll        c14
	FUNC_END()

;/*********************************************************************************************************
;  ARMv7 无效 DCACHE
;*********************************************************************************************************/

FUNC_DEF(armDCacheV7InvalidateAll)
	armDCacheV7Op   InvalidateAll   c6
	FUNC_END()

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
