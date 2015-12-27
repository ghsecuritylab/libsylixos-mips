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
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 12 月 01 日
**
** 描        述: MIPS32 体系构架 CACHE 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0
#include "arch/mips/common/cp0/mipsCp0.h"
/*********************************************************************************************************
  L2 CACHE 支持
*********************************************************************************************************/
#if LW_CFG_MIPS_CACHE_L2 > 0
/*********************************************************************************************************
  L1 CACHE 状态
*********************************************************************************************************/
static INT      iCacheStatus = 0;
#define L1_CACHE_I_EN   0x01
#define L1_CACHE_D_EN   0x02
#define L1_CACHE_EN     (L1_CACHE_I_EN | L1_CACHE_D_EN)
#define L1_CACHE_DIS    0x00
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
extern VOID     mips32DCacheDisable(VOID);
extern VOID     mips32DCacheEnable(VOID);
extern VOID     mips32ICacheDisable(VOID);
extern VOID     mips32ICacheEnable(VOID);

extern VOID     mips32BranchPredictionDisable(VOID);
extern VOID     mips32BranchPredictionEnable(VOID);
extern VOID     mips32BranchPredictorInvalidate(VOID);

extern VOID     mips32DCacheLineFlush(PCHAR  pcAddr);
extern VOID     mips32DCacheLineClear(PCHAR  pcAddr);
extern VOID     mips32DCacheLineInvalidate(PCHAR  pcAddr);

extern VOID     mips32DCacheIndexClear(PCHAR  pcAddr);

extern VOID     mips32ICacheLineInvalidate(PCHAR   pcAddr);
extern VOID     mips32ICacheIndexInvalidate(PCHAR  pcAddr);

extern VOID     mips32ICacheFill(PCHAR  pcAddr);
extern VOID     mips32ICacheIndexStoreTag(PCHAR  pcAddr);
extern VOID     mips32DCacheIndexStoreTag(PCHAR  pcAddr);
/*********************************************************************************************************
  CACHE 信息
*********************************************************************************************************/
static BOOL     _G_bHaveTagHi         = LW_FALSE;                       /*  是否有 TagHi 寄存器         */
static BOOL     _G_bHaveFillI         = LW_FALSE;                       /*  是否有 FillI 操作           */
static BOOL     _G_bHaveHitWritebackD = LW_FALSE;                       /*  是否有 HitWritebackD 操作   */

typedef struct {
    BOOL        CACHE_bPresent;                                         /*  是否存在 Cache              */
    UINT32      CACHE_uiSize;                                           /*  Cache 大小                  */
    UINT32      CACHE_uiLineSize;                                       /*  Cache 行大小                */
    UINT32      CACHE_uiSetNr;                                          /*  组数                        */
    UINT32      CACHE_uiWayNr;                                          /*  路数                        */
    UINT32      CACHE_uiWayStep;                                        /*  路步进                      */
} MIPS_CACHE;

static MIPS_CACHE   _G_ICache, _G_DCache;                               /*  I-Cache 和 D-Cache 信息     */
/*********************************************************************************************************
  CACHE 循环操作时允许的最大大小, 大于该大小时将使用 All 操作
*********************************************************************************************************/
#define MIPS_CACHE_LOOP_OP_MAX_SIZE     (16 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
  CACHE 获得 pvAdrs 与 pvEnd 位置
*********************************************************************************************************/
#define MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiLineSize)              \
        do {                                                                \
            ulEnd  = (addr_t)((size_t)pvAdrs + stBytes);                    \
            pvAdrs = (PVOID)((addr_t)pvAdrs & ~((addr_t)uiLineSize - 1));   \
        } while (0)
