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
** 文   件   名: mips32Cache.c
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 10 月 12 日
**
** 描        述: MIPS32 体系构架 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "../mipsCacheCommon.h"
#include "../../../common/cp0/mipsCp0.h"

static UINT32                           uiMIPS32CacheLineSize;
#define MIPS32_CACHE_LOOP_OP_MAX_SIZE   (32 * LW_CFG_KB_SIZE)
#define CACHE_SIZE                      16*1024
#define CACHE_LINE_SIZE                 32
/*********************************************************************************************************
** 函数名称: mips32CacheEnable
** 功能描述: 使能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  mips32CacheEnable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        mipsICacheEnable();
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */

    } else {
        mipsDCacheEnable();
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
    }

#if LW_CFG_MIPS_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) &&
        (iCacheStatus == L1_CACHE_EN)) {
        mips32L2Enable();
    }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */

    return  (ERROR_NONE);
}

/*********************************************************************************************************
** 函数名称: mips32CacheDisable
** 功能描述: 禁能 CACHE
** 输　入  : cachetype      INSTRUCTION_CACHE / DATA_CACHE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  mips32CacheDisable (LW_CACHE_TYPE  cachetype)
{
    if (cachetype == INSTRUCTION_CACHE) {
        mipsICacheDisable();
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */

    } else {
        mipsDCacheDisable();
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_D_EN;
        }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
    }

#if LW_CFG_MIPS_CACHE_L2 > 0
    if ((LW_CPU_GET_CUR_ID() == 0) &&
        (iCacheStatus == L1_CACHE_DIS)) {
        mips32L2Disable();
    }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */

    return  (ERROR_NONE);
}

/*********************************************************************************************************
** 函数名称: mips32CacheLock
** 功能描述: 锁定指定类型的 CACHE
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  mips32CacheLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: mips32CacheUnlock
** 功能描述: 解锁指定类型的 CACHE
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  mips32CacheUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}

/*********************************************************************************************************
** 函数名称: mips32CacheFlush
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 由于 L2 为物理地址 tag 所以这里暂时使用 L2 全部回写指令.
*********************************************************************************************************/
static INT  mips32CacheFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == DATA_CACHE) {
        if (stBytes >= MIPS32_CACHE_LOOP_OP_MAX_SIZE) {
                                                                        /*  全部回写                    */
            mipsDCacheFlush(pvAdrs, (PVOID)(pvAdrs + CACHE_SIZE), uiMIPS32CacheLineSize);

        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiMIPS32CacheLineSize);
                                                                        /*  部分回写                    */
            mipsDCacheFlush(pvAdrs, (PVOID)ulEnd, uiMIPS32CacheLineSize);
        }

#if LW_CFG_MIPS_CACHE_L2 > 0
        mipsL2FlushAll();
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
    }

    return  (ERROR_NONE);
}

/*********************************************************************************************************
** 函数名称: mips32CacheFlushPage
** 功能描述: CACHE 脏数据回写
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  mips32CacheFlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == DATA_CACHE) {
        if (stBytes >= MIPS32_CACHE_LOOP_OP_MAX_SIZE) {
                                                                        /*  全部回写                    */
            mipsDCacheFlush(pvAdrs, (PVOID)(pvAdrs + CACHE_SIZE), uiMIPS32CacheLineSize);

        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiMIPS32CacheLineSize);
                                                                        /*  部分回写                    */
            mipsDCacheFlush(pvAdrs, (PVOID)ulEnd, uiMIPS32CacheLineSize);
        }

#if LW_CFG_MIPS_CACHE_L2 > 0
        mipsL2Flush(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
    }

    return  (ERROR_NONE);
}

/*********************************************************************************************************
** 函数名称: mips32CacheInvalidate
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址 (pvAdrs 必须等于物理地址)
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 此函数如果操作 DCACHE pvAdrs 虚拟地址与物理地址必须相同.
*********************************************************************************************************/
static INT  mips32CacheInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= MIPS32_CACHE_LOOP_OP_MAX_SIZE) {
            mipsICacheInvalidateAll();                                   /*  ICACHE 全部无效            */

        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiMIPS32CacheLineSize);
            mipsICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiMIPS32CacheLineSize);
        }
    } else {
        if (stBytes > 0) {                                              /*  必须 > 0                   */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;

            if (ulStart & ((addr_t)uiMIPS32CacheLineSize - 1)) {        /*  起始地址非 cache line 对齐 */
                ulStart &= ~((addr_t)uiMIPS32CacheLineSize - 1);
                mipsDCacheClear((PVOID)ulStart, (PVOID)ulStart, uiMIPS32CacheLineSize);
                ulStart += uiMIPS32CacheLineSize;
            }

            if (ulEnd & ((addr_t)uiMIPS32CacheLineSize - 1)) {          /*  结束地址非 cache line 对齐 */
                ulEnd &= ~((addr_t)uiMIPS32CacheLineSize - 1);
                mipsDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, uiMIPS32CacheLineSize);
            }
                                                                        /*  仅无效对齐部分              */
            mipsICacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, uiMIPS32CacheLineSize);

