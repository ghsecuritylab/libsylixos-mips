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
** 文   件   名: pciLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 09 月 28 日
**
** 描        述: PCI 总线驱动模型.
**
** BUG:
2015.08.26  支持 PCI_MECHANISM_0 模式.
2016.04.25  增加 设备驱动匹配 支持函数.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "pciDev.h"
#include "pciProc.h"
/*********************************************************************************************************
  PCI 主控器
*********************************************************************************************************/
PCI_CONFIG      *_G_p_pciConfig;
/*********************************************************************************************************
  PCI 主控器配置 IO 操作 PCI_MECHANISM_0
*********************************************************************************************************/
#define PCI_CFG_READ(iBus, iSlot, iFunc, iOft, iLen, pvRet)                                             \
        _G_p_pciConfig->PCIC_pDrvFuncs0->cfgRead(iBus, iSlot, iFunc, iOft, iLen, pvRet)
#define PCI_CFG_WRITE(iBus, iSlot, iFunc, iOft, iLen, uiData)                                           \
        _G_p_pciConfig->PCIC_pDrvFuncs0->cfgWrite(iBus, iSlot, iFunc, iOft, iLen, uiData)
#define PCI_VPD_READ(iBus, iSlot, iFunc, iPos, pucBuf, iLen)                                            \
        _G_p_pciConfig->PCIC_pDrvFuncs0->vpdRead(iBus, iSlot, iFunc, iPos, pucBuf, iLen)
#define PCI_IRQ_GET(iBus, iSlot, iFunc, iMsiEn, iLine, iPin, pulVector)                                 \
        _G_p_pciConfig->PCIC_pDrvFuncs0->irqGet(iBus, iSlot, iFunc, iMsiEn, iLine, iPin, pulVector)
#define PCI_CFG_SPCL(iBus, uiMsg)                                                                       \
        _G_p_pciConfig->PCIC_pDrvFuncs0->cfgSpcl(iBus, uiMsg)
#define PCI_CFG_SPCL_IS_EN()                                                                            \
        _G_p_pciConfig->PCIC_pDrvFuncs0->cfgSpcl
/*********************************************************************************************************
  PCI 主控器配置地址 PCI_MECHANISM_1 2
*********************************************************************************************************/
#define PCI_CONFIG_ADDR0()          _G_p_pciConfig->PCIC_ulConfigAddr
#define PCI_CONFIG_ADDR1()          _G_p_pciConfig->PCIC_ulConfigData
#define PCI_CONFIG_ADDR2()          _G_p_pciConfig->PCIC_ulConfigBase
/*********************************************************************************************************
  PCI 主控器配置 IO 操作 PCI_MECHANISM_1 2
*********************************************************************************************************/
#define PCI_IN_BYTE(addr)           _G_p_pciConfig->PCIC_pDrvFuncs12->ioInByte((addr))
#define PCI_IN_WORD(addr)           _G_p_pciConfig->PCIC_pDrvFuncs12->ioInWord((addr))
#define PCI_IN_DWORD(addr)          _G_p_pciConfig->PCIC_pDrvFuncs12->ioInDword((addr))
#define PCI_OUT_BYTE(addr, data)    _G_p_pciConfig->PCIC_pDrvFuncs12->ioOutByte((UINT8)(data), (addr))
#define PCI_OUT_WORD(addr, data)    _G_p_pciConfig->PCIC_pDrvFuncs12->ioOutWord((UINT16)(data), (addr))
#define PCI_OUT_DWORD(addr, data)   _G_p_pciConfig->PCIC_pDrvFuncs12->ioOutDword((UINT32)(data), (addr))
/*********************************************************************************************************
  PCI 是否为无效 VENDOR_ID
*********************************************************************************************************/
#define PCI_VENDOR_ID_IS_INVALIDATE(vendor)     \
        (((vendor) == 0xffff) || ((vendor) == 0x0000))