/*********************************************************************************************************
  CACHE 回写管线
*********************************************************************************************************/
#define MIPS_PIPE_FLUSH()               MIPS_EXEC_INS("sync")
/*********************************************************************************************************
  内部函数
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: mips32DCacheClear
** 功能描述: D-CACHE 脏数据回写并无效
** 输　入  : pvStart       开始地址
**           pvEnd         结束地址
**           uiStep        步进
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID    mips32DCacheClear (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    PCHAR   pcAddr;

    for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
        mips32DCacheLineClear(pcAddr);
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** 函数名称: mips32DCacheFlush
** 功能描述: D-CACHE 脏数据回写
** 输　入  : pvStart       开始地址
**           pvEnd         结束地址
**           uiStep        步进
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID    mips32DCacheFlush (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    PCHAR   pcAddr;

    if (_G_bHaveHitWritebackD) {
        for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
            mips32DCacheLineFlush(pcAddr);
        }
        MIPS_PIPE_FLUSH();
    } else {
        mips32DCacheClear(pvStart, pvEnd, uiStep);
    }
}
/*********************************************************************************************************
** 函数名称: mips32DCacheInvalidate
** 功能描述: D-CACHE 脏数据无效
** 输　入  : pvStart       开始地址
**           pvEnd         结束地址
**           uiStep        步进
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID    mips32DCacheInvalidate (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    PCHAR   pcAddr;

    for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
        mips32DCacheLineInvalidate(pcAddr);
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** 函数名称: mips32ICacheInvalidate
** 功能描述: I-CACHE 脏数据无效
** 输　入  : pvStart       开始地址
**           pvEnd         结束地址
**           uiStep        步进
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID    mips32ICacheInvalidate (PVOID  pvStart, PVOID  pvEnd, UINT32  uiStep)
{
    PCHAR   pcAddr;

    for (pcAddr = (PCHAR)pvStart; pcAddr < (PCHAR)pvEnd; pcAddr += uiStep) {
        mips32ICacheLineInvalidate(pcAddr);
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** 函数名称: mips32DCacheClearAll
** 功能描述: D-CACHE 所有数据回写并无效
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID    mips32DCacheClearAll (VOID)
{
    INT     iWay;
    PCHAR   pcAddr;
    PCHAR   pcLineAddr;
    PCHAR   pcBaseAddr = (PCHAR)A_K0BASE;
    PCHAR   pcEndAddr  = (PCHAR)(A_K0BASE + _G_DCache.CACHE_uiSize);

    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_DCache.CACHE_uiLineSize) {
        for (pcAddr = pcLineAddr, iWay = 0;
             iWay < _G_DCache.CACHE_uiWayNr;
             iWay++, pcAddr += _G_DCache.CACHE_uiWayStep) {
            mips32DCacheIndexClear(pcAddr);
        }
    }
    MIPS_PIPE_FLUSH();
}
/*********************************************************************************************************
** 函数名称: mips32DCacheFlushAll
** 功能描述: D-CACHE 所有数据回写
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID    mips32DCacheFlushAll (VOID)
{
    mips32DCacheClearAll();
}
/*********************************************************************************************************
** 函数名称: mips32ICacheInvalidateAll
** 功能描述: I-CACHE 所有数据无效
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID    mips32ICacheInvalidateAll (VOID)
{
    INT     iWay;
    PCHAR   pcAddr;
    PCHAR   pcLineAddr;
    PCHAR   pcBaseAddr = (PCHAR)A_K0BASE;
    PCHAR   pcEndAddr  = (PCHAR)(A_K0BASE + _G_ICache.CACHE_uiSize);

    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_ICache.CACHE_uiLineSize) {
        for (pcAddr = pcLineAddr, iWay = 0;
             iWay < _G_ICache.CACHE_uiWayNr;
             iWay++, pcAddr += _G_ICache.CACHE_uiWayStep) {
            mips32ICacheIndexInvalidate(pcAddr);
        }
    }
    MIPS_PIPE_FLUSH();
}
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
        mips32ICacheEnable();
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus |= L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
        mips32BranchPredictionEnable();

    } else {
        mips32DCacheEnable();
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
        mips32ICacheDisable();
#if LW_CFG_MIPS_CACHE_L2 > 0
        if (LW_CPU_GET_CUR_ID() == 0) {
            iCacheStatus &= ~L1_CACHE_I_EN;
        }
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
        mips32BranchPredictionDisable();
        
    } else {
        mips32DCacheDisable();
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
static INT	mips32CacheFlush (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mips32DCacheFlushAll();                                     /*  全部回写                    */
        
        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            mips32DCacheFlush(pvAdrs, (PVOID)ulEnd,                     /*  部分回写                    */
                              _G_DCache.CACHE_uiLineSize);
        }
        
