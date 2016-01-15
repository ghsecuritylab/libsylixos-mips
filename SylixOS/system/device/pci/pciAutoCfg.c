/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: pciAutoCfg.c
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2015 年 12 月 15 日
**
** 描        述: PCI 总线自动配置.
**
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "pciAutoCfg.h"
/*********************************************************************************************************
  参数定义
*********************************************************************************************************/
#define __PCI_AUTO_CFG_CMD_MASK             0xffff0000
#define __PCI_AUTO_CFG_NO_ALLOCATION        0xffffffff

#define __PCI_AUTO_CFG_ABSENT_F             0xffff
#define __PCI_AUTO_CFG_ABSENT_0             0x0000
/*********************************************************************************************************
  需要 API_PciAutoCfgCtl 设置
*********************************************************************************************************/
#define __PCI_AUTO_CFG_CARDBUS_IO16_SIZE    0x1000
#define __PCI_AUTO_CFG_CARDBUS_MEMIO32_SIZE (1 * 1024 * 1024)
#define __PCI_AUTO_CFG_CARDBUS_MEM32_SIZE   (1 * 1024 * 1024)
/*********************************************************************************************************
  自动配置选项
*********************************************************************************************************/
typedef struct {
    INT                                     PACO_iIndex;
    UINT32                                  PACO_uiBusMax;

    UINT32                                  PACO_uiMem32Addr;
    UINT32                                  PACO_uiMem32Size;
    UINT32                                  PACO_uiMemIo32Addr;
    UINT32                                  PACO_uiMemIo32Size;
    UINT32                                  PACO_uiIo32Addr;
    UINT32                                  PACO_uiIo32Size;
    UINT32                                  PACO_uiIo16Addr;
    UINT32                                  PACO_uiIo16Size;

    INT                                     PACO_iCacheSize;

    UINT32                                  PACO_uiLatencyMax;

    BOOL                                    PACO_bAutoIntRouting;
    PCI_AUTO_CFG_INT_ASSIGN_FUNC_PTR        PACO_pfuncIntAssign;

    PCI_AUTO_CFG_INCLUDE_FUNC_PTR           PACO_pfuncInclude;

    PCI_AUTO_CFG_BRIDGE_PRE_INIT_FUNC_PTR   PACO_pfuncBridgePreInit;
    PCI_AUTO_CFG_BRIDGE_POST_INIT_FUNC_PTR  PACO_pfuncBridgePostInit;

    PCI_AUTO_CFG_ROLLCALL_FUNC_PTR          PACO_pfuncRollcall;

    BOOL                                    PACO_bInitFlag;             /* 内部使用                     */
    PCI_AUTO_CFG_LOG_MSG_FUNC_PTR           PACO_pfuncLogMsg;

    PCI_AUTO_CFG_LATENCY_MAX_FUNC_PTR       PACO_pfuncLatencyMax;
    PVOID                                   PACO_pvLatencyMaxArg;

    UINT32                                  PACO_uiMemBusResMin;        /* 每个总线基本内存保留最小值   */
    UINT32                                  PACO_uiMemBusExtraRes;
    UINT32                                  PACO_uiMemMax;              /* 内存最大值                   */
    UINT32                                  PACO_uiMem32Used;           /* 32 位内存空间实际使用量      */
    UINT32                                  PACO_uiMemIo32Used;         /* 32 位内存 IO 空间实际使用量  */
    UINT32                                  PACO_uiIo32Used;            /* 32 位 IO 空间实际使用量      */
    UINT32                                  PACO_uiIo16Used;            /* 16 位 IO 空间实际使用量      */
    PCI_AUTO_CFG_MEM_BUS_EXTRA_FUNC_PTR     PACO_pfuncMemBusExtra;

    BOOL                                    PACO_bFb2bEnable;           /* Fast Back TO Back 使能       */
    BOOL                                    PACO_bFb2bActive;           /* Fast Back TO Back 有效       */

    PCI_AUTO_CFG_LOC                       *PACO_ppaclFuncList;
    INT                                     PACO_iFuncListEntries;

    BOOL                                    PACO_bResourcesMin;
} __PCI_AUTO_CFG_OPTS;
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static __PCI_AUTO_CFG_OPTS       _G_pacoPciAutoCfgOpts;
static __PCI_AUTO_CFG_OPTS      *_G_ppacoPciAutoCfgOpts = LW_NULL;

#ifdef PCI_AUTO_CFG_LIST_STATIC                                         /* PCI_AUTO_CFG_LIST_STATIC     */
static PCI_AUTO_CFG_LOC          _G_paclPciAutoCfgList[PCI_AUTO_CFG_LIST_MAX];
#endif                                                                  /* PCI_AUTO_CFG_LIST_STATIC     */

static PCI_AUTO_CFG_LOC         *_G_ppaclPciAutoCfgList  = LW_NULL;
static UINT32                    _G_uiPciAutoCfgListSize = 0;

static UINT8                     _G_ucPciAutoCfgIntRoutingTable[4] = {
                                    (UINT8)0xff, (UINT8)0xff, (UINT8)0xff, (UINT8)0xff
                                 };
/*********************************************************************************************************
  Log 消息
*********************************************************************************************************/
#ifdef PCI_AUTO_CFG_DEBUG
static BOOL        _G_b_PciAutoCfgDebug = LW_TRUE;
#else
static BOOL        _G_b_PciAutoCfgDebug = LW_FALSE;
#define __PCI_AUTO_CFG_LOG_MSG(s, arg0, arg1, arg2, arg3, arg4, arg5)       do {                        \
            if ((_G_ppacoPciAutoCfgOpts->PACO_bInitFlag   == LW_TRUE) &&                                \
                (_G_ppacoPciAutoCfgOpts->PACO_pfuncLogMsg != LW_NULL)) {                                \
                (*(_G_ppacoPciAutoCfgOpts->PACO_pfuncLogMsg))(s, arg0, arg1, arg2, arg3, arg4, arg5);   \
            } else {                                                                                    \
                                                                                                        \
            }                                                                                           \
        } while (0)
#define __PCI_AUTO_CFG_DEBUG_MSG(s, arg0, arg1, arg2, arg3, arg4, arg5)     do {                        \
            if (_G_b_PciAutoCfgDebug == LW_TRUE) {                                                      \
                __PCI_AUTO_CFG_LOG_MSG(s, arg0, arg1, arg2, arg3, arg4, arg5);                          \
            }                                                                                           \
        } while (0)
#endif
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
static INT  __pciAutoCfgBusProbe(__PCI_AUTO_CFG_OPTS  *ppacoOpts,
                                 INT                   iPriBus,
                                 INT                   iSecBus,
                                 PCI_AUTO_CFG_LOC     *ppaclLoc,
                                 PCI_AUTO_CFG_LOC    **pppaclList,
                                 UINT32               *puiListSize);
static VOID  __pciAutoCfgDevConfig(__PCI_AUTO_CFG_OPTS  *ppacoOpts,
                                   INT                   iBus,
                                   PCI_AUTO_CFG_LOC    **pppaclList,
                                   UINT32               *puiSize);