#if LW_CFG_MIPS_CACHE_L2 > 0
            mipsL2Invalidate(pvAdrs, stBytes);                          /*  虚拟与物理地址必须相同     */
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }

    return  (ERROR_NONE);
}

/*********************************************************************************************************
** 函数名称: mips32CacheInvalidatePage
** 功能描述: 指定类型的 CACHE 使部分无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  mips32CacheInvalidatePage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= MIPS32_CACHE_LOOP_OP_MAX_SIZE) {
            mipsICacheInvalidateAll();                                  /*  ICACHE 全部无效             */

        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiMIPS32CacheLineSize);
            mipsICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiMIPS32CacheLineSize);
        }
    } else {
        if (stBytes > 0) {                                              /*  必须 > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;

            if (ulStart & ((addr_t)uiMIPS32CacheLineSize - 1)) {        /*  起始地址非 cache line 对齐  */
                ulStart &= ~((addr_t)uiMIPS32CacheLineSize - 1);
                mipsDCacheClear((PVOID)ulStart, (PVOID)ulStart, uiMIPS32CacheLineSize);
                ulStart += uiMIPS32CacheLineSize;
            }

            if (ulEnd & ((addr_t)uiMIPS32CacheLineSize - 1)) {          /*  结束地址非 cache line 对齐  */
                ulEnd &= ~((addr_t)uiMIPS32CacheLineSize - 1);
                mipsDCacheClear((PVOID)ulEnd, (PVOID)ulEnd, uiMIPS32CacheLineSize);
            }
                                                                        /*  仅无效对齐部分              */
            mipsDCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, uiMIPS32CacheLineSize);

#if LW_CFG_MIPS_CACHE_L2 > 0
            mipsL2Invalidate(pvPdrs, stBytes);                          /*  虚拟与物理地址必须相同      */
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
        } else {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "stBytes == 0.\r\n");
        }
    }

    return  (ERROR_NONE);
}

/*********************************************************************************************************
** 函数名称: mips32CacheClear
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : 由于 L2 为物理地址 tag 所以这里暂时使用 L2 全部回写并无效指令.
*********************************************************************************************************/
static INT  mips32CacheClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= MIPS32_CACHE_LOOP_OP_MAX_SIZE) {
            mipsICacheInvalidateAll();                                  /*  ICACHE 全部无效             */

        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiMIPS32CacheLineSize);
            mipsICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiMIPS32CacheLineSize);
        }
    } else {
        if (stBytes >= MIPS32_CACHE_LOOP_OP_MAX_SIZE) {
                                                                        /*  全部回写并无效              */
            mipsDCacheClear(pvAdrs, (PVOID)(pvAdrs + CACHE_SIZE), uiMIPS32CacheLineSize);

        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiMIPS32CacheLineSize);
                                                                        /*  部分回写并无效              */
            mipsDCacheClear(pvAdrs, (PVOID)ulEnd, uiMIPS32CacheLineSize);
        }

#if LW_CFG_MIPS_CACHE_L2 > 0
        mipsL2ClearAll();
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
    }

    return  (ERROR_NONE);
}

/*********************************************************************************************************
** 函数名称: mips32CacheClearPage
** 功能描述: 指定类型的 CACHE 使部分或全部清空(回写内存)并无效(访问不命中)
** 输　入  : cachetype     CACHE 类型
**           pvAdrs        虚拟地址
**           pvPdrs        物理地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  mips32CacheClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= MIPS32_CACHE_LOOP_OP_MAX_SIZE) {
            mipsICacheInvalidateAll();                                  /*  ICACHE 全部无效             */

        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiMIPS32CacheLineSize);
            mipsICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiMIPS32CacheLineSize);
        }
    } else {
        if (stBytes >= MIPS32_CACHE_LOOP_OP_MAX_SIZE) {
                                                                        /*  全部回写并无效              */
            mipsDCacheClear(pvAdrs, (PVOID)(pvAdrs + CACHE_SIZE), uiMIPS32CacheLineSize);

        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiMIPS32CacheLineSize);
                                                                        /*  部分回写并无效              */
            mipsDCacheClear(pvAdrs, (PVOID)ulEnd, uiMIPS32CacheLineSize);
        }

