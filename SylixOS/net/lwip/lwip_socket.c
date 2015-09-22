/*********************************************************************************************************
**
**                                    �й�������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: lwip_socket.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 12 �� 18 ��
**
** ��        ��: socket �ӿ�. (ʹ�� sylixos io ϵͳ���� lwip �ļ�������)

** BUG:
2013.01.02  �� socket �ṹ�м��� errno ��¼, SO_ERROR ͳһ�� socket �㴦��.
2013.01.10  ���� POSIX Ҫ��������ȷ��ȡ����.
2013.01.20  �������� SO_ERROR ��һ������:
            BSD socket �涨, ���ʹ�� NBIO ִ�� connect ����, ���ܳɹ���񶼻������˳�, �������ִ������
            ����, �� errno Ϊ EINPROGRESS. ��ʱӦ�ó���ʹ�� select ���� poll ��������, ������ӳɹ�, ��
            ��ʱͨ�� getsockopt �� SO_ERROR ѡ���ȡ�Ľ��Ӧ��Ϊ 0, ������Ϊ������������. 
            ����, ����ͨ����ͨ�� select ����ʱ, ��Ҫ�ύ���µĴ����, ���Ը��� socket ��ǰ��¼�Ĵ����.
2013.01.30  ʹ�� hash ��, �ӿ� socket_event �ٶ�.
2013.04.27  ���� hostbyaddr() α����, ���� netdb �����ⲿ C ��ʵ��.
2013.04.28  ���� __IfIoctl() ��ģ�� BSD ϵͳ ifioctl �Ĳ��ֹ���.
2013.04.29  lwip_fcntl() �����⵽ O_NONBLOCK λ�⻹������λ, �����. ������Ҫ����.
2013.09.24  ioctl() ����� SIOCGSIZIFCONF ��֧��.
            ioctl() ֧�� IPv6 ��ַ����.
2013.11.17  ����� SOCK_CLOEXEC �� SOCK_NONBLOCK ��֧��.
2013.11.21  ���� accept4() ����.
2014.03.22  ���� AF_PACKET ֧��.
2014.04.01  ���� socket �ļ��� mmap ��֧��.
2014.05.02  __SOCKET_CHECHK() �жϳ���ʱ��ӡ debug ��Ϣ.
            socket ���� monitor ���������.
            ���� __ifIoctl() �� if_indextoname �����Ĵ���.
2014.05.03  �����ȡ�������͵Ľӿ�.
2014.05.07  __ifIoctlIf() ����ʹ�� netif ioctl.
2014.11.07  AF_PACKET ioctl() ֧�ֻ������ڲ���.
2015.03.02  ���� socket ��λ����, ���� socket ������Ҫ���� SO_LINGER Ϊ�����ر�ģʽ.
2015.03.17  netif ioctl ���������ģʽ������.
2015.04.06  *_socket_event() ������Ҫ socket lock ����.
2015.04.17  ��������չ ioctl ר�ŷ�װ.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "sys/socket.h"
#include "sys/un.h"
#include "lwip/mem.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "net/if.h"
#include "net/if_dl.h"
#include "net/if_arp.h"
#include "net/if_type.h"
#include "netdb.h"
#include "lwip_if.h"
#include "./packet/af_packet.h"
#include "./unix/af_unix.h"
#if LW_CFG_NET_WIRELESS_EN > 0
#include "net/if_wireless.h"
#include "./wireless/lwip_wl.h"
#endif                                                                  /*  LW_CFG_NET_WIRELESS_EN > 0  */
/*********************************************************************************************************
  ipv6 extern vars
*********************************************************************************************************/
#if LWIP_IPV6
const struct in6_addr in6addr_any                = IN6ADDR_ANY_INIT;
const struct in6_addr in6addr_loopback           = IN6ADDR_LOOPBACK_INIT;
const struct in6_addr in6addr_nodelocal_allnodes = IN6ADDR_NODELOCAL_ALLNODES_INIT;
const struct in6_addr in6addr_linklocal_allnodes = IN6ADDR_LINKLOCAL_ALLNODES_INIT;
#endif
/*********************************************************************************************************
  �ļ��ṹ (֧�ֵ�Э��� AF_INET AF_INET6 AF_RAW AF_UNIX)
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE        SOCK_lineManage;                                /*  ��������                    */
    INT                 SOCK_iFamily;                                   /*  Э���                      */
    
    union {
        INT             SOCKF_iLwipFd;                                  /*  lwip �ļ�������             */
#if LW_CFG_NET_UNIX_EN > 0
        AF_UNIX_T      *SOCKF_pafunix;                                  /*  AF_UNIX ���ƿ�              */
#endif
        AF_PACKET_T    *SOCKF_pafpacket;                                /*  AF_PACKET ���ƿ�            */
    } SOCK_family;
    
    INT                 SOCK_iHash;                                     /*  hash ���±�                 */
    INT                 SOCK_iSoErr;                                    /*  ���һ�δ���                */
    LW_SEL_WAKEUPLIST   SOCK_selwulist;
} SOCKET_T;

#define SOCK_iLwipFd    SOCK_family.SOCKF_iLwipFd
#define SOCK_pafunix    SOCK_family.SOCKF_pafunix
#define SOCK_pafpacket  SOCK_family.SOCKF_pafpacket
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static LONG             __socketOpen(LW_DEV_HDR *pdevhdr, PCHAR  pcName, INT  iFlag, mode_t  mode);
static INT              __socketClose(SOCKET_T *psock);
static ssize_t          __socketRead(SOCKET_T *psock, PVOID  pvBuffer, size_t  stLen);
static ssize_t          __socketWrite(SOCKET_T *psock, CPVOID  pvBuffer, size_t  stLen);
static INT              __socketIoctl(SOCKET_T *psock, INT  iCmd, PVOID  pvArg);
static INT              __socketMmap(SOCKET_T *psock, PLW_DEV_MMAP_AREA  pdmap);
static INT              __socketUnmap(SOCKET_T *psock, PLW_DEV_MMAP_AREA  pdmap);
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_DEV_HDR           _G_devhdrSocket;
static LW_OBJECT_HANDLE     _G_hSockSelMutex;
static LW_OBJECT_HANDLE     _G_hSockMutex;

#define __SOCKET_HASH_SIZE  16
#define __SOCKET_HASH_MASK  (__SOCKET_HASH_SIZE - 1)
static LW_LIST_LINE_HEADER  _G_plineSocket[__SOCKET_HASH_SIZE];

#define __SOCKET_LOCK()     API_SemaphoreMPend(_G_hSockMutex, LW_OPTION_WAIT_INFINITE)
#define __SOCKET_UNLOCK()   API_SemaphoreMPost(_G_hSockMutex)
/*********************************************************************************************************
  lwip �� unix �ڲ����� (�����ʵ�ִ���ֻ�����¼���Чʱ���ܸ��� piSoErr)
*********************************************************************************************************/
extern int              __lwip_have_event(int s, int type, int *piSoErr);
#if LW_CFG_NET_UNIX_EN > 0
extern int              __unix_have_event(AF_UNIX_T *pafunix, int type, int *piSoErr);
#endif
extern int              __packet_have_event(AF_PACKET_T *pafpacket, int type, int  *piSoErr);
/*********************************************************************************************************
  socket fd ��Ч�Լ��
*********************************************************************************************************/
#define __SOCKET_CHECHK()   if (psock == (SOCKET_T *)PX_ERROR) {    \
                                _ErrorHandle(EBADF);    \
                                return  (PX_ERROR); \
                            }   \
                            iosFdGetType(s, &iType);    \
                            if (iType != LW_DRV_TYPE_SOCKET) { \
                                _DebugHandle(__ERRORMESSAGE_LEVEL, "not a socket file.\r\n");   \
                                _ErrorHandle(ENOTSOCK);    \
                                return  (PX_ERROR); \
                            }
/*********************************************************************************************************
  socket fd hash ���� (pafunix >> 4) �� 32 ���� 64 λ CPU ʱ���Ե�λ 0 .
*********************************************************************************************************/
#define __SOCKET_LWIP_HASH(lwipfd)      (lwipfd & __SOCKET_HASH_MASK)
#define __SOCKET_UNIX_HASH(pafunix)     (((size_t)pafunix >> 4) & __SOCKET_HASH_MASK)
#define __SOCKET_PACKET_HASH(pafpacket) __SOCKET_UNIX_HASH(pafpacket)
/*********************************************************************************************************
** ��������: __lwip_socket_event
** ��������: lwip socket �����¼�
** �䡡��  : lwipfd      lwip �ļ�
**           type        �¼�����
**           iSoErr      ���µ� SO_ERROR ��ֵ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  __lwip_socket_event (int  lwipfd, LW_SEL_TYPE type, INT  iSoErr)
{
    INT             iHash = __SOCKET_LWIP_HASH(lwipfd);
    PLW_LIST_LINE   plineTemp;
    SOCKET_T       *psock;

    for (plineTemp  = _G_plineSocket[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        psock = (SOCKET_T *)plineTemp;
        if (psock->SOCK_iLwipFd == lwipfd) {
            psock->SOCK_iSoErr = iSoErr;                                /*  ���� SO_ERROR               */
            SEL_WAKE_UP_ALL(&psock->SOCK_selwulist, type);
            break;
        }
    }
}
/*********************************************************************************************************
** ��������: __unix_socket_event
** ��������: unix socket �����¼�
** �䡡��  : pafunix     ���ƿ�
**           type        �¼�����
**           iSoErr      ���µ� SO_ERROR ��ֵ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_NET_UNIX_EN > 0

void  __unix_socket_event (AF_UNIX_T  *pafunix, LW_SEL_TYPE type, INT  iSoErr)
{
    INT             iHash = __SOCKET_UNIX_HASH(pafunix);
    PLW_LIST_LINE   plineTemp;
    SOCKET_T       *psock;

    for (plineTemp  = _G_plineSocket[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        psock = (SOCKET_T *)plineTemp;
        if (psock->SOCK_pafunix == pafunix) {
            psock->SOCK_iSoErr = iSoErr;                                /*  ���� SO_ERROR               */
            SEL_WAKE_UP_ALL(&psock->SOCK_selwulist, type);
            break;
        }
    }
}

