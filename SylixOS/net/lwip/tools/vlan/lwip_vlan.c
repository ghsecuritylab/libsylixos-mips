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
** 文   件   名: lwip_vlan.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 07 月 13 日
**
** 描        述: lwip vlan 支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "../../lwip_if.h"
#include "lwip/ip.h"
#include "netif/etharp.h"
/*********************************************************************************************************
  网络接口数量宏定义
*********************************************************************************************************/
#if LW_CFG_NET_VLAN_EN > 0
#define __LW_NETIF_MAX_NUM              10
/*********************************************************************************************************
  内部变量
*********************************************************************************************************/
static INT          _G_iNetifVlanIdTbl[__LW_NETIF_MAX_NUM];
#endif                                                                  /*  LW_CFG_NET_VLAN_EN > 0      */
/*********************************************************************************************************
** 函数名称: etharp_vlan_set_hook
** 功能描述: 设置网络以太网络分组 VLAN ID (LWIP_HOOK_VLAN_SET)
** 输　入  : pvNetif             以太网络接口.
             pvEthhdr            以太数据包头.
             pvVlanhdr           VLAN 数据包头.
** 输　出  : 0: 无
**           1: 已添加 VLAN ID
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int  etharp_vlan_set_hook (PVOID pvNetif, PVOID pvEthhdr, PVOID pvVlanhdr)
{
#if LW_CFG_NET_VLAN_EN > 0
    REGISTER struct netif        *netif   = (struct netif *)pvNetif;
    REGISTER struct eth_vlan_hdr *vlanhdr = (struct eth_vlan_hdr *)pvVlanhdr;
    
    if (netif->num < __LW_NETIF_MAX_NUM) {
        if (_G_iNetifVlanIdTbl[netif->num] > 0) {
            vlanhdr->prio_vid = htons((u16_t)_G_iNetifVlanIdTbl[netif->num]);
            return  (1);
        }
    }
#endif                                                                  /*  LW_CFG_NET_VLAN_EN > 0      */
    
    return  (0);
}
/*********************************************************************************************************
** 函数名称: etharp_vlan_check_hook
** 功能描述: 检查网络以太网络分组 VLAN ID (LWIP_HOOK_VLAN_SET)
** 输　入  : pvNetif             以太网络接口.
             pvEthhdr            以太数据包头.
             pvVlanhdr           VLAN 数据包头.
** 输　出  : 0: 不接收
**           1: 接收
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
int  etharp_vlan_check_hook (PVOID pvNetif, PVOID pvEthhdr, PVOID pvVlanhdr)
{
#if LW_CFG_NET_VLAN_EN > 0
    REGISTER struct netif        *netif   = (struct netif *)pvNetif;
    REGISTER struct eth_vlan_hdr *vlanhdr = (struct eth_vlan_hdr *)pvVlanhdr;

    if (netif->num < __LW_NETIF_MAX_NUM) {
        if (_G_iNetifVlanIdTbl[netif->num] > 0) {
            if (vlanhdr->prio_vid != htons((u16_t)_G_iNetifVlanIdTbl[netif->num])) {
                return  (0);
            }
        }
    }
#endif                                                                  /*  LW_CFG_NET_VLAN_EN > 0      */

    return  (1);
}
/*********************************************************************************************************
** 函数名称: API_VlanSet
** 功能描述: 设置一个网络接口的 VLAN ID
** 输　入  : pcEthIf           以太网络接口名.
             iVlanId           VLAN ID (-1 表示允许任何 VLAN ID 通过)
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
#if LW_CFG_NET_VLAN_EN > 0

LW_API  
INT  API_VlanSet (CPCHAR  pcEthIf, INT  iVlanId)
{
    struct netif  *pnetif;

    if (!pcEthIf) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    LWIP_NETIF_LOCK();
    pnetif = netif_find((char *)pcEthIf);
    if (!pnetif) {
        LWIP_NETIF_UNLOCK();
        _ErrorHandle(ENODEV);
        return  (PX_ERROR);
    }
    
    if (pnetif->num < __LW_NETIF_MAX_NUM) {
        _G_iNetifVlanIdTbl[pnetif->num] = iVlanId;
    }
    LWIP_NETIF_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_VlanGet
** 功能描述: 获取一个网络接口的 VLAN ID
** 输　入  : pcEthIf           以太网络接口名.
             piVlanId          VLAN ID (-1 表示允许任何 VLAN ID 通过)
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_VlanGet (CPCHAR  pcEthIf, INT  *piVlanId)
{
    struct netif  *pnetif;

    if (!pcEthIf || !piVlanId) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    LWIP_NETIF_LOCK();
    pnetif = netif_find((char *)pcEthIf);
    if (!pnetif) {
        LWIP_NETIF_UNLOCK();
        _ErrorHandle(ENODEV);
        return  (PX_ERROR);
    }
    
    if (pnetif->num < __LW_NETIF_MAX_NUM) {
        *piVlanId = _G_iNetifVlanIdTbl[pnetif->num];
    }
    LWIP_NETIF_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_VlanShow
** 功能描述: 显示所有网络接口的 VLAN ID
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_VlanShow (VOID)
{
    static const CHAR   cVlanInfoHdr[] = "\n"
    "INDEX  VLAN ID\n"
    "----- ---------\n";
    
    INT     i;
    
    printf(cVlanInfoHdr);
    
    LWIP_NETIF_LOCK();
    for (i = 0; i < __LW_NETIF_MAX_NUM; i++) {
        if (_G_iNetifVlanIdTbl[i] >= 0) {
            printf("%5d %9d\n", i, _G_iNetifVlanIdTbl[i]);
        }
    }
    LWIP_NETIF_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __tshellVlan
** 功能描述: 系统命令 "vlan"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellVlan (INT  iArgC, PCHAR  *ppcArgV)
{
    INT    iError;
    INT    iVlanId;
    
    if (iArgC == 1) {
        API_VlanShow();
        return  (ERROR_NONE);
    }
    
    if (lib_strcmp(ppcArgV[1], "set") == 0) {
        if (iArgC != 4) {
            fprintf(stderr, "argments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        if ((sscanf(ppcArgV[3], "%d", &iVlanId) != 1) ||
            (iVlanId < 0) || (iVlanId > 65535)) {
            fprintf(stderr, "VLAN ID error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        iError = API_VlanSet(ppcArgV[2], iVlanId);
        if (iError < 0) {
            fprintf(stderr, "VLAN ID Set error : %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
    } else if (lib_strcmp(ppcArgV[1], "clear") == 0) {
        if (iArgC != 3) {
            fprintf(stderr, "argments error!\n");
            return  (-ERROR_TSHELL_EPARAM);
        }
        
        iVlanId = -1;
        iError = API_VlanSet(ppcArgV[2], iVlanId);
        if (iError < 0) {
            fprintf(stderr, "VLAN ID Clear error : %s\n", lib_strerror(errno));
            return  (PX_ERROR);
        }
    } else {
        fprintf(stderr, "argments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __netVlanInit
** 功能描述: 初始化 vlan 管理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  __netVlanInit (VOID)
{
    INT     i;
    
    for (i = 0; i < __LW_NETIF_MAX_NUM; i++) {
        _G_iNetifVlanIdTbl[i] = -1;
    }
    
#if LW_CFG_SHELL_EN > 0
    API_TShellKeywordAdd("vlan", __tshellVlan);
    API_TShellFormatAdd("vlan", " [{set | clear}] [netifname]");
    API_TShellHelpAdd("vlan",   "show | set | clear net interface VLAN ID.\n"
                                "eg. vlan               show all vlan id.\n"
                                "    vlan set en0 3     set netif:en0 VLAN ID 3.\n"
                                "    vlan clear en0     clear netif:en0 VLAN ID.\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
}

#endif                                                                  /*  LW_CFG_NET_VLAN_EN > 0      */
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
