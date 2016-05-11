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
** 文   件   名: ppcCache60x.c
**
** 创   建   人: Yang.HaiFeng (杨海峰)
**
** 文件创建日期: 2016 年 01 月 18 日
**
** 描        述: PowerPC 60X 体系构架 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "ppcCache60x.h"
/*********************************************************************************************************
  L2 CACHE 支持
*********************************************************************************************************/
#if LW_CFG_PPC_CACHE_L2 > 0
#include "../l2/ppcL2.h"
/*********************************************************************************************************
  L1 CACHE 状态
*********************************************************************************************************/
static INT              iCacheStatus = 0;
#define L1_CACHE_I_EN   0x01
#define L1_CACHE_D_EN   0x02
#define L1_CACHE_EN     (L1_CACHE_I_EN | L1_CACHE_D_EN)
#define L1_CACHE_DIS    0x00
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
/*********************************************************************************************************
  CACHE 循环操作时允许的最大大小, 大于该大小时将使用 All 操作
*********************************************************************************************************/
#define PPC_CACHE_LOOP_OP_MAX_SIZE     (8 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
  CACHE 获得 pvAdrs 与 pvEnd 位置
*********************************************************************************************************/
#define PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiLineSize)               \
        do {                                                                \
            ulEnd  = (addr_t)((size_t)pvAdrs + stBytes);                    \
            pvAdrs = (PVOID)((addr_t)pvAdrs & ~((addr_t)uiLineSize - 1));   \
        } while (0)
/*********************************************************************************************************
  CACHE 信息
*********************************************************************************************************/
typedef struct {
    UINT32              CACHE_uiSize;                                   /*  Cache 大小                  */
    UINT32              CACHE_uiLineSize;                               /*  Cache 行大小                */
    UINT32              CACHE_uiSetNr;                                  /*  组数                        */
    UINT32              CACHE_uiWayNr;                                  /*  路数                        */
    UINT32              CACHE_uiWayStep;                                /*  路步进                      */
} PPC_CACHE;

static PPC_CACHE        _G_ICache, _G_DCache;                           /*  I-Cache 和 D-Cache 信息     */
/*********************************************************************************************************
  Pointer of a page-aligned cacheable region to use as a flush buffer.
*********************************************************************************************************/
UINT8                  *G_pucPpc60xCacheReadBuffer;
/*********************************************************************************************************
  声明 CACHE 驱动
*********************************************************************************************************/
extern PPC60X_L1C_DRIVER    G_ppc603CacheDriver;
extern PPC60X_L1C_DRIVER    G_ppc604CacheDriver;
extern PPC60X_L1C_DRIVER    G_ppc745xCacheDriver;
extern PPC60X_L1C_DRIVER    G_ppc83xxCacheDriver;
extern PPC60X_L1C_DRIVER    G_ppcEC603CacheDriver;

static PPC60X_L1C_DRIVER   *_G_pCacheDriver = LW_NULL;
/*********************************************************************************************************
  定义 CACHE 操作宏
*********************************************************************************************************/
#define ppc60xDCacheDisable                 _G_pCacheDriver->L1CD_pfuncDCacheDisable
#define ppc60xDCacheEnable                  _G_pCacheDriver->L1CD_pfuncDCacheEnable

#define ppc60xICacheDisable                 _G_pCacheDriver->L1CD_pfuncICacheDisable
#define ppc60xICacheEnable                  _G_pCacheDriver->L1CD_pfuncICacheEnable

#define ppc60xDCacheClearAll                _G_pCacheDriver->L1CD_pfuncDCacheClearAll
#define ppc60xDCacheFlushAll                _G_pCacheDriver->L1CD_pfuncDCacheFlushAll
#define ppc60xICacheInvalidateAll           _G_pCacheDriver->L1CD_pfuncICacheInvalidateAll

