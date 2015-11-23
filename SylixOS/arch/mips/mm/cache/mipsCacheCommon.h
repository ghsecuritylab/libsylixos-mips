/**********************************************************************************************************
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
** 文   件   名: mipsCacheCommon.h
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 09 月 01 日
**
** 描        述: MIPS 体系架构 CACHE 通用函数支持.
*********************************************************************************************************/

#ifndef __MIPSCACHECOMMON_H
#define __MIPSCACHECOMMON_H

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0

VOID    mipsICacheEnable(VOID);
VOID    mipsDCacheEnable(VOID);
VOID    mipsICacheDisable(VOID);
VOID    mipsDCacheDisable(VOID);
VOID    mipsICacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
VOID    mipsICacheInvalidateAll(VOID);
VOID    mipsDCacheInvalidate(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
VOID    mipsDCacheFlush(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);
VOID    mipsDCacheClear(PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep);

/*********************************************************************************************************
  CACHE 获得 pvAdrs 与 pvEnd 位置
*********************************************************************************************************/

#define MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiLineSize)              \
        do {                                                                \
            ulEnd  = (addr_t)((size_t)pvAdrs + stBytes);                    \
            pvAdrs = (PVOID)((addr_t)pvAdrs & ~((addr_t)uiLineSize - 1));   \
        } while (0)

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
#endif                                                                  /*  __MIPSCACHECOMMON_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