/*********************************************************************************************************
** 函数名称: API_PciAutoCfgLibInit
** 功能描述: 自动配置库初始化
** 输　入  : pvArg      初始化库所需参数, 当前为保留参数
** 输　出  : 自动配置操作句柄
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PVOID  __pciAutoCfgLibInit (PVOID pvArg)
{
    lib_bzero(&_G_pacoPciAutoCfgOpts, sizeof(__PCI_AUTO_CFG_OPTS));
    
    _G_pacoPciAutoCfgOpts.PACO_iIndex      = -1;
    _G_ppacoPciAutoCfgOpts                 = &_G_pacoPciAutoCfgOpts;
    _G_ppacoPciAutoCfgOpts->PACO_bInitFlag = LW_TRUE;

    return  ((PVOID)_G_ppacoPciAutoCfgOpts);
}
/*********************************************************************************************************
** 函数名称: API_PciAutoCfgAddrAlign
** 功能描述: 对齐地址并检查边界
** 输　入  : uiBase             起始地址
**           uiLimit            结束地址
**           uiSizeReq          请求大小
**           puiAlignedBase     获得的对齐地址
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
INT  API_PciAutoCfgAddrAlign (UINT32  uiBase, UINT32  uiLimit, UINT32  uiSizeReq, UINT32 *puiAlignedBase)
{
    UINT32      uiSizeMask    = 0;
    UINT32      uiAlignAdjust = 0;

    uiSizeMask = uiSizeReq - 1;
    __PCI_AUTO_CFG_DEBUG_MSG("API_PciAutoCfgAddrAlign: sizemask[%08x]\n", uiSizeMask, 0, 0, 0, 0, 0);
    if ((uiBase & uiSizeMask) > 0) {
        uiAlignAdjust = uiSizeReq - (uiBase & uiSizeMask);
        __PCI_AUTO_CFG_DEBUG_MSG("API_PciAutoCfgAddrAlign: adjustment [%08x]\n",
                                 uiAlignAdjust, 0, 0, 0, 0, 0);
    } else {
        __PCI_AUTO_CFG_DEBUG_MSG("API_PciAutoCfgAddrAlign: already aligned\n", 0, 0, 0, 0, 0, 0);
        uiAlignAdjust = 0;
    }

    if (((uiBase + uiAlignAdjust) < uiBase ) ||
        ((uiBase + uiAlignAdjust) > uiLimit)) {
        __PCI_AUTO_CFG_DEBUG_MSG("API_PciAutoCfgAddrAlign: base + adjustment [%08x]" " limit [%08x]\n",
                                 (uiBase + uiAlignAdjust), uiLimit, 0, 0, 0, 0);
        return  (PX_ERROR);
    }

    if (puiAlignedBase) {
        *puiAlignedBase = uiBase + uiAlignAdjust;
    }

    if (((uiBase + uiAlignAdjust + uiSizeReq) < uiBase ) ||
        ((uiBase + uiAlignAdjust + uiSizeReq) > uiLimit)) {
        __PCI_AUTO_CFG_DEBUG_MSG("API_PciAutoCfgAddrAlign: base+adjustment+req [%08x]" " limit [%08x]\n",
                                 (uiBase + uiAlignAdjust + uiSizeReq), uiLimit, 0, 0, 0, 0);
        return  (PX_ERROR);
    }

    __PCI_AUTO_CFG_DEBUG_MSG("API_PciAutoCfgAddrAlign: new aligned base [%08x]\n",
                             (uiBase + uiAlignAdjust), 0, 0, 0, 0, 0);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pciAutoCfgFb2bFuncCheck
** 功能描述: 检测是否支持 FBTB (Fast Back TO Back) 功能
** 输　入  : iBus           总线号
**           iDevice        设备号
**           iFunction      功能号
**           pvArg          参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciAutoCfgFb2bFuncCheck (INT  iBus, INT  iDevice, INT  iFunction, PVOID  pvArg)
{
    INT         iRet     = PX_ERROR;
    UINT16      usStatus = 0x0000;

    iRet = API_PciConfigInWord(iBus, iDevice, iFunction, PCI_STATUS, &usStatus);
    if ((iRet != ERROR_NONE) ||
        (!(usStatus & PCI_STATUS_FAST_BACK))) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pciAutoCfgFb2bFuncSet
** 功能描述: FBTB (Fast Back TO Back) 功能设置
** 输　入  : iBus           总线号
**           iDevice        设备号
**           iFunction      功能号
**           pvArg          参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciAutoCfgFb2bFuncSet (INT  iBus, INT  iSlot, INT  iFunc, PVOID  pvArg)
{
    INT         iRet     = PX_ERROR;
    UINT16      usStatus = 0x0000;

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, PCI_STATUS, &usStatus);
    if ((iRet != ERROR_NONE) ||
        (!(usStatus & PCI_STATUS_FAST_BACK))) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigModifyDword(iBus, iSlot, iFunc, PCI_COMMAND,
                                    (PCI_COMMAND_MASK | PCI_COMMAND_FAST_BACK), PCI_COMMAND_FAST_BACK);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __pciAutoCfgFb2bFuncClear
** 功能描述: FBTB (Fast Back TO Back) 功能清除
** 输　入  : iBus           总线号
**           iDevice        设备号
**           iFunction      功能号
**           pvArg          参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciAutoCfgFb2bFuncClear (INT  iBus, INT  iDevice, INT  iFunction, PVOID  pvArg)
{
    INT         iRet     = PX_ERROR;
    UINT16      usStatus = 0x0000;

    iRet = API_PciConfigInWord(iBus, iDevice, iFunction, PCI_STATUS, &usStatus);
    if ((iRet != ERROR_NONE) ||
        (!(usStatus & PCI_STATUS_FAST_BACK))) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigModifyDword(iBus, iDevice, iFunction,
                                    PCI_COMMAND, (PCI_COMMAND_MASK | PCI_COMMAND_FAST_BACK), 0);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __pciAutoCfgFb2bDisable
** 功能描述: FBTB (Fast Back TO Back) 禁能
** 输　入  : ppacoOpts      选项句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciAutoCfgFb2bDisable (__PCI_AUTO_CFG_OPTS *ppacoOpts)
{
    INT     iRet = PX_ERROR;

    if ( ppacoOpts->PACO_bFb2bEnable != LW_TRUE) {
        return  (PX_ERROR);
    }
    ppacoOpts->PACO_bFb2bEnable = LW_FALSE;

    iRet = API_PciTraversalDev(0, LW_TRUE, (INT (*)())__pciAutoCfgFb2bFuncClear, LW_NULL);
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __pciAutoCfgFb2bEnable
** 功能描述: FBTB (Fast Back TO Back) 使能
** 输　入  : ppacoOpts      选项句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciAutoCfgFb2bEnable (__PCI_AUTO_CFG_OPTS *ppacoOpts)
{
    INT     iRet = PX_ERROR;

    if (ppacoOpts->PACO_bFb2bEnable != LW_TRUE) {
        return  (PX_ERROR);
    }

    iRet = __pciAutoCfgFb2bFuncCheck(0, 0, 0, LW_NULL);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciTraversalDev(0, LW_TRUE, (INT (*)())__pciAutoCfgFb2bFuncCheck, LW_NULL);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = __pciAutoCfgFb2bFuncSet(0, 0, 0, LW_NULL);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciTraversalDev(0, LW_TRUE, (INT (*)())__pciAutoCfgFb2bFuncSet, LW_NULL);
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __pciAutoCfgSystemSet
** 功能描述: 更新系统信息
** 输　入  : ppacsSystem        系统控制句柄
**           ppacoOpts          选项句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciAutoCfgSystemUpdate (PCI_AUTO_CFG_SYSTEM *ppacsSystem, __PCI_AUTO_CFG_OPTS *ppacoOpts)
{
    if (ppacsSystem == (PCI_AUTO_CFG_SYSTEM *)ppacoOpts) {
        return;
    }

    ppacsSystem->PACS_iIndex              = ppacoOpts->PACO_iIndex;
    ppacsSystem->PACS_uiBusMax            = ppacoOpts->PACO_uiBusMax;
    ppacsSystem->PACS_uiMem32Addr         = ppacoOpts->PACO_uiMem32Addr;
    ppacsSystem->PACS_uiMem32Size         = ppacoOpts->PACO_uiMem32Size;
    ppacsSystem->PACS_uiMemIo32Addr       = ppacoOpts->PACO_uiMemIo32Addr;
    ppacsSystem->PACS_uiMemIo32Size       = ppacoOpts->PACO_uiMemIo32Size;
    ppacsSystem->PACS_uiIo32Addr          = ppacoOpts->PACO_uiIo32Addr;
    ppacsSystem->PACS_uiIo32Size          = ppacoOpts->PACO_uiIo32Size;
    ppacsSystem->PACS_uiIo16Addr          = ppacoOpts->PACO_uiIo16Addr;
    ppacsSystem->PACS_uiIo16Size          = ppacoOpts->PACO_uiIo16Size;
    ppacsSystem->PACS_iCacheSize          = ppacoOpts->PACO_iCacheSize;
    ppacsSystem->PACS_uiLatencyMax        = ppacoOpts->PACO_uiLatencyMax;
    ppacsSystem->PACS_bAutoIntRouting     = ppacoOpts->PACO_bAutoIntRouting;
    ppacsSystem->PACS_pfuncInclude        = ppacoOpts->PACO_pfuncInclude;
    ppacsSystem->PACS_pfuncIntAssign      = ppacoOpts->PACO_pfuncIntAssign;
    ppacsSystem->PACS_pfuncBridgePreInit  = ppacoOpts->PACO_pfuncBridgePreInit;
    ppacsSystem->PACS_pfuncBridgePostInit = ppacoOpts->PACO_pfuncBridgePostInit;
    ppacsSystem->PACS_pfuncRollcall       = ppacoOpts->PACO_pfuncRollcall;
}
/*********************************************************************************************************
** 函数名称: __pciAutoCfgBusConfig
** 功能描述: 总线配置
** 输　入  : ppacoPciOpts       操作选项
**           iBus               总线号
**           ppaclList          列表
**           puiListSize        获得到的列表大小
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciAutoCfgBusConfig (__PCI_AUTO_CFG_OPTS  *ppacoOpts,
                                    PCI_AUTO_CFG_LOC     *ppaclLoc,
                                    PCI_AUTO_CFG_LOC    **pppaclList,
                                    UINT32               *puiSize)
{
    UINT8       ucBus;
    UINT8       ucLatencyMax;
    UINT32      uiDevVend;
    UINT32      uiTemp1;
    UINT32      uiTemp2;
    UINT32      uiTemp3;
    UINT32      uiAlignedBase;

    if (ppacoOpts->PACO_pfuncBridgePreInit != LW_NULL) {
        API_PciConfigInDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                             PCI_VENDOR_ID, &uiDevVend);

        (ppacoOpts->PACO_pfuncBridgePreInit)((PCI_AUTO_CFG_SYSTEM *)ppacoOpts, ppaclLoc, uiDevVend);
    }

    API_PciConfigModifyDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                             PCI_IO_BASE, 0xffff0000, 0xffff0000);

    if ((ppaclLoc->PACL_iAttribute & PCI_AUTO_CFG_ATTR_BUS_PREFETCH) &&
        (ppacoOpts->PACO_uiMem32Size > 0                       )) {
        __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgBusConfig: Configuring prefetch aperture\n",
                                 0, 0, 0, 0, 0, 0);

        API_PciAutoCfgAddrAlign(ppacoOpts->PACO_uiMem32Addr,
                                (ppacoOpts->PACO_uiMem32Addr + ppacoOpts->PACO_uiMem32Size),
                                0x100000,
                                &uiAlignedBase);
        __PCI_AUTO_CFG_DEBUG_MSG("PF Mem Base orig[0x%08x] new[0x%08x] adj[0x%08x]\n",
                                 ppacoOpts->PACO_uiMem32Addr,
                                 uiAlignedBase,
                                 (uiAlignedBase - (ppacoOpts->PACO_uiMem32Addr)), 0, 0, 0);

        ppacoOpts->PACO_uiMem32Used += (uiAlignedBase - (ppacoOpts->PACO_uiMem32Addr));
        ppacoOpts->PACO_uiMem32Size -= (uiAlignedBase - (ppacoOpts->PACO_uiMem32Addr));
        ppacoOpts->PACO_uiMem32Addr  = uiAlignedBase;
        API_PciConfigOutDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                              PCI_PREF_BASE_UPPER32, 0);
        API_PciConfigModifyDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                                 PCI_PREF_MEMORY_BASE, 0x0000fff0,(ppacoOpts->PACO_uiMem32Addr >>(20-4)));

        API_PciConfigInDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                             PCI_PREF_MEMORY_BASE, &uiTemp2);
        uiTemp1 = ((uiTemp2 & 0x0000fff0) << 16);
        __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgBusConfig: PF Mem Base [0x%08x]\n", uiTemp1, 0, 0, 0, 0, 0);
    }

    if (ppacoOpts->PACO_uiIo16Size > 0) {
        API_PciAutoCfgAddrAlign(ppacoOpts->PACO_uiIo16Addr,
                                (ppacoOpts->PACO_uiIo16Addr + ppacoOpts->PACO_uiIo16Size),
                                0x1000,
                                &uiAlignedBase);
        __PCI_AUTO_CFG_DEBUG_MSG("I/O 16 Base orig[0x%08x] new[0x%08x] adj[0x%08x]\n",
                                 (ppacoOpts->PACO_uiIo16Addr),
                                 uiAlignedBase,
                                 (uiAlignedBase - ppacoOpts->PACO_uiIo16Addr), 0, 0, 0);

        ppacoOpts->PACO_uiIo16Used += (uiAlignedBase - ppacoOpts->PACO_uiIo16Addr);
        ppacoOpts->PACO_uiIo16Size -= (uiAlignedBase - ppacoOpts->PACO_uiIo16Addr);
        ppacoOpts->PACO_uiIo16Addr  = uiAlignedBase;
        API_PciConfigModifyDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                                 PCI_IO_BASE, 0x000000f0, (ppacoOpts->PACO_uiIo16Addr >> 16));
        API_PciConfigModifyDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                                 PCI_IO_BASE_UPPER16, 0x0000ffff, (ppacoOpts->PACO_uiIo16Addr >> 16));

        API_PciConfigInDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                             PCI_IO_BASE, &uiTemp1);
        API_PciConfigInDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                             PCI_IO_BASE_UPPER16, &uiTemp2);
        uiTemp3 = (((uiTemp1 & (UINT32)0xf0) << (12 - 4)) & 0x0000ffff);
        uiTemp1 = uiTemp3 | ((uiTemp2 << 16) & 0xffff0000);
        __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgBusConfig: IO16 Base Address [0x%08x]\n",
                                 uiTemp1, 0, 0, 0, 0, 0);

    }

    if (ppacoOpts->PACO_uiMemIo32Size  > 0) {
        API_PciAutoCfgAddrAlign(ppacoOpts->PACO_uiMemIo32Addr,
                                (ppacoOpts->PACO_uiMemIo32Addr + ppacoOpts->PACO_uiMemIo32Size),
                                0x100000,
                                &uiAlignedBase);
        __PCI_AUTO_CFG_DEBUG_MSG("Memory Base orig[0x%08x] new[0x%08x] adj[0x%08x]\n",
                                 ppacoOpts->PACO_uiMemIo32Addr,
                                 uiAlignedBase,
                                 (uiAlignedBase - ppacoOpts->PACO_uiMemIo32Addr), 0, 0, 0);

        ppacoOpts->PACO_uiMemIo32Used += (uiAlignedBase - ppacoOpts->PACO_uiMemIo32Addr);
        ppacoOpts->PACO_uiMemIo32Size -= (uiAlignedBase - ppacoOpts->PACO_uiMemIo32Addr);
        ppacoOpts->PACO_uiMemIo32Addr  = uiAlignedBase;
        API_PciConfigModifyDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                                 PCI_MEMORY_BASE, 0x0000fff0, (ppacoOpts->PACO_uiMemIo32Addr >> (20-4)));

        API_PciConfigInDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                             PCI_MEMORY_BASE, &uiTemp2);
        uiTemp1 = ((uiTemp2 & 0x0000fff0) << 16);
        __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgBusConfig: Mem Base Address [0x%08x]\n",
                                 uiTemp1, 0, 0, 0, 0, 0);
    }

    API_PciConfigInByte(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                        PCI_SECONDARY_BUS, &ucBus);

    __pciAutoCfgDevConfig(ppacoOpts, ucBus, pppaclList, puiSize);

    if (ppacoOpts->PACO_uiIo16Size  > 0) {
        API_PciAutoCfgAddrAlign(ppacoOpts->PACO_uiIo16Addr,
                                (ppacoOpts->PACO_uiIo16Addr + ppacoOpts->PACO_uiIo16Size),
                                0x1000,
                                &uiAlignedBase);
        __PCI_AUTO_CFG_DEBUG_MSG("I/O 16 Lim orig[0x%08x] new[0x%08x] adj[0x%08x]\n",
                                 (ppacoOpts->PACO_uiIo16Addr),
                                 uiAlignedBase,
                                 (uiAlignedBase - ppacoOpts->PACO_uiIo16Addr), 0, 0, 0);

        ppacoOpts->PACO_uiIo16Used += (uiAlignedBase - ppacoOpts->PACO_uiIo16Addr);
        ppacoOpts->PACO_uiIo16Size -= (uiAlignedBase - ppacoOpts->PACO_uiIo16Addr);
        ppacoOpts->PACO_uiIo16Addr  = uiAlignedBase;
        API_PciConfigModifyDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                                 PCI_IO_BASE, 0x0000f000, (ppacoOpts->PACO_uiIo16Addr - 1));
        API_PciConfigModifyDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                                 PCI_IO_BASE_UPPER16, 0xffff0000, (ppacoOpts->PACO_uiIo16Addr - 1));

        API_PciConfigInDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                             PCI_IO_BASE, &uiTemp1);
        API_PciConfigInDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                             PCI_IO_BASE_UPPER16, &uiTemp2);
        uiTemp3  = ((uiTemp1 & (UINT32)0xf000) & 0x0000ffff);
        uiTemp1  = uiTemp3 | (uiTemp2 & 0xffff0000);
        uiTemp1 |= 0x00000FFF;
        __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgBusConfig: IO Limit [0x%08x]\n", uiTemp1, 0, 0, 0, 0, 0);
    }

    if (ppacoOpts->PACO_uiMemIo32Size > 0) {
        API_PciAutoCfgAddrAlign(ppacoOpts->PACO_uiMemIo32Addr,
                                (ppacoOpts->PACO_uiMemIo32Addr + ppacoOpts->PACO_uiMemIo32Size),
                                0x100000,
                                &uiAlignedBase);
        __PCI_AUTO_CFG_DEBUG_MSG("MemIo Lim orig[0x%08x] new[0x%08x] adj[0x%08x]\n",
                                 ppacoOpts->PACO_uiMemIo32Addr,
                                 uiAlignedBase,
                                 (uiAlignedBase - ppacoOpts->PACO_uiMemIo32Addr), 0, 0, 0);

        ppacoOpts->PACO_uiMemIo32Used += (uiAlignedBase - ppacoOpts->PACO_uiMemIo32Addr);
        ppacoOpts->PACO_uiMemIo32Size -= (uiAlignedBase - ppacoOpts->PACO_uiMemIo32Addr);
        ppacoOpts->PACO_uiMemIo32Addr  = uiAlignedBase;
        API_PciConfigModifyDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                                 PCI_MEMORY_BASE, 0xfff00000, (ppacoOpts->PACO_uiMemIo32Addr - 1));

        API_PciConfigInDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                             PCI_MEMORY_BASE, &uiTemp2);
        uiTemp1 = (uiTemp2 & 0xfff00000);
        uiTemp1 |= 0x000FFFFF;
        __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgBusConfig: MemIo Limit [0x%08x]\n", uiTemp1, 0, 0, 0, 0, 0);
    }

    if ((ppaclLoc->PACL_iAttribute & PCI_AUTO_CFG_ATTR_BUS_PREFETCH) &&
        (ppacoOpts->PACO_uiMem32Size > 0                       )) {
        API_PciAutoCfgAddrAlign(ppacoOpts->PACO_uiMem32Addr,
                                (ppacoOpts->PACO_uiMem32Addr + ppacoOpts->PACO_uiMem32Size),
                                0x100000,
                                &uiAlignedBase);
        __PCI_AUTO_CFG_DEBUG_MSG("PF Lim orig[0x%08x] new[0x%08x] adj[0x%08x]\n",
                                 ppacoOpts->PACO_uiMem32Addr,
                                 uiAlignedBase,
                                 (uiAlignedBase - (ppacoOpts->PACO_uiMem32Addr)), 0, 0, 0);

        ppacoOpts->PACO_uiMem32Used += (uiAlignedBase - (ppacoOpts->PACO_uiMem32Addr));
        ppacoOpts->PACO_uiMem32Size -= (uiAlignedBase - (ppacoOpts->PACO_uiMem32Addr));
        ppacoOpts->PACO_uiMem32Addr  = uiAlignedBase;
        API_PciConfigOutDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                              PCI_PREF_LIMIT_UPPER32, 0);
        API_PciConfigModifyDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                                 PCI_PREF_MEMORY_BASE, 0xfff00000,(ppacoOpts->PACO_uiMem32Addr - 1));

        API_PciConfigInDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                             PCI_PREF_MEMORY_BASE, &uiTemp2);
        uiTemp1  = (uiTemp2 & 0xfff00000);
        uiTemp1 |= 0x000FFFFF;
        __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgBusConfig: PF Mem Limit [0x%08x]\n",uiTemp1, 0, 0, 0, 0, 0);
    }

    if (ppacoOpts->PACO_pfuncBridgePostInit != LW_NULL) {
        API_PciConfigInDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                             PCI_VENDOR_ID, &uiDevVend);

        (ppacoOpts->PACO_pfuncBridgePostInit)((PCI_AUTO_CFG_SYSTEM *)ppacoOpts, ppaclLoc, uiDevVend);
    }

    if ((_G_ppacoPciAutoCfgOpts->PACO_bInitFlag       == LW_TRUE) &&
        (_G_ppacoPciAutoCfgOpts->PACO_pfuncLatencyMax != LW_NULL)) {
        ucLatencyMax = (*_G_ppacoPciAutoCfgOpts->PACO_pfuncLatencyMax)(
                           ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                           _G_ppacoPciAutoCfgOpts->PACO_pvLatencyMaxArg);
    } else {
        ucLatencyMax = ppacoOpts->PACO_uiLatencyMax;
    }

    API_PciConfigOutByte(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                         PCI_SEC_LATENCY_TIMER, ucLatencyMax);

    API_PciConfigOutDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                          PCI_COMMAND,
                          (UINT32)(PCI_COMMAND_MASK   |
                                   PCI_COMMAND_IO     |
                                   PCI_COMMAND_MEMORY |
                                   PCI_COMMAND_MASTER));
}
/*********************************************************************************************************
** 函数名称: __pciAutoCfgCardBusConfig
** 功能描述: 总线配置
** 输　入  : ppacoPciOpts       操作选项
**           iBus               总线号
**           ppaclList          列表
**           puiListSize        获得到的列表大小
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciAutoCfgCardBusConfig (__PCI_AUTO_CFG_OPTS  *ppacoOpts,
                                        PCI_AUTO_CFG_LOC     *ppaclLoc,
                                        PCI_AUTO_CFG_LOC    **pppaclList,
                                        UINT32               *puiSize)
{
    UINT8       ucLatencyMax         = 0;
    UINT32      uiDevVend            = 0;
    UINT32      uiAlignedBase        = 0;
    UINT32      uiCardBusIo16Size    = __PCI_AUTO_CFG_CARDBUS_IO16_SIZE;
    UINT32      uiCardBusMemIo32Size = __PCI_AUTO_CFG_CARDBUS_MEMIO32_SIZE;
    UINT32      uiCardBusMem32Size   = __PCI_AUTO_CFG_CARDBUS_MEM32_SIZE;
    UINT32      uiSizeAdj            = 0;

    API_PciConfigModifyWord(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                            PCI_CB_BRIDGE_CONTROL, PCI_CB_BRIDGE_CTL_CB_RESET,PCI_CB_BRIDGE_CTL_CB_RESET);

    if (ppacoOpts->PACO_pfuncBridgePreInit != LW_NULL) {
        API_PciConfigInDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                             PCI_VENDOR_ID, &uiDevVend);

        (ppacoOpts->PACO_pfuncBridgePreInit)((PCI_AUTO_CFG_SYSTEM *)ppacoOpts, ppaclLoc, uiDevVend);
    }

    API_PciConfigModifyDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                             PCI_CB_CAPABILITY_LIST, 0xffff0000, 0xffff0000);

    if ((ppaclLoc->PACL_iAttribute & PCI_AUTO_CFG_ATTR_BUS_PREFETCH) &&
        (ppacoOpts->PACO_uiMem32Size > 0                       )) {
        __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgCardBusConfig: Configuring prefetch aperture\n",
                                 0, 0, 0, 0, 0, 0);

        API_PciAutoCfgAddrAlign(ppacoOpts->PACO_uiMem32Addr,
                                (ppacoOpts->PACO_uiMem32Addr + ppacoOpts->PACO_uiMem32Size),
                                uiCardBusMem32Size,
                                &uiAlignedBase);
        __PCI_AUTO_CFG_DEBUG_MSG("PF Mem Base orig[0x%08x] new[0x%08x] adj[0x%08x]\n",
                                 ppacoOpts->PACO_uiMem32Addr,
                                 uiAlignedBase,
                                 (uiAlignedBase - (ppacoOpts->PACO_uiMem32Addr)), 0, 0, 0);

        uiSizeAdj                    = uiAlignedBase - ppacoOpts->PACO_uiMem32Addr + uiCardBusMem32Size;
        ppacoOpts->PACO_uiMem32Used += uiSizeAdj;
        ppacoOpts->PACO_uiMem32Size -= uiSizeAdj;
        ppacoOpts->PACO_uiMem32Addr  = uiSizeAdj;
        API_PciConfigOutDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                              PCI_CB_MEMORY_BASE_0, uiAlignedBase);

        __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgCardBusConfig: PF Mem Base [0x%08x]\n",
                                 ppacoOpts->PACO_uiMem32Addr, 0, 0, 0, 0, 0);

        API_PciConfigOutDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                              PCI_CB_MEMORY_LIMIT_0, ppacoOpts->PACO_uiMem32Addr - 1);

        API_PciConfigModifyDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                                 PCI_CB_BRIDGE_CONTROL,
                                 PCI_CB_BRIDGE_CTL_PREFETCH_MEM0, PCI_CB_BRIDGE_CTL_PREFETCH_MEM0);
    }

    if (ppacoOpts->PACO_uiIo16Size  > 0) {
        API_PciAutoCfgAddrAlign(ppacoOpts->PACO_uiIo16Addr,
                                (ppacoOpts->PACO_uiIo16Addr + ppacoOpts->PACO_uiIo16Size),
                                uiCardBusIo16Size,
                                &uiAlignedBase);
        __PCI_AUTO_CFG_DEBUG_MSG("I/O 16 Base orig[0x%08x] new[0x%08x] adj[0x%08x]\n",
                                 ppacoOpts->PACO_uiIo16Addr,
                                 uiAlignedBase,
                                 (uiAlignedBase - ppacoOpts->PACO_uiIo16Addr), 0, 0, 0);

        uiSizeAdj                   = uiAlignedBase - ppacoOpts->PACO_uiIo16Addr + uiCardBusIo16Size;
        ppacoOpts->PACO_uiIo16Used += uiSizeAdj;
        ppacoOpts->PACO_uiIo16Size -= uiSizeAdj;
        ppacoOpts->PACO_uiIo16Addr += uiSizeAdj;

        API_PciConfigOutDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                              PCI_CB_IO_BASE_0, uiAlignedBase);
        __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgCardBusConfig: IO16 Base Address [0x%08x]\n",
                                 ppacoOpts->PACO_uiIo16Addr, 0, 0, 0, 0, 0);

        API_PciConfigOutDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                              PCI_CB_IO_LIMIT_0, ppacoOpts->PACO_uiIo16Addr - 1);
    }

    if (ppacoOpts->PACO_uiMemIo32Size > 0) {
        API_PciAutoCfgAddrAlign(ppacoOpts->PACO_uiMemIo32Addr,
                                (ppacoOpts->PACO_uiMemIo32Addr + ppacoOpts->PACO_uiMemIo32Size),
                                uiCardBusMemIo32Size,
                                &uiAlignedBase);
        __PCI_AUTO_CFG_DEBUG_MSG("Memory Base orig[0x%08x] new[0x%08x] adj[0x%08x]\n",
                                 ppacoOpts->PACO_uiMemIo32Addr,
                                 uiAlignedBase,
                                 (uiAlignedBase - ppacoOpts->PACO_uiMemIo32Addr), 0, 0, 0);

        uiSizeAdj = uiAlignedBase - ppacoOpts->PACO_uiMemIo32Addr + uiCardBusMemIo32Size;
        ppacoOpts->PACO_uiMemIo32Used += uiSizeAdj;
        ppacoOpts->PACO_uiMemIo32Size -= uiSizeAdj;
        ppacoOpts->PACO_uiMemIo32Addr += uiSizeAdj;
        API_PciConfigOutDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                              PCI_CB_MEMORY_BASE_1, uiAlignedBase);

        __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgCardBusConfig: Mem Base Address [0x%08x]\n",
                                 ppacoOpts->PACO_uiMemIo32Addr, 0, 0, 0, 0, 0);

        API_PciConfigOutDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                              PCI_CB_MEMORY_LIMIT_1, ppacoOpts->PACO_uiMemIo32Addr - 1);

        API_PciConfigModifyDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                                 PCI_CB_BRIDGE_CONTROL,
                                 PCI_CB_BRIDGE_CTL_PREFETCH_MEM1, 0);
    }

    if ((_G_ppacoPciAutoCfgOpts->PACO_bInitFlag       == LW_TRUE) &&
        (_G_ppacoPciAutoCfgOpts->PACO_pfuncLatencyMax != LW_NULL)) {
        ucLatencyMax = (*_G_ppacoPciAutoCfgOpts->PACO_pfuncLatencyMax)(
                           ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                           _G_ppacoPciAutoCfgOpts->PACO_pvLatencyMaxArg);
    } else {
        ucLatencyMax = ppacoOpts->PACO_uiLatencyMax;
    }

    API_PciConfigOutByte(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                         PCI_SEC_LATENCY_TIMER, ucLatencyMax);

    API_PciConfigOutDword(ppaclLoc->PACL_iBus, ppaclLoc->PACL_iDevice, ppaclLoc->PACL_iFunction,
                          PCI_COMMAND,
                          (UINT32)(PCI_COMMAND_MASK   |
                                   PCI_COMMAND_IO     |
                                   PCI_COMMAND_MEMORY |
                                   PCI_COMMAND_MASTER));
}
/*********************************************************************************************************
** 函数名称: __pciAutoCfgIoAlloc
** 功能描述: 选择合适的 MEM 空间 (不能有 64 位地址)
** 输　入  : ppacoPciOpts       操作选项
**           ppaclFunc          功能句柄
**           puiAlloc           获取到的内存
**           uiSize             需要的内存大小
**           uiAddrInfo         地址类型信息
** 输　出  : 是否为 64 位
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT  __pciAutoCfgMemAlloc (__PCI_AUTO_CFG_OPTS *ppacoOpts,
                                   PCI_AUTO_CFG_LOC    *ppaclFunc,
                                   UINT32              *puiAlloc,
                                   UINT32               uiSize,
                                   UINT32               uiAddrInfo)
{
    INT         iRet            = PX_ERROR;
    UINT32     *puiBase         = LW_NULL;
    UINT32     *puiAvail        = LW_NULL;
    UINT32     *puiUpdate       = LW_NULL;
    UINT32      uiAlignedBase   = 0;
    UINT32      uiSizeAdj       = 0;
    UINT        uiRegister64Bit = 0;

    switch (uiAddrInfo & (UINT32)PCI_BASE_ADDRESS_MEM_TYPE_MASK) {

    case PCI_BASE_ADDRESS_MEM_TYPE_1M:
        break;

    case PCI_BASE_ADDRESS_MEM_TYPE_64:
        uiRegister64Bit = 1;
        break;

    case PCI_BASE_ADDRESS_MEM_TYPE_32:
        break;

    case (PCI_BASE_ADDRESS_MEM_TYPE_1M | PCI_BASE_ADDRESS_MEM_TYPE_64):
    default:
        *puiAlloc = __PCI_AUTO_CFG_NO_ALLOCATION;
        return 0;
    }

    if ((uiAddrInfo & PCI_BASE_ADDRESS_MEM_PREFETCH               ) &&
        ((ppaclFunc->PACL_iAttribute) & PCI_AUTO_CFG_ATTR_BUS_PREFETCH)) {
        __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgMemAlloc: PF Mem requested\n", 0, 0, 0, 0, 0, 0);

        puiBase   = &(ppacoOpts->PACO_uiMem32Addr);
        puiAvail  = &(ppacoOpts->PACO_uiMem32Size);
        puiUpdate = &(ppacoOpts->PACO_uiMem32Used);
        if (*puiAvail > 0) {
            iRet = API_PciAutoCfgAddrAlign(*puiBase, (*puiBase + *puiAvail), uiSize, &uiAlignedBase);
        }

        if (iRet != ERROR_NONE) {
            __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgMemAlloc: No PF Mem available Trying MemIO\n",
                                     0, 0, 0, 0, 0, 0);

            puiBase   = &(ppacoOpts->PACO_uiMemIo32Addr);
            puiAvail  = &(ppacoOpts->PACO_uiMemIo32Size);
            puiUpdate = &(ppacoOpts->PACO_uiMemIo32Used);
            if (*puiAvail > 0) {
                iRet = API_PciAutoCfgAddrAlign(*puiBase, (*puiBase + *puiAvail), uiSize, &uiAlignedBase);
            }

            if (iRet != ERROR_NONE) {
                __PCI_AUTO_CFG_LOG_MSG("Warning: PCI PF Mem alloc failed\n", 0, 0, 0, 0, 0, 0);
                *puiAlloc = __PCI_AUTO_CFG_NO_ALLOCATION;
                return  (0);
            }
        }
    } else {
        puiBase   = &(ppacoOpts->PACO_uiMemIo32Addr);
        puiAvail  = &(ppacoOpts->PACO_uiMemIo32Size);
        puiUpdate = &(ppacoOpts->PACO_uiMemIo32Used);
        if (*puiAvail > 0) {
            iRet = API_PciAutoCfgAddrAlign(*puiBase, (*puiBase + *puiAvail), uiSize, &uiAlignedBase);
        }

        if (iRet != ERROR_NONE) {
            __PCI_AUTO_CFG_LOG_MSG("Warning: PCI Memory allocation failed\n", 0, 0, 0, 0, 0, 0);
            *puiAlloc = __PCI_AUTO_CFG_NO_ALLOCATION;
            return  (0);
        }
    }

    __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgMemAlloc: \n", 0, 0, 0, 0, 0, 0);
    __PCI_AUTO_CFG_DEBUG_MSG("  Pre: puiBase[0x%08x], puiAvail[0x%08x]\n",
                             (INT)(*puiBase), (INT)(*puiAvail), 0, 0, 0, 0);

    *puiAlloc   = uiAlignedBase;
    uiSizeAdj   = (uiAlignedBase - *puiBase) + uiSize;
    *puiBase   += uiSizeAdj;
    *puiAvail  -= uiSizeAdj;
    *puiUpdate += uiSizeAdj;

    __PCI_AUTO_CFG_DEBUG_MSG("  Post: puiBase[0x%08x], puiAvail[0x%08x]\n",
                             (INT)(*puiBase), (INT)(*puiAvail), 0, 0, 0, 0);

    return  (uiRegister64Bit);
}
/*********************************************************************************************************
** 函数名称: __pciAutoCfgIoAlloc
** 功能描述: 选择合适的 I/O 空间 (不能有 64 位地址)
** 输　入  : ppacoPciOpts       操作选项
**           ppaclFunc          功能句柄
**           puiAlloc           获取到的内存
**           uiSize             需要的内存大小
** 输　出  : 是否为 64 位
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT  __pciAutoCfgIoAlloc (__PCI_AUTO_CFG_OPTS *ppacoOpts,
                                  PCI_AUTO_CFG_LOC    *ppaclFunc,
                                  UINT32              *puiAlloc,
                                  UINT32               uiSize)
{
    INT         iRet          = PX_ERROR;
    UINT32     *puiBase       = LW_NULL;
    UINT32     *puiAvail      = LW_NULL;
    UINT32      uiAlignedBase = 0;
    UINT32      uiSizeAdj     = 0;

    if (puiAlloc == LW_NULL) {
        return  (0);
    }

    if ((ppaclFunc->PACL_iBus == 0                                   ) &&
        ((ppaclFunc->PACL_iAttribute & PCI_AUTO_CFG_ATTR_BUS_4GB_IO) != 0)) {
        __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgIoAlloc: 32-bit I/O\n", 0, 0, 0, 0, 0, 0);
        puiBase  = &(ppacoOpts->PACO_uiMemIo32Addr);
        puiAvail = &(ppacoOpts->PACO_uiMemIo32Size);
    } else {
        __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgIoAlloc: 16-bit I/O\n", 0, 0, 0, 0, 0, 0);
        puiBase  = &(ppacoOpts->PACO_uiIo16Addr);
        puiAvail = &(ppacoOpts->PACO_uiIo16Size);
    }

    if (*puiAvail > 0) {
        iRet = API_PciAutoCfgAddrAlign(*puiBase, (*puiBase + *puiAvail), uiSize, &uiAlignedBase);
    }

    if (iRet != ERROR_NONE) {
        __PCI_AUTO_CFG_LOG_MSG("Warning: PCI I/O allocation failed\n", 0, 0, 0, 0, 0, 0);
        *puiAlloc = __PCI_AUTO_CFG_NO_ALLOCATION;
        return  (0);
    }

    __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgIoAlloc: Pre/Post alloc: \n", 0, 0, 0, 0, 0, 0);
    __PCI_AUTO_CFG_DEBUG_MSG("  Pre: puiBase[0x%08x], puiAvail[0x%08x]\n",
                             (INT)(*puiBase), (INT)(*puiAvail), 0, 0, 0, 0);

    *puiAlloc  = uiAlignedBase;
    uiSizeAdj  = (uiAlignedBase - *puiBase) + uiSize;
    *puiBase  += uiSizeAdj;
    *puiAvail -= uiSizeAdj;

    if ((ppaclFunc->PACL_iBus == 0                                   ) &&
        ((ppaclFunc->PACL_iAttribute & PCI_AUTO_CFG_ATTR_BUS_4GB_IO) != 0)) {
        ppacoOpts->PACO_uiIo32Used += uiSizeAdj;
    } else {
        ppacoOpts->PACO_uiIo16Used += uiSizeAdj;
    }

    __PCI_AUTO_CFG_DEBUG_MSG("  Post: puiBase[0x%08x], puiAvail[0x%08x]\n",
                             (INT)(*puiBase), (INT)(*puiAvail), 0, 0, 0, 0);

    return  (0);
}
/*********************************************************************************************************
** 函数名称: __pciAutoCfgIntAssign
** 功能描述: 配置中断
** 输　入  : ppacoPciOpts       操作选项
**           ppaclFunc          功能句柄
** 输　出  : 是否为 64 位
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT8  __pciAutoCfgIntAssign (__PCI_AUTO_CFG_OPTS *ppacoOpts, PCI_AUTO_CFG_LOC *ppaclFunc)
{
    REGISTER INT                        i           = 0;
    UINT8                               ucRet       = 0xFF;
    UINT8                               ucIntPin    = 0;

    PCI_AUTO_CFG_INT_ASSIGN_FUNC_PTR    pfuncHandle = ppacoOpts->PACO_pfuncIntAssign;

    API_PciConfigInByte(ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                        PCI_INTERRUPT_PIN, &ucIntPin);
    if ((!(ppacoOpts->PACO_bAutoIntRouting)) && (ucIntPin != 0)) {
        if (pfuncHandle != LW_NULL ) {
            ucRet = pfuncHandle((PCI_AUTO_CFG_SYSTEM *)ppacoOpts, ppaclFunc, ucIntPin);

            return  (ucRet);
        }

    }

    switch (ppaclFunc->PACL_iBus) {

    case 0:
        if ((pfuncHandle != LW_NULL) &&
            (ucIntPin    != 0      )) {
            ucRet = pfuncHandle((PCI_AUTO_CFG_SYSTEM *)ppacoOpts, ppaclFunc, ucIntPin);
        }

        if (((ppaclFunc->PACL_iAttribute) & PCI_AUTO_CFG_ATTR_BUS_PCI) > 0) {
            for (i = 0; i < 4; i++) {
                if (pfuncHandle != LW_NULL) {
                    _G_ucPciAutoCfgIntRoutingTable[i]  = pfuncHandle((PCI_AUTO_CFG_SYSTEM *)ppacoOpts,
                                                                     ppaclFunc, (i + 1));
                }
            }

        }
        break;

    default:
        ucRet = _G_ucPciAutoCfgIntRoutingTable[(((ppaclFunc->PACL_iDevice) +
                                               (ucIntPin - 1)              +
                                               (ppaclFunc->PACL_iOffset)) % 4)];
        break;
    }

    __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgIntAssign: int for [%d,%d,%d] pin %d is [%d]\n",
                             ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                             ucIntPin, ucRet, 0);

    return  (ucRet);
}
/*********************************************************************************************************
** 函数名称: API_PciAutoCfgBarConfig
** 功能描述: BAR 配置
** 输　入  : ppacsSystem        操作选项
**           ppaclFunc          功能列表
**           uiBarOffset        位置偏移
**           uiSize             大小及对齐信息
**           uiAddrInfo         地址信息
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
INT  API_PciAutoCfgBarConfig (PCI_AUTO_CFG_SYSTEM  *ppacsSystem,
                              PCI_AUTO_CFG_LOC     *ppaclFunc,
                              UINT32                uiBarOffset,
                              UINT32                uiSize,
                              UINT32                uiAddrInfo)
{
    __PCI_AUTO_CFG_OPTS  *ppacoOpts       = LW_NULL;
    UINT32                uiAddr          = 0;
    UINT32                uiSpaceEnable   = 0;
    UINT32                uiBaseAddrMask  = 0;
    UINT                  uiRegister64Bit = 0;

    if ((_G_ppacoPciAutoCfgOpts                 == LW_NULL ) ||
        (_G_ppacoPciAutoCfgOpts->PACO_bInitFlag == LW_FALSE)) {
        ppacoOpts = __pciAutoCfgLibInit(LW_NULL);
        API_PciAutoCfgCtl(ppacoOpts, PCI_AUTO_CFG_CTL_STRUCT_COPY, (PVOID)ppacsSystem);
    } else {
        ppacoOpts = _G_ppacoPciAutoCfgOpts;
    }

    if ((uiAddrInfo & PCI_BASE_ADDRESS_SPACE) == PCI_BASE_ADDRESS_SPACE_IO) {
        uiSpaceEnable   = PCI_COMMAND_IO;
        uiBaseAddrMask  = 0xFFFFFFFC;
        uiRegister64Bit = __pciAutoCfgIoAlloc(ppacoOpts, ppaclFunc, &uiAddr, uiSize);
    } else {
        uiSpaceEnable   = PCI_COMMAND_MEMORY;
        uiBaseAddrMask  = 0xFFFFFFF0;
        uiRegister64Bit = __pciAutoCfgMemAlloc(ppacoOpts, ppaclFunc, &uiAddr, uiSize, uiAddrInfo);
    }

    if (uiAddr != __PCI_AUTO_CFG_NO_ALLOCATION) {
        __PCI_AUTO_CFG_DEBUG_MSG("API_PciAutoCfgBarConfig:[0x%08x] written to BAR[0x%08x]\n",
                                 uiAddr, uiBarOffset, 0, 0, 0, 0);

        API_PciConfigModifyDword(ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                                 uiBarOffset, uiBaseAddrMask, uiAddr);
        if (uiRegister64Bit) {
            API_PciConfigOutDword(ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice,ppaclFunc->PACL_iFunction,
                                  uiBarOffset + 4, 0);
        }

        API_PciConfigModifyDword(ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                                PCI_COMMAND, (PCI_COMMAND_MASK | uiSpaceEnable), uiSpaceEnable);
    }

    __pciAutoCfgSystemUpdate(ppacsSystem, ppacoOpts);

    return  (uiRegister64Bit);
}
/*********************************************************************************************************
** 函数名称: API_PciAutoCfgFuncDisable
** 功能描述: 禁能指定功能
** 输　入  : ppaclFunc      功能控制句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
VOID  API_PciAutoCfgFuncDisable (PCI_AUTO_CFG_LOC *ppaclFunc)
{
    UINT8       ucTemp;
    UINT16      usTemp;

    if ((ppaclFunc->PACL_iAttribute) & PCI_AUTO_CFG_ATTR_DEV_EXCLUDE) {
        return;
    }

    __PCI_AUTO_CFG_DEBUG_MSG("pciAutoCfgFuncDisable: disable device [%d,%d,%d,0x%02x]\n",
                             ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                             ppaclFunc->PACL_iAttribute,
                             0, 0);

    usTemp = (PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
    API_PciConfigModifyDword(ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                             PCI_COMMAND, (PCI_COMMAND_MASK | usTemp), 0x0);

    API_PciConfigInByte(ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                        PCI_HEADER_TYPE, &ucTemp);
    ucTemp &= PCI_HEADER_TYPE_MASK;

    switch (ucTemp) {

    case PCI_HEADER_TYPE_NORMAL:
        API_PciConfigModifyDword(ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                                 PCI_ROM_ADDRESS, 0x1, 0);
        break;

    case PCI_HEADER_PCI_PCI:
        API_PciConfigModifyDword(ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                                 PCI_ROM_ADDRESS1, 0x1, 0);
        break;

    default:
        break;
    }

    return;
}
/*********************************************************************************************************
** 函数名称: API_PciAutoCfgFuncEnable
** 功能描述: 使能指定功能
** 输　入  : ppacsSystem        系统控制句柄
**           ppaclFunc          功能控制句柄
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
VOID  API_PciAutoCfgFuncEnable (PCI_AUTO_CFG_SYSTEM *ppacsSystem, PCI_AUTO_CFG_LOC *ppaclFunc)
{
    __PCI_AUTO_CFG_OPTS    *ppacoOpts    = LW_NULL;
    UINT16                  usClass      = 0;
    UINT8                   ucIntLine    = 0xff;
    UINT8                   ucLatencyMax = 0;

    if ((_G_ppacoPciAutoCfgOpts                 == LW_NULL ) ||
        (_G_ppacoPciAutoCfgOpts->PACO_bInitFlag == LW_FALSE)) {
        ppacoOpts = __pciAutoCfgLibInit(LW_NULL);
        API_PciAutoCfgCtl(ppacoOpts, PCI_AUTO_CFG_CTL_STRUCT_COPY, (PVOID)ppacsSystem);
    } else {
        ppacoOpts = _G_ppacoPciAutoCfgOpts;
    }

    if ((ppaclFunc->PACL_iAttribute) & PCI_AUTO_CFG_ATTR_DEV_EXCLUDE) {
        return;
    }

    __PCI_AUTO_CFG_DEBUG_MSG("API_PciAutoCfgFuncEnable: enable device [%d,%d,%d,0x%02x]\n",
                             ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                             ppaclFunc->PACL_iAttribute, 0, 0);

    API_PciConfigOutByte(ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                         PCI_CACHE_LINE_SIZE, (UINT8)ppacoOpts->PACO_iCacheSize);

    if ((ppacoOpts->PACO_bInitFlag       == LW_TRUE) &&
        (ppacoOpts->PACO_pfuncLatencyMax != LW_NULL)) {
        ucLatencyMax = (*ppacoOpts->PACO_pfuncLatencyMax)(ppaclFunc->PACL_iBus,
                                                          ppaclFunc->PACL_iDevice,
                                                          ppaclFunc->PACL_iFunction,
                                                          ppacoOpts->PACO_pvLatencyMaxArg);

    } else {
        ucLatencyMax = ppacoOpts->PACO_uiLatencyMax;
    }

    API_PciConfigOutByte(ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                         PCI_LATENCY_TIMER, ucLatencyMax);
    API_PciConfigInWord(ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                        PCI_CLASS_DEVICE, &usClass);
    API_PciConfigModifyDword(ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                             PCI_COMMAND, (PCI_COMMAND_MASK | PCI_COMMAND_MASTER), PCI_COMMAND_MASTER);

    ucIntLine = __pciAutoCfgIntAssign(ppacoOpts, ppaclFunc);
    API_PciConfigOutByte(ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                         PCI_INTERRUPT_LINE, ucIntLine);
    API_PciConfigOutWord(ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                         PCI_STATUS, (UINT16)0xFFFF);

    __pciAutoCfgSystemUpdate(ppacsSystem, ppacoOpts);

    return;
}
/*********************************************************************************************************
** 函数名称: __pciAutoCfgDevConfig
** 功能描述: 给某个功能分配内存空间
** 输　入  : ppacoPciOpts       操作选项
**           iBus               总线号
**           ppaclList          列表
**           puiListSize        获得到的列表大小
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciAutoCfgFuncConfig (__PCI_AUTO_CFG_OPTS  *ppacoOpts,
                                     PCI_AUTO_CFG_LOC     *ppaclFunc)
{
    INT         iRet = PX_ERROR;
    UINT32      uiBarMax;
    INT         iBarIndex = 0;
    UINT32      uiBarOffset;
    UINT32      uiReadVar;
    UINT32      uiAddrInfo;
    UINT32      uiSizeMask;
    UINT8       ucHeaderType;
    UINT32      uiDevVend;

    if ((ppacoOpts->PACO_pfuncInclude) != LW_NULL) {
        iRet = API_PciConfigInDword(ppaclFunc->PACL_iBus,
                                    ppaclFunc->PACL_iDevice,
                                    ppaclFunc->PACL_iFunction,
                                    PCI_VENDOR_ID, &uiDevVend);
        if (iRet != ERROR_NONE) {
            return;
        }
        iRet = (ppacoOpts->PACO_pfuncInclude)((PCI_AUTO_CFG_SYSTEM *)ppacoOpts, ppaclFunc, uiDevVend);
        if (iRet != ERROR_NONE) {
            if ((ppaclFunc->PACL_iAttribute & PCI_AUTO_CFG_ATTR_BUS_PCI) == 0) {
                ppaclFunc->PACL_iAttribute |= PCI_AUTO_CFG_ATTR_DEV_EXCLUDE;
                __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgFuncConfig: exc [%d,%d,%d,0x%02x]\n",
                                         ppaclFunc->PACL_iBus,
                                         ppaclFunc->PACL_iDevice,
                                         ppaclFunc->PACL_iFunction,
                                         ppaclFunc->PACL_iAttribute,
                                         0, 0);
                return;
            }
        }
    }

    API_PciAutoCfgFuncDisable(ppaclFunc);

    iRet = API_PciHeaderTypeGet(ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                                &ucHeaderType);
    if (iRet != ERROR_NONE) {
        return;
    }

    switch (ucHeaderType) {

    case PCI_HEADER_TYPE_NORMAL:
        uiBarMax = 6;
        break;

    case PCI_HEADER_TYPE_BRIDGE:
        uiBarMax = 2;
        break;

    case PCI_HEADER_TYPE_CARDBUS:
        uiBarMax = 1;
        break;

    default:
        uiBarMax = 0;
        break;
    }

    for (iBarIndex = 0; iBarIndex < uiBarMax; iBarIndex++) {
        uiBarOffset = PCI_BASE_ADDRESS_0 + (iBarIndex * 4);
        API_PciConfigOutDword(ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                              uiBarOffset, 0xFFFFFFFF);
        API_PciConfigInDword(ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                             uiBarOffset, &uiReadVar);
        if (uiReadVar == 0) {
            continue;
        }

        uiAddrInfo = uiReadVar &
                     ((PCI_BASE_ADDRESS_SPACE)         |
                      (PCI_BASE_ADDRESS_MEM_TYPE_MASK) |
                      (PCI_BASE_ADDRESS_MEM_PREFETCH));
        if ((uiAddrInfo & PCI_BASE_ADDRESS_SPACE) == PCI_BASE_ADDRESS_SPACE_IO) {
            __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgFuncConfig: IO Space found at BAR[%d]\n",
                                     iBarIndex, 0, 0, 0, 0, 0);
            uiSizeMask = (1 << 2);
        } else {
            __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgFuncConfig: MemSpace found at BAR[%d]\n",
                                     iBarIndex, 0, 0, 0, 0, 0);
            uiSizeMask = (1 << 4);
        }

        for ( ; uiSizeMask; uiSizeMask <<= 1) {
            if (uiReadVar & uiSizeMask) {
                iBarIndex += API_PciAutoCfgBarConfig((PCI_AUTO_CFG_SYSTEM *)ppacoOpts,
                                                     ppaclFunc, uiBarOffset, uiSizeMask, uiAddrInfo);
                break;
            }
        }
    }
}
/*********************************************************************************************************
** 函数名称: __pciAutoCfgFuncConfigAll
** 功能描述: 配置本列表上的设备
** 输　入  : ppacoPciOpts       操作选项
**           ppaclList          列表
**           iListSize          列表大小
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciAutoCfgFuncConfigAll (__PCI_AUTO_CFG_OPTS  *ppacoOpts,
                                        PCI_AUTO_CFG_LOC     *ppaclList,
                                        UINT32               uiListSize)
{
    PCI_AUTO_CFG_LOC       *ppaclFunc;
    UINT32                  uiLoop;
    UINT32                  uiEnd;

    ppaclFunc = ppaclList;
    uiEnd     = uiListSize;
    __pciAutoCfgDevConfig(ppacoOpts, ppaclList->PACL_iBus, &ppaclFunc, &uiEnd);

    ppaclFunc = ppaclList;
    for (uiLoop = 0; uiLoop < uiListSize; uiLoop++) {
        API_PciAutoCfgFuncEnable((PCI_AUTO_CFG_SYSTEM *)ppacoOpts, ppaclFunc);
        ppaclFunc++;
    }
}
/*********************************************************************************************************
** 函数名称: __pciAutoCfgDevConfig
** 功能描述: 位设备所有功能分配内存空间
** 输　入  : ppacoPciOpts       操作选项
**           iBus               总线号
**           ppaclList          列表
**           puiListSize        获得到的列表大小
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pciAutoCfgDevConfig (__PCI_AUTO_CFG_OPTS  *ppacoOpts,
                                    INT                   iBus,
                                    PCI_AUTO_CFG_LOC    **pppaclList,
                                    UINT32               *puiSize)
{
    PCI_AUTO_CFG_LOC       *ppaclFunc;
    INT                     iBusNext;
    UINT16                  usClass;

    while (*puiSize > 0) {
        ppaclFunc = *pppaclList;
        iBusNext = ppaclFunc->PACL_iBus;

        if (iBusNext < iBus) {
            return;
        }

        __pciAutoCfgFuncConfig(ppacoOpts, ppaclFunc);
        (*puiSize)--;
        (*pppaclList)++;

        API_PciConfigInWord(ppaclFunc->PACL_iBus, ppaclFunc->PACL_iDevice, ppaclFunc->PACL_iFunction,
                            PCI_CLASS_DEVICE, &usClass);

        switch (usClass) {

        case PCI_CLASS_BRIDGE_CARDBUS:
            __pciAutoCfgCardBusConfig(ppacoOpts, ppaclFunc, pppaclList, puiSize);
            break;

        case PCI_CLASS_BRIDGE_PCI:
            __pciAutoCfgBusConfig(ppacoOpts, ppaclFunc, pppaclList, puiSize);
            break;

        default:
            break;
        }
    }
}
/*********************************************************************************************************
** 函数名称: __pciAutoCfgDevProbe
** 功能描述: 探测单一总线上的所有设备
** 输　入  : ppacoOpts          操作选项
**           iBus               当前总线号
**           iOffset            中断向量路由
**           iInheritAttrib     继承属性
**           pppaclList         下一控制块入口
**           piListSize         设备数量
** 输　出  : 设备总数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT32  __pciAutoCfgDevProbe (__PCI_AUTO_CFG_OPTS  *ppacoOpts,
                                     INT                   iBus,
                                     INT                   iOffset,
                                     INT                   iInheritAttrib,
                                     PCI_AUTO_CFG_LOC    **pppaclList,
                                     UINT32               *puiListSize)
{
    PCI_AUTO_CFG_LOC        paclLoc;
    UINT16                  usClass;
    UINT32                  uiDevVend;
    INT                     iDevice;
    INT                     iFunction;
    INT                     iSubBus;
    UINT8                   ucTemp;
    UINT32                  uiTemp;

    lib_bzero((PVOID)&paclLoc, sizeof(PCI_AUTO_CFG_LOC));
    paclLoc.PACL_iBus = iBus;
    iSubBus           = iBus;

    for (iDevice = 0; iDevice < PCI_MAX_SLOTS; iDevice++) {
        paclLoc.PACL_iDevice = iDevice;

        for (iFunction = 0; iFunction < PCI_MAX_FUNCTIONS; iFunction++) {
            paclLoc.PACL_iFunction = iFunction;

            API_PciConfigInDword(paclLoc.PACL_iBus, paclLoc.PACL_iDevice, paclLoc.PACL_iFunction,
                                 PCI_VENDOR_ID, &uiDevVend);
            if (((uiDevVend & 0x0000ffff) == __PCI_AUTO_CFG_ABSENT_F) ||
                ((uiDevVend & 0x0000ffff) == __PCI_AUTO_CFG_ABSENT_0)) {
                if (iFunction == 0) {
                    break;
                } else {
                    continue;
                }
            }

            paclLoc.PACL_iOffset    = iOffset;
            paclLoc.PACL_iAttribute = 0;
            API_PciConfigInWord(paclLoc.PACL_iBus, paclLoc.PACL_iDevice, paclLoc.PACL_iFunction,
                                PCI_CLASS_DEVICE, &usClass);
            switch (usClass) {

            case PCI_CLASS_BRIDGE_HOST:
                paclLoc.PACL_iAttribute |= (PCI_AUTO_CFG_ATTR_DEV_EXCLUDE | PCI_AUTO_CFG_ATTR_BUS_HOST);
                break;

            case PCI_CLASS_BRIDGE_ISA:
                paclLoc.PACL_iAttribute |= PCI_AUTO_CFG_ATTR_BUS_ISA;
                break;

            case PCI_CLASS_DISPLAY_VGA:
                paclLoc.PACL_iAttribute |= PCI_AUTO_CFG_ATTR_DEV_DISPLAY;
                iInheritAttrib          &=(PCI_AUTO_CFG_ATTR_BUS_4GB_IO | PCI_AUTO_CFG_ATTR_BUS_PREFETCH);
                paclLoc.PACL_iAttribute |= iInheritAttrib;
                __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgDevProbe: inheriting attribute 0x%x to"
                                         " local attribute 0x%x (bus %d)\n",
                                         iInheritAttrib,
                                         paclLoc.PACL_iAttribute,
                                         paclLoc.PACL_iBus,
                                         0, 0, 0);
                break;

            case PCI_CLASS_BRIDGE_CARDBUS:
            case PCI_CLASS_BRIDGE_PCI:
                paclLoc.PACL_iAttribute |= PCI_AUTO_CFG_ATTR_BUS_PCI;
                if (iInheritAttrib & PCI_AUTO_CFG_ATTR_BUS_4GB_IO) {
                    API_PciConfigInByte(paclLoc.PACL_iBus, paclLoc.PACL_iDevice, paclLoc.PACL_iFunction,
                                        PCI_IO_BASE, &ucTemp);
                    if ((ucTemp & 0x0F) == 0x01) {
                        API_PciConfigInByte(paclLoc.PACL_iBus,
                                            paclLoc.PACL_iDevice,
                                            paclLoc.PACL_iFunction,
                                            PCI_IO_LIMIT, &ucTemp);
                        if ((ucTemp & 0x0F) == 0x01) {
                            paclLoc.PACL_iAttribute |= PCI_AUTO_CFG_ATTR_BUS_4GB_IO;
                            __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgDevProbe: 4G I/O \n", 0, 0, 0, 0, 0, 0);
                        }
                    }
                }

                API_PciConfigModifyDword(paclLoc.PACL_iBus, paclLoc.PACL_iDevice, paclLoc.PACL_iFunction,
                                         PCI_PREF_MEMORY_BASE, 0xfff0fff0, 0x0000fff0);
                API_PciConfigOutDword(paclLoc.PACL_iBus, paclLoc.PACL_iDevice, paclLoc.PACL_iFunction,
                                      PCI_PREF_LIMIT_UPPER32, 0);
                API_PciConfigOutDword(paclLoc.PACL_iBus, paclLoc.PACL_iDevice, paclLoc.PACL_iFunction,
                                      PCI_PREF_BASE_UPPER32, 0xffffffff);

                if (iInheritAttrib & PCI_AUTO_CFG_ATTR_BUS_PREFETCH) {
                    API_PciConfigInDword(paclLoc.PACL_iBus, paclLoc.PACL_iDevice, paclLoc.PACL_iFunction,
                                         PCI_PREF_MEMORY_BASE, &uiTemp);
                    if (uiTemp != 0) {
                        paclLoc.PACL_iAttribute |= PCI_AUTO_CFG_ATTR_BUS_PREFETCH;
                        __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgDevProbe: PF present\n", 0, 0, 0, 0, 0, 0);
                    }
                }
                break;

            default:
                iInheritAttrib &= (PCI_AUTO_CFG_ATTR_BUS_4GB_IO | PCI_AUTO_CFG_ATTR_BUS_PREFETCH);
                paclLoc.PACL_iAttribute |= iInheritAttrib;
                break;
            }

            if (*puiListSize < PCI_AUTO_CFG_LIST_MAX) {
                lib_memcpy (*pppaclList, &paclLoc, sizeof(PCI_AUTO_CFG_LOC));
                (*pppaclList)++;
                (*puiListSize)++;
            }

            if (paclLoc.PACL_iAttribute & PCI_AUTO_CFG_ATTR_BUS_PCI) {
                __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgDevProbe: scanning bus[%d]\n",
                                          (iSubBus + 1), 0, 0, 0, 0, 0 );

                iSubBus = __pciAutoCfgBusProbe(ppacoOpts,
                                               iBus,
                                               iSubBus + 1,
                                               &paclLoc,
                                               pppaclList,
                                               puiListSize);
            }

            if (iFunction == 0) {
                API_PciConfigInByte(paclLoc.PACL_iBus, paclLoc.PACL_iDevice, paclLoc.PACL_iFunction,
                                    PCI_HEADER_TYPE, &ucTemp);
                if ((ucTemp & PCI_HEADER_MULTI_FUNC) == 0) {
                    break;
                }
            }
        }
    }

    return  (iSubBus);
}
/*********************************************************************************************************
** 函数名称: API_PciAutoCfgDevReset
** 功能描述: 禁能设备并复位所有可写位
** 输　入  : ppaclPciLoc    设备参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciAutoCfgDevReset (PCI_AUTO_CFG_LOC *ppaclPciLoc)
{
    INT     iRet = PX_ERROR;

    iRet = API_PciConfigModifyDword(ppaclPciLoc->PACL_iBus,
                                    ppaclPciLoc->PACL_iDevice,
                                    ppaclPciLoc->PACL_iFunction,
                                    PCI_COMMAND, ~PCI_COMMAND_MASK, 0);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __pciAutoCfgBusProbe
** 功能描述: 配置桥并遍历桥下的所有设备
** 输　入  : ppacoPciOpts       操作选项
**           iPriBus            主总线
**           iSecBus            二级总线
**           ppaclLoc           设备信息
**           pppaclList         下一级设备信息
**           puiListSize        列表大小
** 输　出  : 总线列表数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciAutoCfgBusProbe (__PCI_AUTO_CFG_OPTS  *ppacoOpts,
                                  INT                   iPriBus,
                                  INT                   iSecBus,
                                  PCI_AUTO_CFG_LOC     *ppaclLoc,
                                  PCI_AUTO_CFG_LOC    **pppaclList,
                                  UINT32               *puiListSize)
{
    INT     iSubBus = 0xff;
    INT     iOffset = 0;

    API_PciAutoCfgDevReset(ppaclLoc);
    API_PciAutoCfgBusNumberSet(ppaclLoc, iPriBus, iSecBus, 0xff);

    __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgBusProbe: using bridge [%d,%d,%d,0x%02x]\n",
                                (ppaclLoc->PACL_iBus),
                                (ppaclLoc->PACL_iDevice),
                                (ppaclLoc->PACL_iFunction),
                                (ppaclLoc->PACL_iAttribute),
                                0, 0);
    __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgBusProbe: calling pciAutoDevProbe on bus [%d]\n",
                                iSecBus, 0, 0, 0, 0, 0);

    ppaclLoc->PACL_iOffset += (iPriBus > 0) ? (ppaclLoc->PACL_iDevice % 4) : 0;
    iOffset = (UINT8)ppaclLoc->PACL_iOffset;
    __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgBusProbe: int route offset for bridge is [%d]\n",
            iOffset, 0, 0, 0, 0, 0);

    iSubBus = __pciAutoCfgDevProbe(ppacoOpts,
                                      iSecBus,
                                      iOffset,
                                      (ppaclLoc->PACL_iAttribute),
                                      pppaclList,
                                      puiListSize);
    __PCI_AUTO_CFG_DEBUG_MSG("__pciAutoCfgBusProbe: post-config subordinate bus as [%d]\n",
                                iSubBus, 0, 0, 0, 0, 0);

    API_PciAutoCfgBusNumberSet(ppaclLoc, iPriBus, iSecBus, iSubBus);

    return  (iSubBus);
}
/*********************************************************************************************************
** 函数名称: __pciAutoCfgListCreate
** 功能描述: 扫描所有并创建列表
** 输　入  : ppacoOpts      操作选项
**           piListSize     列表大小
** 输　出  : 设备列表
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PCI_AUTO_CFG_LOC *__pciAutoCfgListCreate (__PCI_AUTO_CFG_OPTS *ppacoOpts,
                                                 UINT32              *puiListSize)
{
    INT                     iRet = PX_ERROR;
    PCI_AUTO_CFG_LOC        paclLoc;
    PCI_AUTO_CFG_LOC       *ppaclList;
    PCI_AUTO_CFG_LOC       *ppaclListNew;
    INT                     iBusAttr;
    UINT32                  uiMaxBus;

#if defined(PCI_AUTO_CFG_LIST_STATIC)                                   /* PCI_AUTO_CFG_LIST_STATIC     */
    ppaclList    = &_G_paclPciAutoCfgList[0];
    ppaclListNew = ppaclList;
