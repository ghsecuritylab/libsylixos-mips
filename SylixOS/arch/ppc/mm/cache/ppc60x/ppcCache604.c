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
** 文   件   名: ppcCache604.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2016 年 03 月 30 日
**
** 描        述: PowerPC 604 体系构架 CACHE 驱动.
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
extern VOID     ppc604DCacheDisable(VOID);
extern VOID     ppc604DCacheEnable(VOID);
extern VOID     ppc604ICacheDisable(VOID);
extern VOID     ppc604ICacheEnable(VOID);

extern VOID     ppc604DCacheClearAll(VOID);
extern VOID     ppc604DCacheFlushAll(VOID);
extern VOID     ppc604ICacheInvalidateAll(VOID);

extern VOID     ppc604DCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc604DCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc604DCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
extern VOID     ppc604ICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

extern VOID     ppc604BranchPredictionDisable(VOID);
extern VOID     ppc604BranchPredictionEnable(VOID);
extern VOID     ppc604BranchPredictorInvalidate(VOID);
/*********************************************************************************************************
  604 CACHE 驱动
*********************************************************************************************************/
PPC60X_L1C_DRIVER  G_ppc604CacheDriver = {
    ppc604DCacheDisable,
    ppc604DCacheEnable,
    ppc604ICacheDisable,
    ppc604ICacheEnable,

    ppc604DCacheClearAll,
    ppc604DCacheFlushAll,
    ppc604ICacheInvalidateAll,

    ppc604DCacheClear,
    ppc604DCacheFlush,
    ppc604DCacheInvalidate,
    ppc604ICacheInvalidate,

    ppc604BranchPredictionDisable,
    ppc604BranchPredictionEnable,
    ppc604BranchPredictorInvalidate,
};

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