#define ppc60xDCacheClear                   _G_pCacheDriver->L1CD_pfuncDCacheClear
#define ppc60xDCacheFlush                   _G_pCacheDriver->L1CD_pfuncDCacheFlush
#define ppc60xDCacheInvalidate              _G_pCacheDriver->L1CD_pfuncDCacheInvalidate
#define ppc60xICacheInvalidate              _G_pCacheDriver->L1CD_pfuncICacheInvalidate

#define ppc60xBranchPredictionDisable       _G_pCacheDriver->L1CD_pfuncBranchPredictionDisable
#define ppc60xBranchPredictionEnable        _G_pCacheDriver->L1CD_pfuncBranchPredictionEnable
#define ppc60xBranchPredictorInvalidate     _G_pCacheDriver->L1CD_pfuncBranchPredictorInvalidate
/*********************************************************************************************************
  内部函数
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: ppc60xCacheEnable
** 功能描述: 使能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppc60xCacheEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        ppc60xICacheEnable();
#if LW_CFG_PPC_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
        ppc60xBranchPredictionEnable();

    } else {
        ppc60xDCacheEnable();
#if LW_CFG_PPC_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

#if LW_CFG_PPC_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) &&
        (iCacheStatus == L1_CACHE_EN)) {
        ppcL2Enable();
    }
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppc60xCacheDisable
** 功能描述: 禁能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppc60xCacheDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        ppc60xICacheDisable();
#if LW_CFG_PPC_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
        ppc60xBranchPredictionDisable();

    } else {
        ppc60xDCacheDisable();
#if LW_CFG_PPC_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

#if LW_CFG_PPC_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) &&
        (iCacheStatus == L1_CACHE_DIS)) {
        ppcL2Disable();
    }
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppc60xCacheFlush
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 由于 L2 为物理地址 tag 所以这里暂时使用 L2 全部回写指令.
*********************************************************************************************************/
static INT  ppc60xCacheFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == DATA_CACHE) {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xDCacheFlushAll();                                     /*  全部回写                    */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppc60xDCacheFlush(pvAdrs, (PVOID)ulEnd,                     /*  部分回写                    */
                              _G_DCache.CACHE_uiLineSize);
        }

#if LW_CFG_PPC_CACHE_L2 > 0
        ppcL2FlushAll();
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppc60xCacheFlushPage
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppc60xCacheFlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == DATA_CACHE) {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xDCacheFlushAll();                                     /*  全部回写                    */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppc60xDCacheFlush(pvAdrs, (PVOID)ulEnd,                     /*  部分回写                    */
                              _G_DCache.CACHE_uiLineSize);
        }

#if LW_CFG_PPC_CACHE_L2 > 0
        ppcL2Flush(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppc60xCacheInvalidate
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址 (pvAdrs 必须等于物理地址)
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 此函数如果操作 DCACHE pvAdrs 虚拟地址与物理地址必须相同.
*********************************************************************************************************/
static INT  ppc60xCacheInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xICacheInvalidateAll();                                /*  ICACHE 全部无效             */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            ppc60xICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes > 0) {                                              /*  必须 > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;

            if (ulStart & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {   /*  起始地址非 cache line 对齐  */
                ulStart &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                ppc60xDCacheClear((PVOID)ulStart, (PVOID)ulStart, _G_DCache.CACHE_uiLineSize);
                ulStart += _G_DCache.CACHE_uiLineSize;
            }

            if (ulEnd & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {     /*  结束地址非 cache line 对齐  */
                ulEnd &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                ppc60xDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
            }
                                                                        /*  仅无效对齐部分              */
            ppc60xDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);