#else                                                                   /* PCI_AUTO_CFG_LIST_RECLAIM    */
    ppaclList = lib_malloc(sizeof(PCI_AUTO_CFG_LOC) * PCI_AUTO_CFG_LIST_MAX);
    if (ppaclList == LW_NULL) {
        return  (LW_NULL);
    }
    ppaclListNew = ppaclList;
#endif                                                                  /* PCI_AUTO_CFG_LIST_STATIC     */

    _G_uiPciAutoCfgListSize = 0;
    *puiListSize            = 0;

    paclLoc.PACL_iBus       = 0;
    paclLoc.PACL_iDevice    = 0;
    paclLoc.PACL_iFunction  = 0;

    if (ppacoOpts->PACO_uiIo32Size == 0) {
        iBusAttr = PCI_AUTO_CFG_ATTR_BUS_PREFETCH;
    } else {
        iBusAttr = PCI_AUTO_CFG_ATTR_BUS_4GB_IO | PCI_AUTO_CFG_ATTR_BUS_PREFETCH;
    }
    uiMaxBus = __pciAutoCfgDevProbe(ppacoOpts,
                                    paclLoc.PACL_iBus,
                                    (INT)0,
                                    iBusAttr,
                                    &ppaclList,
                                    puiListSize);
    if (uiMaxBus <= 0) {
        ppacoOpts->PACO_uiBusMax = 0;
        return  (LW_NULL);
    }
    ppacoOpts->PACO_uiBusMax = uiMaxBus;
    iRet = API_PciConfigBusMaxSet(ppacoOpts->PACO_iIndex, ppacoOpts->PACO_uiBusMax);
    if (iRet != ERROR_NONE) {
        return  (LW_NULL);
    }

    return(ppaclListNew);
}
/*********************************************************************************************************
** 函数名称: __pciAutoCfgFunc
** 功能描述: 自动配置功能
** 输　入  : pvOpts     选项参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciAutoCfgFunc (PVOID  pvOpts)
{
    __PCI_AUTO_CFG_OPTS    *ppacoOpts  = LW_NULL;
    PCI_AUTO_CFG_LOC       *ppaclList  = LW_NULL;
    UINT32                  uiListSize = 0;
    BOOL                    bFlag      = LW_FALSE;

    if (pvOpts == LW_NULL) {
        return  (PX_ERROR);
    }

    ppacoOpts = (__PCI_AUTO_CFG_OPTS *)pvOpts;
    if (ppacoOpts->PACO_pfuncRollcall != LW_NULL) {
        bFlag = LW_FALSE;
        while (!bFlag) {
            ppaclList = __pciAutoCfgListCreate(ppacoOpts, &uiListSize);
            if ((*ppacoOpts->PACO_pfuncRollcall)() == ERROR_NONE) {
                bFlag = LW_TRUE;
            }
#ifdef PCI_AUTO_RECLAIM_LIST                                            /* PCI_AUTO_RECLAIM_LIST        */
            lib_free(ppaclList);