#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */
/*********************************************************************************************************
** ��������: __packet_socket_event
** ��������: packet socket �����¼�
** �䡡��  : pafpacket   ���ƿ�
**           type        �¼�����
**           iSoErr      ���µ� SO_ERROR ��ֵ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
void  __packet_socket_event (AF_PACKET_T *pafpacket, LW_SEL_TYPE type, INT  iSoErr)
{
    INT             iHash = __SOCKET_PACKET_HASH(pafpacket);
    PLW_LIST_LINE   plineTemp;
    SOCKET_T       *psock;
    
    for (plineTemp  = _G_plineSocket[iHash];
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
         
        psock = (SOCKET_T *)plineTemp;
        if (psock->SOCK_pafpacket == pafpacket) {
            psock->SOCK_iSoErr = iSoErr;                                /*  ���� SO_ERROR               */
            SEL_WAKE_UP_ALL(&psock->SOCK_selwulist, type);
            break;
        }
    }
}
/*********************************************************************************************************
** ��������: __ifConfSize
** ��������: �������ӿ��б���������Ҫ���ڴ��С
** �䡡��  : piSize    �����ڴ��С
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID __ifConfSize (INT  *piSize)
{
           INT       iNum = 0;
    struct netif    *pnetif;
    
    for (pnetif = netif_list; pnetif != LW_NULL; pnetif = pnetif->next) {
        iNum += sizeof(struct ifreq);
    }
    
    *piSize = iNum;
}
/*********************************************************************************************************
** ��������: __ifConf
** ��������: �������ӿ��б�����
** �䡡��  : pifconf   �б����滺��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID __ifConf (struct ifconf  *pifconf)
{
           INT           iSize;
           INT           iNum = 0;
    struct netif        *pnetif;
    struct ifreq        *pifreq;
    struct sockaddr_in  *psockaddrin;
    
    iSize = pifconf->ifc_len / sizeof(struct ifreq);                    /*  ����������                  */
    
    pifreq = pifconf->ifc_req;
    for (pnetif = netif_list; pnetif != LW_NULL; pnetif = pnetif->next) {
        if (iNum >= iSize) {
            break;
        }
        pifreq->ifr_name[0] = pnetif->name[0];
        pifreq->ifr_name[1] = pnetif->name[1];
        pifreq->ifr_name[2] = (char)(pnetif->num + '0');
        pifreq->ifr_name[3] = PX_EOS;
        
        psockaddrin = (struct sockaddr_in *)&(pifreq->ifr_addr);
        psockaddrin->sin_len    = sizeof(struct sockaddr_in);
        psockaddrin->sin_family = AF_INET;
        psockaddrin->sin_port   = 0;
        psockaddrin->sin_addr.s_addr = pnetif->ip_addr.addr;
        
        iNum++;
        pifreq++;
    }
    
    pifconf->ifc_len = iNum * sizeof(struct ifreq);                     /*  ��ȡ����                    */
}
/*********************************************************************************************************
** ��������: __ifFindByName
** ��������: ͨ���ӿ�����ȡ�ӿڽṹ
** �䡡��  : pcName    ����ӿ���
** �䡡��  : ����ӿڽṹ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static struct netif *__ifFindByName (CPCHAR  pcName)
{
    struct netif    *pnetif;
    
    for (pnetif = netif_list; pnetif != LW_NULL; pnetif = pnetif->next) {  
        if ((pcName[0] == pnetif->name[0]) &&
            (pcName[1] == pnetif->name[1]) &&
            (pcName[2] == pnetif->num + '0')) {                         /*  ƥ������ӿ�                */
            break;
        }
    }
    
    if (pnetif == LW_NULL) {
        return  (LW_NULL);
    
    } else {
        return  (pnetif);
    }
}
/*********************************************************************************************************
** ��������: __ifFindByName
** ��������: ͨ�� index ��ȡ�ӿڽṹ
** �䡡��  : iIndex   ����ṹ index
** �䡡��  : ����ӿڽṹ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static struct netif *__ifFindByIndex (INT  iIndex)
{
    struct netif    *pnetif;
    
    for (pnetif = netif_list; pnetif != LW_NULL; pnetif = pnetif->next) {  
        if ((int)pnetif->num == iIndex) {
            break;
        }
    }
    
    if (pnetif == LW_NULL) {
        return  (LW_NULL);
    
    } else {
        return  (pnetif);
    }
}
/*********************************************************************************************************
** ��������: __ifReq6Size
** ��������: �������ӿ� IPv6 ��ַ��Ŀ
** �䡡��  : ifreq6    ifreq6 ������ƿ�
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __ifReq6Size (struct in6_ifreq  *pifreq6)
{
    INT              i;
    INT              iNum   = 0;
    struct netif    *pnetif = __ifFindByIndex(pifreq6->ifr6_ifindex);
    
    if (pnetif == LW_NULL) {
        _ErrorHandle(EADDRNOTAVAIL);                                    /*  δ�ҵ�ָ��������ӿ�        */
        return  (PX_ERROR);
    }
    
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        if (ip6_addr_isvalid(pnetif->ip6_addr_state[i])) {
            iNum++;
        }
    }
    
    pifreq6->ifr6_len = iNum * sizeof(struct in6_ifr_addr);             /*  ��дʵ�ʴ�С                */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ifSubIoctlIf