#if LW_CFG_PPC_CACHE_L2 > 0
            ppcL2Invalidate(pvAdrs, stBytes);                           /*  虚拟与物理地址必须相同      */
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppc60xCacheInvalidatePage
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppc60xCacheInvalidatePage (LW_CACHE_TYPE    cachetype,
                                       PVOID            pvAdrs,
                                       PVOID            pvPdrs,
                                       size_t           stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xICacheInvalidateAll();                                /*  ICACHE 全部无效             */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            ppc60xICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes > 0) {                                              /*  必须 > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;

            if (ulStart & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {   /*  起始地址非 cache line 对齐  */
                ulStart &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                ppc60xDCacheClear((PVOID)ulStart, (PVOID)ulStart, _G_DCache.CACHE_uiLineSize);
                ulStart += _G_DCache.CACHE_uiLineSize;
            }

            if (ulEnd & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {     /*  结束地址非 cache line 对齐  */
                ulEnd &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                ppc60xDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
            }
                                                                        /*  仅无效对齐部分              */
            ppc60xDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);

#if LW_CFG_PPC_CACHE_L2 > 0
            ppcL2Invalidate(pvPdrs, stBytes);                           /*  虚拟与物理地址必须相同      */
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppc60xCacheClear
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 由于 L2 为物理地址 tag 所以这里暂时使用 L2 全部回写并无效指令.
*********************************************************************************************************/
static INT  ppc60xCacheClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xICacheInvalidateAll();                                /*  ICACHE 全部无效             */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            ppc60xICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xDCacheClearAll();                                     /*  全部回写并无效              */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppc60xDCacheClear(pvAdrs, (PVOID)ulEnd,
                              _G_DCache.CACHE_uiLineSize);              /*  部分回写并无效              */
        }

#if LW_CFG_PPC_CACHE_L2 > 0
        ppcL2ClearAll();
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppc60xCacheClearPage
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppc60xCacheClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xICacheInvalidateAll();                                /*  ICACHE 全部无效             */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            ppc60xICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xDCacheClearAll();                                     /*  全部回写并无效              */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppc60xDCacheClear(pvAdrs, (PVOID)ulEnd,
                              _G_DCache.CACHE_uiLineSize);              /*  部分回写并无效              */
        }