/*********************************************************************************************************
  前置声明
*********************************************************************************************************/
static PCI_DEVICE_ID_HANDLE __pciDevMatchId(PCI_DEV_HANDLE hDevHandle, PCI_DEVICE_ID_HANDLE hId);
/*********************************************************************************************************
** 函数名称: API_PciConfigInit
** 功能描述: 安装 pci 主控器驱动程序
** 输　入  : ppcicfg  pci 主控器驱动程序
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciConfigInit (PCI_CONFIG *ppcicfg)
{
    if (_G_p_pciConfig == LW_NULL) {
        _G_p_pciConfig =  ppcicfg;
        
#if LW_CFG_PROCFS_EN > 0
        __procFsPciInit();
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */

        API_PciDevInit();
        API_PciDevListCreate();
        API_PciDrvInit();
    }
    
    if (_G_p_pciConfig->PCIC_ulLock == LW_OBJECT_HANDLE_INVALID) {
        _G_p_pciConfig->PCIC_ulLock = API_SemaphoreMCreate("pci_lock", LW_PRIO_DEF_CEILING,
                                                           LW_OPTION_WAIT_PRIORITY |
                                                           LW_OPTION_DELETE_SAFE |
                                                           LW_OPTION_INHERIT_PRIORITY |
                                                           LW_OPTION_OBJECT_GLOBAL,
                                                           LW_NULL);
    }
    
    return  (_G_p_pciConfig) ? (ERROR_NONE) : (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_PciConfigReset
** 功能描述: 停止 pci 所有设备 (只遍历最上层总线设备)
** 输　入  : iRebootType   系统复位类型
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
VOID  API_PciConfigReset (INT  iRebootType)
{
    (VOID)iRebootType;
    
    if (_G_p_pciConfig == LW_NULL) {
        return;
    }
    
    API_PciTraversalDev(0, LW_FALSE, (INT (*)())API_PciFuncDisable, LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_PciLock
** 功能描述: pci 锁定
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciLock (VOID)
{
    if (_G_p_pciConfig->PCIC_ulLock) {
        API_SemaphoreMPend(_G_p_pciConfig->PCIC_ulLock, LW_OPTION_WAIT_INFINITE);
        return  (ERROR_NONE);
        
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_PciUnlock
** 功能描述: pci 解锁
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciUnlock (VOID)
{
    if (_G_p_pciConfig->PCIC_ulLock) {
        API_SemaphoreMPost(_G_p_pciConfig->PCIC_ulLock);
        return  (ERROR_NONE);
        
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_PciConfigInByte
** 功能描述: 从 pci 配置空间获取一个字节
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           iOft      配置空间地址偏移
**           pucValue  获取的结果
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciConfigInByte (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT8 *pucValue)
{
    INTREG  iregInterLevel;
    UINT8   ucRet = 0;
    UINT32  uiDword;
    INT     iRetVal = PX_ERROR;
    
    iregInterLevel = KN_INT_DISABLE();
    
    switch (_G_p_pciConfig->PCIC_ucMechanism) {
    
    case PCI_MECHANISM_0:
        if (PCI_CFG_READ(iBus, iSlot, iFunc, iOft, 1, (PVOID)&ucRet) < 0) {
            ucRet = 0xff;
        } else {
            iRetVal = ERROR_NONE;
        }
        break;
    
    case PCI_MECHANISM_1:
        PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
        ucRet = PCI_IN_BYTE(PCI_CONFIG_ADDR1() + (iOft & 0x3));
        iRetVal = ERROR_NONE;
        break;
        
    case PCI_MECHANISM_2:
        PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0xf0 | (iFunc << 1));
        PCI_OUT_BYTE(PCI_CONFIG_ADDR1(), iBus);
        uiDword = PCI_IN_DWORD(PCI_CONFIG_ADDR2() | ((iSlot & 0x000f) << 8) | (iOft & 0xfc));
        PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0);
        uiDword >>= (iOft & 0x03) * 8;
        ucRet   = (UINT8)(uiDword & 0xff);
        iRetVal = ERROR_NONE;
        break;
        
    default:
        break;
    }
    
    KN_INT_ENABLE(iregInterLevel);

    if (pucValue) {
        *pucValue = ucRet;
    }

    return  (iRetVal);
}
/*********************************************************************************************************
** 函数名称: API_PciConfigInWord
** 功能描述: 从 pci 配置空间获取一个字 (16 bit)
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           iOft      配置空间地址偏移
**           pusValue  获取的结果
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciConfigInWord (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT16 *pusValue)
{
    INTREG  iregInterLevel;
    UINT16  usRet = 0;
    UINT32  uiDword;
    INT     iRetVal = PX_ERROR;
    
    if (iOft & 0x01) {
        return  (PX_ERROR);
    }

    iregInterLevel = KN_INT_DISABLE();

    switch (_G_p_pciConfig->PCIC_ucMechanism) {
    
    case PCI_MECHANISM_0:
        if (PCI_CFG_READ(iBus, iSlot, iFunc, iOft, 2, (PVOID)&usRet) < 0) {
            usRet = 0xffff;
        } else {
            iRetVal = ERROR_NONE;
        }
        break;

    case PCI_MECHANISM_1:
        PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
        usRet = PCI_IN_WORD(PCI_CONFIG_ADDR1() + (iOft & 0x2));
        iRetVal = ERROR_NONE;
        break;

    case PCI_MECHANISM_2:
        PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0xf0 | (iFunc << 1));
        PCI_OUT_BYTE(PCI_CONFIG_ADDR1(), iBus);
        uiDword = PCI_IN_DWORD(PCI_CONFIG_ADDR2() | ((iSlot & 0x000f) << 8) | (iOft & 0xfc));
        PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0);
        uiDword >>= (iOft & 0x02) * 8;
        usRet   = (UINT16)(uiDword & 0xffff);
        iRetVal = ERROR_NONE;
        break;

    default:
        break;
    }
    
    KN_INT_ENABLE(iregInterLevel);

    if (pusValue) {
        *pusValue = usRet;
    }

    return  (iRetVal);
}
/*********************************************************************************************************
** 函数名称: API_PciConfigInDword
** 功能描述: 从 pci 配置空间获取一个双字 (32 bit)
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           iOft      配置空间地址偏移
**           puiValue  获取的结果
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciConfigInDword (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT32 *puiValue)
{
    INTREG  iregInterLevel;
    UINT32  uiRet   = 0;
    INT     iRetVal = PX_ERROR;
    
    if (iOft & 0x03) {
        return  (PX_ERROR);
    }
    
    iregInterLevel = KN_INT_DISABLE();

    switch (_G_p_pciConfig->PCIC_ucMechanism) {
    
    case PCI_MECHANISM_0:
        if (PCI_CFG_READ(iBus, iSlot, iFunc, iOft, 4, (PVOID)&uiRet) < 0) {
            uiRet = 0xffffffff;
        } else {
            iRetVal = ERROR_NONE;
        }
        break;

    case PCI_MECHANISM_1:
        PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
        uiRet = PCI_IN_DWORD(PCI_CONFIG_ADDR1());
        iRetVal = ERROR_NONE;
        break;

    case PCI_MECHANISM_2:
        PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0xf0 | (iFunc << 1));
        PCI_OUT_BYTE(PCI_CONFIG_ADDR1(), iBus);
        uiRet = PCI_IN_DWORD(PCI_CONFIG_ADDR2() | ((iSlot & 0x000f) << 8) | (iOft & 0xfc));
        PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0);
        iRetVal = ERROR_NONE;
        break;

    default:
        break;
    }
    
    KN_INT_ENABLE(iregInterLevel);

    if (puiValue) {
        *puiValue = uiRet;
    }

    return  (iRetVal);
}
/*********************************************************************************************************
** 函数名称: API_PciConfigOutByte
** 功能描述: 向 pci 配置空间写入一个字节
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           iOft      配置空间地址偏移
**           ucValue   写入的数据
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciConfigOutByte (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT8 ucValue)
{
    INTREG  iregInterLevel;
    UINT32  uiRet;
    UINT32  uiMask = 0x000000ff;
    
    iregInterLevel = KN_INT_DISABLE();

    switch (_G_p_pciConfig->PCIC_ucMechanism) {
    
    case PCI_MECHANISM_0:
        PCI_CFG_WRITE(iBus, iSlot, iFunc, iOft, 1, ucValue);
        break;

    case PCI_MECHANISM_1:
        PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
        PCI_OUT_BYTE((PCI_CONFIG_ADDR1() + (iOft & 0x3)), ucValue);
        break;

    case PCI_MECHANISM_2:
        PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0xf0 | (iFunc << 1));
        PCI_OUT_BYTE(PCI_CONFIG_ADDR1(), iBus);
        uiRet    = PCI_IN_DWORD(PCI_CONFIG_ADDR2() | ((iSlot & 0x000f) << 8) | (iOft & 0xfc));
        ucValue  = (ucValue & uiMask) << ((iOft & 0x03) * 8);
        uiMask <<= (iOft & 0x03) * 8;
        uiRet    = (uiRet & ~uiMask) | ucValue;
        PCI_OUT_DWORD((PCI_CONFIG_ADDR2() | ((iSlot & 0x000f) << 8) | (iOft & 0xfc)), uiRet);
        PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0);
        break;

    default:
        break;
    }
    
    KN_INT_ENABLE(iregInterLevel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciConfigOutWord
** 功能描述: 向 pci 配置空间写入一个字 (16 bit)
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           iOft      配置空间地址偏移
**           usValue   写入的数据
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciConfigOutWord (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT16 usValue)
{
    INTREG  iregInterLevel;
    UINT32  uiRet;
    UINT32  uiMask = 0x0000ffff;
    
    if (iOft & 0x01) {
        return  (PX_ERROR);
    }
    
    iregInterLevel = KN_INT_DISABLE();

    switch (_G_p_pciConfig->PCIC_ucMechanism) {
    
    case PCI_MECHANISM_0:
        PCI_CFG_WRITE(iBus, iSlot, iFunc, iOft, 2, usValue);
        break;

    case PCI_MECHANISM_1:
        PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
        PCI_OUT_WORD((PCI_CONFIG_ADDR1() + (iOft & 0x2)), usValue);
        break;

    case PCI_MECHANISM_2:
        PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0xf0 | (iFunc << 1));
        PCI_OUT_BYTE(PCI_CONFIG_ADDR1(), iBus);
        uiRet    = PCI_IN_DWORD(PCI_CONFIG_ADDR2() | ((iSlot & 0x000f) << 8) | (iOft & 0xfc));
        usValue  = (usValue & uiMask) << ((iOft & 0x02) * 8);
        uiMask <<= (iOft & 0x02) * 8;
        uiRet    = (uiRet & ~uiMask) | usValue;
        PCI_OUT_DWORD((PCI_CONFIG_ADDR2() | ((iSlot & 0x000f) << 8) | (iOft & 0xfc)), uiRet);
        PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0);
        break;

    default:
        break;
    }
    
    KN_INT_ENABLE(iregInterLevel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciConfigOutDword
** 功能描述: 向 pci 配置空间写入一个双字 (32 bit)
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           iOft      配置空间地址偏移
**           uiValue   写入的数据
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciConfigOutDword (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT32 uiValue)
{
    INTREG  iregInterLevel;
    
    if (iOft & 0x03) {
        return  (PX_ERROR);
    }
    
    iregInterLevel = KN_INT_DISABLE();
    
    switch (_G_p_pciConfig->PCIC_ucMechanism) {
    
    case PCI_MECHANISM_0:
        PCI_CFG_WRITE(iBus, iSlot, iFunc, iOft, 4, uiValue);
        break;

    case PCI_MECHANISM_1:
        PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | (iOft & 0xfc) | 0x80000000));
        PCI_OUT_DWORD(PCI_CONFIG_ADDR1(), uiValue);
        break;

    case PCI_MECHANISM_2:
        PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0xf0 | (iFunc << 1));
        PCI_OUT_BYTE(PCI_CONFIG_ADDR1(), iBus);
        PCI_OUT_DWORD((PCI_CONFIG_ADDR2() | ((iSlot & 0x000f) << 8) | (iOft & 0xfc)), uiValue);
        PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0);
        break;

    default:
        break;
    }
    
    KN_INT_ENABLE(iregInterLevel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciConfigModifyByte
** 功能描述: 向 pci 配置空间带有掩码位的修改一个字节
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           iOft      配置空间地址偏移
**           ucMask    掩码
**           ucValue   写入的数据
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciConfigModifyByte (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT8 ucMask, UINT8 ucValue)
{
    INTREG  iregInterLevel;
    UINT8   ucTemp;
    INT     iRet = PX_ERROR;
    
    iregInterLevel = KN_INT_DISABLE();
    
    if (API_PciConfigInByte(iBus, iSlot, iFunc, iOft, &ucTemp) == ERROR_NONE) {
        ucTemp = (ucTemp & ~ucMask) | (ucValue & ucMask);
        iRet   = API_PciConfigOutByte(iBus, iSlot, iFunc, iOft, ucTemp);
    }
    
    KN_INT_ENABLE(iregInterLevel);
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PciConfigModifyWord
** 功能描述: 向 pci 配置空间带有掩码位的修改一个字 (16 bit)
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           iOft      配置空间地址偏移
**           usMask    掩码
**           usValue   写入的数据
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciConfigModifyWord (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT16 usMask, UINT16 usValue)
{
    INTREG  iregInterLevel;
    UINT16  usTemp;
    INT     iRet = PX_ERROR;
    
    if (iOft & 0x01) {
        return  (PX_ERROR);
    }
    
    iregInterLevel = KN_INT_DISABLE();
    
    if (API_PciConfigInWord(iBus, iSlot, iFunc, iOft, &usTemp) == ERROR_NONE) {
        usTemp = (usTemp & ~usMask) | (usValue & usMask);
        iRet   = API_PciConfigOutWord(iBus, iSlot, iFunc, iOft, usTemp);
    }
    
    KN_INT_ENABLE(iregInterLevel);
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PciConfigModifyDword
** 功能描述: 向 pci 配置空间带有掩码位的修改一个双字 (32 bit)
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           iOft      配置空间地址偏移
**           uiMask    掩码
**           uiValue   写入的数据
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciConfigModifyDword (INT iBus, INT iSlot, INT iFunc, INT iOft, UINT32 uiMask, UINT32 uiValue)
{
    INTREG  iregInterLevel;
    UINT32  uiTemp;
    INT     iRet = PX_ERROR;
    
    if (iOft & 0x03) {
        return  (PX_ERROR);
    }
    
    iregInterLevel = KN_INT_DISABLE();
    
    if (API_PciConfigInDword(iBus, iSlot, iFunc, iOft, &uiTemp) == ERROR_NONE) {
        uiTemp = (uiTemp & ~uiMask) | (uiValue & uiMask);
        iRet   = API_PciConfigOutDword(iBus, iSlot, iFunc, iOft, uiTemp);
    }
    
    KN_INT_ENABLE(iregInterLevel);
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PciSpecialCycle
** 功能描述: 向 pci 总线上广播一个消息
** 输　入  : iBus      总线号
**           uiMsg     消息
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciSpecialCycle (INT iBus, UINT32 uiMsg)
{
    INTREG  iregInterLevel;
    INT     iRet  = ERROR_NONE;
    INT     iSlot = 0x1f;
    INT     iFunc = 0x07;
    
    iregInterLevel = KN_INT_DISABLE();
    
    switch (_G_p_pciConfig->PCIC_ucMechanism) {
    
    case PCI_MECHANISM_0:
        if (PCI_CFG_SPCL_IS_EN()) {
            PCI_CFG_SPCL(iBus, uiMsg);
        }
        break;
    
    case PCI_MECHANISM_1:
        PCI_OUT_DWORD(PCI_CONFIG_ADDR0(), (PCI_PACKET(iBus, iSlot, iFunc) | 0x80000000));
        PCI_OUT_DWORD(PCI_CONFIG_ADDR1(), uiMsg);
        break;
    
    case PCI_MECHANISM_2:
        PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0xff);
        PCI_OUT_BYTE(PCI_CONFIG_ADDR1(), 0x00);
        PCI_OUT_DWORD((PCI_CONFIG_ADDR2() | ((iSlot & 0x000f) << 8)), uiMsg);
        PCI_OUT_BYTE(PCI_CONFIG_ADDR0(), 0);
        break;
    
    default:
        iRet = PX_ERROR;
        break;
    }
    
    KN_INT_ENABLE(iregInterLevel);
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PciFindDev
** 功能描述: 查询一个指定的 pci 设备
** 输　入  : usVendorId    供应商号
**           usDeviceId    设备 ID
**           iInstance     第几个设备实例
**           piBus         获取的总线号
**           piSlot        获取的插槽号
**           piFunc        获取的功能号
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciFindDev (UINT16  usVendorId, 
                     UINT16  usDeviceId, 
                     INT     iInstance,
                     INT    *piBus, 
                     INT    *piSlot, 
                     INT    *piFunc)
{
    INT         iBus;
    INT         iSlot;
    INT         iFunc;
    
    INT         iRet = PX_ERROR;

    UINT8       ucHeader;
    UINT16      usVendorTemp;
    UINT16      usDeviceTemp;
    
    if (iInstance < 0) {
        return  (PX_ERROR);
    }
    
    for (iBus = 0; iBus < PCI_MAX_BUS; iBus++) {
        for (iSlot = 0; iSlot < PCI_MAX_SLOTS; iSlot++) {
            for (iFunc = 0; iFunc < PCI_MAX_FUNCTIONS; iFunc++) {
            
                API_PciConfigInWord(iBus, iSlot, iFunc, PCI_VENDOR_ID, &usVendorTemp);
                API_PciConfigInWord(iBus, iSlot, iFunc, PCI_DEVICE_ID, &usDeviceTemp);
                
                if (PCI_VENDOR_ID_IS_INVALIDATE(usVendorTemp)) {        /*  没有设备存在                */
                    if (iFunc == 0) {
                        break;
                    }
                    continue;                                           /*  next function               */
                }
                
                if ((usVendorTemp == usVendorId) && 
                    (usDeviceTemp == usDeviceId)) {                     /*  匹配查询条件                */
                    
                    if (iInstance == 0) {
                        if (piBus) {
                            *piBus = iBus;
                        }
                        if (piSlot) {
                            *piSlot = iSlot;
                        }
                        if (piFunc) {
                            *piFunc = iFunc;
                        }
                        iRet = ERROR_NONE;
                        goto    __out;
                    }
                    iInstance--;
                }
                
                if (iFunc == 0) {
                    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_HEADER_TYPE, &ucHeader);
                    
                    if ((ucHeader & PCI_HEADER_MULTI_FUNC) != PCI_HEADER_MULTI_FUNC) {
                        break;
                    }
                }
            }
        }
    }
    