#endif                                                                  /* PCI_AUTO_RECLAIM_LIST        */

            if (bFlag == LW_TRUE) {
                break;
            }
        }
    }

    ppaclList = __pciAutoCfgListCreate(ppacoOpts, &uiListSize);

    __pciAutoCfgFuncConfigAll(ppacoOpts, ppaclList, uiListSize);

    _G_ppaclPciAutoCfgList  = ppaclList;
    _G_uiPciAutoCfgListSize = uiListSize;

#ifdef PCI_AUTO_RECLAIM_LIST                                            /* PCI_AUTO_RECLAIM_LIST        */
    lib_free(ppaclList);
    _G_ppaclPciAutoCfgList  = LW_NULL;
    _G_uiPciAutoCfgListSize = 0;
#endif                                                                  /* PCI_AUTO_RECLAIM_LIST        */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciAutoCfgBusNumberSet
** 功能描述: 配置桥并遍历桥下的设备数量 (Type 1 PCI Configuration Space Header)
** 输　入  : ppacoPciOpts       操作选项
**           iPrimary           主总线
**           iSecondary         二级总线
**           iSubordinate       下属总线
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_API
INT  API_PciAutoCfgBusNumberSet (PCI_AUTO_CFG_LOC *ppaclLoc,
                                    INT                  iPrimary,
                                    INT                  iSecondary,
                                    INT                  iSubordinate)
{
    INT         iRet    = PX_ERROR;
    UINT32      uiValue = 0;

    uiValue = (iSubordinate << 16) + (iSubordinate << 8) + iPrimary;
    iRet    = API_PciConfigModifyDword(ppaclLoc->PACL_iBus,
                                       ppaclLoc->PACL_iDevice,
                                       ppaclLoc->PACL_iFunction,
                                       PCI_PRIMARY_BUS, 0x00ffffff, uiValue);
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PciAutoCfgCtl
** 功能描述: 自动配置选项参数获取与配置
** 输　入  : pvOpts     选项参数
**           iCmd       命令
**           pvArg      命令参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciAutoCfgCtl (PVOID  pvOpts, INT  iCmd, PVOID  pvArg)
{
    PCI_AUTO_CFG_SYSTEM     *ppacsSystem = LW_NULL;
    __PCI_AUTO_CFG_OPTS     *ppacoOpts   = LW_NULL;

    ppacoOpts = (__PCI_AUTO_CFG_OPTS *)pvOpts;
    if (ppacoOpts == LW_NULL) {
        return  (PX_ERROR);
    }

    switch (iCmd) {

    case PCI_AUTO_CFG_CTL_STRUCT_COPY:
        if (pvArg == LW_NULL) {
            return(PX_ERROR);
        }

        ppacsSystem = (PCI_AUTO_CFG_SYSTEM *)pvArg;

        ppacoOpts->PACO_iIndex              = ppacsSystem->PACS_iIndex;
        ppacoOpts->PACO_uiBusMax            = ppacsSystem->PACS_uiBusMax;

        ppacoOpts->PACO_uiMem32Addr         = ppacsSystem->PACS_uiMem32Addr;
        ppacoOpts->PACO_uiMem32Size         = ppacsSystem->PACS_uiMem32Size;
        ppacoOpts->PACO_uiMemIo32Addr       = ppacsSystem->PACS_uiMemIo32Addr;
        ppacoOpts->PACO_uiMemIo32Size       = ppacsSystem->PACS_uiMemIo32Size;
        ppacoOpts->PACO_uiIo32Addr          = ppacsSystem->PACS_uiIo32Addr;
        ppacoOpts->PACO_uiIo32Size          = ppacsSystem->PACS_uiIo32Size;
        ppacoOpts->PACO_uiIo16Addr          = ppacsSystem->PACS_uiIo16Addr;
        ppacoOpts->PACO_uiIo16Size          = ppacsSystem->PACS_uiIo16Size;

        ppacoOpts->PACO_uiLatencyMax        = ppacsSystem->PACS_uiLatencyMax;

        ppacoOpts->PACO_iCacheSize          = ppacsSystem->PACS_iCacheSize;

        ppacoOpts->PACO_pfuncInclude        = ppacsSystem->PACS_pfuncInclude;

        ppacoOpts->PACO_bAutoIntRouting     = ppacsSystem->PACS_bAutoIntRouting;
        ppacoOpts->PACO_pfuncIntAssign      = ppacsSystem->PACS_pfuncIntAssign;

        ppacoOpts->PACO_pfuncBridgePreInit  = ppacsSystem->PACS_pfuncBridgePreInit;
        ppacoOpts->PACO_pfuncBridgePostInit = ppacsSystem->PACS_pfuncBridgePostInit;

        ppacoOpts->PACO_pfuncRollcall       = ppacsSystem->PACS_pfuncRollcall;
        break;

    case PCI_AUTO_CFG_CTL_MINIMIZE_RESOURCES:
        ppacoOpts->PACO_uiMemBusResMin = (BOOL)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_FBB_ENABLE:
        ppacoOpts->PACO_bFb2bEnable = LW_TRUE;
        ppacoOpts->PACO_bFb2bActive = LW_FALSE;
        if (__pciAutoCfgFb2bEnable(ppacoOpts) == ERROR_NONE) {
            ppacoOpts->PACO_bFb2bActive = LW_TRUE;
        }
        if (pvArg != LW_NULL) {
            (*(BOOL *)pvArg) = ppacoOpts->PACO_bFb2bActive;
        }
        break;

    case PCI_AUTO_CFG_CTL_FBB_DISABLE:
        if ((ppacoOpts->PACO_bFb2bEnable == LW_TRUE) ||
            (ppacoOpts->PACO_bFb2bActive == LW_TRUE)) {
            __pciAutoCfgFb2bDisable(ppacoOpts);
            ppacoOpts->PACO_bFb2bActive = LW_FALSE;
        }
        ppacoOpts->PACO_bFb2bEnable = LW_FALSE;
        break;

    case PCI_AUTO_CFG_CTL_FBB_UPDATE:
        if ((ppacoOpts->PACO_bInitFlag == LW_TRUE  ) &&
            (ppacoOpts->PACO_bFb2bEnable == LW_TRUE)) {
            ppacoOpts->PACO_bFb2bActive = __pciAutoCfgFb2bEnable(ppacoOpts);
        }
        if (pvArg != LW_NULL) {
            (*(BOOL *)pvArg) = ppacoOpts->PACO_bFb2bActive;
        }
        break;

    case PCI_AUTO_CFG_CTL_FBB_STATUS_GET:
        (*(BOOL *)pvArg) = ppacoOpts->PACO_bFb2bEnable;
        break;

    case PCI_AUTO_CFG_CTL_MAX_LAT_FUNC_SET:
        ppacoOpts->PACO_pfuncLatencyMax = (PCI_AUTO_CFG_LATENCY_MAX_FUNC_PTR)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_MAX_LAT_ARG_SET:
        ppacoOpts->PACO_pvLatencyMaxArg = pvArg;
        break;

    case PCI_AUTO_CFG_CTL_MSG_LOG_FUNC_SET:
        ppacoOpts->PACO_pfuncLogMsg = (PCI_AUTO_CFG_LOG_MSG_FUNC_PTR)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_MAX_BUS_SET:
        ppacoOpts->PACO_uiBusMax = (INT)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_MAX_BUS_GET:
        (*(INT *)pvArg) = ppacoOpts->PACO_uiBusMax;
        break;

    case PCI_AUTO_CFG_CTL_CACHE_SIZE_SET:
        ppacoOpts->PACO_iCacheSize = (INT)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_CACHE_SIZE_GET:
        *(INT *)pvArg = ppacoOpts->PACO_iCacheSize;
        break;

    case PCI_AUTO_CFG_CTL_MAX_LAT_ALL_SET:
        ppacoOpts->PACO_uiLatencyMax = (UINT)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_MAX_LAT_ALL_GET:
        if (ppacoOpts->PACO_pfuncLatencyMax == LW_NULL) {
            *(UINT *)pvArg = ppacoOpts->PACO_uiLatencyMax;
        } else {
            *(UINT *)pvArg = 0xffffffff;
        }
        break;

    case PCI_AUTO_CFG_CTL_AUTO_INT_ROUTE_SET:
        ppacoOpts->PACO_bAutoIntRouting = (BOOL)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_AUTO_INT_ROUTE_GET:
        *(BOOL *)pvArg = ppacoOpts->PACO_bAutoIntRouting;
        break;

    case PCI_AUTO_CFG_CTL_MEM32_LOC_SET:
        ppacoOpts->PACO_uiMem32Addr = (UINT)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_MEM32_SIZE_SET:
        ppacoOpts->PACO_uiMem32Size = (UINT)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_MEM32_SIZE_GET:
        (*(UINT32 *)pvArg) = ppacoOpts->PACO_uiMem32Used;
        break;

    case PCI_AUTO_CFG_CTL_MEMIO32_LOC_SET:
        ppacoOpts->PACO_uiMemIo32Addr = (UINT)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_MEMIO32_SIZE_SET:
        ppacoOpts->PACO_uiMemIo32Size = (UINT)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_MEMIO32_SIZE_GET:
        *(UINT *)pvArg = ppacoOpts->PACO_uiMemIo32Used;
        break;

    case PCI_AUTO_CFG_CTL_IO32_LOC_SET:
        ppacoOpts->PACO_uiIo32Addr = (UINT)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_IO32_SIZE_SET:
        ppacoOpts->PACO_uiIo32Size = (UINT)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_IO32_SIZE_GET:
        *(UINT *)pvArg = ppacoOpts->PACO_uiIo32Used;
        break;

    case PCI_AUTO_CFG_CTL_IO16_LOC_SET:
        ppacoOpts->PACO_uiIo16Addr = (UINT)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_IO16_SIZE_SET:
        ppacoOpts->PACO_uiIo16Size = (UINT)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_IO16_SIZE_GET:
        *(UINT *)pvArg = ppacoOpts->PACO_uiIo16Used;
        break;

    case PCI_AUTO_CFG_CTL_INCLUDE_FUNC_SET:
        ppacoOpts->PACO_pfuncInclude = (PCI_AUTO_CFG_INCLUDE_FUNC_PTR)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_INT_ASSIGN_FUNC_SET:
        ppacoOpts->PACO_pfuncIntAssign = (PCI_AUTO_CFG_INT_ASSIGN_FUNC_PTR)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_BRIDGE_PRE_CONFIG_FUNC_SET:
        ppacoOpts->PACO_pfuncBridgePreInit = (PCI_AUTO_CFG_BRIDGE_PRE_INIT_FUNC_PTR)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_BRIDGE_POST_CONFIG_FUNC_SET:
        ppacoOpts->PACO_pfuncBridgePostInit = (PCI_AUTO_CFG_BRIDGE_POST_INIT_FUNC_PTR)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_ROLLCALL_FUNC_SET:
        ppacoOpts->PACO_pfuncRollcall = (PCI_AUTO_CFG_ROLLCALL_FUNC_PTR)pvArg;
        break;

    case PCI_AUTO_CFG_CTL_TEMP_SPACE_SET:
        ppacoOpts->PACO_ppaclFuncList    = ((PCI_AUTO_CFG_MEM *)pvArg)->PACM_pvMemAddr;
        ppacoOpts->PACO_iFuncListEntries =
                         ((PCI_AUTO_CFG_MEM *)pvArg)->PACM_iMemSize / sizeof(PCI_AUTO_CFG_LOC);
        break;

    default:
        return(PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciAutoCfgInit
** 功能描述: 自动配置初始化
**              Status Register
**              Command Register
**              Latency Timer
**              Cache Line Size
**              Memory and/or I/O Base Address and Limit Registers
**              Primary, Secondary, Subordinate Bus Number (For PCI-PCI Bridges)
**              Expansion ROM Disable
**              Interrupt Line
** 输　入  : ppacsSystem        自动配置系统参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PciAutoCfgInit (PCI_AUTO_CFG_SYSTEM *ppacsSystem)
{
    __PCI_AUTO_CFG_OPTS     *ppacoOpts = LW_NULL;

    ppacoOpts = __pciAutoCfgLibInit(LW_NULL);
    API_PciAutoCfgCtl(ppacoOpts, PCI_AUTO_CFG_CTL_STRUCT_COPY, (PVOID)ppacsSystem);

    __pciAutoCfgFunc(ppacoOpts);
    __pciAutoCfgSystemUpdate(ppacsSystem, ppacoOpts);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