** ��������: ����ӿ� ioctl ���� (��������ӿ�)
** �䡡��  : iCmd      ����
**           pvArg     ����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ifSubIoctlIf (INT  iCmd, PVOID  pvArg)
{
    INT              iRet   = PX_ERROR;
    struct ifreq    *pifreq = LW_NULL;
    struct netif    *pnetif;
    
    pifreq = (struct ifreq *)pvArg;
    
    pnetif = __ifFindByName(pifreq->ifr_name);
    if (pnetif == LW_NULL) {
        _ErrorHandle(EADDRNOTAVAIL);                                    /*  δ�ҵ�ָ��������ӿ�        */
        return  (iRet);
    }

    switch (iCmd) {
    
    case SIOCGIFFLAGS:                                                  /*  ��ȡ���� flag               */
        pifreq->ifr_flags = 0;
        if (pnetif->flags & NETIF_FLAG_UP) {
            pifreq->ifr_flags |= IFF_UP;
        }
        if (pnetif->flags & NETIF_FLAG_BROADCAST) {
            pifreq->ifr_flags |= IFF_BROADCAST;
        }
        if (pnetif->flags & NETIF_FLAG_POINTTOPOINT) {
            pifreq->ifr_flags |= IFF_POINTOPOINT;
        }
        if (pnetif->flags & NETIF_FLAG_LINK_UP) {
            pifreq->ifr_flags |= IFF_RUNNING;
        }
        if (pnetif->flags & NETIF_FLAG_IGMP) {
            pifreq->ifr_flags |= IFF_MULTICAST;
        }
        if ((pnetif->flags & NETIF_FLAG_ETHARP) == 0) {
            pifreq->ifr_flags |= IFF_NOARP;
        }
        if (pnetif->ip_addr.addr == ntohl(INADDR_LOOPBACK)) {
            pifreq->ifr_flags |= IFF_LOOPBACK;
        }
        if ((pnetif->flags2 & NETIF_FLAG2_PROMISC)) {
            pifreq->ifr_flags |= IFF_PROMISC;
        }
        iRet = ERROR_NONE;
        break;
        
    case SIOCSIFFLAGS:                                                  /*  �������� flag               */
        if ((pifreq->ifr_flags & IFF_PROMISC) && 
            !(pnetif->flags2 & NETIF_FLAG2_PROMISC)) {                  /*  ����ģʽ����                */
            if (!pnetif->ioctl) {
                break;
            }
            iRet = pnetif->ioctl(pnetif, SIOCSIFFLAGS, pvArg);
            if (iRet < ERROR_NONE) {
                break;
            }
            pnetif->flags2 |= NETIF_FLAG2_PROMISC;
        
        } else if (!(pifreq->ifr_flags & IFF_PROMISC) && 
                   (pnetif->flags2 & NETIF_FLAG2_PROMISC)) {            /*  ����ģʽ�ر�                */
            if (!pnetif->ioctl) {
                break;
            }
            iRet = pnetif->ioctl(pnetif, SIOCSIFFLAGS, pvArg);
            if (iRet < ERROR_NONE) {
                break;
            }
            pnetif->flags2 &= ~NETIF_FLAG2_PROMISC;
        }
        
        if (pifreq->ifr_flags & IFF_UP) {
            netif_set_up(pnetif);
        } else {
            netif_set_down(pnetif);
        }
        iRet = ERROR_NONE;
        break;
        
    case SIOCGIFTYPE:                                                   /*  �����������                */
        if (pnetif->flags & NETIF_FLAG_POINTTOPOINT) {
            pifreq->ifr_type = IFT_PPP;
        } else if (pnetif->flags & (NETIF_FLAG_ETHERNET | NETIF_FLAG_ETHARP)) {
            pifreq->ifr_type = IFT_ETHER;
        } else if (pnetif->num == 0) {
            pifreq->ifr_type = IFT_LOOP;
        } else {
            pifreq->ifr_type = IFT_OTHER;
        }
        iRet = ERROR_NONE;
        break;
        
    case SIOCGIFINDEX:                                                  /*  ������� index              */
        pifreq->ifr_ifindex = (int)pnetif->num;
        iRet = ERROR_NONE;
        break;
        
    case SIOCGIFMTU:                                                    /*  ������� mtu                */
        pifreq->ifr_mtu = pnetif->mtu;
        iRet = ERROR_NONE;
        break;
        
    case SIOCSIFMTU:                                                    /*  �������� mtu                */
        _ErrorHandle(ENOSYS);
        break;
        
    case SIOCGIFHWADDR:                                                 /*  ���������ַ                */
        if (pnetif->flags & (NETIF_FLAG_ETHERNET | NETIF_FLAG_ETHARP)) {
            INT i;
            for (i = 0; i < IFHWADDRLEN; i++) {
                pifreq->ifr_hwaddr.sa_data[i] = pnetif->hwaddr[i];
            }
            pifreq->ifr_hwaddr.sa_family = ARPHRD_ETHER;
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
        
    case SIOCSIFHWADDR:                                                 /*  ���� mac ��ַ               */
        if (pnetif->flags & (NETIF_FLAG_ETHERNET | NETIF_FLAG_ETHARP)) {
            INT i;
            INT iIsUp = netif_is_up(pnetif);
            if (pnetif->ioctl) {
                netif_set_down(pnetif);                                 /*  �ر�����                    */
                iRet = pnetif->ioctl(pnetif, SIOCSIFHWADDR, pvArg);
                if (iRet == ERROR_NONE) {
                    for (i = 0; i < IFHWADDRLEN; i++) {
                        pnetif->hwaddr[i] = pifreq->ifr_hwaddr.sa_data[i];
                    }
                }
                if (iIsUp) {
                    netif_set_up(pnetif);                               /*  ��������                    */
                }
            } else {
                _ErrorHandle(ENOTSUP);
            }
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
    }
    
    return  iRet;
}
/*********************************************************************************************************
** ��������: __ifSubIoctl4
** ��������: ����ӿ� ioctl ���� (��� ipv4)
** �䡡��  : iCmd      ����
**           pvArg     ����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ifSubIoctl4 (INT  iCmd, PVOID  pvArg)
{
           INT           iRet   = PX_ERROR;
    struct ifreq        *pifreq = LW_NULL;
    struct netif        *pnetif;
    struct sockaddr_in  *psockaddrin;
    
    pifreq = (struct ifreq *)pvArg;
    
    pnetif = __ifFindByName(pifreq->ifr_name);
    if (pnetif == LW_NULL) {
        _ErrorHandle(EADDRNOTAVAIL);                                    /*  δ�ҵ�ָ��������ӿ�        */
        return  (iRet);
    }
    
    switch (iCmd) {                                                     /*  ����Ԥ����                  */
    
    case SIOCGIFADDR:                                                   /*  ��ȡ��ַ����                */
    case SIOCGIFNETMASK:
    case SIOCGIFDSTADDR:
    case SIOCGIFBRDADDR:
        psockaddrin = (struct sockaddr_in *)&(pifreq->ifr_addr);
        psockaddrin->sin_len    = sizeof(struct sockaddr_in);
        psockaddrin->sin_family = AF_INET;
        psockaddrin->sin_port   = 0;
        break;
        
    case SIOCSIFADDR:                                                   /*  ���õ�ַ����                */
    case SIOCSIFNETMASK:
    case SIOCSIFDSTADDR:
    case SIOCSIFBRDADDR:
        psockaddrin = (struct sockaddr_in *)&(pifreq->ifr_addr);
        break;
    }
    
    switch (iCmd) {                                                     /*  �������                  */
        
    case SIOCGIFADDR:                                                   /*  ��ȡ���� IP                 */
        psockaddrin->sin_addr.s_addr = pnetif->ip_addr.addr;
        iRet = ERROR_NONE;
        break;
    
    case SIOCGIFNETMASK:                                                /*  ��ȡ���� mask               */
        psockaddrin->sin_addr.s_addr = pnetif->netmask.addr;
        iRet = ERROR_NONE;
        break;
        
    case SIOCGIFDSTADDR:                                                /*  ��ȡ����Ŀ���ַ            */
        if (pnetif->flags & NETIF_FLAG_POINTTOPOINT) {
            psockaddrin->sin_addr.s_addr = INADDR_ANY;
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
        
    case SIOCGIFBRDADDR:                                                /*  ��ȡ�����㲥��ַ            */
        if (pnetif->flags & NETIF_FLAG_BROADCAST) {
            psockaddrin->sin_addr.s_addr = (pnetif->ip_addr.addr | (~pnetif->netmask.addr));
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
        
    case SIOCSIFADDR:                                                   /*  ����������ַ                */
        if (psockaddrin->sin_family == AF_INET) {
            ip_addr_t ipaddr;
            ipaddr.addr = psockaddrin->sin_addr.s_addr;
            netif_set_ipaddr(pnetif, &ipaddr);
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EAFNOSUPPORT);
        }
        break;
        
    case SIOCSIFNETMASK:                                                /*  ������������                */
        if (psockaddrin->sin_family == AF_INET) {
            ip_addr_t ipaddr;
            ipaddr.addr = psockaddrin->sin_addr.s_addr;
            netif_set_netmask(pnetif, &ipaddr);
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EAFNOSUPPORT);
        }
        break;
        
    case SIOCSIFDSTADDR:                                                /*  ��������Ŀ���ַ            */
        if (pnetif->flags & NETIF_FLAG_POINTTOPOINT) {
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
        
    case SIOCSIFBRDADDR:                                                /*  ���������㲥��ַ            */
        if (pnetif->flags & NETIF_FLAG_BROADCAST) {
            iRet = ERROR_NONE;
        } else {
            _ErrorHandle(EINVAL);
        }
        break;
    }
    
    return  iRet;
}
/*********************************************************************************************************
** ��������: __ifSubIoctl6
** ��������: ����ӿ� ioctl ���� (��� ipv6)
** �䡡��  : iCmd      ����
**           pvArg     ����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ifSubIoctl6 (INT  iCmd, PVOID  pvArg)
{
#define __LWIP_GET_IPV6_FROM_NETIF() \
        pifr6addr->ifr6a_addr.un.u32_addr[0] = pnetif->ip6_addr[i].addr[0]; \
        pifr6addr->ifr6a_addr.un.u32_addr[1] = pnetif->ip6_addr[i].addr[1]; \
        pifr6addr->ifr6a_addr.un.u32_addr[2] = pnetif->ip6_addr[i].addr[2]; \
        pifr6addr->ifr6a_addr.un.u32_addr[3] = pnetif->ip6_addr[i].addr[3];
        
#define __LWIP_SET_IPV6_TO_NETIF() \
        pnetif->ip6_addr[i].addr[0] = pifr6addr->ifr6a_addr.un.u32_addr[0]; \
        pnetif->ip6_addr[i].addr[1] = pifr6addr->ifr6a_addr.un.u32_addr[1]; \
        pnetif->ip6_addr[i].addr[2] = pifr6addr->ifr6a_addr.un.u32_addr[2]; \
        pnetif->ip6_addr[i].addr[3] = pifr6addr->ifr6a_addr.un.u32_addr[3]; 
        
#define __LWIP_CMP_IPV6_WITH_NETIF() \
        (pnetif->ip6_addr[i].addr[0] == pifr6addr->ifr6a_addr.un.u32_addr[0]) && \
        (pnetif->ip6_addr[i].addr[1] == pifr6addr->ifr6a_addr.un.u32_addr[1]) && \
        (pnetif->ip6_addr[i].addr[2] == pifr6addr->ifr6a_addr.un.u32_addr[2]) && \
        (pnetif->ip6_addr[i].addr[3] == pifr6addr->ifr6a_addr.un.u32_addr[3]) 

           INT           i;
           INT           iSize;
           INT           iNum = 0;
           INT           iRet    = PX_ERROR;
    struct in6_ifreq    *pifreq6 = LW_NULL;
    struct in6_ifr_addr *pifr6addr;
    struct netif        *pnetif;
    
    pifreq6 = (struct in6_ifreq *)pvArg;
    
    pnetif = __ifFindByIndex(pifreq6->ifr6_ifindex);
    if (pnetif == LW_NULL) {
        _ErrorHandle(EADDRNOTAVAIL);                                    /*  δ�ҵ�ָ��������ӿ�        */
        return  (iRet);
    }
    
    iSize = pifreq6->ifr6_len / sizeof(struct in6_ifr_addr);            /*  ����������                  */
    pifr6addr = pifreq6->ifr6_addr_array;
    
    switch (iCmd) {                                                     /*  �������                  */
    
    case SIOCGIFADDR6:                                                  /*  ��ȡ���� IP                 */
        for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
            if (iNum >= iSize) {
                break;
            }
            if (ip6_addr_isvalid(pnetif->ip6_addr_state[i])) {
                __LWIP_GET_IPV6_FROM_NETIF();
                if (ip6_addr_isloopback(&pnetif->ip6_addr[i])) {
                    pifr6addr->ifr6a_prefixlen = 128;
                } else if (ip6_addr_islinklocal(&pnetif->ip6_addr[i])) {
                    pifr6addr->ifr6a_prefixlen = 6;
                } else {
                    pifr6addr->ifr6a_prefixlen = 64;                    /*  TODO: Ŀǰ�޷���ȡ          */
                }
                iNum++;
                pifr6addr++;
            }
        }
        pifreq6->ifr6_len = iNum * sizeof(struct in6_ifr_addr);         /*  ��дʵ�ʴ�С                */
        iRet = ERROR_NONE;
        break;
        
    case SIOCSIFADDR6:                                                  /*  ��ȡ���� IP                 */
        if (iSize != 1) {                                               /*  ÿ��ֻ������һ�� IP ��ַ    */
            _ErrorHandle(EOPNOTSUPP);
            break;
        }
        if (IN6_IS_ADDR_LOOPBACK(&pifr6addr->ifr6a_addr) ||
            IN6_IS_ADDR_LINKLOCAL(&pifr6addr->ifr6a_addr)) {            /*  �����ֶ��������������͵ĵ�ַ*/
            _ErrorHandle(EADDRNOTAVAIL);
            break;
        }
        for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {                 /*  ������ͼ����                */
            if (ip6_addr_isinvalid(pnetif->ip6_addr_state[i])) {
                __LWIP_SET_IPV6_TO_NETIF();
                pnetif->ip6_addr_state[i] = IP6_ADDR_VALID | IP6_ADDR_TENTATIVE;
                break;
            }
        }
        if (i >= LWIP_IPV6_NUM_ADDRESSES) {                             /*  �޷�����                    */
            for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {             /*  ���ȸ����ݶ���ַ            */
                if (!ip6_addr_islinklocal(&pnetif->ip6_addr[i]) &&
                    ip6_addr_istentative(pnetif->ip6_addr_state[i])) {
                    __LWIP_SET_IPV6_TO_NETIF();
                    pnetif->ip6_addr_state[i] = IP6_ADDR_VALID | IP6_ADDR_TENTATIVE;
                    break;
                }
            }
        }
        if (i >= LWIP_IPV6_NUM_ADDRESSES) {                             /*  �޷�����                    */
            for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {             /*  �滻�� linklocal ��ַ       */
                if (!ip6_addr_islinklocal(&pnetif->ip6_addr[i])) {
                    __LWIP_SET_IPV6_TO_NETIF();
                    pnetif->ip6_addr_state[i] = IP6_ADDR_VALID | IP6_ADDR_TENTATIVE;
                    break;
                }
            }
        }
        iRet = ERROR_NONE;
        break;
        
    case SIOCDIFADDR6:                                                  /*  ɾ��һ�� IPv6 ��ַ          */
        if (iSize != 1) {                                               /*  ÿ��ֻ������һ�� IP ��ַ    */
            _ErrorHandle(EOPNOTSUPP);
            break;
        }
        for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
            if (ip6_addr_isvalid(pnetif->ip6_addr_state[i])) {
                if (__LWIP_CMP_IPV6_WITH_NETIF()) {                     /*  TODO û���ж�ǰ׺����       */
                    pnetif->ip6_addr_state[i] = IP6_ADDR_INVALID;
                    break;
                }
            }
        }
        iRet = ERROR_NONE;
        break;
        
    default:
        _ErrorHandle(ENOSYS);                                           /*  TODO: ����������δʵ��      */
        break;
    }
    
    return  iRet;
}
/*********************************************************************************************************
** ��������: __ifSubIoctlCommon
** ��������: ͨ������ӿ� ioctl ����
** �䡡��  : iCmd      ����
**           pvArg     ����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ifSubIoctlCommon (INT  iCmd, PVOID  pvArg)
{
    INT     iRet = PX_ERROR;
    
    switch (iCmd) {
    
    case SIOCGIFCONF: {                                                 /*  ��������б�                */
            struct ifconf *pifconf = (struct ifconf *)pvArg;
            __ifConf(pifconf);
            iRet = ERROR_NONE;
            break;
        }
    
    case SIOCGSIZIFCONF: {                                              /*  SIOCGIFCONF ���軺���С    */
            INT  *piSize = (INT *)pvArg;
            __ifConfSize(piSize);
            iRet = ERROR_NONE;
            break;
        }
    
    case SIOCGSIZIFREQ6: {                                              /*  ���ָ������ ipv6 ��ַ����  */
            struct in6_ifreq *pifreq6 = (struct in6_ifreq *)pvArg;
            iRet = __ifReq6Size(pifreq6);
            break;
        }
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __ifIoctlInet
** ��������: INET ����ӿ� ioctl ����
** �䡡��  : iCmd      ����
**           pvArg     ����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ifIoctlInet (INT  iCmd, PVOID  pvArg)
{
    INT     iRet = PX_ERROR;
    
    if (pvArg == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (iRet);
    }
    
    if (iCmd == SIOCGIFNAME) {                                          /*  ���������                  */
        struct ifreq *pifreq = (struct ifreq *)pvArg;
        if (if_indextoname(pifreq->ifr_ifindex, pifreq->ifr_name)) {
            iRet = ERROR_NONE;
        }
        return  (iRet);
    }
    
    switch (iCmd) {                                                     /*  ����Ԥ����                  */
    
    case SIOCGIFCONF:                                                   /*  ͨ������ӿڲ���            */
    case SIOCGSIZIFCONF:
    case SIOCGSIZIFREQ6:
        LWIP_NETIF_LOCK();                                              /*  �����ٽ���                  */
        iRet = __ifSubIoctlCommon(iCmd, pvArg);
        LWIP_NETIF_UNLOCK();                                            /*  �˳��ٽ���                  */
        break;
    
    case SIOCSIFFLAGS:                                                  /*  ��������ӿڲ���            */
    case SIOCGIFFLAGS:
    case SIOCGIFTYPE:
    case SIOCGIFINDEX:
    case SIOCGIFMTU:
    case SIOCSIFMTU:
    case SIOCGIFHWADDR:
    case SIOCSIFHWADDR:
        LWIP_NETIF_LOCK();                                              /*  �����ٽ���                  */
        iRet = __ifSubIoctlIf(iCmd, pvArg);
        LWIP_NETIF_UNLOCK();                                            /*  �˳��ٽ���                  */
        break;
    
    case SIOCGIFADDR:                                                   /*  ipv4 ����                   */
    case SIOCGIFNETMASK:
    case SIOCGIFDSTADDR:
    case SIOCGIFBRDADDR:
    case SIOCSIFADDR:
    case SIOCSIFNETMASK:
    case SIOCSIFDSTADDR:
    case SIOCSIFBRDADDR:
        LWIP_NETIF_LOCK();                                              /*  �����ٽ���                  */
        iRet = __ifSubIoctl4(iCmd, pvArg);
        LWIP_NETIF_UNLOCK();                                            /*  �˳��ٽ���                  */
        break;
        
    case SIOCGIFADDR6:                                                  /*  ipv6 ����                   */
    case SIOCGIFNETMASK6:
    case SIOCGIFDSTADDR6:
    case SIOCSIFADDR6:
    case SIOCSIFNETMASK6:
    case SIOCSIFDSTADDR6:
    case SIOCDIFADDR6:
        LWIP_NETIF_LOCK();                                              /*  �����ٽ���                  */
        iRet = __ifSubIoctl6(iCmd, pvArg);
        LWIP_NETIF_UNLOCK();                                            /*  �˳��ٽ���                  */
        break;
    
    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __ifIoctlWireless
** ��������: WEXT ����ӿ� ioctl ����
** �䡡��  : iCmd      ����
**           pvArg     ����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_NET_WIRELESS_EN > 0

static INT  __ifIoctlWireless (INT  iCmd, PVOID  pvArg)
{
    INT     iRet = PX_ERROR;
    
    if ((iCmd >= SIOCIWFIRST) && (iCmd <= SIOCIWLASTPRIV)) {            /*  ������������                */
        LWIP_NETIF_LOCK();                                              /*  �����ٽ���                  */
        iRet = wext_handle_ioctl(iCmd, (struct ifreq *)pvArg);
        LWIP_NETIF_UNLOCK();                                            /*  �˳��ٽ���                  */
        if (iRet) {
            _ErrorHandle(lib_abs(iRet));
            iRet = PX_ERROR;
        }
    } else {
        _ErrorHandle(ENOSYS);
    }
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_NET_WIRELESS_EN > 0  */
/*********************************************************************************************************
** ��������: __ifIoctlPacket
** ��������: PACKET ����ӿ� ioctl ����
** �䡡��  : iCmd      ����
**           pvArg     ����
** �䡡��  : �������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ifIoctlPacket (INT  iCmd, PVOID  pvArg)
{
    INT     iRet = PX_ERROR;
    
    if (pvArg == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (iRet);
    }
    
    if (iCmd == SIOCGIFNAME) {                                          /*  ���������                  */
        struct ifreq *pifreq = (struct ifreq *)pvArg;
        if (if_indextoname(pifreq->ifr_ifindex, pifreq->ifr_name)) {
            iRet = ERROR_NONE;
        }
        return  (iRet);
    }
    
    switch (iCmd) {                                                     /*  ����Ԥ����                  */
    
    case SIOCGIFCONF:                                                   /*  ͨ������ӿڲ���            */
    case SIOCGSIZIFCONF:
    case SIOCGSIZIFREQ6:
        LWIP_NETIF_LOCK();                                              /*  �����ٽ���                  */
        iRet = __ifSubIoctlCommon(iCmd, pvArg);
        LWIP_NETIF_UNLOCK();                                            /*  �˳��ٽ���                  */
        break;
    
    case SIOCSIFFLAGS:                                                  /*  ��������ӿڲ���            */
    case SIOCGIFFLAGS:
    case SIOCGIFTYPE:
    case SIOCGIFINDEX:
    case SIOCGIFMTU:
    case SIOCSIFMTU:
    case SIOCGIFHWADDR:
    case SIOCSIFHWADDR:
        LWIP_NETIF_LOCK();                                              /*  �����ٽ���                  */
        iRet = __ifSubIoctlIf(iCmd, pvArg);
        LWIP_NETIF_UNLOCK();                                            /*  �˳��ٽ���                  */
        break;
    
    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __socketInit
** ��������: ��ʼ�� sylixos socket ϵͳ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __socketInit (VOID)
{
    static INT              iDrv = 0;
    struct file_operations  fileop;
    
    if (iDrv > 0) {
        return;
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));
    
    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __socketOpen;
    fileop.fo_release  = LW_NULL;
    fileop.fo_open     = __socketOpen;
    fileop.fo_close    = __socketClose;
    fileop.fo_read     = __socketRead;
    fileop.fo_write    = __socketWrite;
    fileop.fo_ioctl    = __socketIoctl;
    fileop.fo_mmap     = __socketMmap;
    fileop.fo_unmap    = __socketUnmap;
    
    iDrv = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_SOCKET);
    if (iDrv < 0) {
        return;
    }
    
    DRIVER_LICENSE(iDrv,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(iDrv,      "SICS");
    DRIVER_DESCRIPTION(iDrv, "lwip socket driver v2.0");
    
    iosDevAddEx(&_G_devhdrSocket, LWIP_SYLIXOS_SOCKET_NAME, iDrv, DT_SOCK);
    
    _G_hSockMutex = API_SemaphoreMCreate("socket_lock", LW_PRIO_DEF_CEILING, 
                                         LW_OPTION_WAIT_FIFO | LW_OPTION_DELETE_SAFE |
                                         LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                         LW_NULL);
    
    _G_hSockSelMutex = API_SemaphoreMCreate("socksel_lock", LW_PRIO_DEF_CEILING, 
                                            LW_OPTION_WAIT_FIFO | LW_OPTION_DELETE_SAFE |
                                            LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                            LW_NULL);
}
/*********************************************************************************************************
** ��������: __socketOpen
** ��������: socket open ����
** �䡡��  : pdevhdr   �豸
**           pcName    ����
**           iFlag     ѡ��
**           mode      δʹ��
** �䡡��  : socket �ṹ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static LONG  __socketOpen (LW_DEV_HDR *pdevhdr, PCHAR  pcName, INT  iFlag, mode_t  mode)
{
    SOCKET_T    *psock;
    
    psock = (SOCKET_T *)__SHEAP_ALLOC(sizeof(SOCKET_T));
    if (psock == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    
    psock->SOCK_iFamily = AF_UNSPEC;
    psock->SOCK_iLwipFd = PX_ERROR;
    psock->SOCK_iHash   = PX_ERROR;
    
    lib_bzero(&psock->SOCK_selwulist, sizeof(LW_SEL_WAKEUPLIST));
    psock->SOCK_selwulist.SELWUL_hListLock = _G_hSockSelMutex;
    
    return  ((LONG)psock);
}
/*********************************************************************************************************
** ��������: __socketClose
** ��������: socket close ����
** �䡡��  : psock     socket �ṹ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __socketClose (SOCKET_T *psock)
{
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        if (psock->SOCK_pafunix) {
            unix_close(psock->SOCK_pafunix);
        }
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */
        
    case AF_PACKET:                                                     /*  PACKET                      */
        if (psock->SOCK_pafpacket) {
            packet_close(psock->SOCK_pafpacket);
        }
        break;
        
    default:                                                            /*  ����ʹ�� lwip               */
        if (psock->SOCK_iLwipFd >= 0) {
            lwip_close(psock->SOCK_iLwipFd);
        }
        break;
    }
    
    SEL_WAKE_UP_TERM(&psock->SOCK_selwulist);
    
    __SOCKET_LOCK();
    if (psock->SOCK_iHash >= 0) {
        _List_Line_Del(&psock->SOCK_lineManage, &_G_plineSocket[psock->SOCK_iHash]);
    }
    __SOCKET_UNLOCK();
    
    __SHEAP_FREE(psock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __socketClose
** ��������: socket read ����
** �䡡��  : psock     socket �ṹ
**           pvBuffer  �����ݻ���
**           stLen     ��������С
** �䡡��  : ���ݸ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __socketRead (SOCKET_T *psock, PVOID  pvBuffer, size_t  stLen)
{
    ssize_t     sstNum = 0;

    switch (psock->SOCK_iFamily) {

#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        if (psock->SOCK_pafunix) {
            sstNum = unix_recvfrom(psock->SOCK_pafunix, pvBuffer, stLen, 0, LW_NULL, LW_NULL);
        }
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        if (psock->SOCK_pafpacket) {
            sstNum = packet_recvfrom(psock->SOCK_pafpacket, pvBuffer, stLen, 0, LW_NULL, LW_NULL);
        }
        break;
        
    default:                                                            /*  ����ʹ�� lwip               */
        if (psock->SOCK_iLwipFd >= 0) {
            sstNum = lwip_read(psock->SOCK_iLwipFd, pvBuffer, stLen);
        }
        break;
    }
    
    return  (sstNum);
}
/*********************************************************************************************************
** ��������: __socketClose
** ��������: socket write ����
** �䡡��  : psock     socket �ṹ
**           pvBuffer  д���ݻ���
**           stLen     д���ݴ�С
** �䡡��  : ���ݸ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __socketWrite (SOCKET_T *psock, CPVOID  pvBuffer, size_t  stLen)
{
    ssize_t     sstNum = 0;

    switch (psock->SOCK_iFamily) {

#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        if (psock->SOCK_pafunix) {
            sstNum = unix_sendto(psock->SOCK_pafunix, pvBuffer, stLen, 0, LW_NULL, 0);
        }
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        if (psock->SOCK_pafpacket) {
            sstNum = packet_sendto(psock->SOCK_pafpacket, pvBuffer, stLen, 0, LW_NULL, 0);
        }
        break;
        
    default:                                                            /*  ����ʹ�� lwip               */
        if (psock->SOCK_iLwipFd >= 0) {
            sstNum = lwip_write(psock->SOCK_iLwipFd, pvBuffer, stLen);
        }
        break;
    }
    
    return  (sstNum);
}
/*********************************************************************************************************
** ��������: __socketClose
** ��������: socket ioctl ����
** �䡡��  : psock     socket �ṹ
**           iCmd      ����
**           pvArg     ����
** �䡡��  : ���ݸ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __socketIoctl (SOCKET_T *psock, INT  iCmd, PVOID  pvArg)
{
           INT                 iRet = PX_ERROR;
    struct stat               *pstatGet;
           PLW_SEL_WAKEUPNODE  pselwun;
           
    if (iCmd == FIOFSTATGET) {
        pstatGet = (struct stat *)pvArg;
        pstatGet->st_dev     = (dev_t)&_G_devhdrSocket;
        pstatGet->st_ino     = (ino_t)0;                                /*  �൱��Ψһ�ڵ�              */
        pstatGet->st_mode    = 0666 | S_IFSOCK;
        pstatGet->st_nlink   = 1;
        pstatGet->st_uid     = 0;
        pstatGet->st_gid     = 0;
        pstatGet->st_rdev    = 1;
        pstatGet->st_size    = 0;
        pstatGet->st_blksize = 0;
        pstatGet->st_blocks  = 0;
        pstatGet->st_atime   = API_RootFsTime(LW_NULL);
        pstatGet->st_mtime   = API_RootFsTime(LW_NULL);
        pstatGet->st_ctime   = API_RootFsTime(LW_NULL);
        return  (ERROR_NONE);
    }
    
    switch (psock->SOCK_iFamily) {
    
    case AF_UNSPEC:                                                     /*  ��Ч                        */
        _ErrorHandle(ENOSYS);
        break;
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        if (psock->SOCK_pafunix) {
            switch (iCmd) {
            
            case FIOSELECT:
                pselwun = (PLW_SEL_WAKEUPNODE)pvArg;
                SEL_WAKE_NODE_ADD(&psock->SOCK_selwulist, pselwun);
                if (__unix_have_event(psock->SOCK_pafunix, 
                                      pselwun->SELWUN_seltypType,
                                      &psock->SOCK_iSoErr)) {
                    SEL_WAKE_UP(pselwun);
                }
                iRet = ERROR_NONE;
                break;
                
            case FIOUNSELECT:
                SEL_WAKE_NODE_DELETE(&psock->SOCK_selwulist, (PLW_SEL_WAKEUPNODE)pvArg);
                iRet = ERROR_NONE;
                break;
                
            default:
                iRet = unix_ioctl(psock->SOCK_pafunix, iCmd, pvArg);
                break;
            }
        }
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */
        
    case AF_PACKET:                                                     /*  PACKET                      */
        if (psock->SOCK_pafpacket) {
            switch (iCmd) {
            
            case FIOSELECT:
                pselwun = (PLW_SEL_WAKEUPNODE)pvArg;
                SEL_WAKE_NODE_ADD(&psock->SOCK_selwulist, pselwun);
                if (__packet_have_event(psock->SOCK_pafpacket, 
                                        pselwun->SELWUN_seltypType,
                                        &psock->SOCK_iSoErr)) {
                    SEL_WAKE_UP(pselwun);
                }
                iRet = ERROR_NONE;
                break;
                
            case FIOUNSELECT:
                SEL_WAKE_NODE_DELETE(&psock->SOCK_selwulist, (PLW_SEL_WAKEUPNODE)pvArg);
                iRet = ERROR_NONE;
                break;
            
            case SIOCGIFCONF:                                           /*  ͨ������ӿڲ���            */
            case SIOCGSIZIFCONF:
            case SIOCGSIZIFREQ6:
            case SIOCSIFFLAGS:
            case SIOCGIFFLAGS:
            case SIOCGIFTYPE:
            case SIOCGIFINDEX:
            case SIOCGIFMTU:
            case SIOCSIFMTU:
            case SIOCGIFHWADDR:
            case SIOCSIFHWADDR:
                iRet = __ifIoctlPacket(iCmd, pvArg);
                break;
                
            default:
                iRet = packet_ioctl(psock->SOCK_pafpacket, iCmd, pvArg);
                break;
            }
        }
        break;
        
    default:                                                            /*  ����ʹ�� lwip               */
        if (psock->SOCK_iLwipFd >= 0) {
            switch (iCmd) {
            
            case FIOSELECT:
                pselwun = (PLW_SEL_WAKEUPNODE)pvArg;
                SEL_WAKE_NODE_ADD(&psock->SOCK_selwulist, pselwun);
                if (__lwip_have_event(psock->SOCK_iLwipFd, 
                                      pselwun->SELWUN_seltypType,
                                      &psock->SOCK_iSoErr)) {
                    SEL_WAKE_UP(pselwun);
                }
                iRet = ERROR_NONE;
                break;
            
            case FIOUNSELECT:
                SEL_WAKE_NODE_DELETE(&psock->SOCK_selwulist, (PLW_SEL_WAKEUPNODE)pvArg);
                iRet = ERROR_NONE;
                break;
                
            case FIOGETFL:
                if (pvArg) {
                    *(int *)pvArg  = lwip_fcntl(psock->SOCK_iLwipFd, F_GETFL, 0);
                    *(int *)pvArg |= O_RDWR;
                }
                iRet = ERROR_NONE;
                break;
                
            case FIOSETFL:
                {
                    INT iIsNonBlk = (INT)((INT)pvArg & O_NONBLOCK);     /*  ����λ���ܴ���              */
                    iRet = lwip_fcntl(psock->SOCK_iLwipFd, F_SETFL, iIsNonBlk);
                }
                break;
                
            case FIONREAD:
                if (pvArg) {
                    *(INT *)pvArg = 0;
                }
                iRet = lwip_ioctl(psock->SOCK_iLwipFd, (long)iCmd, pvArg);
                break;
                
            case SIOCGSIZIFCONF:
            case SIOCGIFCONF:
            case SIOCSIFADDR:
            case SIOCSIFNETMASK:
            case SIOCSIFDSTADDR:
            case SIOCSIFBRDADDR:
            case SIOCSIFFLAGS:
            case SIOCGIFADDR:
            case SIOCGIFNETMASK:
            case SIOCGIFDSTADDR:
            case SIOCGIFBRDADDR:
            case SIOCGIFFLAGS:
            case SIOCGIFTYPE:
            case SIOCGIFNAME:
            case SIOCGIFINDEX:
            case SIOCGIFMTU:
            case SIOCSIFMTU:
            case SIOCGIFHWADDR:
            case SIOCSIFHWADDR:
            case SIOCGSIZIFREQ6:
            case SIOCSIFADDR6:
            case SIOCSIFNETMASK6:
            case SIOCSIFDSTADDR6:
            case SIOCGIFADDR6:
            case SIOCGIFNETMASK6:
            case SIOCGIFDSTADDR6:
            case SIOCDIFADDR6:
                iRet = __ifIoctlInet(iCmd, pvArg);
                break;
            
            default:
#if LW_CFG_NET_WIRELESS_EN > 0
                if ((iCmd >= SIOCIWFIRST) &&
                    (iCmd <= SIOCIWLASTPRIV)) {                         /*  ������������                */
                    iRet = __ifIoctlWireless(iCmd, pvArg);
                } else 
#endif                                                                  /*  LW_CFG_NET_WIRELESS_EN > 0  */
                {
                    iRet = lwip_ioctl(psock->SOCK_iLwipFd, (long)iCmd, pvArg);
                }
                break;
            }
        }
        break;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __socketMmap
** ��������: socket mmap ����
** �䡡��  : psock         socket �ṹ
**           pdmap         ����ռ���Ϣ
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __socketMmap (SOCKET_T *psock, PLW_DEV_MMAP_AREA  pdmap)
{
    INT     iRet;

    if (!pdmap) {
        return  (PX_ERROR);
    }

    switch (psock->SOCK_iFamily) {
        
    case AF_PACKET:                                                     /*  PACKET                      */
        if (psock->SOCK_pafpacket) {
            iRet = packet_mmap(psock->SOCK_pafpacket, pdmap);
        } else {
            iRet = PX_ERROR;
        }
        break;
        
    default:
        _ErrorHandle(ENOTSUP);
        iRet = PX_ERROR;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __socketUnmap
** ��������: socket unmap ����
** �䡡��  : psock         socket �ṹ
**           pdmap         ����ռ���Ϣ
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __socketUnmap (SOCKET_T *psock, PLW_DEV_MMAP_AREA  pdmap)
{
    INT     iRet;

    if (!pdmap) {
        return  (PX_ERROR);
    }

    switch (psock->SOCK_iFamily) {
        
    case AF_PACKET:                                                     /*  PACKET                      */
        if (psock->SOCK_pafpacket) {
            iRet = packet_unmap(psock->SOCK_pafpacket, pdmap);
        } else {
            iRet = PX_ERROR;
        }
        break;
        
    default:
        _ErrorHandle(ENOTSUP);
        iRet = PX_ERROR;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __socketReset
** ��������: socket ��λ����
** �䡡��  : psock         socket �ṹ
** �䡡��  : NONE.
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __socketReset (PLW_FD_ENTRY  pfdentry)
{
    struct linger   lingerReset = {1, 0};
    SOCKET_T       *psock       = (SOCKET_T *)pfdentry->FDENTRY_lValue;
    
    if (psock && 
        ((psock->SOCK_iFamily == AF_INET) || 
        ((psock->SOCK_iFamily == AF_INET6)))) {
        lwip_setsockopt(psock->SOCK_iLwipFd, SOL_SOCKET, SO_LINGER, 
                        &lingerReset, sizeof(struct linger));
    }
}
/*********************************************************************************************************
** ��������: lwip_sendmsg
** ��������: lwip sendmsg
** �䡡��  : lwipfd      lwip �ļ�
**           msg         ��Ϣ
**           flags       flag
** �䡡��  : �������ݳ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  lwip_sendmsg (int  s, const struct msghdr *msg, int flags)
{
	if (msg->msg_iovlen == 1) {
		return  (lwip_sendto(s, msg->msg_iov->iov_base, msg->msg_iov->iov_len, flags,
				             (const struct sockaddr *)msg->msg_name, msg->msg_namelen));
	
	} else {
	    struct iovec    liovec,*msg_iov;
		size_t          msg_iovlen;
		unsigned int    i, totalsize;
		ssize_t         size;
		char           *lbuf;
		char           *temp;
		
		msg_iov    = msg->msg_iov;
		msg_iovlen = msg->msg_iovlen;
		
		for (i = 0, totalsize = 0; i < msg_iovlen; i++) {
		    if ((msg_iov[i].iov_len == 0) || (msg_iov[i].iov_base == LW_NULL)) {
		        _ErrorHandle(EINVAL);
		        return  (PX_ERROR);
		    }
			totalsize += (unsigned int)msg_iov[i].iov_len;
		}
		
		lbuf = (char *)mem_malloc(totalsize);
        if (lbuf == LW_NULL) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
		
		liovec.iov_base = (PVOID)lbuf;
		liovec.iov_len  = (size_t)totalsize;
		
		size = totalsize;
		
		temp = lbuf;
		for (i = 0; size > 0 && i < msg_iovlen; i++) {
			int     qty = msg_iov[i].iov_len;
			lib_memcpy(temp, msg_iov[i].iov_base, qty);
			temp += qty;
			size -= qty;
		}
		
		size = lwip_sendto(s, liovec.iov_base, liovec.iov_len, flags, 
		                   (const struct sockaddr *)msg->msg_name, msg->msg_namelen);
		                   
        mem_free(lbuf);
		
		return  (size);
	}
}
/*********************************************************************************************************
** ��������: lwip_recvmsg
** ��������: lwip recvmsg
** �䡡��  : lwipfd      lwip �ļ�
**           msg         ��Ϣ
**           flags       flag
** �䡡��  : ���յ����ݳ���
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  lwip_recvmsg (int  s, struct msghdr *msg, int flags)
{
    msg->msg_controllen = 0;
	
	if (msg->msg_iovlen == 1) {
		return  (lwip_recvfrom(s, msg->msg_iov->iov_base, msg->msg_iov->iov_len, flags,
				               (struct sockaddr *)msg->msg_name, &msg->msg_namelen));
    
    } else {
        struct iovec    liovec, *msg_iov;
		size_t          msg_iovlen;
		unsigned int    i, totalsize;
		ssize_t         size;
		char           *lbuf;
		char           *temp;
		
		msg_iov    = msg->msg_iov;
		msg_iovlen = msg->msg_iovlen;
		
		for (i = 0, totalsize = 0; i < msg_iovlen; i++) {
		    if ((msg_iov[i].iov_len == 0) || (msg_iov[i].iov_base == LW_NULL)) {
		        _ErrorHandle(EINVAL);
		        return  (PX_ERROR);
		    }
			totalsize += (unsigned int)msg_iov[i].iov_len;
        }
        
        lbuf = (char *)mem_malloc(totalsize);
        if (lbuf == LW_NULL) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
        
		liovec.iov_base = (PVOID)lbuf;
		liovec.iov_len  = (size_t)totalsize;
		
		size = lwip_recvfrom(s, liovec.iov_base, liovec.iov_len, flags, 
		                     (struct sockaddr *)msg->msg_name, &msg->msg_namelen);
		
		temp = lbuf;
		for (i = 0; size > 0 && i < msg_iovlen; i++) {
			size_t   qty = (size_t)((size > msg_iov[i].iov_len) ? msg_iov[i].iov_len : size);
			lib_memcpy(msg_iov[i].iov_base, temp, qty);
			temp += qty;
			size -= qty;
		}
		
		mem_free(lbuf);
		
		return  (size);
    }
}
/*********************************************************************************************************
** ��������: socketpair
** ��������: BSD socketpair
** �䡡��  : domain        ��
**           type          ����
**           protocol      Э��
**           sv            ���������ɶ��ļ�������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  socketpair (int domain, int type, int protocol, int sv[2])
{
#if LW_CFG_NET_UNIX_EN > 0
    INT          iError;
    SOCKET_T    *psock[2];
    
    if (!sv) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (domain != AF_UNIX) {                                            /*  ��֧�� unix ��Э��          */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    sv[0] = socket(AF_UNIX, type, protocol);                            /*  ���� socket                 */
    if (sv[0] < 0) {
        return  (PX_ERROR);
    }
    
    sv[1] = socket(AF_UNIX, type, protocol);                            /*  �����ڶ��� socket           */
    if (sv[1] < 0) {
        close(sv[0]);
        return  (PX_ERROR);
    }
    
    psock[0] = (SOCKET_T *)iosFdValue(sv[0]);
    psock[1] = (SOCKET_T *)iosFdValue(sv[1]);
    
    __KERNEL_SPACE_ENTER();
    iError = unix_connect2(psock[0]->SOCK_pafunix, psock[1]->SOCK_pafunix);
    __KERNEL_SPACE_EXIT();
    
    if (iError < 0) {
        close(sv[0]);
        close(sv[1]);
        return  (PX_ERROR);
    }
    
    MONITOR_EVT_INT5(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_SOCKPAIR, 
                     domain, type, protocol, sv[0], sv[1], LW_NULL);
    
    return  (ERROR_NONE);
    
#else
    _ErrorHandle(ENOSYS);
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */
}
/*********************************************************************************************************
** ��������: socket
** ��������: BSD socket
** �䡡��  : domain    Э����
**           type      ����
**           protocol  Э��
** �䡡��  : fd
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  socket (int domain, int type, int protocol)
{
    INT          iHash;
    INT          iFd     = PX_ERROR;
    INT          iLwipFd = PX_ERROR;
    
    SOCKET_T    *psock     = LW_NULL;
#if LW_CFG_NET_UNIX_EN > 0
    AF_UNIX_T   *pafunix   = LW_NULL;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */
    AF_PACKET_T *pafpacket = LW_NULL;
    
    INT          iCloExec;
    BOOL         iNonBlock;
    
    if (type & SOCK_CLOEXEC) {                                          /*  SOCK_CLOEXEC ?              */
        type &= ~SOCK_CLOEXEC;
        iCloExec = FD_CLOEXEC;
    } else {
        iCloExec = 0;
    }
    
    if (type & SOCK_NONBLOCK) {                                         /*  SOCK_NONBLOCK ?             */
        type &= ~SOCK_NONBLOCK;
        iNonBlock = 1;
    } else {
        iNonBlock = 0;
    }

    __KERNEL_SPACE_ENTER();
    switch (domain) {

#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        pafunix = unix_socket(domain, type, protocol);
        if (pafunix == LW_NULL) {
            __KERNEL_SPACE_EXIT();
            goto    __error_handle;
        }
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        pafpacket = packet_socket(domain, type, protocol);
        if (pafpacket == LW_NULL) {
            __KERNEL_SPACE_EXIT();
            goto    __error_handle;
        }
        break;
    
    case AF_INET:                                                       /*  IPv4 / v6                   */
    case AF_INET6:
        iLwipFd = lwip_socket(domain, type, protocol);
        if (iLwipFd < 0) {
            __KERNEL_SPACE_EXIT();
            goto    __error_handle;
        }
        break;
        
    default:
        _ErrorHandle(EAFNOSUPPORT);
        __KERNEL_SPACE_EXIT();
        goto    __error_handle;
    }
    __KERNEL_SPACE_EXIT();
    
    iFd = open(LWIP_SYLIXOS_SOCKET_NAME, O_RDWR);
    if (iFd < 0) {
        goto    __error_handle;
    }
    psock = (SOCKET_T *)iosFdValue(iFd);
    if (psock == (SOCKET_T *)PX_ERROR) {
        goto    __error_handle;
    }
    psock->SOCK_iFamily = domain;
    
    switch (domain) {

#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        psock->SOCK_pafunix = pafunix;
        iHash = __SOCKET_UNIX_HASH(pafunix);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        psock->SOCK_pafpacket = pafpacket;
        iHash = __SOCKET_PACKET_HASH(pafpacket);
        break;
        
    default:
        psock->SOCK_iLwipFd = iLwipFd;                                  /*  save lwip fd                */
        iHash = __SOCKET_LWIP_HASH(iLwipFd);
        break;
    }
    
    __SOCKET_LOCK();
    psock->SOCK_iHash = iHash;
    _List_Line_Add_Tail(&psock->SOCK_lineManage, &_G_plineSocket[iHash]);
    __SOCKET_UNLOCK();
    
    if (iCloExec) {
        API_IosFdSetCloExec(iFd, iCloExec);
    }
    
    if (iNonBlock) {
        __KERNEL_SPACE_ENTER();
        __socketIoctl(psock, FIONBIO, &iNonBlock);
        __KERNEL_SPACE_EXIT();
    }
    
    MONITOR_EVT_INT4(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_SOCKET, 
                     domain, type, protocol, iFd, LW_NULL);
    
    return  (iFd);
    
__error_handle:
    if (iFd >= 0) {
        close(iFd);
    }
    
#if LW_CFG_NET_UNIX_EN > 0
    if (pafunix) {
        unix_close(pafunix);
    }
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    if (pafpacket) {
        packet_close(pafpacket);
        
    } else if (iLwipFd >= 0) {
        lwip_close(iLwipFd);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: accept4
** ��������: BSD accept4
** �䡡��  : s         socket fd
**           addr      address
**           addrlen   address len
**           flags     SOCK_CLOEXEC, SOCK_NONBLOCK
** �䡡��  : new socket fd
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  accept4 (int s, struct sockaddr *addr, socklen_t *addrlen, int flags)
{
    INT          iHash;
    SOCKET_T    *psock   = (SOCKET_T *)iosFdValue(s);
    SOCKET_T    *psockNew;
    INT          iType;
    
#if LW_CFG_NET_UNIX_EN > 0
    AF_UNIX_T   *pafunix   = LW_NULL;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    AF_PACKET_T *pafpacket = LW_NULL;
    INT          iRet      = PX_ERROR;
    INT          iFdNew    = PX_ERROR;
    
    INT          iCloExec;
    BOOL         iNonBlock;
    
    __SOCKET_CHECHK();
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    if (flags & SOCK_CLOEXEC) {                                         /*  SOCK_CLOEXEC ?              */
        iCloExec = FD_CLOEXEC;
    } else {
        iCloExec = 0;
    }
    
    if (flags & SOCK_NONBLOCK) {                                        /*  SOCK_NONBLOCK ?             */
        iNonBlock = 1;
    } else {
        iNonBlock = 0;
    }
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        pafunix = unix_accept(psock->SOCK_pafunix, addr, addrlen);
        if (pafunix == LW_NULL) {
            __KERNEL_SPACE_EXIT();
            goto    __error_handle;
        }
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */
        
    case AF_PACKET:                                                     /*  PACKET                      */
        pafpacket = packet_accept(psock->SOCK_pafpacket, addr, addrlen);
        if (pafpacket == LW_NULL) {
            __KERNEL_SPACE_EXIT();
            goto    __error_handle;
        }
        break;
        
    default:
        iRet = lwip_accept(psock->SOCK_iLwipFd, addr, addrlen);         /*  lwip_accept                 */
        if (iRet < 0) {
            __KERNEL_SPACE_EXIT();
            goto    __error_handle;
        }
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    iFdNew = open(LWIP_SYLIXOS_SOCKET_NAME, O_RDWR);                    /*  new fd                      */
    if (iFdNew < 0) {
        goto    __error_handle;
    }
    psockNew = (SOCKET_T *)iosFdValue(iFdNew);
    psockNew->SOCK_iFamily = psock->SOCK_iFamily;
    
    switch (psockNew->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        psockNew->SOCK_pafunix = pafunix;
        iHash = __SOCKET_UNIX_HASH(pafunix);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        psockNew->SOCK_pafpacket = pafpacket;
        iHash = __SOCKET_PACKET_HASH(pafpacket);
        break;
        
    default:
        psockNew->SOCK_iLwipFd = iRet;                                  /*  save lwip fd                */
        iHash = __SOCKET_LWIP_HASH(iRet);
        break;
    }
    
    __SOCKET_LOCK();
    psockNew->SOCK_iHash = iHash;
    _List_Line_Add_Tail(&psockNew->SOCK_lineManage, &_G_plineSocket[iHash]);
    __SOCKET_UNLOCK();
    
    if (iCloExec) {
        API_IosFdSetCloExec(iFdNew, iCloExec);
    }
    
    if (iNonBlock) {
        __KERNEL_SPACE_ENTER();
        __socketIoctl(psockNew, FIONBIO, &iNonBlock);
        __KERNEL_SPACE_EXIT();
    }
    
    MONITOR_EVT_INT2(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_ACCEPT, 
                     s, iFdNew, LW_NULL);
    
    return  (iFdNew);
    
__error_handle:
    psock->SOCK_iSoErr = errno;
    
#if LW_CFG_NET_UNIX_EN > 0
    if (pafunix) {
        unix_close(pafunix);
    }
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */
    
    if (pafpacket) {
        packet_close(pafpacket);
    
    } else if (iRet >= 0) {
        lwip_close(iRet);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: accept
** ��������: BSD accept
** �䡡��  : s         socket fd
**           addr      address
**           addrlen   address len
** �䡡��  : new socket fd
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  accept (int s, struct sockaddr *addr, socklen_t *addrlen)
{
    return  (accept4(s, addr, addrlen, 0));
}
/*********************************************************************************************************
** ��������: bind
** ��������: BSD bind
** �䡡��  : s         socket fd
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  bind (int s, const struct sockaddr *name, socklen_t namelen)
{
    SOCKET_T   *psock = (SOCKET_T *)iosFdValue(s);
    INT         iType;
    INT         iRet = PX_ERROR;
    
    __SOCKET_CHECHK();
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {

#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        iRet = unix_bind(psock->SOCK_pafunix, name, namelen);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        iRet = packet_bind(psock->SOCK_pafpacket, name, namelen);
        break;
        
    default:
        iRet = lwip_bind(psock->SOCK_iLwipFd, name, namelen);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (iRet < ERROR_NONE) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_INT1(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_BIND, 
                         s, LW_NULL);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: bind
** ��������: BSD bind
** �䡡��  : s         socket fd
**           how       SHUT_RD  SHUT_RDWR  SHUT_WR
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  shutdown (int s, int how)
{
    SOCKET_T   *psock = (SOCKET_T *)iosFdValue(s);
    INT         iType;
    INT         iRet = PX_ERROR;
    
    __SOCKET_CHECHK();
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        iRet = unix_shutdown(psock->SOCK_pafunix, how);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        iRet = packet_shutdown(psock->SOCK_pafpacket, how);
        break;
        
    default:
        iRet = lwip_shutdown(psock->SOCK_iLwipFd, how);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (iRet < ERROR_NONE) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_INT2(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_SHUTDOWN, 
                         s, how, LW_NULL);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: connect
** ��������: BSD connect
** �䡡��  : s         socket fd
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  connect (int s, const struct sockaddr *name, socklen_t namelen)
{
    SOCKET_T   *psock = (SOCKET_T *)iosFdValue(s);
    INT         iType;
    INT         iRet = PX_ERROR;
    
    __SOCKET_CHECHK();
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        iRet = unix_connect(psock->SOCK_pafunix, name, namelen);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        iRet = packet_connect(psock->SOCK_pafpacket, name, namelen);
        break;
        
    default:
        iRet = lwip_connect(psock->SOCK_iLwipFd, name, namelen);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (iRet < ERROR_NONE) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_INT1(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_CONNECT, 
                         s, LW_NULL);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: getsockname
** ��������: BSD getsockname
** �䡡��  : s         socket fd
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  getsockname (int s, struct sockaddr *name, socklen_t *namelen)
{
    SOCKET_T   *psock = (SOCKET_T *)iosFdValue(s);
    INT         iType;
    INT         iRet = PX_ERROR;
    
    __SOCKET_CHECHK();
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        iRet = unix_getsockname(psock->SOCK_pafunix, name, namelen);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        iRet = packet_getsockname(psock->SOCK_pafpacket, name, namelen);
        break;
        
    default:
        iRet = lwip_getsockname(psock->SOCK_iLwipFd, name, namelen);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (iRet < ERROR_NONE) {
        psock->SOCK_iSoErr = errno;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: getpeername
** ��������: BSD getpeername
** �䡡��  : s         socket fd
**           name      address
**           namelen   address len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  getpeername (int s, struct sockaddr *name, socklen_t *namelen)
{
    SOCKET_T   *psock = (SOCKET_T *)iosFdValue(s);
    INT         iType;
    INT         iRet = PX_ERROR;
    
    __SOCKET_CHECHK();
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        iRet = unix_getpeername(psock->SOCK_pafunix, name, namelen);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        iRet = packet_getpeername(psock->SOCK_pafpacket, name, namelen);
        break;
        
    default:
        iRet = lwip_getpeername(psock->SOCK_iLwipFd, name, namelen);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (iRet < ERROR_NONE) {
        psock->SOCK_iSoErr = errno;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: setsockopt
** ��������: BSD setsockopt
** �䡡��  : s         socket fd
**           level     level
**           optname   option
**           optval    option value
**           optlen    option value len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  setsockopt (int s, int level, int optname, const void *optval, socklen_t optlen)
{
    SOCKET_T   *psock = (SOCKET_T *)iosFdValue(s);
    INT         iType;
    INT         iRet = PX_ERROR;
    
    __SOCKET_CHECHK();
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        iRet = unix_setsockopt(psock->SOCK_pafunix, level, optname, optval, optlen);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        iRet = packet_setsockopt(psock->SOCK_pafpacket, level, optname, optval, optlen);
        break;
        
    default:
        iRet = lwip_setsockopt(psock->SOCK_iLwipFd, level, optname, optval, optlen);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (iRet < ERROR_NONE) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_INT3(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_SOCKOPT, 
                         s, level, optname, LW_NULL);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: getsockopt
** ��������: BSD getsockopt
** �䡡��  : s         socket fd
**           level     level
**           optname   option
**           optval    option value
**           optlen    option value len
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  getsockopt (int s, int level, int optname, void *optval, socklen_t *optlen)
{
    SOCKET_T   *psock = (SOCKET_T *)iosFdValue(s);
    INT         iType;
    INT         iRet = PX_ERROR;
    
    __SOCKET_CHECHK();
    
    if (level == SOL_SOCKET) {                                          /*  ͳһ���� SO_ERROR           */
        if (optname == SO_ERROR) {
            if (!optval || *optlen < sizeof(INT)) {
                _ErrorHandle(EINVAL);
                return  (iRet);
            }
            *(INT *)optval = psock->SOCK_iSoErr;
            psock->SOCK_iSoErr = ERROR_NONE;
            return  (ERROR_NONE);
        }
    }
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        iRet = unix_getsockopt(psock->SOCK_pafunix, level, optname, optval, optlen);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        iRet = packet_getsockopt(psock->SOCK_pafpacket, level, optname, optval, optlen);
        break;
        
    default:
        iRet = lwip_getsockopt(psock->SOCK_iLwipFd, level, optname, optval, optlen);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (iRet < ERROR_NONE) {
        psock->SOCK_iSoErr = errno;
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: listen
** ��������: BSD listen
** �䡡��  : s         socket fd
**           backlog   back log num
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int listen (int s, int backlog)
{
    SOCKET_T   *psock = (SOCKET_T *)iosFdValue(s);
    INT         iType;
    INT         iRet = PX_ERROR;
    
    __SOCKET_CHECHK();
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        iRet = unix_listen(psock->SOCK_pafunix, backlog);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        iRet = packet_listen(psock->SOCK_pafpacket, backlog);
        break;
        
    default:
        iRet = lwip_listen(psock->SOCK_iLwipFd, backlog);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (iRet < ERROR_NONE) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_INT2(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_LISTEN, 
                         s, backlog, LW_NULL);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: recv
** ��������: BSD recv
** �䡡��  : s         socket fd
**           mem       buffer
**           len       buffer len
**           flags     flag
** �䡡��  : NUM
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  recv (int s, void *mem, size_t len, int flags)
{
    SOCKET_T   *psock = (SOCKET_T *)iosFdValue(s);
    INT         iType;
    ssize_t     sstRet = PX_ERROR;
    
    __SOCKET_CHECHK();
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        sstRet = (ssize_t)unix_recv(psock->SOCK_pafunix, mem, len, flags);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        sstRet = (ssize_t)packet_recv(psock->SOCK_pafpacket, mem, len, flags);
        break;
        
    default:
        sstRet = (ssize_t)lwip_recv(psock->SOCK_iLwipFd, mem, len, flags);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (sstRet <= 0) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_RECV, 
                          s, flags, sstRet, LW_NULL);
    }
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: recvfrom
** ��������: BSD recvfrom
** �䡡��  : s         socket fd
**           mem       buffer
**           len       buffer len
**           flags     flag
**           from      packet from
**           fromlen   name len
** �䡡��  : NUM
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  recvfrom (int s, void *mem, size_t len, int flags,
                   struct sockaddr *from, socklen_t *fromlen)
{
    SOCKET_T   *psock = (SOCKET_T *)iosFdValue(s);
    INT         iType;
    ssize_t     sstRet = PX_ERROR;
    
    __SOCKET_CHECHK();
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        sstRet = (ssize_t)unix_recvfrom(psock->SOCK_pafunix, mem, len, flags, from, fromlen);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        sstRet = (ssize_t)packet_recvfrom(psock->SOCK_pafpacket, mem, len, flags, from, fromlen);
        break;
        
    default:
        sstRet = (ssize_t)lwip_recvfrom(psock->SOCK_iLwipFd, mem, len, flags, from, fromlen);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (sstRet <= 0) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_RECV, 
                          s, flags, sstRet, LW_NULL);
    }
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: recvmsg
** ��������: BSD recvmsg
** �䡡��  : s             �׽���
**           msg           ��Ϣ
**           flags         �����־
** �䡡��  : NUM (�˳��Ȳ�����������Ϣ����)
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ssize_t  recvmsg (int  s, struct msghdr *msg, int flags)
{
    SOCKET_T   *psock = (SOCKET_T *)iosFdValue(s);
    INT         iType;
    ssize_t     sstRet = PX_ERROR;
    
    __SOCKET_CHECHK();
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        sstRet = (ssize_t)unix_recvmsg(psock->SOCK_pafunix, msg, flags);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        sstRet = (ssize_t)packet_recvmsg(psock->SOCK_pafpacket, msg, flags);
        break;
        
    default:
        sstRet = (ssize_t)lwip_recvmsg(psock->SOCK_iLwipFd, msg, flags);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (sstRet <= 0) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_RECV, 
                          s, flags, sstRet, LW_NULL);
    }
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: send
** ��������: BSD send
** �䡡��  : s         socket fd
**           data      send buffer
**           size      send len
**           flags     flag
** �䡡��  : NUM
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  send (int s, const void *data, size_t size, int flags)
{
    SOCKET_T   *psock = (SOCKET_T *)iosFdValue(s);
    INT         iType;
    ssize_t     sstRet = PX_ERROR;
    
    __SOCKET_CHECHK();
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        sstRet = (ssize_t)unix_send(psock->SOCK_pafunix, data, size, flags);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        sstRet = (ssize_t)packet_send(psock->SOCK_pafpacket, data, size, flags);
        break;
        
    default:
        sstRet = (ssize_t)lwip_send(psock->SOCK_iLwipFd, data, size, flags);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (sstRet <= 0) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_SEND, 
                          s, flags, sstRet, LW_NULL);
    }
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: sendto
** ��������: BSD sendto
** �䡡��  : s         socket fd
**           data      send buffer
**           size      send len
**           flags     flag
**           to        packet to
**           tolen     name len
** �䡡��  : NUM
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ssize_t  sendto (int s, const void *data, size_t size, int flags,
                 const struct sockaddr *to, socklen_t tolen)
{
    SOCKET_T   *psock = (SOCKET_T *)iosFdValue(s);
    INT         iType;
    ssize_t     sstRet = PX_ERROR;
    
    __SOCKET_CHECHK();
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        sstRet = (ssize_t)unix_sendto(psock->SOCK_pafunix, data, size, flags, to, tolen);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        sstRet = (ssize_t)packet_sendto(psock->SOCK_pafpacket, data, size, flags, to, tolen);
        break;
        
    default:
        sstRet = (ssize_t)lwip_sendto(psock->SOCK_iLwipFd, data, size, flags, to, tolen);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (sstRet <= 0) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_SEND, 
                          s, flags, sstRet, LW_NULL);
    }
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: sendmsg
** ��������: BSD sendmsg
** �䡡��  : s             �׽���
**           msg           ��Ϣ
**           flags         �����־
** �䡡��  : NUM (�˳��Ȳ�����������Ϣ����)
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ssize_t  sendmsg (int  s, const struct msghdr *msg, int flags)
{
    SOCKET_T   *psock = (SOCKET_T *)iosFdValue(s);
    INT         iType;
    ssize_t     sstRet = PX_ERROR;
    
    __SOCKET_CHECHK();
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    __KERNEL_SPACE_ENTER();
    switch (psock->SOCK_iFamily) {
    
#if LW_CFG_NET_UNIX_EN > 0
    case AF_UNIX:                                                       /*  UNIX ��Э��                 */
        sstRet = (ssize_t)unix_sendmsg(psock->SOCK_pafunix, msg, flags);
        break;
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    case AF_PACKET:                                                     /*  PACKET                      */
        sstRet = (ssize_t)packet_sendmsg(psock->SOCK_pafpacket, msg, flags);
        break;
        
    default:
        sstRet = (ssize_t)lwip_sendmsg(psock->SOCK_iLwipFd, msg, flags);
        break;
    }
    __KERNEL_SPACE_EXIT();
    
    if (sstRet <= 0) {
        psock->SOCK_iSoErr = errno;
    
    } else {
        MONITOR_EVT_LONG3(MONITOR_EVENT_ID_NETWORK, MONITOR_EVENT_NETWORK_SEND, 
                          s, flags, sstRet, LW_NULL);
    }
    
    return  (sstRet);
}
/*********************************************************************************************************
** ��������: gethostbyname
** ��������: BSD gethostbyname
** �䡡��  : name      domain name
** �䡡��  : hostent
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
struct hostent  *gethostbyname (const char *name)
{
    return  (lwip_gethostbyname(name));
}
/*********************************************************************************************************
** ��������: gethostbyname_r
** ��������: BSD gethostbyname_r
** �䡡��  : name      domain name
**           ret       hostent buffer
**           buf       result buffer
**           buflen    buf len
**           result    result return
**           h_errnop  error
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int gethostbyname_r (const char *name, struct hostent *ret, char *buf,
                     size_t buflen, struct hostent **result, int *h_errnop)
{
    return  (lwip_gethostbyname_r(name, ret, buf, buflen, result, h_errnop));
}
/*********************************************************************************************************
** ��������: gethostbyaddr_r
** ��������: BSD gethostbyname_r
** �䡡��  : addr      domain addr
**           length    socketaddr len
**           type      AF_INET
** �䡡��  : hostent
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
struct hostent *gethostbyaddr (const void *addr, socklen_t length, int type)
{
    errno = ENOSYS;
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: gethostbyaddr_r
** ��������: BSD gethostbyname_r
** �䡡��  : addr      domain addr
**           length    socketaddr len
**           type      AF_INET
**           ret       hostent buffer
**           buf       result buffer
**           buflen    buf len
**           result    result return
**           h_errnop  error
** �䡡��  : hostent
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
struct hostent *gethostbyaddr_r (const void *addr, socklen_t length, int type,
                                 struct hostent *ret, char  *buffer, int buflen, int *h_errnop)
{
    errno = ENOSYS;
    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: freeaddrinfo
** ��������: BSD freeaddrinfo
** �䡡��  : ai        addrinfo
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
void  freeaddrinfo (struct addrinfo *ai)
{
    lwip_freeaddrinfo(ai);
}
/*********************************************************************************************************
** ��������: getaddrinfo
** ��������: BSD getaddrinfo
** �䡡��  : nodename  node name
**           servname  server name
**           hints     addrinfo
**           res       result
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
int  getaddrinfo (const char *nodename, const char *servname,
                  const struct addrinfo *hints, struct addrinfo **res)
{
    return  (lwip_getaddrinfo(nodename, servname, hints, res));
}
#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/