__out:
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PciFindClass
** 功能描述: 查询指定设备类型的 pci 设备
** 输　入  : usClassCode   类型编码 (PCI_CLASS_STORAGE_SCSI, PCI_CLASS_DISPLAY_XGA ...)
**           iInstance     第几个设备实例
**           piBus         获取的总线号
**           piSlot        获取的插槽号
**           piFunc        获取的功能号
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciFindClass (UINT16  usClassCode, 
                       INT     iInstance,
                       INT    *piBus, 
                       INT    *piSlot, 
                       INT    *piFunc)
{
    INT         iBus;
    INT         iSlot;
    INT         iFunc;
    
    INT         iRet = PX_ERROR;

    UINT8       ucHeader;
    UINT16      usVendorTemp;
    UINT16      usClassTemp;
    
    if (iInstance < 0) {
        return  (PX_ERROR);
    }
    
    for (iBus = 0; iBus < PCI_MAX_BUS; iBus++) {
        for (iSlot = 0; iSlot < PCI_MAX_SLOTS; iSlot++) {
            for (iFunc = 0; iFunc < PCI_MAX_FUNCTIONS; iFunc++) {
            
                API_PciConfigInWord(iBus, iSlot, iFunc, PCI_VENDOR_ID, &usVendorTemp);
                
                if (PCI_VENDOR_ID_IS_INVALIDATE(usVendorTemp)) {        /*  没有设备存在                */
                    if (iFunc == 0) {
                        break;
                    }
                    continue;                                           /*  next function               */
                }
            
                API_PciConfigInWord(iBus, iSlot, iFunc, PCI_CLASS_DEVICE, &usClassTemp);
                
                if (usClassTemp == usClassCode) {
                
                    if (iInstance == 0) {
                        if (piBus) {
                            *piBus = iBus;
                        }
                        if (piSlot) {
                            *piSlot = iSlot;
                        }
                        if (piFunc) {
                            *piFunc = iFunc;
                        }
                        iRet = ERROR_NONE;
                        goto    __out;
                    }
                    iInstance--;
                }
                
                if (iFunc == 0) {
                    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_HEADER_TYPE, &ucHeader);
                    
                    if ((ucHeader & PCI_HEADER_MULTI_FUNC) != PCI_HEADER_MULTI_FUNC) {
                        break;
                    }
                }
            }
        }
    }
    
