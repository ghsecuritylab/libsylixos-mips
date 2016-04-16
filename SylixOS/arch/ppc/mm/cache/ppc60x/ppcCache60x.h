/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: ppcCache60x.h
**
** 创   建   人: Yang.HaiFeng (杨海峰)
**
** 文件创建日期: 2016 年 01 月 18 日
**
** 描        述: PowerPC 60X 体系构架 CACHE 驱动.
*********************************************************************************************************/

#ifndef __ARCH_PPCCACHE60X_H
#define __ARCH_PPCCACHE60X_H

/*********************************************************************************************************
  CACHE 驱动
*********************************************************************************************************/

typedef struct {
    VOID     (*L1CD_pfuncDCacheDisable)(VOID);
    VOID     (*L1CD_pfuncDCacheEnable)(VOID);
    VOID     (*L1CD_pfuncICacheDisable)(VOID);
    VOID     (*L1CD_pfuncICacheEnable)(VOID);

    VOID     (*L1CD_pfuncDCacheClearAll)(VOID);
    VOID     (*L1CD_pfuncDCacheFlushAll)(VOID);
    VOID     (*L1CD_pfuncICacheInvalidateAll)(VOID);

    VOID     (*L1CD_pfuncDCacheClear)(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
    VOID     (*L1CD_pfuncDCacheFlush)(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
    VOID     (*L1CD_pfuncDCacheInvalidate)(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
    VOID     (*L1CD_pfuncICacheInvalidate)(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

    VOID     (*L1CD_pfuncBranchPredictionDisable)(VOID);
    VOID     (*L1CD_pfuncBranchPredictionEnable)(VOID);
    VOID     (*L1CD_pfuncBranchPredictorInvalidate)(VOID);
} PPC60X_L1C_DRIVER;

VOID  ppc60xCacheInit(LW_CACHE_OP *pcacheop,
                      CACHE_MODE   uiInstruction,
                      CACHE_MODE   uiData,
                      CPCHAR       pcMachineName);

VOID  ppc60xCacheReset(CPCHAR  pcMachineName);

#endif                                                                  /*  __ARCH_PPCCACHE60X_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
