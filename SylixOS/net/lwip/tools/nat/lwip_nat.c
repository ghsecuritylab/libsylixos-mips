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
** 文   件   名: lwip_nat.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 03 月 19 日
**
** 描        述: lwip NAT 支持包.

** BUG:
2011.03.06  修正 gcc 4.5.1 相关 warning.
2011.07.30  nats 命令不能在网络终端(例如: telnet)中被调用, 因为他要使用 NAT 锁, printf 如果最终打印到网络
            上, 将会和 netproto 任务抢占 NAT 锁.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/net/include/net_net.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_NET_NAT_EN > 0)
#include "socket.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/inet.h"
#include "lwip_natlib.h"
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
VOID        __natPoolCreate(VOID);
VOID        __natTimer(VOID);
INT         __natMapAdd(ip_addr_t  *pipaddr, u16_t  usPort, u16_t  AssPort, u8_t  ucProto);
INT         __natMapDelete(ip_addr_t  *pipaddr, u16_t  usPort, u16_t  AssPort, u8_t  ucProto);
INT         __natAliasAdd(const ip_addr_t  *pipaddrAlias, 
                          const ip_addr_t  *ipaddrSLocalIp,
                          const ip_addr_t  *ipaddrELocalIp);
INT         __natAliasDelete(const ip_addr_t  *pipaddrAlias);

#if LW_CFG_PROCFS_EN > 0
VOID        __procFsNatInit(VOID);
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */

#if LW_CFG_SHELL_EN > 0
static INT  __tshellNat(INT  iArgC, PCHAR  ppcArgV[]);
static INT  __tshellNatMap(INT  iArgC, PCHAR  ppcArgV[]);
static INT  __tshellNatAlias(INT  iArgC, PCHAR  ppcArgV[]);
static INT  __tshellNatShow(INT  iArgC, PCHAR  ppcArgV[]);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  NAT 本地内网与 AP 外网网络接口
*********************************************************************************************************/
extern struct netif        *_G_pnetifNatLocal;
extern struct netif        *_G_pnetifNatAp;
/*********************************************************************************************************
  NAT 操作锁
*********************************************************************************************************/
extern LW_OBJECT_HANDLE     _G_ulNatOpLock;
/*********************************************************************************************************
  NAT 刷新定时器
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_ulNatTimer;
/*********************************************************************************************************
** 函数名称: API_INetNatInit
** 功能描述: internet NAT 初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
VOID  API_INetNatInit (VOID)
{
    static   BOOL   bIsInit = LW_FALSE;
    
    if (bIsInit) {
        return;
    }
    
    __natPoolCreate();                                                  /*  创建 NAT 控制块缓冲池       */
    
    _G_ulNatOpLock = API_SemaphoreMCreate("nat_lock", LW_PRIO_DEF_CEILING, 
                                          LW_OPTION_INHERIT_PRIORITY |
                                          LW_OPTION_DELETE_SAFE |
                                          LW_OPTION_OBJECT_GLOBAL,
                                          LW_NULL);                     /*  创建 NAT 操作锁             */
                                          
    _G_ulNatTimer = API_TimerCreate("nat_timer", LW_OPTION_ITIMER | LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    
    API_TimerStart(_G_ulNatTimer, 
                   (LW_TICK_HZ * 60),
                   LW_OPTION_AUTO_RESTART,
                   (PTIMER_CALLBACK_ROUTINE)__natTimer,
                   LW_NULL);                                            /*  每分钟执行一次              */
    
#if LW_CFG_PROCFS_EN > 0
    __procFsNatInit();
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
    
#if LW_CFG_SHELL_EN > 0
    API_TShellKeywordAdd("nat", __tshellNat);
    API_TShellFormatAdd("nat",  " [stop] | {[LAN netif] [WAN netif]}");
    API_TShellHelpAdd("nat",   "start or stop NAT network.\n"
                               "eg. nat wl2 en1 (start NAT network use wl2 as LAN, en1 as WAN)\n"
                               "    nat stop    (stop NAT network)\n");
    
    API_TShellKeywordAdd("natalias", __tshellNatAlias);
    API_TShellFormatAdd("natalias",  " {[add] [alias] [LAN start] [LAN end]} | {[del] [alias]}");
    API_TShellHelpAdd("natalias",    "add or delete NAT alias.\n"
                                     "eg. natalias add 11.22.33.44 192.168.1.2 192.168.1.3  (add alias to 192..2 ~ 192..3)\n"
                                     "    natalias del 11.22.33.44                          (delete alias)\n");
                                     
    API_TShellKeywordAdd("natmap", __tshellNatMap);
    API_TShellFormatAdd("natmap",  " {[add] | [del]} [WAN port] [LAN port] [LAN IP] [protocol]");
    API_TShellHelpAdd("natmap",    "add or delete NAT maps.\n"
                                     "eg. natmap add 80 80 192.168.1.2 tcp (map webserver as 192..2)\n"
                                     "    natmap del 80 80 192.168.1.2 tcp (unmap webserver as 192..2)\n");
                                     
    API_TShellKeywordAdd("nats", __tshellNatShow);
    API_TShellHelpAdd("nats",   "show NAT networking infomation.\n");
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
    
    bIsInit = LW_TRUE;
}
/*********************************************************************************************************
** 函数名称: API_INetNatStart
** 功能描述: 启动 NAT (外网网络接口必须为 lwip 默认的路由接口)
** 输　入  : pcLocalNetif          本地内网网络接口名
**           pcApNetif             外网网络接口名
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_INetNatStart (CPCHAR  pcLocalNetif, CPCHAR  pcApNetif)
{
    struct netif   *pnetifLocal;
    struct netif   *pnetifAp;

    if (!pcLocalNetif || !pcApNetif) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (_G_pnetifNatLocal || _G_pnetifNatAp) {                          /*  NAT 正在工作                */
        _ErrorHandle(EISCONN);
        return  (PX_ERROR);
    }
    
    __NAT_OP_LOCK();                                                    /*  锁定 NAT 链表               */
    pnetifLocal = netif_find((PCHAR)pcLocalNetif);
    pnetifAp    = netif_find((PCHAR)pcApNetif);
    
    if (!pnetifLocal || !pnetifAp) {                                    /*  有网络接口不存在            */
        __NAT_OP_UNLOCK();                                              /*  解锁 NAT 链表               */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    __KERNEL_ENTER();
    _G_pnetifNatLocal = pnetifLocal;                                    /*  保存网络接口                */
    _G_pnetifNatAp    = pnetifAp;
    __KERNEL_EXIT();
    
    __NAT_OP_UNLOCK();                                                  /*  解锁 NAT 链表               */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_INetNatStop
** 功能描述: 停止 NAT (注意: 删除 NAT 网络接口时, 必须要先停止 NAT 网络接口)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_INetNatStop (VOID)
{
    if (!_G_pnetifNatLocal || !_G_pnetifNatAp) {                        /*  NAT 没有工作                */
        _ErrorHandle(ENOTCONN);
        return  (PX_ERROR);
    }
    
    __NAT_OP_LOCK();                                                    /*  锁定 NAT 链表               */
    
    __KERNEL_ENTER();
    _G_pnetifNatLocal = LW_NULL;
    _G_pnetifNatAp    = LW_NULL;
    __KERNEL_EXIT();
    
    __NAT_OP_UNLOCK();                                                  /*  解锁 NAT 链表               */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_INetNatMapAdd
** 功能描述: 创建内网外网映射
** 输　入  : pcLocalIp         内网 IP
**           usLocalPort       内网 端口
**           usAssPort         映射 端口
**           ucProto           协议 IP_PROTO_TCP / IP_PROTO_UDP
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_INetNatMapAdd (CPCHAR  pcLocalIp, UINT16  usLocalPort, UINT16  usAssPort, UINT8  ucProto)
{
    ip_addr_t  ipaddr;
    INT        iRet;
    
    if (!pcLocalIp) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (usAssPort >= LW_CFG_NET_NAT_MIN_PORT) {
        _ErrorHandle(EADDRINUSE);
        return  (PX_ERROR);
    }
    
    if ((ucProto != IP_PROTO_TCP) && (ucProto != IP_PROTO_UDP)) {
        _ErrorHandle(ENOPROTOOPT);
        return  (PX_ERROR);
    }
    
    if (!ipaddr_aton(pcLocalIp, &ipaddr)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iRet = __natMapAdd(&ipaddr, htons(usLocalPort), htons(usAssPort), ucProto);
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_INetNatMapDelete
** 功能描述: 删除内网外网映射
** 输　入  : pcLocalIp         内网 IP
**           usLocalPort       内网 端口
**           usAssPort         映射 端口
**           ucProto           协议 IP_PROTO_TCP / IP_PROTO_UDP
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_INetNatMapDelete (CPCHAR  pcLocalIp, UINT16  usLocalPort, UINT16  usAssPort, UINT8  ucProto)
{
    ip_addr_t  ipaddr;
    INT        iRet;
    
    if (!pcLocalIp) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (usAssPort >= LW_CFG_NET_NAT_MIN_PORT) {
        _ErrorHandle(EADDRINUSE);
        return  (PX_ERROR);
    }
    
    if ((ucProto != IP_PROTO_TCP) && (ucProto != IP_PROTO_UDP)) {
        _ErrorHandle(ENOPROTOOPT);
        return  (PX_ERROR);
    }
    
    if (!ipaddr_aton(pcLocalIp, &ipaddr)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iRet = __natMapDelete(&ipaddr, htons(usLocalPort), htons(usAssPort), ucProto);
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_INetNatAliasAdd
** 功能描述: NAT 增加网络别名 (公网 IP)
** 输　入  : pcAliasIp         别名 IP
**           pcSLocalIp        本地 IP 起始位置
**           pcELocalIp        本地 IP 结束位置
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_INetNatAliasAdd (CPCHAR  pcAliasIp, CPCHAR  pcSLocalIp, CPCHAR  pcELocalIp)
{
    ip_addr_t  ipaddrAlias;
    ip_addr_t  ipaddrSLocalIp;
    ip_addr_t  ipaddrELocalIp;
    
    INT        iRet;
    
    if (!pcAliasIp || !pcSLocalIp || !pcELocalIp) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!ipaddr_aton(pcAliasIp, &ipaddrAlias)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!ipaddr_aton(pcSLocalIp, &ipaddrSLocalIp)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!ipaddr_aton(pcELocalIp, &ipaddrELocalIp)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iRet = __natAliasAdd(&ipaddrAlias, &ipaddrSLocalIp, &ipaddrELocalIp);
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_INetNatAliasDelete
** 功能描述: NAT 删除网络别名 (公网 IP)
** 输　入  : pcAliasIp         别名 IP
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_INetNatAliasDelete (CPCHAR  pcAliasIp)
{
    ip_addr_t  ipaddrAlias;
    INT        iRet;
    
    if (!pcAliasIp) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (!ipaddr_aton(pcAliasIp, &ipaddrAlias)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    iRet = __natAliasDelete(&ipaddrAlias);
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __tshellNat
** 功能描述: 系统命令 "nat"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static INT  __tshellNat (INT  iArgC, PCHAR  ppcArgV[])
{
    if (iArgC == 3) {
        if (API_INetNatStart(ppcArgV[1], ppcArgV[2]) != ERROR_NONE) {
            fprintf(stderr, "can not start NAT network, errno : %s\n", lib_strerror(errno));
        
        } else {
            printf("NAT network started, [LAN : %s] [WAN : %s]\n", ppcArgV[1], ppcArgV[2]);
        }
    
    } else if (iArgC == 2) {
        if (lib_strcmp(ppcArgV[1], "stop") == 0) {
            API_INetNatStop();
            printf("NAT network stoped.\n");
        }

    } else {
        fprintf(stderr, "option error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellNatMap
** 功能描述: 系统命令 "natmap"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellNatMap (INT  iArgC, PCHAR  ppcArgV[])
{
    INT         iError;
    INT         iWanPort;
    INT         iLanPort;
    UINT8       ucProto;
    
    if (iArgC != 6) {
        fprintf(stderr, "option error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (sscanf(ppcArgV[2], "%d", &iWanPort) != 1) {
        fprintf(stderr, "WAN port error option error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    if (sscanf(ppcArgV[3], "%d", &iLanPort) != 1) {
        fprintf(stderr, "LAN port error option error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (lib_strcasecmp(ppcArgV[5], "tcp") == 0) {
        ucProto = IPPROTO_TCP;
        
    } else if (lib_strcasecmp(ppcArgV[5], "udp") == 0) {
        ucProto = IPPROTO_UDP;
    
    } else {
        fprintf(stderr, "protocol option error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (lib_strcmp(ppcArgV[1], "add") == 0) {
        iError = API_INetNatMapAdd(ppcArgV[4], (UINT16)iLanPort, (UINT16)iWanPort, ucProto);
        if (iError < 0) {
            fprintf(stderr, "add NAT map error : %s!\n", lib_strerror(errno));
            return  (-ERROR_TSHELL_EPARAM);
        }
        
    } else if (lib_strcmp(ppcArgV[1], "del") == 0) {
        iError = API_INetNatMapDelete(ppcArgV[4], (UINT16)iLanPort, (UINT16)iWanPort, ucProto);
        if (iError < 0) {
            fprintf(stderr, "delete NAT map error : %s!\n", lib_strerror(errno));
            return  (-ERROR_TSHELL_EPARAM);
        }
        
    } else {
        fprintf(stderr, "option error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellNatAlias
** 功能描述: 系统命令 "natalias"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellNatAlias (INT  iArgC, PCHAR  ppcArgV[])
{
    INT    iError;
    
    if (iArgC == 5) {
        if (lib_strcmp(ppcArgV[1], "add")) {
            goto    __error;
        }
        iError = API_INetNatAliasAdd(ppcArgV[2], ppcArgV[3], ppcArgV[4]);
        if (iError < 0) {
            fprintf(stderr, "add NAT alias error : %s!\n", lib_strerror(errno));
            return  (-ERROR_TSHELL_EPARAM);
        }
    
    } else if (iArgC == 3) {
        if (lib_strcmp(ppcArgV[1], "del")) {
            goto    __error;
        }
        iError = API_INetNatAliasDelete(ppcArgV[2]);
        if (iError < 0) {
            fprintf(stderr, "delete NAT alias error : %s!\n", lib_strerror(errno));
            return  (-ERROR_TSHELL_EPARAM);
        }
    
    } else {
__error:
        fprintf(stderr, "option error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellNatShow
** 功能描述: 系统命令 "nats"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellNatShow (INT  iArgC, PCHAR  ppcArgV[])
{
    INT     iFd;
    CHAR    cBuffer[512];
    ssize_t sstNum;
    
    iFd = open("/proc/net/nat/info", O_RDONLY);
    if (iFd < 0) {
        fprintf(stderr, "can not open /proc/net/nat/info : %s\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    do {
        sstNum = read(iFd, cBuffer, sizeof(cBuffer));
        if (sstNum > 0) {
            write(STDOUT_FILENO, cBuffer, (size_t)sstNum);
        }
    } while (sstNum > 0);
    
    close(iFd);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_NAT_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