__out:
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PciTraversal
** 功能描述: pci 总线遍历
** 输　入  : pfuncCall     遍历回调函数
**           pvArg         回调函数参数
**           iMaxBusNum    最大总线号
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciTraversal (INT (*pfuncCall)(), PVOID pvArg, INT iMaxBusNum)
{
    INT         iBus;
    INT         iSlot;
    INT         iFunc;

    UINT8       ucHeader;
    UINT16      usVendorTemp;
    
    if (!pfuncCall || (iMaxBusNum < 0)) {
        return  (PX_ERROR);
    }
    
    iMaxBusNum = (iMaxBusNum > (PCI_MAX_BUS - 1)) ? (PCI_MAX_BUS - 1) : iMaxBusNum;
    
    for (iBus = 0; iBus <= iMaxBusNum; iBus++) {
        for (iSlot = 0; iSlot < PCI_MAX_SLOTS; iSlot++) {
            for (iFunc = 0; iFunc < PCI_MAX_FUNCTIONS; iFunc++) {
            
                API_PciConfigInWord(iBus, iSlot, iFunc, PCI_VENDOR_ID, &usVendorTemp);
                
                if (PCI_VENDOR_ID_IS_INVALIDATE(usVendorTemp)) {        /*  没有设备存在                */
                    if (iFunc == 0) {
                        break;                                          /*  next slot                   */
                    }
                    continue;                                           /*  next function               */
                }
                
                if (pfuncCall(iBus, iSlot, iFunc, pvArg) != ERROR_NONE) {
                    goto    __out;
                }
                
                if (iFunc == 0) {
                    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_HEADER_TYPE, &ucHeader);
                    
                    if ((ucHeader & PCI_HEADER_MULTI_FUNC) != PCI_HEADER_MULTI_FUNC) {
                        break;
                    }
                }
            }
        }
    }
    
