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
** 文   件   名: pciMsi.h
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2015 年 09 月 10 日
**
** 描        述: PCI 总线 MSI(Message Signaled Interrupts) 管理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
#include "linux/bitops.h"
#include "linux/log2.h"
#include "pciMsi.h"
/*********************************************************************************************************
** 函数名称: API_PciMsixClearSet
** 功能描述: MSIx 控制域清除与设置
** 输　入  : iBus               总线号
**           iSlot              插槽
**           iFunc              功能
**           iMsixCapOft        偏移地址
**           usClear            清除标志
**           usSet              设置标志
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsixClearSet (INT     iBus,
                          INT     iSlot,
                          INT     iFunc,
                          INT     iMsixCapOft,
                          UINT16  usClear,
                          UINT16  usSet)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    UINT16      usNew     = 0;

    if (iMsixCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsixCapOft + PCI_MSIX_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    usNew  = usControl & (~usClear);
    usNew |= usSet;
    if (usNew != usControl) {
        iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, iMsixCapOft + PCI_MSIX_FLAGS, usNew);
    } else {
        iRet = ERROR_NONE;
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiMsgWrite
** 功能描述: 写 MSI 消息
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           iMsiCapOft     偏移地址
**           ucMultiple     倍数
**           ppmmMsg        消息内容
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiMsgWrite (INT          iBus,
                         INT          iSlot,
                         INT          iFunc,
                         INT          iMsiCapOft,
                         UINT8        ucMultiple,
                         PCI_MSI_MSG *ppmmMsg)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    if (!ppmmMsg) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    usControl &= ~PCI_MSI_FLAGS_QSIZE;
    usControl |= ucMultiple << 4;
    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigOutDword(iBus, iSlot, iFunc,
                                 iMsiCapOft + PCI_MSI_ADDRESS_LO, ppmmMsg->uiAddressLo);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iRet = API_PciConfigOutDword(iBus, iSlot, iFunc,
                                     iMsiCapOft + PCI_MSI_ADDRESS_HI, ppmmMsg->uiAddressHi);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        iRet = API_PciConfigOutWord(iBus, iSlot, iFunc,
                                    iMsiCapOft + PCI_MSI_DATA_64, (UINT16)ppmmMsg->uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    } else {
        iRet = API_PciConfigOutWord(iBus, iSlot, iFunc,
                                    iMsiCapOft + PCI_MSI_DATA_32, (UINT16)ppmmMsg->uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiMsgRead
** 功能描述: 读 MSI 消息
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           iMsiCapOft     偏移地址
**           ucMultiple     倍数
**           ppmmMsg        消息内容
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiMsgRead (INT          iBus,
                        INT          iSlot,
                        INT          iFunc,
                        INT          iMsiCapOft,
                        UINT8        ucMultiple,
                        PCI_MSI_MSG *ppmmMsg)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    if (!ppmmMsg) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInDword(iBus, iSlot, iFunc,
                                iMsiCapOft + PCI_MSI_ADDRESS_LO, &ppmmMsg->uiAddressLo);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iRet = API_PciConfigInDword(iBus, iSlot, iFunc,
                                    iMsiCapOft + PCI_MSI_ADDRESS_HI, &ppmmMsg->uiAddressHi);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        iRet = API_PciConfigInWord(iBus, iSlot, iFunc,
                                   iMsiCapOft + PCI_MSI_DATA_64, (UINT16 *)&ppmmMsg->uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    } else {
        ppmmMsg->uiAddressHi = 0;
        iRet = API_PciConfigInWord(iBus, iSlot, iFunc,
                                   iMsiCapOft + PCI_MSI_DATA_32, (UINT16 *)&ppmmMsg->uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiPendingSet
** 功能描述: MSI Pending 设置
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           iMsiCapOft     偏移地址
**           uiPending      指定位
**           uiFlag         新标志
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiPendingSet (INT     iBus,
                           INT     iSlot,
                           INT     iFunc,
                           INT     iMsiCapOft,
                           UINT32  uiPending,
                           UINT32  uiFlag)
{
    INT         iRet        = PX_ERROR;
    UINT16      usControl   = 0;
    INT         iIs64       = 0;
    INT         iIsMask     = 0;
    INT         iPendingPos = 0;
    UINT32      uiPendinged = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASKBIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iPendingPos = iMsiCapOft + PCI_MSI_PENDING_64;
    } else {
        iPendingPos = iMsiCapOft + PCI_MSI_PENDING_32;
    }
    uiPendinged &= ~uiPending;
    uiPendinged |=  uiFlag;
    iRet = API_PciConfigOutDword(iBus, iSlot, iFunc, iPendingPos, uiPendinged);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiPendingGet
** 功能描述: 获取 MSI Pending 获取
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           iMsiCapOft     偏移地址
**           puiPending     获取到的状态
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiPendingGet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, UINT32 *puiPending)
{
    INT         iRet        = PX_ERROR;
    UINT16      usControl   = 0;
    INT         iEnable     = 0;
    INT         iIs64       = 0;
    INT         iIsMask     = 0;
    INT         iPendingPos = 0;
    UINT32      uiPendinged = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASKBIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iPendingPos = iMsiCapOft + PCI_MSI_PENDING_64;
    } else {
        iPendingPos = iMsiCapOft + PCI_MSI_PENDING_32;
    }
    iRet = API_PciConfigInDword(iBus, iSlot, iFunc, iPendingPos, &uiPendinged);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (puiPending) {
        *puiPending = uiPendinged;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiPendingPosGet
** 功能描述: 获取 MSI Pending 位置
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           iMsiCapOft     偏移地址
**           piPendingPos   标记位置
**                          0  掩码标记无效
**                          1  掩码标记有效
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiPendingPosGet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, INT *piPendingPos)
{
    INT         iRet        = PX_ERROR;
    UINT16      usControl   = 0;
    INT         iEnable     = 0;
    INT         iIs64       = 0;
    INT         iIsMask     = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASKBIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    if (piPendingPos) {
        iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
        if (iIs64) {
            *piPendingPos = iMsiCapOft + PCI_MSI_PENDING_64;
        } else {
            *piPendingPos = iMsiCapOft + PCI_MSI_PENDING_32;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiMaskSet
** 功能描述: MSI 掩码设置
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           iMsiCapOft     偏移地址
**           uiMask         掩码
**           uiFlag         新标志
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiMaskSet (INT     iBus,
                        INT     iSlot,
                        INT     iFunc,
                        INT     iMsiCapOft,
                        UINT32  uiMask,
                        UINT32  uiFlag)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;
    INT         iIsMask   = 0;
    INT         iMaskPos  = 0;
    UINT32      uiMasked  = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASKBIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iMaskPos = iMsiCapOft + PCI_MSI_MASK_64;
    } else {
        iMaskPos = iMsiCapOft + PCI_MSI_MASK_32;
    }
    uiMasked &= ~uiMask;
    uiMasked |= uiFlag;
    iRet = API_PciConfigOutDword(iBus, iSlot, iFunc, iMaskPos, uiMasked);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiMaskGet
** 功能描述: 获取 MSI 掩码
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           iMsiCapOft     偏移地址
**           puiMask        掩码
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiMaskGet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, UINT32 *puiMask)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;
    INT         iIsMask   = 0;
    INT         iMaskPos  = 0;
    UINT32      uiMasked  = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASKBIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
    if (iIs64) {
        iMaskPos = iMsiCapOft + PCI_MSI_MASK_64;
    } else {
        iMaskPos = iMsiCapOft + PCI_MSI_MASK_32;
    }
    iRet = API_PciConfigInDword(iBus, iSlot, iFunc, iMaskPos, &uiMasked);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (puiMask) {
        *puiMask = uiMasked;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiMaskPosGet
** 功能描述: 获取 MSI 掩码位置
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           iMsiCapOft     偏移地址
**           piMaskPos      标记位置
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiMaskPosGet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, INT *piMaskPos)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;
    INT         iEnable   = 0;
    INT         iIs64     = 0;
    INT         iIsMask   = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    iEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    if (!iEnable) {
        return  (PX_ERROR);
    }
    iIsMask = !!(usControl & PCI_MSI_FLAGS_MASKBIT);
    if (!iIsMask) {
        return  (PX_ERROR);
    }
    if (piMaskPos) {
        iIs64 = !!(usControl & PCI_MSI_FLAGS_64BIT);
        if (iIs64) {
            *piMaskPos = iMsiCapOft + PCI_MSI_MASK_64;
        } else {
            *piMaskPos = iMsiCapOft + PCI_MSI_MASK_32;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiMaskConvert
** 功能描述: MSI 掩码转换
** 输　入  : uiMask     掩码
** 输　出  : 转换后的掩码
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
UINT32  API_PciMsiMaskConvert (UINT32  uiMask)
{
    if (uiMask >= 5) {
        return  (0xffffffff);
    }

    return  ((1 << (1 << uiMask)) - 1);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiMultipleGet
** 功能描述: 获取 MSI 消息数量
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           iMsiCapOft     偏移地址
**           piMultiCap     消息消息数量
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiMultipleGet (INT  iBus,
                            INT  iSlot,
                            INT  iFunc,
                            INT  iMsiCapOft,
                            INT  iNvec,
                            INT *piMultiple)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (piMultiple) {
        *piMultiple = ilog2(__roundup_pow_of_two(iNvec));
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiVecCountGet
** 功能描述: 获取 MSI 向量数
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           iMsiCapOft     偏移地址
**           piVecCount     向量数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiVecCountGet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, INT *piVecCount)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (piVecCount) {
        *piVecCount = 1 << ((usControl & PCI_MSI_FLAGS_QMASK) >> 1);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiMultiCapGet
** 功能描述: 获取 MSI 消息数量
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           iMsiCapOft     偏移地址
**           piMultiCap     消息消息数量
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiMultiCapGet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, INT *piMultiCap)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (piMultiCap) {
        *piMultiCap = (usControl & PCI_MSI_FLAGS_QMASK) >> 1;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsi64BitGet
** 功能描述: 获取 MSI 64 位地址
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           iMsiCapOft     偏移地址
**           pi64Bit        64 位地址标志
**                          0  64 位地址标记无效
**                          1  64 位地址标记有效
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsi64BitGet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, INT *pi64Bit)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (pi64Bit) {
        *pi64Bit = !!(usControl & PCI_MSI_FLAGS_64BIT);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiMaskBitGet
** 功能描述: 获取 MSI 掩码位
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           iMsiCapOft     偏移地址
**           piMaskBit      掩码标志
**                          0  掩码标记无效
**                          1  掩码标记有效
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiMaskBitGet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, INT *piMaskBit)
{
    INT         iRet      = PX_ERROR;
    UINT16      usControl = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }
    if (piMaskBit) {
        *piMaskBit = !!(usControl & PCI_MSI_FLAGS_MASKBIT);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiEnableSet
** 功能描述: MSI 使能控制
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           iMsiCapOft     偏移地址
**           iEnable        使能与禁能标志
**                          0  禁能
**                          1  使能
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiEnableSet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, INT  iEnable)
{
    INT         iRet         = PX_ERROR;
    UINT16      usControl    = 0;
    UINT16      usControlNew = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    usControlNew = usControl & (~(PCI_MSI_FLAGS_ENABLE));
    if (iEnable) {
        usControlNew |= PCI_MSI_FLAGS_ENABLE;
    }
    iRet = API_PciConfigOutWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, usControlNew);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PciMsiEnableGet
** 功能描述: 获取 MSI 使能控制状态
** 输　入  : iBus           总线号
**           iSlot          插槽
**           iFunc          功能
**           iMsiCapOft     偏移地址
**           piEnable       使能与禁能标志
**                          0  禁能
**                          1  使能
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciMsiEnableGet (INT  iBus, INT  iSlot, INT  iFunc, INT  iMsiCapOft, INT *piEnable)
{
    INT         iRet         = PX_ERROR;
    UINT16      usControl    = 0;

    if (iMsiCapOft <= 0) {
        return  (PX_ERROR);
    }

    iRet = API_PciConfigInWord(iBus, iSlot, iFunc, iMsiCapOft + PCI_MSI_FLAGS, &usControl);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (piEnable) {
        *piEnable = !!(usControl & PCI_MSI_FLAGS_ENABLE);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