#if LW_CFG_MIPS_CACHE_L2 > 0
        mipsL2Clear(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
    }

    return  (ERROR_NONE);
}

/*********************************************************************************************************
** 函数名称: mips32CacheTextUpdate
** 功能描述: 清空(回写内存) D CACHE 无效(访问不命中) I CACHE
** 输　入  : pvAdrs        虚拟地址
**           stBytes       长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
** 注  意  : L2 cache 为统一 CACHE 所以 text update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT  mips32CacheTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (stBytes >= MIPS32_CACHE_LOOP_OP_MAX_SIZE) {
                                                                        /*  DCACHE 全部回写             */
        mipsDCacheFlush(pvAdrs, (PVOID)(pvAdrs + CACHE_SIZE), uiMIPS32CacheLineSize);
        mipsICacheInvalidateAll();                                      /*  ICACHE 全部无效             */

    } else {
        MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiMIPS32CacheLineSize);
        mipsDCacheFlush(pvAdrs, (PVOID)ulEnd, uiMIPS32CacheLineSize);   /*  部分回写                    */
        mipsICacheInvalidate(pvAdrs, (PVOID)ulEnd, uiMIPS32CacheLineSize);
    }
#if 0
    if (LW_NCPUS > 1) {
        mipsBranchPredictorInvalidateInnerShareable();                  /*  所有核清除分支预测          */
    }
#endif
    return  (ERROR_NONE);
}

/*********************************************************************************************************
** 函数名称: mips32CacheInit
** 功能描述: 初始化 CACHE
** 输　入  : pcacheop       CACHE 操作函数集
**           uiInstruction  指令 CACHE 参数
**           uiData         数据 CACHE 参数
**           pcMachineName  机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  mips32CacheInit (LW_CACHE_OP *pcacheop,
                      CACHE_MODE   uiInstruction,
                      CACHE_MODE   uiData,
                      CPCHAR       pcMachineName)
{
    UINT32  uiCP0COFG1;

#define MIPS32_CP0CFG1IL_LINESIZE(x)    ((x) & M_Config1IL)
#define MIPS32_CACHE_LINESIZE(x)        (2 << MIPS32_CP0CFG1IL_LINESIZE(x))

#if LW_CFG_MIPS_CACHE_L2 > 0
    mipsL2Init(uiInstruction, uiData, pcMachineName);
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */

    uiCP0COFG1                   = mipsCp0Config1Read();
    pcacheop->CACHEOP_iCacheLine = MIPS32_CACHE_LINESIZE(uiCP0COFG1);
    uiMIPS32CacheLineSize         = (UINT32)pcacheop->CACHEOP_iCacheLine;

    _DebugFormat(__LOGMESSAGE_LEVEL, "MIPS Cache line size = %d byte.\r\n",
                 pcacheop->CACHEOP_iCacheLine);

    if ((lib_strcmp(pcMachineName, MIPS_MACHINE_NONE) == 0) ||
        (lib_strcmp(pcMachineName, MIPS_MACHINE_24KF) == 0)) {
        pcacheop->CACHEOP_iILoc      = CACHE_LOCATION_VIPT;
        pcacheop->CACHEOP_iDLoc      = CACHE_LOCATION_VIPT;

    }

    pcacheop->CACHEOP_pfuncEnable  = mips32CacheEnable;
    pcacheop->CACHEOP_pfuncDisable = mips32CacheDisable;

    pcacheop->CACHEOP_pfuncLock    = mips32CacheLock;                   /*  暂时不支持锁定操作          */
    pcacheop->CACHEOP_pfuncUnlock  = mips32CacheUnlock;

    pcacheop->CACHEOP_pfuncFlush          = mips32CacheFlush;
    pcacheop->CACHEOP_pfuncFlushPage      = mips32CacheFlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = mips32CacheInvalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = mips32CacheInvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = mips32CacheClear;
    pcacheop->CACHEOP_pfuncClearPage      = mips32CacheClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = mips32CacheTextUpdate;

#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}
/*********************************************************************************************************
** 函数名称: mips32CacheReset
** 功能描述: 复位 CACHE
** 输　入  : pcMachineName  机器名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 如果有 lockdown 必须首先 unlock & invalidate 才能启动
*********************************************************************************************************/
VOID  mips32CacheReset (CPCHAR  pcMachineName)
{
    mipsICacheInvalidateAll();
    mipsDCacheDisable();
    mipsICacheDisable();
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