#if LW_CFG_PPC_CACHE_L2 > 0
        ppcL2Clear(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppc60xCacheLock
** 功能描述: 锁定指定类型的 CACHE
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppc60xCacheLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: ppc60xCacheUnlock
** 功能描述: 解锁指定类型的 CACHE
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppc60xCacheUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: ppc60xCacheTextUpdate
** 功能描述: 清空(回写内存) D CACHE 无效(访问不命中) I CACHE
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : L2 cache 为统一 CACHE 所以 text update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT  ppc60xCacheTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
        ppc60xDCacheFlushAll();                                         /*  DCACHE 全部回写             */
        ppc60xICacheInvalidateAll();                                    /*  ICACHE 全部无效             */

    } else {
        PVOID   pvAdrsBak = pvAdrs;

        PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
        ppc60xDCacheFlush(pvAdrs, (PVOID)ulEnd,
                          _G_DCache.CACHE_uiLineSize);                  /*  部分回写                    */

        PPC_CACHE_GET_END(pvAdrsBak, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
        ppc60xICacheInvalidate(pvAdrsBak, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppc60xCacheDataUpdate
** 功能描述: 回写 D CACHE (仅回写 CPU 独享级 CACHE)
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
**           bInv                          是否为回写无效
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : L2 cache 为统一 CACHE 所以 data update 不需要操作 L2 cache.
*********************************************************************************************************/
INT  ppc60xCacheDataUpdate (PVOID  pvAdrs, size_t  stBytes, BOOL  bInv)
{
    addr_t  ulEnd;

    if (bInv == LW_FALSE) {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xDCacheFlushAll();                                     /*  全部回写                    */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppc60xDCacheFlush(pvAdrs, (PVOID)ulEnd,
                              _G_DCache.CACHE_uiLineSize);              /*  部分回写                    */
        }

    } else {
        if (stBytes >= PPC_CACHE_LOOP_OP_MAX_SIZE) {
            ppc60xDCacheClearAll();                                     /*  全部回写                    */

        } else {
            PPC_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            ppc60xDCacheClear(pvAdrs, (PVOID)ulEnd,
                              _G_DCache.CACHE_uiLineSize);              /*  部分回写                    */
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: ppc60xCacheProbe
** 功能描述: CACHE 探测
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  ppc60xCacheProbe (CPCHAR       pcMachineName)
{
    static BOOL  bProbed = LW_FALSE;

    if (bProbed) {
        return  (ERROR_NONE);
    }

    if (lib_strcmp(pcMachineName, PPC_MACHINE_750) == 0) {
        _G_pCacheDriver = &G_ppc604CacheDriver;

        _G_ICache.CACHE_uiLineSize  = 32;
        _G_ICache.CACHE_uiWayNr     = 8;
        _G_ICache.CACHE_uiSetNr     = 128;
        _G_ICache.CACHE_uiSize      = _G_ICache.CACHE_uiSetNr * _G_ICache.CACHE_uiWayNr * \
                                      _G_ICache.CACHE_uiLineSize;
        _G_ICache.CACHE_uiWayStep   = _G_ICache.CACHE_uiSetNr * _G_ICache.CACHE_uiLineSize;

        _G_DCache.CACHE_uiLineSize  = 32;
        _G_DCache.CACHE_uiWayNr     = 8;
        _G_DCache.CACHE_uiSetNr     = 128;
        _G_DCache.CACHE_uiSize      = _G_DCache.CACHE_uiSetNr * _G_DCache.CACHE_uiWayNr * \
                                      _G_DCache.CACHE_uiLineSize;
        _G_DCache.CACHE_uiWayStep   = _G_DCache.CACHE_uiSetNr * _G_DCache.CACHE_uiLineSize;

    } else if (lib_strcmp(pcMachineName, PPC_MACHINE_MPC83XX) == 0) {
        _G_pCacheDriver = &G_ppc83xxCacheDriver;

        _G_ICache.CACHE_uiLineSize  = 32;
        _G_ICache.CACHE_uiWayNr     = 8;
        _G_ICache.CACHE_uiSetNr     = 128;
        _G_ICache.CACHE_uiSize      = _G_ICache.CACHE_uiSetNr * _G_ICache.CACHE_uiWayNr * \
                                      _G_ICache.CACHE_uiLineSize;
        _G_ICache.CACHE_uiWayStep   = _G_ICache.CACHE_uiSetNr * _G_ICache.CACHE_uiLineSize;

        _G_DCache.CACHE_uiLineSize  = 32;
        _G_DCache.CACHE_uiWayNr     = 8;
        _G_DCache.CACHE_uiSetNr     = 128;
        _G_DCache.CACHE_uiSize      = _G_DCache.CACHE_uiSetNr * _G_DCache.CACHE_uiWayNr * \
                                      _G_DCache.CACHE_uiLineSize;
        _G_DCache.CACHE_uiWayStep   = _G_DCache.CACHE_uiSetNr * _G_DCache.CACHE_uiLineSize;

    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
        return  (PX_ERROR);
    }

    /*
     * Alloc a page-aligned cacheable region to use as a flush buffer.
     * Worst case PLRU flush needs 1.5 * cache size.
     */
    G_pucPpc60xCacheReadBuffer = __KHEAP_ALLOC_ALIGN(_G_DCache.CACHE_uiSize * 3 / 2,
                                                     LW_CFG_VMM_PAGE_SIZE);
    if (!G_pucPpc60xCacheReadBuffer) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        return  (PX_ERROR);
    }

    _DebugFormat(__LOGMESSAGE_LEVEL, "L1 I-CACHE size %dKB (%d line size, %d way, %d set).\r\n",
                 _G_ICache.CACHE_uiSize / 1024,
                 _G_ICache.CACHE_uiLineSize,
                 _G_ICache.CACHE_uiWayNr,
                 _G_ICache.CACHE_uiSetNr);

    _DebugFormat(__LOGMESSAGE_LEVEL, "L1 D-CACHE size %dKB (%d line size, %d way, %d set).\r\n",
                 _G_DCache.CACHE_uiSize / 1024,
                 _G_DCache.CACHE_uiLineSize,
                 _G_DCache.CACHE_uiWayNr,
                 _G_DCache.CACHE_uiSetNr);

    bProbed = LW_TRUE;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: archCacheInit
** 功能描述: 初始化 CACHE
** 输　入  : pcacheop       CACHE 操作函数集
**           uiInstruction  指令 CACHE 参数
**           uiData         数据 CACHE 参数
**           pcMachineName  机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  ppc60xCacheInit (LW_CACHE_OP *pcacheop,
                       CACHE_MODE   uiInstruction,
                       CACHE_MODE   uiData,
                       CPCHAR       pcMachineName)
{
    INT     iError;

    iError = ppc60xCacheProbe(pcMachineName);
    if (iError != ERROR_NONE) {
        return;
    }

#if LW_CFG_PPC_CACHE_L2 > 0
    ppcL2Init(uiInstruction, uiData, pcMachineName);
#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */

#if LW_CFG_SMP_EN > 0
    pcacheop->CACHEOP_ulOption = CACHE_TEXT_UPDATE_MP;
#else
    pcacheop->CACHEOP_ulOption = 0ul;
#endif                                                                  /*  LW_CFG_SMP_EN               */

    pcacheop->CACHEOP_iILoc = CACHE_LOCATION_VIPT;
    pcacheop->CACHEOP_iDLoc = CACHE_LOCATION_VIPT;

    pcacheop->CACHEOP_iICacheLine = _G_ICache.CACHE_uiLineSize;
    pcacheop->CACHEOP_iDCacheLine = _G_DCache.CACHE_uiLineSize;

    pcacheop->CACHEOP_iICacheWaySize = _G_ICache.CACHE_uiWayStep;
    pcacheop->CACHEOP_iDCacheWaySize = _G_DCache.CACHE_uiWayStep;

    _DebugFormat(__LOGMESSAGE_LEVEL, "PowerPC I-Cache line size = %d bytes Way size = %d bytes.\r\n",
                 pcacheop->CACHEOP_iICacheLine, pcacheop->CACHEOP_iICacheWaySize);
    _DebugFormat(__LOGMESSAGE_LEVEL, "PowerPC D-Cache line size = %d bytes Way size = %d bytes.\r\n",
                 pcacheop->CACHEOP_iDCacheLine, pcacheop->CACHEOP_iDCacheWaySize);

    pcacheop->CACHEOP_pfuncEnable  = ppc60xCacheEnable;
    pcacheop->CACHEOP_pfuncDisable = ppc60xCacheDisable;

    pcacheop->CACHEOP_pfuncLock   = ppc60xCacheLock;                    /*  暂时不支持锁定操作          */
    pcacheop->CACHEOP_pfuncUnlock = ppc60xCacheUnlock;

    pcacheop->CACHEOP_pfuncFlush          = ppc60xCacheFlush;
    pcacheop->CACHEOP_pfuncFlushPage      = ppc60xCacheFlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = ppc60xCacheInvalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = ppc60xCacheInvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = ppc60xCacheClear;
    pcacheop->CACHEOP_pfuncClearPage      = ppc60xCacheClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = ppc60xCacheTextUpdate;
    pcacheop->CACHEOP_pfuncDataUpdate     = ppc60xCacheDataUpdate;

#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: archCacheReset
** 功能描述: 复位 CACHE
** 输　入  : pcMachineName  机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 如果有 lockdown 必须首先 unlock & invalidate 才能启动
*********************************************************************************************************/
VOID  ppc60xCacheReset (CPCHAR  pcMachineName)
{
    INT     iError;

    iError = ppc60xCacheProbe(pcMachineName);
    if (iError != ERROR_NONE) {
        return;
    }

    ppc60xICacheInvalidateAll();
    ppc60xDCacheDisable();
    ppc60xICacheDisable();
    ppc60xBranchPredictorInvalidate();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
