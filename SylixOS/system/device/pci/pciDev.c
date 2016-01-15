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
** 文   件   名: pciDev.c
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2015 年 12 月 23 日
**
** 描        述: PCI 总线设备管理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)

#include "endian.h"
#include "pciDev.h"
#include "pciDrv.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static UINT                     _GuiPciDevTotalNum  = 0;
static LW_OBJECT_HANDLE         _GulPciDevLock      = LW_OBJECT_HANDLE_INVALID;
static LW_LIST_LINE_HEADER      _GplinePciDevHeader = LW_NULL;

#define __PCI_DEV_LOCK()        API_SemaphoreMPend(_GulPciDevLock, LW_OPTION_WAIT_INFINITE)
#define __PCI_DEV_UNLOCK()      API_SemaphoreMPost(_GulPciDevLock)
/*********************************************************************************************************
** 函数名称: API_PciDevInterDisable
** 功能描述: 禁能 PCI 设备中断
** 输　入  : hHandle    设备句柄
**           ulVector   中断向量
**           pfuncIsr   中断服务函数
**           pvArg      中断服务函数参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevInterDisable (PCI_DEV_HANDLE   hHandle,
                             ULONG            ulVector,
                             PINT_SVR_ROUTINE pfuncIsr,
                             PVOID            pvArg)
{
    INT     iRet = PX_ERROR;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    if ((hHandle->PDT_ulDevIrqVector    != ulVector) ||
        (hHandle->PDT_pfuncDevIrqHandle != pfuncIsr) ||
        (hHandle->PDT_pvDevIrqArg       != pvArg   )) {
        return  (PX_ERROR);
    }

    iRet = API_InterVectorDisable(ulVector);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevInterEnable
** 功能描述: 使能 PCI 设备中断
** 输　入  : hHandle    设备句柄
**           ulVector   中断向量
**           pfuncIsr   中断服务函数
**           pvArg      中断服务函数参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevInterEnable (PCI_DEV_HANDLE   hHandle,
                            ULONG            ulVector,
                            PINT_SVR_ROUTINE pfuncIsr,
                            PVOID            pvArg)
{
    INT     iRet = PX_ERROR;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    if ((hHandle->PDT_ulDevIrqVector    != ulVector) ||
        (hHandle->PDT_pfuncDevIrqHandle != pfuncIsr) ||
        (hHandle->PDT_pvDevIrqArg       != pvArg   )) {
        return  (PX_ERROR);
    }

    iRet = API_InterVectorEnable(ulVector);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevInterDisonnect
** 功能描述: 设置 PCI 设备解除中断连接
** 输　入  : hHandle    设备句柄
**           ulVector   中断向量
**           pfuncIsr   中断服务函数
**           pvArg      中断服务函数参数
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevInterDisonnect (PCI_DEV_HANDLE    hHandle,
                               ULONG             ulVector,
                               PINT_SVR_ROUTINE  pfuncIsr,
                               PVOID             pvArg)
{
    INT     iRet = PX_ERROR;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    if ((hHandle->PDT_ulDevIrqVector    != ulVector) ||
        (hHandle->PDT_pfuncDevIrqHandle != pfuncIsr) ||
        (hHandle->PDT_pvDevIrqArg       != pvArg   )) {
        return  (PX_ERROR);
    }

    iRet = API_PciInterDisconnect(ulVector, pfuncIsr, pvArg);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    lib_bzero(hHandle->PDT_cDevIrqName, PCI_DEV_IRQ_NAME_MAX);
    hHandle->PDT_pfuncDevIrqHandle = LW_NULL;
    hHandle->PDT_pvDevIrqArg = LW_NULL;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevInterConnect
** 功能描述: 设置 PCI 设备中断连接
** 输　入  : hHandle    设备句柄
**           ulVector   中断向量
**           pfuncIsr   中断服务函数
**           pvArg      中断服务函数参数
**           pcName     中断服务名称
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevInterConnect (PCI_DEV_HANDLE    hHandle,
                             ULONG             ulVector,
                             PINT_SVR_ROUTINE  pfuncIsr,
                             PVOID             pvArg,
                             CPCHAR            pcName)
{
    INT     iRet = PX_ERROR;

    if ((hHandle  == LW_NULL) ||
        (pfuncIsr == LW_NULL)) {
        return  (PX_ERROR);
    }

    if (hHandle->PDT_ulDevIrqVector != ulVector) {
        return  (PX_ERROR);
    }

    iRet = API_PciInterConnect(ulVector, pfuncIsr, pvArg, pcName);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    lib_strncpy(hHandle->PDT_cDevIrqName, pcName, PCI_DEV_IRQ_NAME_MAX);
    hHandle->PDT_pfuncDevIrqHandle = pfuncIsr;
    hHandle->PDT_pvDevIrqArg       = pvArg;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevInterVectorGet
** 功能描述: 获取中断向量
** 输　入  : hHandle        设备句柄
**           pulVector      中断向量
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevInterVectorGet (PCI_DEV_HANDLE  hHandle, ULONG *pulVector)
{
    INT     iRet        = PX_ERROR;
    UINT8   ucMsiCapPos = 0x00;
    INT     iLine       = 0;
    INT     iPin        = 0;
    INT     iHdrType    = 0;

    if (hHandle == LW_NULL) {
        return  (PX_ERROR);
    }

    iHdrType = hHandle->PDT_phDevHdr.PCIH_ucType & PCI_HEADER_TYPE_MASK;
    switch (iHdrType) {

    case PCI_HEADER_TYPE_NORMAL:
        iLine = hHandle->PDT_phDevHdr.PCIH_pcidHdr.PCID_ucIntLine;
        iPin  = hHandle->PDT_phDevHdr.PCIH_pcidHdr.PCID_ucIntPin;;
        break;

    case PCI_HEADER_TYPE_BRIDGE:
    case PCI_HEADER_TYPE_CARDBUS:
        return  (PX_ERROR);
        break;

    default:
        return  (PX_ERROR);
    }

    iRet = API_PciCapFind(hHandle->PDT_iDevBus, hHandle->PDT_iDevDevice, hHandle->PDT_iDevFunction,
                          PCI_CAP_ID_MSI, &ucMsiCapPos);
    if (iRet == ERROR_NONE) {
        iRet = API_PciMsiEnableGet(hHandle->PDT_iDevBus, hHandle->PDT_iDevDevice, hHandle->PDT_iDevFunction,
                                   ucMsiCapPos, &hHandle->PDT_iDevIrqMsiEn);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    } else {
        hHandle->PDT_iDevIrqMsiEn = LW_FALSE;
    }

    iRet = API_PciIrqGet(hHandle->PDT_iDevBus, hHandle->PDT_iDevDevice, hHandle->PDT_iDevFunction,
                         hHandle->PDT_iDevIrqMsiEn, iLine, iPin, &hHandle->PDT_ulDevIrqVector);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    if (pulVector) {
        *pulVector = hHandle->PDT_ulDevIrqVector;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pciDevConfigBlockOp
** 功能描述: 按块操作 PCI 设备配置空间
** 输　入  : hHandle    设备控制句柄
**           iPos       偏移位置
**           pucBuf     数据缓冲区
**           iLen       数据长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static
INT  __pciDevConfigBlockOp (PCI_DEV_HANDLE  hHandle, INT  iPos, UINT8 *pucBuf, INT  iLen,
                            INT (*pfuncOpt)(PCI_DEV_HANDLE  hHandle, INT  iPos, UINT8 *pucBuf, INT  iLen))
{
    INT     iRet = PX_ERROR;

    if ((iPos & 1) && iLen >= 1) {
        iRet = pfuncOpt(hHandle, iPos, pucBuf, 1);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }

        iPos++; pucBuf++; iLen--;
    }

    if ((iPos & 3) && iLen >= 2) {
        iRet = pfuncOpt(hHandle, iPos, pucBuf, 2);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }

        iPos += 2; pucBuf += 2; iLen -= 2;
    }

    while (iLen >= 4) {
        iRet = pfuncOpt(hHandle, iPos, pucBuf, 4);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }

        iPos += 4; pucBuf += 4; iLen -= 4;
    }

    if (iLen >= 2) {
        iRet = pfuncOpt(hHandle, iPos, pucBuf, 2);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }

        iPos += 2; pucBuf += 2; iLen -= 2;
    }

    iRet = pfuncOpt(hHandle, iPos, pucBuf, 1);
    if ((iLen) &&
        (iRet != ERROR_NONE)) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pciDevConfigBlockRead
** 功能描述: 按块读取 PCI 设备配置空间
** 输　入  : hHandle    设备控制句柄
**           iPos       偏移位置
**           pucBuf     数据缓冲区
**           iLen       数据长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static
INT  __pciDevConfigBlockRead (PCI_DEV_HANDLE  hHandle, INT  iPos, UINT8 *pucBuf, INT  iLen)
{
    return  (__pciDevConfigBlockOp(hHandle, iPos, pucBuf, iLen, API_PciDevConfigRead));
}
/*********************************************************************************************************
** 函数名称: __pciDevConfigBlockWrite
** 功能描述: 按块写入 PCI 设备配置空间
** 输　入  : hHandle    设备控制句柄
**           iPos       偏移位置
**           pucBuf     数据缓冲区
**           iLen       数据长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
static
INT  __pciDevConfigBlockWrite (PCI_DEV_HANDLE  hHandle, INT  iPos, UINT8 *pucBuf, INT  iLen)
{
    return  (__pciDevConfigBlockOp(hHandle, iPos, pucBuf, iLen, API_PciDevConfigWrite));
}
/*********************************************************************************************************
** 函数名称: API_PciDevConfigRead
** 功能描述: 读取 PCI 设备配置空间
** 输　入  : hHandle    设备控制句柄
**           iPos       偏移位置
**           pucBuf     数据缓冲区
**           iLen       数据长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevConfigRead (PCI_DEV_HANDLE  hHandle, INT  iPos, UINT8 *pucBuf, INT  iLen)
{
    INT         iRet   = PX_ERROR;
    UINT8       ucData = -1;
    UINT16      usData = -1;
    UINT32      uiData = -1;

    if (!((iLen == 1) || (iLen == 2) || (iLen == 4))) {
        return  (__pciDevConfigBlockRead(hHandle, iPos, pucBuf, iLen));
    }

    if (iPos >= PCI_CONFIG_LEN_MAX) {
        return  (PX_ERROR);
    }

    switch (iLen) {

    case 1:
        iRet = API_PciConfigInByte(hHandle->PDT_iDevBus,
                                   hHandle->PDT_iDevDevice,
                                   hHandle->PDT_iDevFunction,
                                   iPos, &ucData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        pucBuf[0] = (UINT8)ucData;
        break;

    case 2:
        iRet = API_PciConfigInWord(hHandle->PDT_iDevBus,
                                   hHandle->PDT_iDevDevice,
                                   hHandle->PDT_iDevFunction,
                                   iPos, &usData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        ((UINT16 *)pucBuf)[0] = htole16(usData);
        break;

    case 4:
        iRet = API_PciConfigInDword(hHandle->PDT_iDevBus,
                                    hHandle->PDT_iDevDevice,
                                    hHandle->PDT_iDevFunction,
                                    iPos, &uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        ((UINT32 *)pucBuf)[0] = htole32(uiData);
        break;

    default:
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevConfigWrite
** 功能描述: 写入 PCI 设备配置空间
** 输　入  : hHandle    设备控制句柄
**           iPos       偏移位置
**           pucBuf     数据缓冲区
**           iLen       数据长度
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevConfigWrite (PCI_DEV_HANDLE  hHandle, INT  iPos, UINT8 *pucBuf, INT  iLen)
{
    INT         iRet   = PX_ERROR;
    UINT8       ucData = -1;
    UINT16      usData = -1;
    UINT32      uiData = -1;

    if (!((iLen == 1) || (iLen == 2) || (iLen == 4))) {
        return  (__pciDevConfigBlockWrite(hHandle, iPos, pucBuf, iLen));
    }

    if (iPos >= PCI_CONFIG_LEN_MAX) {
        return  (PX_ERROR);
    }

    switch (iLen) {

    case 1:
        ucData = (UINT8)pucBuf[0];
        iRet = API_PciConfigOutByte(hHandle->PDT_iDevBus,
                                    hHandle->PDT_iDevDevice,
                                    hHandle->PDT_iDevFunction,
                                    iPos, ucData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        break;

    case 2:
        usData = le16toh(((UINT16 *)pucBuf)[0]);
        iRet = API_PciConfigOutWord(hHandle->PDT_iDevBus,
                                    hHandle->PDT_iDevDevice,
                                    hHandle->PDT_iDevFunction,
                                    iPos, usData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        break;

    case 4:
        uiData = le32toh(((UINT32 *)pucBuf)[0]);
        iRet = API_PciConfigOutDword(hHandle->PDT_iDevBus,
                                     hHandle->PDT_iDevDevice,
                                     hHandle->PDT_iDevFunction,
                                     iPos, uiData);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
        break;

    default:
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellPciDevAddDel
** 功能描述: 添加或删除设备
** 输　入  : iAdd           是否为添加操作
**           iAll           是否为操作全部设备
**           iBus           总线号
**           iDevice        设备号
**           iFunction      功能号
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static
INT  __tshellPciDevAddDel (INT  iAdd, INT  iAll, INT  iBus, INT  iDevice, INT  iFunction)
{
    PCI_DEV_HANDLE      hDevHandle = LW_NULL;
    INT                 iRet = PX_ERROR;

    if ((iAdd == LW_TRUE) && (iAll == LW_TRUE)) {
        API_PciDevListCreate();
    } else if ((iAdd == LW_TRUE) && (iAll == LW_FALSE)) {
        hDevHandle = API_PciDevAdd(iBus, iDevice, iFunction);
        if (hDevHandle == LW_NULL) {
            goto  __error_handle;
        }
    }

    if ((iAdd == LW_FALSE) && (iAll == LW_TRUE)) {
        API_PciDevDelete(LW_NULL);
    } else if ((iAdd == LW_FALSE) && (iAll == LW_FALSE)) {
        hDevHandle = API_PciDevHandleGet(iBus, iDevice, iFunction);
        if (hDevHandle == LW_NULL) {
            goto  __error_handle;
        }
        iRet = API_PciDevDelete(hDevHandle);
        if (iRet != ERROR_NONE) {
            goto  __error_handle;
        }
    }

    return  (ERROR_NONE);

__error_handle:
    fprintf(stderr, "probe pci device error.\n");
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __tshellPciDevCmdShow
** 功能描述: 打印 PCI 设备列表
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static
VOID  __tshellPciDevCmdShow (VOID)
{
    static PCHAR        pcPciDevShowHdr = \
    " INDEX   BUS   DEV   FUNC  VENDOR DEVICE  SUBV   SUBD           DRVNAME\n"
    "------- ----- ----- ------ ------ ------ ------ ------ ------------------------\n";

    PLW_LIST_LINE       plineTemp  = LW_NULL;
    PCI_DEV_HANDLE      hDevHandle = LW_NULL;
    PCI_DRV_HANDLE      hDrvHandle = LW_NULL;
    REGISTER INT        i;

    printf("pci dev number total : %d\n", _GuiPciDevTotalNum);
    printf(pcPciDevShowHdr);

    __PCI_DEV_LOCK();
    i = 0;
    for (plineTemp  = _GplinePciDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDevHandle = _LIST_ENTRY(plineTemp, PCI_DEV_TCB, PDT_lineDevNode);
        hDrvHandle = (PCI_DRV_HANDLE)hDevHandle->PDT_pvDevDriver;
        printf("%7d %5d %5d %6d %6x %6x %6x %6x %-24s\n",
               i,
               hDevHandle->PDT_iDevBus,
               hDevHandle->PDT_iDevDevice,
               hDevHandle->PDT_iDevFunction,
               hDevHandle->PDT_phDevHdr.PCIH_pcidHdr.PCID_usVendorId,
               hDevHandle->PDT_phDevHdr.PCIH_pcidHdr.PCID_usDeviceId,
               hDevHandle->PDT_phDevHdr.PCIH_pcidHdr.PCID_usSubVendorId,
               hDevHandle->PDT_phDevHdr.PCIH_pcidHdr.PCID_usSubSystemId,
               ((hDrvHandle == LW_NULL) ? "NULL" : hDrvHandle->PDT_cDrvName));
        i += 1;
    }
    __PCI_DEV_UNLOCK();

    fflush(stdout);                                                     /*  清空输出                    */
}
/*********************************************************************************************************
** 函数名称: __tshellPciDevCmd
** 功能描述: PCI 命令 "pcidev"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static
INT  __tshellPciDevCmd (INT  iArgC, PCHAR  ppcArgV[])
{
    INT     iRet;
    INT     iBus;
    INT     iDevice;
    INT     iFunction;
    INT     iAddFlag = LW_FALSE;
    INT     iAllFlag = LW_FALSE;

    if (iArgC == 1) {
        __tshellPciDevCmdShow();

        return  (ERROR_NONE);
    }

    if ((lib_strcmp(ppcArgV[1], "add") == 0) && (iArgC == 3)) {
        iAddFlag = LW_TRUE;

        if (lib_strcmp(ppcArgV[2], "all") == 0) {
            iAllFlag = LW_TRUE;
        } else {
            iAllFlag = LW_FALSE;
            iRet = sscanf(ppcArgV[2], "%d:%d.%d", &iBus, &iDevice, &iFunction);
            if (iRet != 3) {
                fprintf(stderr, "pci device address format error.\n");
                goto  __error_handle;
            }
        }

        iRet = __tshellPciDevAddDel(iAddFlag, iAllFlag, iBus, iDevice, iFunction);
        if (iRet != ERROR_NONE) {
            goto  __error_handle;
        }
    } else if ((lib_strcmp(ppcArgV[1], "del") == 0) && (iArgC == 3)) {
        iAddFlag = LW_FALSE;

        if (lib_strcmp(ppcArgV[2], "all") == 0) {
            iAllFlag = LW_TRUE;
        } else {
            iAllFlag = LW_FALSE;
            iRet = sscanf(ppcArgV[2], "%d:%d.%d", &iBus, &iDevice, &iFunction);
            if (iRet != 3) {
                fprintf(stderr, "pci device address format error.\n");
                goto  __error_handle;
            }
        }

        iRet = __tshellPciDevAddDel(iAddFlag, iAllFlag, iBus, iDevice, iFunction);
        if (iRet != ERROR_NONE) {
            goto  __error_handle;
        }
    } else {
        goto  __error_handle;
    }

    return  (ERROR_NONE);

__error_handle:
    fprintf(stderr, "argments error!\n");
    return  (-ERROR_TSHELL_EPARAM);
}
/*********************************************************************************************************
** 函数名称: API_PciDevHandleGet
** 功能描述: 获取一个设备的句柄
** 输　入  : iBus           总线号
**           iDevice        设备号
**           iFunction      功能号
** 输　出  : 设备控制句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
PCI_DEV_HANDLE  API_PciDevHandleGet (INT  iBus, INT  iDevice, INT  iFunction)
{
    PLW_LIST_LINE       plineTemp  = LW_NULL;
    PCI_DEV_HANDLE      hDevHandle = LW_NULL;

    hDevHandle = LW_NULL;
    __PCI_DEV_LOCK();
    for (plineTemp  = _GplinePciDevHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        hDevHandle = _LIST_ENTRY(plineTemp, PCI_DEV_TCB, PDT_lineDevNode);
        if ((hDevHandle->PDT_iDevBus      == iBus     ) &&
            (hDevHandle->PDT_iDevDevice   == iDevice  ) &&
            (hDevHandle->PDT_iDevFunction == iFunction)) {
            break;
        }
    }
    __PCI_DEV_UNLOCK();

    if (plineTemp) {
        return  (hDevHandle);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: API_PciDevAdd
** 功能描述: 增加一个设备
** 输　入  : iBus           总线号
**           iDevice        设备号
**           iFunction      功能号
** 输　出  : 设备控制句柄
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
PCI_DEV_HANDLE  API_PciDevAdd (INT  iBus, INT  iDevice, INT  iFunction)
{
    PCI_DEV_HANDLE      hDevHandle = LW_NULL;
    PCI_HDR             phPciHdr;

    hDevHandle = API_PciDevHandleGet(iBus, iDevice, iFunction);
    if (hDevHandle != LW_NULL) {
        return  (hDevHandle);
    }

    lib_bzero(&phPciHdr, sizeof(PCI_HDR));
    API_PciGetHeader(iBus, iDevice, iFunction, &phPciHdr);

    switch (phPciHdr.PCIH_ucType & PCI_HEADER_TYPE_MASK) {

    case PCI_HEADER_TYPE_NORMAL:
        hDevHandle = (PCI_DEV_HANDLE)__SHEAP_ZALLOC(sizeof(PCI_DEV_TCB));
        if (hDevHandle == LW_NULL) {
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            break;
        }
        hDevHandle->PDT_iDevBus = iBus;
        hDevHandle->PDT_iDevDevice = iDevice;
        hDevHandle->PDT_iDevFunction = iFunction;
        lib_memcpy(&hDevHandle->PDT_phDevHdr, &phPciHdr, sizeof(PCI_HDR));

        __PCI_DEV_LOCK();
        _List_Line_Add_Ahead(&hDevHandle->PDT_lineDevNode, &_GplinePciDevHeader);
        _GuiPciDevTotalNum += 1;
        __PCI_DEV_UNLOCK();
        break;

    case PCI_HEADER_TYPE_BRIDGE:
    case PCI_HEADER_TYPE_CARDBUS:
        hDevHandle = LW_NULL;
        break;

    default:
        hDevHandle = LW_NULL;
        break;
    }

    return  (hDevHandle);
}
/*********************************************************************************************************
** 函数名称: API_PciDevDelete
** 功能描述: 删除一个 PCI 设备
** 输　入  : hHandle       设备控制句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevDelete (PCI_DEV_HANDLE  hHandle)
{
    PLW_LIST_LINE       plineTemp  = LW_NULL;
    PCI_DEV_HANDLE      hDevHandle = LW_NULL;

    if (hHandle == LW_NULL) {
        __PCI_DEV_LOCK();
        for (plineTemp  = _GplinePciDevHeader;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {
            hDevHandle = _LIST_ENTRY(plineTemp, PCI_DEV_TCB, PDT_lineDevNode);
            _List_Line_Del(&hDevHandle->PDT_lineDevNode, &_GplinePciDevHeader);
            _GuiPciDevTotalNum -= 1;
            __SHEAP_FREE(hDevHandle);
        }
        __PCI_DEV_UNLOCK();

        return  (ERROR_NONE);
    }

    hDevHandle = API_PciDevHandleGet(hHandle->PDT_iDevBus,
                                     hHandle->PDT_iDevDevice,
                                     hHandle->PDT_iDevFunction);
    if ((hDevHandle == LW_NULL) ||
        (hDevHandle != hHandle)) {
        return  (PX_ERROR);
    }

    __PCI_DEV_LOCK();
    _List_Line_Del(&hHandle->PDT_lineDevNode, &_GplinePciDevHeader);
    _GuiPciDevTotalNum -= 1;
    __PCI_DEV_UNLOCK();

    __SHEAP_FREE(hHandle);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevDrvDel
** 功能描述: 删除一个 PCI 设备的驱动
** 输　入  : hDevHandle     设备控制句柄
**           hDrvHandle     驱动句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevDrvDel (PCI_DEV_HANDLE  hDevHandle, PCI_DRV_HANDLE  hDrvHandle)
{
    if ((hDevHandle == LW_NULL) ||
        (hDrvHandle == LW_NULL)) {
        return  (PX_ERROR);
    }

    if (hDevHandle->PDT_pvDevDriver != hDrvHandle) {
        return  (PX_ERROR);
    }

    __PCI_DEV_LOCK();
    hDevHandle->PDT_pvDevDriver = LW_NULL;
    __PCI_DEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevDrvUpdate
** 功能描述: 更新一个 PCI 设备的驱动
** 输　入  : hDevHandle     设备控制句柄
**           hDrvHandle     驱动句柄
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevDrvUpdate (PCI_DEV_HANDLE  hDevHandle, PCI_DRV_HANDLE  hDrvHandle)
{
    if ((hDevHandle == LW_NULL) ||
        (hDrvHandle == LW_NULL)) {
        return  (PX_ERROR);
    }

    if ((hDevHandle->PDT_pvDevDriver != LW_NULL) &&
        (hDevHandle->PDT_pvDevDriver != hDrvHandle)) {
        return  (PX_ERROR);
    }

    if ((hDevHandle->PDT_pvDevDriver == LW_NULL) ||
        (hDevHandle->PDT_pvDevDriver == hDrvHandle)) {
        __PCI_DEV_LOCK();
        hDevHandle->PDT_pvDevDriver = (PVOID)hDrvHandle;
        __PCI_DEV_UNLOCK();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pciDevListCreate
** 功能描述: 创建设备列表回调, 上层已经加锁, 不需要再进行加锁操作
** 输　入  : iBus           总线号
**           iDevice        设备号
**           iFunction      功能号
**           pvArg          参数
** 输　出  : ERROR_NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pciDevListCreate (INT  iBus, INT  iDevice, INT  iFunction, PVOID pvArg)
{
    API_PciDevAdd(iBus, iDevice, iFunction);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevListCreate
** 功能描述: 创建 PCI 设备列表
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevListCreate (VOID)
{
    API_PciLock();
    API_PciTraversal(__pciDevListCreate, LW_NULL, PCI_MAX_BUS - 1);
    API_PciUnlock();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PciDevInit
** 功能描述: PCI 设备管理初始化
** 输　入  : NONE
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PciDevInit (VOID)
{
    _GuiPciDevTotalNum  = 0;
    _GulPciDevLock      = LW_OBJECT_HANDLE_INVALID;
    _GplinePciDevHeader = LW_NULL;

    _GulPciDevLock = API_SemaphoreMCreate("pci_dev_lock", LW_PRIO_DEF_CEILING,
                                          LW_OPTION_INHERIT_PRIORITY |
                                          LW_OPTION_DELETE_SAFE |
                                          LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (_GulPciDevLock == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    API_TShellKeywordAdd("pcidev", __tshellPciDevCmd);
    API_TShellFormatAdd("pcidev", " [add | del] [[all] | 1:0.1]");
    API_TShellHelpAdd("pcidev", "show, add, del pci device table\n"
                                "eg. pcidev\n"
                                "    pcidev add all\n"
                                "    pcidev add 1:0.1\n"
                                "    pcidev del 1:0.1\n");

    return  (ERROR_NONE);
}
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
