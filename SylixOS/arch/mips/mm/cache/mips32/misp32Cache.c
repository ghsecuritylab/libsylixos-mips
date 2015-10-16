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
#if 0
    UINT32  uiCP0COFG1;

#define ARMv7_CCSIDR_LINESIZE_MASK      0x7
#define ARMv7_CCSIDR_LINESIZE(x)        ((x) & ARMv7_CCSIDR_LINESIZE_MASK)
#define ARMv7_CACHE_LINESIZE(x)         (16 << ARMv7_CCSIDR_LINESIZE(x))

#if LW_CFG_MIPS_CACHE_L2 > 0
    mipsL2Init(uiInstruction, uiData, pcMachineName);
#endif                                                                  /*  LW_CFG_ARM_CACHE_L2 > 0     */

    uiCP0COFG1                     = mipsCp0Config1Read();
    pcacheop->CACHEOP_iCacheLine = ARMv7_CACHE_LINESIZE(uiCP0COFG1);
    uiArmV7CacheLineSize         = (UINT32)pcacheop->CACHEOP_iCacheLine;

    _DebugFormat(__LOGMESSAGE_LEVEL, "ARMv7 Cache line size = %d byte.\r\n",
                 pcacheop->CACHEOP_iCacheLine);

    if ((lib_strcmp(pcMachineName, ARM_MACHINE_A5) == 0) ||
        (lib_strcmp(pcMachineName, ARM_MACHINE_A7) == 0)) {
        pcacheop->CACHEOP_iILoc      = CACHE_LOCATION_VIPT;
        pcacheop->CACHEOP_iDLoc      = CACHE_LOCATION_PIPT;

    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_A9) == 0) {
        pcacheop->CACHEOP_iILoc      = CACHE_LOCATION_VIPT;
        pcacheop->CACHEOP_iDLoc      = CACHE_LOCATION_PIPT;
        armAuxControlFeatureEnable(AUX_CTRL_A9_L1_PREFETCH);            /*  Cortex-A9 使能 L1 预取      */

    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_A8) == 0) {
        pcacheop->CACHEOP_iILoc      = CACHE_LOCATION_VIPT;
        pcacheop->CACHEOP_iDLoc      = CACHE_LOCATION_PIPT;
        armAuxControlFeatureEnable(AUX_CTRL_A8_FORCE_ETM_CLK |
                                   AUX_CTRL_A8_FORCE_MAIN_CLK |
                                   AUX_CTRL_A8_L1NEON |
                                   AUX_CTRL_A8_FORCE_NEON_CLK |
                                   AUX_CTRL_A8_FORCE_NEON_SIGNAL);

    } else if (lib_strcmp(pcMachineName, ARM_MACHINE_A15) == 0) {
        pcacheop->CACHEOP_iILoc      = CACHE_LOCATION_PIPT;
        pcacheop->CACHEOP_iDLoc      = CACHE_LOCATION_PIPT;
        armAuxControlFeatureEnable(AUX_CTRL_A15_FORCE_MAIN_CLK |
                                   AUX_CTRL_A15_FORCE_NEON_CLK);

    } else if ((lib_strcmp(pcMachineName, ARM_MACHINE_A53)     == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_A57)     == 0) ||
               (lib_strcmp(pcMachineName, ARM_MACHINE_FT1500A) == 0)) { /*  ARMv8 32 位模式             */
        pcacheop->CACHEOP_iILoc      = CACHE_LOCATION_PIPT;
        pcacheop->CACHEOP_iDLoc      = CACHE_LOCATION_PIPT;
    }

    pcacheop->CACHEOP_pfuncEnable  = armCacheV7Enable;
    pcacheop->CACHEOP_pfuncDisable = armCacheV7Disable;

    pcacheop->CACHEOP_pfuncLock    = armCacheV7Lock;                    /*  暂时不支持锁定操作          */
    pcacheop->CACHEOP_pfuncUnlock  = armCacheV7Unlock;

    pcacheop->CACHEOP_pfuncFlush          = armCacheV7Flush;
    pcacheop->CACHEOP_pfuncFlushPage      = armCacheV7FlushPage;
    pcacheop->CACHEOP_pfuncInvalidate     = armCacheV7Invalidate;
    pcacheop->CACHEOP_pfuncInvalidatePage = armCacheV7InvalidatePage;
    pcacheop->CACHEOP_pfuncClear          = armCacheV7Clear;
    pcacheop->CACHEOP_pfuncClearPage      = armCacheV7ClearPage;
    pcacheop->CACHEOP_pfuncTextUpdate     = armCacheV7TextUpdate;

#if LW_CFG_VMM_EN > 0
    pcacheop->CACHEOP_pfuncDmaMalloc      = API_VmmDmaAlloc;
    pcacheop->CACHEOP_pfuncDmaMallocAlign = API_VmmDmaAllocAlign;
    pcacheop->CACHEOP_pfuncDmaFree        = API_VmmDmaFree;
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
#endif
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
#if 0
    armICacheInvalidateAll();
    armDCacheV7Disable();
    armICacheDisable();
    armBranchPredictorInvalidate();
#endif
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