#if LW_CFG_MIPS_CACHE_L2 > 0
        mips32L2FlushAll();
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
static INT	mips32CacheFlushPage (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == DATA_CACHE) {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mips32DCacheFlushAll();                                     /*  全部回写                    */
        
        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            mips32DCacheFlush(pvAdrs, (PVOID)ulEnd,                     /*  部分回写                    */
                              _G_DCache.CACHE_uiLineSize);
        }
        
#if LW_CFG_MIPS_CACHE_L2 > 0
        mips32L2Flush(pvPdrs, stBytes);
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
static INT	mips32CacheInvalidate (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mips32ICacheInvalidateAll();                                /*  ICACHE 全部无效             */
        
        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            mips32ICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes > 0) {                                              /*  必须 > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
            
            if (ulStart & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {   /*  起始地址非 cache line 对齐  */
                ulStart &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                mips32DCacheClear((PVOID)ulStart, (PVOID)ulStart, _G_DCache.CACHE_uiLineSize);
                ulStart += _G_DCache.CACHE_uiLineSize;
            }
            
            if (ulEnd & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {     /*  结束地址非 cache line 对齐  */
                ulEnd &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                mips32DCacheClear((PVOID)ulEnd, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
            }
                                                                        /*  仅无效对齐部分              */
            mips32DCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
            
#if LW_CFG_MIPS_CACHE_L2 > 0
            mips32L2Invalidate(pvAdrs, stBytes);                        /*  虚拟与物理地址必须相同      */
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
static INT	mips32CacheInvalidatePage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mips32ICacheInvalidateAll();                                /*  ICACHE 全部无效             */
        
        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            mips32ICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes > 0) {                                              /*  必须 > 0                    */
            addr_t  ulStart = (addr_t)pvAdrs;
                    ulEnd   = ulStart + stBytes;
                    
            if (ulStart & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {   /*  起始地址非 cache line 对齐  */
                ulStart &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                mips32DCacheClear((PVOID)ulStart, (PVOID)ulStart, _G_DCache.CACHE_uiLineSize);
                ulStart += _G_DCache.CACHE_uiLineSize;
            }
            
            if (ulEnd & ((addr_t)_G_DCache.CACHE_uiLineSize - 1)) {     /*  结束地址非 cache line 对齐  */
                ulEnd &= ~((addr_t)_G_DCache.CACHE_uiLineSize - 1);
                mips32DCacheClear((PVOID)ulEnd, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
            }
                                                                        /*  仅无效对齐部分              */
            mips32DCacheInvalidate((PVOID)ulStart, (PVOID)ulEnd, _G_DCache.CACHE_uiLineSize);
            
#if LW_CFG_MIPS_CACHE_L2 > 0
            mips32L2Invalidate(pvPdrs, stBytes);                        /*  虚拟与物理地址必须相同      */
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
static INT	mips32CacheClear (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;

    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mips32ICacheInvalidateAll();                                /*  ICACHE 全部无效             */
            
        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            mips32ICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mips32DCacheClearAll();                                     /*  全部回写并无效              */
        
        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            mips32DCacheClear(pvAdrs, (PVOID)ulEnd,
                              _G_DCache.CACHE_uiLineSize);              /*  部分回写并无效              */
        }
        
#if LW_CFG_MIPS_CACHE_L2 > 0
        mips32L2ClearAll();
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
static INT	mips32CacheClearPage (LW_CACHE_TYPE cachetype, PVOID pvAdrs, PVOID pvPdrs, size_t stBytes)
{
    addr_t  ulEnd;
    
    if (cachetype == INSTRUCTION_CACHE) {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mips32ICacheInvalidateAll();                                /*  ICACHE 全部无效             */
            
        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);
            mips32ICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
        }
    } else {
        if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
            mips32DCacheClearAll();                                     /*  全部回写并无效              */
        
        } else {
            MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);
            mips32DCacheClear(pvAdrs, (PVOID)ulEnd,
                              _G_DCache.CACHE_uiLineSize);              /*  部分回写并无效              */
        }
        
#if LW_CFG_MIPS_CACHE_L2 > 0
        mips32L2Clear(pvPdrs, stBytes);
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */
    }
    
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
static INT	mips32CacheLock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
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
static INT	mips32CacheUnlock (LW_CACHE_TYPE  cachetype, PVOID  pvAdrs, size_t  stBytes)
{
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: mips32CacheTextUpdate
** 功能描述: 清空(回写内存) D CACHE 无效(访问不命中) I CACHE
** 输　入  : pvAdrs                        虚拟地址
**           stBytes                       长度
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : L2 cache 为统一 CACHE 所以 text update 不需要操作 L2 cache.
*********************************************************************************************************/
static INT	mips32CacheTextUpdate (PVOID  pvAdrs, size_t  stBytes)
{
    addr_t  ulEnd;
    
    if (stBytes >= MIPS_CACHE_LOOP_OP_MAX_SIZE) {
        mips32DCacheFlushAll();                                         /*  DCACHE 全部回写             */
        mips32ICacheInvalidateAll();                                    /*  ICACHE 全部无效             */
        
    } else {
        MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_DCache.CACHE_uiLineSize);

        mips32DCacheFlush(pvAdrs, (PVOID)ulEnd,
                          _G_DCache.CACHE_uiLineSize);                  /*  部分回写                    */

        MIPS_CACHE_GET_END(pvAdrs, stBytes, ulEnd, _G_ICache.CACHE_uiLineSize);

        mips32ICacheInvalidate(pvAdrs, (PVOID)ulEnd, _G_ICache.CACHE_uiLineSize);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: mips32CacheProbe
** 功能描述: CACHE 探测
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  mips32CacheProbe (VOID)
{
    UINT32  uiTemp;
    UINT32  uiConfig = mipsCp0ConfigRead();                             /*  读 Config0                  */

    if (!(uiConfig & (M_ConfigMore))) {                                 /*  没有 Config1, 退出          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No CP0 Config1 Register!\r\n");
        return  (PX_ERROR);
    }

    uiConfig = mipsCp0Config1Read();                                    /*  读 Config1                  */
    uiTemp   = (uiConfig & M_Config1IL) >> S_Config1IL;
    if (uiTemp) {
        _G_ICache.CACHE_bPresent   = LW_TRUE;

        _G_ICache.CACHE_uiLineSize = 2 << uiTemp;

        uiTemp                     = (uiConfig & M_Config1IS) >> S_Config1IS;
        _G_ICache.CACHE_uiSetNr    = 64 << uiTemp;

        uiTemp                     = (uiConfig & M_Config1IA) >> S_Config1IA;
        _G_ICache.CACHE_uiWayNr    = 1 + uiTemp;

        _G_ICache.CACHE_uiSize     = _G_ICache.CACHE_uiSetNr * _G_ICache.CACHE_uiWayNr *
                                     _G_ICache.CACHE_uiLineSize;

        _G_ICache.CACHE_uiWayStep  = _G_ICache.CACHE_uiSetNr * _G_ICache.CACHE_uiLineSize;
    } else {                                                            /*  没有 I-Cache                */
        _G_ICache.CACHE_bPresent   = LW_FALSE;
    }

    uiTemp   = (uiConfig & M_Config1DL) >> S_Config1DL;
    if (uiTemp) {
        _G_DCache.CACHE_uiLineSize = 2 << uiTemp;

        uiTemp                     = (uiConfig & M_Config1DS) >> S_Config1DS;
        _G_DCache.CACHE_uiSetNr    = 64 << uiTemp;

        uiTemp                     = (uiConfig & M_Config1DA) >> S_Config1DA;
        _G_DCache.CACHE_uiWayNr    = 1 + uiTemp;

        _G_DCache.CACHE_uiSize     = _G_DCache.CACHE_uiSetNr * _G_DCache.CACHE_uiWayNr *
                                     _G_DCache.CACHE_uiLineSize;

        _G_DCache.CACHE_uiWayStep  = _G_DCache.CACHE_uiSetNr * _G_DCache.CACHE_uiLineSize;
    } else {                                                            /*  没有 D-Cache                */
        _G_DCache.CACHE_bPresent   = LW_FALSE;
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

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: mips32CacheProbe
** 功能描述: CACHE 探测
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  mips32CacheHwInit (VOID)
{
    PCHAR   pcLineAddr;
    PCHAR   pcBaseAddr = (PCHAR)A_K0BASE;
    PCHAR   pcEndAddr;
    CHAR    cTemp;

    (VOID)cTemp;

    mipsCp0TagLoWrite(0);

    if (_G_bHaveTagHi) {
        mipsCp0TagHiWrite(0);
    }

    pcEndAddr = (PCHAR)(A_K0BASE + _G_ICache.CACHE_uiSize);
    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_ICache.CACHE_uiLineSize) {
        /*
         * clear tag to invalidate
         */
        mips32ICacheIndexStoreTag(pcLineAddr);

        if (_G_bHaveFillI) {
            /*
             * fill so data field parity is correct
             */
            mips32ICacheFill(pcLineAddr);

            /*
             * invalidate again – prudent but not strictly necessay
             */
            mips32ICacheIndexStoreTag(pcLineAddr);
        }
    }

    pcEndAddr = (PCHAR)(A_K0BASE + _G_DCache.CACHE_uiSize);
    /*
     * clear all tags
     */
    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_DCache.CACHE_uiLineSize) {
        mips32DCacheIndexStoreTag(pcLineAddr);
    }

    /*
     * load from each line (in cached space)
     */
    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_DCache.CACHE_uiLineSize) {
        cTemp = *pcLineAddr;
    }

    /*
     * clear all tags
     */
    for (pcLineAddr = pcBaseAddr; pcLineAddr < pcEndAddr; pcLineAddr += _G_DCache.CACHE_uiLineSize) {
        mips32DCacheIndexStoreTag(pcLineAddr);
    }

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
VOID  mips32CacheInit (LW_CACHE_OP *pcacheop,
                       CACHE_MODE   uiInstruction,
                       CACHE_MODE   uiData,
                       CPCHAR       pcMachineName)
{
    INT     iError;

    if (lib_strcmp(pcMachineName, MIPS_MACHINE_LS1B) == 0) {
        _G_bHaveTagHi         = LW_FALSE;
        _G_bHaveFillI         = LW_FALSE;
        _G_bHaveHitWritebackD = LW_FALSE;
    }

    iError = mips32CacheProbe();
    if (iError != ERROR_NONE) {
        return;
    }

    iError = mips32CacheHwInit();
    if (iError != ERROR_NONE) {
        return;
    }

#if LW_CFG_MIPS_CACHE_L2 > 0
    mips32L2Init(uiInstruction, uiData, pcMachineName);
#endif                                                                  /*  LW_CFG_MIPS_CACHE_L2 > 0    */

#if LW_CFG_SMP_EN > 0
    pcacheop->CACHEOP_ulOption = CACHE_TEXT_UPDATE_MP;
#else
    pcacheop->CACHEOP_ulOption = 0ul;
#endif                                                                  /*  LW_CFG_SMP_EN               */

    if (_G_ICache.CACHE_uiLineSize && _G_DCache.CACHE_uiLineSize) {
        pcacheop->CACHEOP_iCacheLine = min(_G_ICache.CACHE_uiLineSize, _G_DCache.CACHE_uiLineSize);
    } else {
        pcacheop->CACHEOP_iCacheLine = max(_G_ICache.CACHE_uiLineSize, _G_DCache.CACHE_uiLineSize);
    }

    pcacheop->CACHEOP_iILoc                 = CACHE_LOCATION_VIPT;
    pcacheop->CACHEOP_iDLoc                 = CACHE_LOCATION_VIPT;

    pcacheop->CACHEOP_pfuncEnable           = mips32CacheEnable;
    pcacheop->CACHEOP_pfuncDisable          = mips32CacheDisable;
    
    pcacheop->CACHEOP_pfuncLock             = mips32CacheLock;          /*  暂时不支持锁定操作          */
    pcacheop->CACHEOP_pfuncUnlock           = mips32CacheUnlock;
    
    pcacheop->CACHEOP_pfuncFlush            = mips32CacheFlush;
    pcacheop->CACHEOP_pfuncFlushPage        = mips32CacheFlushPage;
    pcacheop->CACHEOP_pfuncInvalidate       = mips32CacheInvalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage   = mips32CacheInvalidatePage;
    pcacheop->CACHEOP_pfuncClear            = mips32CacheClear;
    pcacheop->CACHEOP_pfuncClearPage        = mips32CacheClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate       = mips32CacheTextUpdate;
    
#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc        = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign   = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree          = API_VmmDmaFree;
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
VOID  mips32CacheReset (CPCHAR  pcMachineName)
{
    mips32ICacheInvalidateAll();
    mips32DCacheDisable();
    mips32ICacheDisable();
    mips32BranchPredictorInvalidate();
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