__out:
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciTraversalDev
** 功能描述: pci 总线遍历上所有的设备
** 输　入  : iBusStart     起始总线号
**           bSubBus       是否遍历桥接总线
**           pfuncCall     遍历回调函数
**           pvArg         回调函数参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciTraversalDev (INT   iBusStart, 
                          BOOL  bSubBus,
                          INT (*pfuncCall)(), 
                          PVOID pvArg)
{
    INT         iBus;
    INT         iSlot;
    INT         iFunc;
    
    UINT16      usVendorId;
    UINT16      usSubClass;
    
    UINT8       ucHeader;
    UINT8       ucSecBus;
    
    INT         iRet;
    
    if (!pfuncCall) {
        return  (PX_ERROR);
    }
    
    iBus = iBusStart;
    
    for (iSlot = 0; iSlot < PCI_MAX_SLOTS; iSlot++) {
        for (iFunc = 0; iFunc < PCI_MAX_FUNCTIONS; iFunc++) {
            
            API_PciConfigInWord(iBus, iSlot, iFunc, PCI_VENDOR_ID, &usVendorId);
            
            if (PCI_VENDOR_ID_IS_INVALIDATE(usVendorId)) {
                if (iFunc == 0) {
                    break;                                              /*  next slot                   */
                }
                continue;                                               /*  next function               */
            }
            
            API_PciConfigInWord(iBus, iSlot, iFunc, PCI_CLASS_DEVICE, &usSubClass);
            
            if ((usSubClass & 0xff00) != (PCI_BASE_CLASS_BRIDGE << 8)) {
                iRet = pfuncCall(iBus, iSlot, iFunc, pvArg);
                if (iRet != ERROR_NONE) {
                    return  (iRet);
                }
            } 
            
            if (bSubBus) {
                if ((usSubClass == PCI_CLASS_BRIDGE_PCI) ||
                    (usSubClass == PCI_CLASS_BRIDGE_CARDBUS)) {         /*  PCI to PCI or PCI to cardbus*/
                    
                    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_SECONDARY_BUS, &ucSecBus);
                    
                    if (ucSecBus > 0) {
                        iRet = API_PciTraversalDev(ucSecBus, bSubBus,
                                                   pfuncCall, pvArg);
                    
                    } else {
                        iRet = ERROR_NONE;
                    }
                    
                    if (iRet != ERROR_NONE) {
                        return  (iRet);
                    }
                }
            }
            
            if (iFunc == 0) {
                API_PciConfigInByte(iBus, iSlot, iFunc, PCI_HEADER_TYPE, &ucHeader);
                
                if ((ucHeader & PCI_HEADER_MULTI_FUNC) != PCI_HEADER_MULTI_FUNC) {
                    break;
                }
            }
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciConfigDev
** 功能描述: 配置 pci 总线上的一个设备
**           首先将清除设备命令, 然后设置 I/O 或者 内存基址, 然后设置高速缓冲器与延迟寄存器,
**           最后将新的指令写入指令寄存器.
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           ulIoBase  IO 基地址
**           ulMemBase 内存基地址
**           ucLatency 延迟时间 (PCI clocks max 255) 
**           uiCommand 命令
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciConfigDev (INT          iBus, 
                       INT          iSlot, 
                       INT          iFunc, 
                       ULONG        ulIoBase, 
                       pci_addr_t   ulMemBase, 
                       UINT8        ucLatency,
                       UINT32       uiCommand)
{
    INT         iBar;
    INT         iFoundMem64;
    pci_addr_t  ulBarValue;
    pci_size_t  ulBarSize;
    UINT32      uiBarResponse;
    UINT8       ucCacheLine;
    UINT8       ucPin;
    UINT32      uiOldCommand;

    API_PciConfigOutDword(iBus, iSlot, iFunc, PCI_COMMAND, 0x0);        /*  清除设备当前命令            */
    
    for (iBar = PCI_BASE_ADDRESS_0; iBar <= PCI_BASE_ADDRESS_5; iBar += 4) {
    
        API_PciConfigOutDword(iBus, iSlot, iFunc, iBar, 0xffffffff);
        API_PciConfigInDword(iBus, iSlot, iFunc, iBar, &uiBarResponse);
        
        if (uiBarResponse == 0) {
            continue;
        }
        
        iFoundMem64 = 0;
        
        if (uiBarResponse & PCI_BASE_ADDRESS_SPACE) {
            ulBarSize  = ~(uiBarResponse & PCI_BASE_ADDRESS_IO_MASK) + 1;
            ulIoBase   = ((ulIoBase - 1) | (ulBarSize - 1)) + 1;
            ulBarValue = ulIoBase;
            ulIoBase   = ulIoBase + ulBarSize;
            
        } else {
            if ((uiBarResponse & PCI_BASE_ADDRESS_MEM_TYPE_MASK) ==
				 PCI_BASE_ADDRESS_MEM_TYPE_64) {
                UINT32  uiBarResponseUpper;
                UINT64  ullBar64;
                
                API_PciConfigOutDword(iBus, iSlot, iFunc, iBar + 4, 0xffffffff);
				API_PciConfigInDword(iBus, iSlot, iFunc, iBar + 4, &uiBarResponseUpper);
				
				ullBar64  = ((UINT64)uiBarResponseUpper << 32) | uiBarResponse;
				ulBarSize = ~(ullBar64 & PCI_BASE_ADDRESS_MEM_MASK) + 1;
				
                iFoundMem64 = 1;
            
            } else {
                ulBarSize = (UINT32)(~(uiBarResponse & PCI_BASE_ADDRESS_MEM_MASK) + 1);
            }
            
            ulMemBase  = ((ulMemBase - 1) | (ulBarSize - 1)) + 1;
            ulBarValue = ulMemBase;
            ulMemBase  = ulMemBase + ulBarSize;
        }

        API_PciConfigOutDword(iBus, iSlot, iFunc, iBar, (UINT32)ulBarValue);
        
        if (iFoundMem64) {
            iBar += 4;
#if LW_CFG_PCI_64 > 0
            API_PciConfigOutDword(iBus, iSlot, iFunc, iBar, (UINT32)(ulBarValue >> 32));
#else
            API_PciConfigOutDword(iBus, iSlot, iFunc, iBar, 0x00000000);
#endif
        }
    }
    
#if LW_CFG_CACHE_EN > 0
    ucCacheLine = (API_CacheLine(DATA_CACHE) >> 2);
#else
    ucCacheLine = (32 >> 2);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    
    API_PciConfigOutByte(iBus, iSlot, iFunc, PCI_CACHE_LINE_SIZE, ucCacheLine);
    API_PciConfigOutByte(iBus, iSlot, iFunc, PCI_LATENCY_TIMER, ucLatency);
    
    /* 
     * Disable interrupt line, if device says it wants to use interrupts
     */
    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_INTERRUPT_PIN, &ucPin);
    if (ucPin != 0) {
        API_PciConfigOutByte(iBus, iSlot, iFunc, PCI_INTERRUPT_LINE, 0xff);
    }
    
    API_PciConfigInDword(iBus, iSlot, iFunc, PCI_COMMAND, &uiOldCommand);
    API_PciConfigOutDword(iBus, iSlot, iFunc, PCI_COMMAND, (uiOldCommand & 0xffff0000) | uiCommand);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciFuncDisable
** 功能描述: 停止 pci 设备功能
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciFuncDisable (INT iBus, INT iSlot, INT iFunc)
{
    return  (API_PciConfigModifyDword(iBus, iSlot, iFunc, PCI_COMMAND, 
                                      PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER, 0));
}
/*********************************************************************************************************
** 函数名称: API_PciInterConnect
** 功能描述: 设置 pci 中断连接
** 输　入  : ulVector  CPU 中断向量 (pci 多块板卡可能会共享 CPU 某一中断向量)
**           pfuncIsr  中断服务函数
**           pvArg     中断服务函数参数
**           pcName    中断服务名称
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciInterConnect (ULONG ulVector, PINT_SVR_ROUTINE pfuncIsr, PVOID pvArg, CPCHAR pcName)
{
    ULONG   ulFlag = 0ul;

    API_InterVectorGetFlag(ulVector, &ulFlag);
    if (!(ulFlag & LW_IRQ_FLAG_QUEUE)) {
        ulFlag |= LW_IRQ_FLAG_QUEUE;
        API_InterVectorSetFlag(ulVector, ulFlag);                       /*  单向量多级中断              */
    }
    
    if (API_InterVectorConnect(ulVector, pfuncIsr, pvArg, pcName) == ERROR_NONE) {
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_PciInterDisonnect
** 功能描述: 设置 pci 解除中断连接
** 输　入  : ulVector  CPU 中断向量 (pci 多块板卡可能会共享 CPU 某一中断向量)
**           pfuncIsr  中断服务函数
**           pvArg     中断服务函数参数
**           pcName    中断服务名称
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciInterDisconnect (ULONG ulVector, PINT_SVR_ROUTINE pfuncIsr, PVOID pvArg)
{
    if (API_InterVectorDisconnect(ulVector, pfuncIsr, pvArg) == ERROR_NONE) {
        return  (ERROR_NONE);
    
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: API_PciGetHader
** 功能描述: 获得指定设备的 pci 头信息
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           p_pcihdr  头信息
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API 
INT  API_PciGetHeader (INT iBus, INT iSlot, INT iFunc, PCI_HDR *p_pcihdr)
{
#define PCI_D  p_pcihdr->PCIH_pcidHdr
#define PCI_B  p_pcihdr->PCIH_pcibHdr
#define PCI_CB p_pcihdr->PCIH_pcicbHdr

    if (p_pcihdr == LW_NULL) {
        return  (PX_ERROR);
    }

    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_HEADER_TYPE, &p_pcihdr->PCIH_ucType);
    p_pcihdr->PCIH_ucType &= PCI_HEADER_TYPE_MASK;

    if (p_pcihdr->PCIH_ucType == PCI_HEADER_TYPE_NORMAL) {              /* PCI iSlot                   */
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_VENDOR_ID, &PCI_D.PCID_usVendorId);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_DEVICE_ID, &PCI_D.PCID_usDeviceId);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_COMMAND,   &PCI_D.PCID_usCommand);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_STATUS,    &PCI_D.PCID_usStatus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS_REVISION, &PCI_D.PCID_ucRevisionId);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS_PROG,     &PCI_D.PCID_ucProgIf);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS_DEVICE,   &PCI_D.PCID_ucSubClass);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS,          &PCI_D.PCID_ucClassCode);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CACHE_LINE_SIZE,&PCI_D.PCID_ucCacheLine);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_LATENCY_TIMER,  &PCI_D.PCID_ucLatency);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_HEADER_TYPE,    &PCI_D.PCID_ucHeaderType);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_BIST,           &PCI_D.PCID_ucBist);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_0, &PCI_D.PCID_uiBase0);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_1, &PCI_D.PCID_uiBase1);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_2, &PCI_D.PCID_uiBase2);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_3, &PCI_D.PCID_uiBase3);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_4, &PCI_D.PCID_uiBase4);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_5, &PCI_D.PCID_uiBase5);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CARDBUS_CIS,    &PCI_D.PCID_uiCis);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_SUBSYSTEM_VENDOR_ID, &PCI_D.PCID_usSubVendorId);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_SUBSYSTEM_ID,        &PCI_D.PCID_usSubSystemId);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_ROM_ADDRESS,   &PCI_D.PCID_uiRomBase);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_INTERRUPT_LINE,&PCI_D.PCID_ucIntLine);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_INTERRUPT_PIN, &PCI_D.PCID_ucIntPin);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_MIN_GNT,       &PCI_D.PCID_ucMinGrant);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_MAX_LAT,       &PCI_D.PCID_ucMaxLatency);

    } else if (p_pcihdr->PCIH_ucType == PCI_HEADER_TYPE_BRIDGE) {       /* PCI to PCI bridge            */
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_VENDOR_ID, &PCI_B.PCIB_usVendorId);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_DEVICE_ID, &PCI_B.PCIB_usDeviceId);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_COMMAND,   &PCI_B.PCIB_usCommand);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_STATUS,    &PCI_B.PCIB_usStatus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS_REVISION, &PCI_B.PCIB_ucRevisionId);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS_PROG,     &PCI_B.PCIB_ucProgIf);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS_DEVICE,   &PCI_B.PCIB_ucSubClass);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS,          &PCI_B.PCIB_ucClassCode);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CACHE_LINE_SIZE,&PCI_B.PCIB_ucCacheLine);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_LATENCY_TIMER,  &PCI_B.PCIB_ucLatency);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_HEADER_TYPE,    &PCI_B.PCIB_ucHeaderType);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_BIST,           &PCI_B.PCIB_ucBist);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_0, &PCI_B.PCIB_uiBase0);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_1, &PCI_B.PCIB_uiBase1);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_PRIMARY_BUS,    &PCI_B.PCIB_ucPriBus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_SECONDARY_BUS,  &PCI_B.PCIB_ucSecBus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_SUBORDINATE_BUS,&PCI_B.PCIB_ucSubBus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_SEC_LATENCY_TIMER, &PCI_B.PCIB_ucSecLatency);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_IO_BASE,           &PCI_B.PCIB_ucIoBase);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_IO_LIMIT,          &PCI_B.PCIB_ucIoLimit);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_SEC_STATUS,        &PCI_B.PCIB_usSecStatus);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_MEMORY_BASE,       &PCI_B.PCIB_usMemBase);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_MEMORY_LIMIT,      &PCI_B.PCIB_usMemLimit);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_PREF_MEMORY_BASE,  &PCI_B.PCIB_usPreBase);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_PREF_MEMORY_LIMIT, &PCI_B.PCIB_usPreLimit);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_PREF_BASE_UPPER32, &PCI_B.PCIB_uiPreBaseUpper);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_PREF_LIMIT_UPPER32, &PCI_B.PCIB_uiPreLimitUpper);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_IO_BASE_UPPER16,    &PCI_B.PCIB_usIoBaseUpper);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_IO_LIMIT_UPPER16, &PCI_B.PCIB_usIoLimitUpper);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_ROM_ADDRESS1,     &PCI_B.PCIB_uiRomBase);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_INTERRUPT_LINE,   &PCI_B.PCIB_ucIntLine);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_INTERRUPT_PIN,    &PCI_B.PCIB_ucIntPin);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_BRIDGE_CONTROL,   &PCI_B.PCIB_usControl);

    } else if (p_pcihdr->PCIH_ucType == PCI_HEADER_TYPE_CARDBUS) {      /* PCI card iBus bridge          */
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_VENDOR_ID, &PCI_CB.PCICB_usVendorId);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_DEVICE_ID, &PCI_CB.PCICB_usDeviceId);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_COMMAND,   &PCI_CB.PCICB_usCommand);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_STATUS,    &PCI_CB.PCICB_usStatus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS_REVISION, &PCI_CB.PCICB_ucRevisionId);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS_PROG,     &PCI_CB.PCICB_ucProgIf);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS_DEVICE,   &PCI_CB.PCICB_ucSubClass);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CLASS,          &PCI_CB.PCICB_ucClassCode);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CACHE_LINE_SIZE, &PCI_CB.PCICB_ucCacheLine);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_LATENCY_TIMER,   &PCI_CB.PCICB_ucLatency);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_HEADER_TYPE,     &PCI_CB.PCICB_ucHeaderType);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_BIST,            &PCI_CB.PCICB_ucBist);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_BASE_ADDRESS_0,  &PCI_CB.PCICB_uiBase0);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CAPABILITY_LIST,      &PCI_CB.PCICB_ucCapPtr);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_CB_SEC_STATUS,   &PCI_CB.PCICB_usSecStatus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CB_PRIMARY_BUS,  &PCI_CB.PCICB_ucPriBus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CB_CARD_BUS,     &PCI_CB.PCICB_ucSecBus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CB_SUBORDINATE_BUS, &PCI_CB.PCICB_ucSubBus);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_CB_LATENCY_TIMER,   &PCI_CB.PCICB_ucSecLatency);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_MEMORY_BASE_0,   &PCI_CB.PCICB_uiMemBase0);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_MEMORY_LIMIT_0,  &PCI_CB.PCICB_uiMemLimit0);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_MEMORY_BASE_1,   &PCI_CB.PCICB_uiMemBase1);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_MEMORY_LIMIT_1,  &PCI_CB.PCICB_uiMemLimit1);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_IO_BASE_0,       &PCI_CB.PCICB_uiIoBase0);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_IO_LIMIT_0,      &PCI_CB.PCICB_uiIoLimit0);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_IO_BASE_1,       &PCI_CB.PCICB_uiIoBase1);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_IO_LIMIT_1,      &PCI_CB.PCICB_uiIoLimit1);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_INTERRUPT_LINE,     &PCI_CB.PCICB_ucIntLine);
        API_PciConfigInByte( iBus, iSlot, iFunc, PCI_INTERRUPT_PIN,      &PCI_CB.PCICB_ucIntPin);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_BRIDGE_CONTROL,     &PCI_CB.PCICB_usControl);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_CB_SUBSYSTEM_VENDOR_ID, &PCI_CB.PCICB_usSubVendorId);
        API_PciConfigInWord( iBus, iSlot, iFunc, PCI_CB_SUBSYSTEM_ID,        &PCI_CB.PCICB_usSubSystemId);
        API_PciConfigInDword(iBus, iSlot, iFunc, PCI_CB_LEGACY_MODE_BASE,    &PCI_CB.PCICB_uiLegacyBase);
    
    } else {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciHeaderTypeGet
** 功能描述: 获取设备头类型.
** 输　入  : iBus        总线号
**           iSlot       插槽
**           iFunc       功能
**           ucType      设备类型(PCI_HEADER_TYPE_NORMAL, PCI_HEADER_TYPE_BRIDGE, PCI_HEADER_TYPE_CARDBUS)
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciHeaderTypeGet (INT iBus, INT iSlot, INT iFunc, UINT8 *ucType)
{
    UINT8       ucHeaderType = 0xFF;

    API_PciConfigInByte(iBus, iSlot, iFunc, PCI_HEADER_TYPE, (UINT8 *)&ucHeaderType);
    if ((ucHeaderType & PCI_HEADER_TYPE_MASK) == PCI_HEADER_TYPE_BRIDGE) {
        ucHeaderType = PCI_HEADER_TYPE_BRIDGE;
    } else if ((ucHeaderType & PCI_HEADER_TYPE_MASK) == PCI_HEADER_TYPE_CARDBUS) {
        ucHeaderType = PCI_HEADER_TYPE_CARDBUS;
    } else if ((ucHeaderType & PCI_HEADER_TYPE_MASK) == PCI_HEADER_TYPE_NORMAL) {
        ucHeaderType = PCI_HEADER_TYPE_NORMAL;
    } else {
        return  (PX_ERROR);
    }

    if (ucType) {
        *ucType = ucHeaderType;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciVpdRead
** 功能描述: 读取 VPD (Vital Product Data) 数据
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           iPos      数据地址
**           pucBuf    结果缓冲区
**           iLen      结果缓冲区大小
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciVpdRead (INT iBus, INT iSlot, INT iFunc, INT iPos, UINT8 *pucBuf, INT iLen)
{
    INTREG  iregInterLevel;
    INT     iRetVal = PX_ERROR;

    iregInterLevel = KN_INT_DISABLE();

    switch (_G_p_pciConfig->PCIC_ucMechanism) {

    case PCI_MECHANISM_0:
        iRetVal = PCI_VPD_READ(iBus, iSlot, iFunc, iPos, pucBuf, iLen);
        break;

    case PCI_MECHANISM_1:
        iRetVal = PX_ERROR;
        break;

    case PCI_MECHANISM_2:
        iRetVal = PX_ERROR;
        break;

    default:
        break;
    }

    KN_INT_ENABLE(iregInterLevel);

    return  (iRetVal);
}
/*********************************************************************************************************
** 函数名称: API_PciIrqGet
** 功能描述: 获取总的向量
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           iMsiEn         是否使能 MSI
**           iLine          中断线
**           iPin           中断引脚
**           pulVector      中断向量
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciIrqGet (INT iBus, INT iSlot, INT iFunc, INT iMsiEn, INT iLine, INT iPin, ULONG *pulVector)
{
    INTREG  iregInterLevel;
    INT     iRetVal = PX_ERROR;

    iregInterLevel = KN_INT_DISABLE();

    switch (_G_p_pciConfig->PCIC_ucMechanism) {

    case PCI_MECHANISM_0:
        iRetVal = PCI_IRQ_GET(iBus, iSlot, iFunc, iMsiEn, iLine, iPin, pulVector);
        break;

    case PCI_MECHANISM_1:
        iRetVal = PX_ERROR;
        break;

    case PCI_MECHANISM_2:
        iRetVal = PX_ERROR;
        break;

    default:
        break;
    }

    KN_INT_ENABLE(iregInterLevel);

    return  (iRetVal);
}
/*********************************************************************************************************
** 函数名称: API_PciConfigFetch
** 功能描述: 读取 VPD (Vital Product Data) 数据
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           iPos      数据地址
**           pucBuf    结果缓冲区
**           iLen      结果缓冲区大小
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciConfigFetch (INT iBus, INT iSlot, INT iFunc, UINT uiPos, UINT uiLen)
{
    /*
     *  TODO
     */

    return  (1);
}
/*********************************************************************************************************
** 函数名称: API_PciConfigHandleGet
** 功能描述: 获取指定 PCI 配置句柄
** 输　入  : iIndex    PCI 配置 (PCI_CONFIG) 索引
** 输　出  : LW_NULL or 句柄
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PCI_CONFIG  *API_PciConfigHandleGet (INT iIndex)
{
    if (_G_p_pciConfig == LW_NULL) {
        return  (LW_NULL);
    }

    if (_G_p_pciConfig->PCIC_iIndex == iIndex) {
        return (_G_p_pciConfig);
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_PciConfigIndexGet
** 功能描述: 通过 PCI 配置句柄获取索引
** 输　入  : ppcHandle      PCI 配置 (PCI_CONFIG) 句柄
** 输　出  : ERROR or 索引
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciConfigIndexGet (PCI_CONFIG *ppcHandle)
{
    if (!ppcHandle) {
        return  (PX_ERROR);
    }

    return  (ppcHandle->PCIC_iIndex);
}
/*********************************************************************************************************
** 函数名称: __procFsPciGetCnt
** 功能描述: 获得 PCI 设备总数
** 输　入  : iBus          总线号
**           iSlot         插槽
**           iFunc         功能号
**           pstBufferSize 缓冲区大小
** 输　出  : ERROR_NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciBusCntGet (INT iBus, INT iSlot, INT iFunc, UINT *puiCount)
{
    (VOID)iBus;
    (VOID)iSlot;
    (VOID)iFunc;

    if (puiCount) {
        *puiCount += 1;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciConfigBusMaxSet
** 功能描述: 设置指定 PCI 最大总线数
** 输　入  : iIndex    PCI 配置 (PCI_CONFIG) 索引
** 输　出  : ERROR or 最大总线数
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciConfigBusMaxSet (INT  iIndex, UINT32  uiBusMax)
{
    if (_G_p_pciConfig == LW_NULL) {
        return  (PX_ERROR);
    }

    if (_G_p_pciConfig->PCIC_iIndex == iIndex) {
        _G_p_pciConfig->PCIC_iBusMax = uiBusMax;
        return (PX_ERROR);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_PciConfigBusMaxGet
** 功能描述: 获取指定 PCI 最大总线数
** 输　入  : iIndex    PCI 配置 (PCI_CONFIG) 索引
** 输　出  : ERROR or 最大总线数
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciConfigBusMaxGet (INT iIndex)
{
    UINT        uiBusNumber = 0;

    if (_G_p_pciConfig == LW_NULL) {
        return  (PX_ERROR);
    }

    if (_G_p_pciConfig->PCIC_iIndex == iIndex) {
        API_PciLock();
        API_PciTraversal(__pciBusCntGet, &uiBusNumber, PCI_MAX_BUS - 1);
        API_PciUnlock();

        _G_p_pciConfig->PCIC_iBusMax = uiBusNumber;
        return (_G_p_pciConfig->PCIC_iBusMax);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_PciIntxEnableSet
** 功能描述: 设置 INTx 使能与禁能
** 输　入  : iBus      总线号
**           iSlot     插槽
**           iFunc     功能
**           iEnable   使能标志
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciIntxEnableSet (INT iBus, INT iSlot, INT iFunc, INT iEnable)
{
    INT         iRet = PX_ERROR;
    UINT16      usCommand, usNew;

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, PCI_COMMAND, &usCommand);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (iEnable) {
        usNew = usCommand & ~PCI_COMMAND_INTX_DISABLE;
    } else {
        usNew = usCommand | PCI_COMMAND_INTX_DISABLE;
    }

    if (usNew != usCommand) {
        iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, PCI_COMMAND, usNew);
    } else {
        iRet = ERROR_NONE;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PciIntxMaskSupported
** 功能描述: 探测 INTx 是否支持掩码
** 输　入  : iBus               总线号
**           iSlot              插槽
**           iFunc              功能
**           piSupported        是否支持
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciIntxMaskSupported (INT iBus, INT iSlot, INT iFunc, INT *piSupported)
{
    INT         iMaskSupported = LW_FALSE;
    INT         iRet           = PX_ERROR;
    UINT16      usOrig         = 0;
    UINT16      usNew          = 0;

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, PCI_COMMAND, &usOrig);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, PCI_COMMAND, usOrig ^ PCI_COMMAND_INTX_DISABLE);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, PCI_COMMAND, &usNew);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if ((usNew ^ usOrig) & ~PCI_COMMAND_INTX_DISABLE) {
        return  (PX_ERROR);
    } else if ((usNew ^ usOrig) & PCI_COMMAND_INTX_DISABLE) {
        iMaskSupported = LW_TRUE;
        iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, PCI_COMMAND, usOrig);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    if (piSupported) {
        *piSupported = iMaskSupported;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevMatchDrv
** 功能描述: PCI 设备驱动匹配
** 输　入  : hDevHandle   设备句柄
**           hDrvHandle   驱动句柄
** 输　出  : 成功: 返回匹配的一个 PCI ID 信息, 失败: 返回 LW_NULL
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PCI_DEVICE_ID_HANDLE API_PciDevMatchDrv (PCI_DEV_HANDLE hDevHandle, PCI_DRV_HANDLE hDrvHandle)
{
    PCI_DEVICE_ID_HANDLE  hId;
    INT                   i;

    if (!hDevHandle || !hDrvHandle) {
        return  (LW_NULL);
    }

    if (hDevHandle->PDT_phDevHdr.PCIH_ucType != PCI_HEADER_TYPE_NORMAL) {
        return  (LW_NULL);
    }

    for (hId = hDrvHandle->PDT_hDrvIdTable, i = 0; i < hDrvHandle->PDT_uiDrvIdTableSize; i++) {
        if (__pciDevMatchId(hDevHandle, hId)) {
            return  (hId);
        }
        hId++;
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_PciDevMatchDrv
** 功能描述: PCI 设备驱动匹配
** 输　入  : hDevHandle   设备句柄
**           hId          一个 ID 对象
** 输　出  : 成功: 返回该 ID对象, 失败: 返回 LW_NULL
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
static PCI_DEVICE_ID_HANDLE __pciDevMatchId (PCI_DEV_HANDLE hDevHandle, PCI_DEVICE_ID_HANDLE hId)
{
    PCI_DEV_HDR    *phdr       = &hDevHandle->PDT_phDevHdr.PCIH_pcidHdr;
    UINT32          uiDevClass = (UINT32)((phdr->PCID_ucClassCode << 16) +
                                          (phdr->PCID_ucSubClass  <<  8) +
                                          phdr->PCID_ucProgIf);

    if (((hId->PDIT_uiVendor    == PCI_ANY_ID) || (hId->PDIT_uiVendor    == phdr->PCID_usVendorId))    &&
        ((hId->PDIT_uiDevice    == PCI_ANY_ID) || (hId->PDIT_uiDevice    == phdr->PCID_usDeviceId))    &&
        ((hId->PDIT_uiSubVendor == PCI_ANY_ID) || (hId->PDIT_uiSubVendor == phdr->PCID_usSubVendorId)) &&
        ((hId->PDIT_uiSubDevice == PCI_ANY_ID) || (hId->PDIT_uiSubDevice == phdr->PCID_usSubSystemId)) &&
        !((hId->PDIT_uiClass ^ uiDevClass) & hId->PDIT_uiClassMask)) {
        return  (hId);
    }

    return  (LW_NULL);
}
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
