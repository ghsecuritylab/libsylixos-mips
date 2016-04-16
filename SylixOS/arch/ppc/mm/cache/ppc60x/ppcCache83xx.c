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
** 文   件   名: ppcCache83xx.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 03 月 30 日
**
** 描        述: PowerPC MPC83XX 体系构架 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "ppcCache60x.h"
/*********************************************************************************************************
  外部接口声明
*********************************************************************************************************/
extern VOID     ppc83xxDCacheDisable(VOID);
extern VOID     ppc83xxDCacheEnable(VOID);
extern VOID     ppc83xxICacheDisable(VOID);
extern VOID     ppc83xxICacheEnable(VOID);

extern VOID     ppc83xxDCacheClearAll(VOID);
extern VOID     ppc83xxDCacheFlushAll(VOID);
extern VOID     ppc83xxICacheInvalidateAll(VOID);

extern VOID     ppc83xxDCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc83xxDCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc83xxDCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc83xxICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

extern VOID     ppc83xxBranchPredictionDisable(VOID);
extern VOID     ppc83xxBranchPredictionEnable(VOID);
extern VOID     ppc83xxBranchPredictorInvalidate(VOID);
/*********************************************************************************************************
  MPC83XX CACHE 驱动
*********************************************************************************************************/
PPC60X_L1C_DRIVER  G_ppc83xxCacheDriver = {
    ppc83xxDCacheDisable,
    ppc83xxDCacheEnable,
    ppc83xxICacheDisable,
    ppc83xxICacheEnable,

    ppc83xxDCacheClearAll,
    ppc83xxDCacheFlushAll,
    ppc83xxICacheInvalidateAll,

    ppc83xxDCacheClear,
    ppc83xxDCacheFlush,
    ppc83xxDCacheInvalidate,
    ppc83xxICacheInvalidate,

    ppc83xxBranchPredictionDisable,
    ppc83xxBranchPredictionEnable,
    ppc83xxBranchPredictorInvalidate,
};

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
