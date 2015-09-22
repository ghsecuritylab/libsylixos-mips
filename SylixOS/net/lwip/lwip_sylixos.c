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
** ��   ��   ��: lwip_sylixos.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 05 �� 06 ��
**
** ��        ��: lwip sylixos �ӿ�.

** BUG:
2009.05.20  _netJobqueueInit ��ʼ��Ӧ���������ʼ���ĺ���, ���Ӱ�ȫ.
2009.07.11  API_NetInit() ��������ʼ��һ��.
2009.08.19  ȥ�� snmp init ����������� lwip 1.3.1 �Ժ�汾���Զ���ʼ��.
2010.02.22  ���� lwip ��ȥ�� loopif �ĳ�ʼ��(Э��ջ���а�װ�ػ�����).
2010.07.28  ���� snmp ʱ����ص��Ͷ����ĳ�ʼ������.
2010.11.01  __netCloseAll() ���� DHCP ����ʹ�ð�ȫģʽ.
2012.12.18  ���� unix_init() ��ʼ�� AF_UNIX ��Э��.
2013.06.21  �������� proc �ļ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0
#include "lwip/tcpip.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/snmp.h"
#include "lwip/snmp_msg.h"
#include "lwip/sockets.h"
#include "./unix/af_unix.h"
#include "./packet/af_packet.h"
#if LW_CFG_PROCFS_EN > 0
#include "./proc/lwip_proc.h"
#endif                                                                  /*  LW_CFG_PROCFS_EN            */
/*********************************************************************************************************
  ���������������к�������
*********************************************************************************************************/
INT   _netJobqueueInit(VOID);
/*********************************************************************************************************
  ���纯������
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0
VOID  __tshellNetInit(VOID);
VOID  __tshellNet6Init(VOID);
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  ��������������
*********************************************************************************************************/
VOID  __inetHostTableInit(VOID);
/*********************************************************************************************************
  �����¼���ʼ��
*********************************************************************************************************/
INT   _netEventInit(VOID);
INT   _netEventDevCreate(VOID);
/*********************************************************************************************************
  �������纯������
*********************************************************************************************************/
#if LW_CFG_LWIP_PPP > 0 || LW_CFG_LWIP_PPPOE > 0
#if __LWIP_USE_PPP_NEW == 0
err_t pppInit(void);
#endif
#endif                                                                  /*  LW_CFG_LWIP_PPP             */
                                                                        /*  LW_CFG_LWIP_PPPOE           */
/*********************************************************************************************************
** ��������: __netCloseAll
** ��������: ϵͳ������ر�ʱ�ص�����.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __netCloseAll (VOID)
{
    PLW_DEV_HDR      pdevhdr;
    PCHAR            pcTail;
    
#if LWIP_DHCP > 0
    struct netif    *netif = netif_list;
#endif                                                                  /*  LWIP_DHCP > 0               */
    
    pdevhdr = iosDevFind(LWIP_SYLIXOS_SOCKET_NAME, &pcTail);
    if (pdevhdr) {
        iosDevFileAbnormal(pdevhdr);                                    /*  �ر����������ļ�            */
    }                                                                   /*  ͬʱ������ļ�����Ϊ�쳣ģʽ*/
    
#if LWIP_DHCP > 0
    for (; netif != LW_NULL; netif = netif->next) {
        if (netif->dhcp && netif->dhcp->pcb) {
            netifapi_netif_common(netif, NULL, dhcp_release);           /*  ��� DHCP ��Լ, ͬʱֹͣ����*/
            netifapi_dhcp_stop(netif);                                  /*  �ͷ���Դ                    */
        }
    }
#endif                                                                  /*  LWIP_DHCP > 0               */
}
/*********************************************************************************************************
** ��������: __netSnmpTimestamp
** ��������: ��ȡ SNMP ʱ���.
** �䡡��  : puiTimestamp       SNMP ʱ��� (�� 10ms Ϊ��λ!)
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __netSnmpGetTimestamp (UINT32  *puiTimestamp)
{
    INT64       i64Ticks = API_TimeGet64();                             /*  ��ȡϵͳʱ��                */
    
    if (puiTimestamp) {
        /*
         *  uiTimestamp ���� 10 ����Ϊ��λ
         */
        *puiTimestamp = (UINT32)((i64Ticks * LW_TICK_HZ * 10) / 1000);
    }
}
/*********************************************************************************************************
** ��������: __netSnmpInit
** ��������: ��ʼ�� SNMP Ĭ����Ϣ.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_SNMP > 0

static VOID  __netSnmpInit (VOID)
{
    static u8_t     ucContactLen  = 17;
    static u8_t     ucLocationLen = 6;
    
    static u8_t     ucDesrLen = 7;
    static u8_t     ucNameLen = 23;
    
    snmp_set_syscontact((u8_t *)"sylixos@gmail.com", &ucContactLen);
    snmp_set_syslocation((u8_t *)"@china", &ucLocationLen);             /*  at CHINA ^_^                */
    
    snmp_set_sysdescr((u8_t *)"sylixos", &ucDesrLen);
    snmp_set_sysname((u8_t *)"device based on sylixos", &ucNameLen);
}

#endif                                                                  /*  LWIP_SNMP > 0               */
/*********************************************************************************************************
** ��������: API_NetInit
** ��������: �����ϵͳ�ں�ע���������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
VOID  API_NetInit (VOID)
{
    static BOOL            bIsInit = LW_FALSE;
    
    if (bIsInit) {
        return;
    } else {
        bIsInit = LW_TRUE;
    }
    
    _netJobqueueInit();                                                 /*  ��������������ҵ��������    */
    _netEventInit();                                                    /*  ��ʼ�������¼�              */
    _netEventDevCreate();                                               /*  ���������¼��豸            */
    
    API_SystemHookAdd((LW_HOOK_FUNC)__netCloseAll, 
                      LW_OPTION_KERNEL_REBOOT);                         /*  ��װϵͳ�ر�ʱ, �ص�����    */
                      
    tcpip_init(LW_NULL, LW_NULL);                                       /*  �Զ�������ʽ��ʼ�� lwip     */
    
#if LW_CFG_NET_UNIX_EN > 0
    unix_init();                                                        /*  ��ʼ�� AF_UNIX ��Э��       */
#endif                                                                  /*  LW_CFG_NET_UNIX_EN > 0      */

    packet_init();                                                      /*  ��ʼ�� AF_PACKET Э��       */
    
    __socketInit();                                                     /*  ��ʼ�� socket ϵͳ          */

#if LW_CFG_LWIP_PPP > 0 || LW_CFG_LWIP_PPPOE > 0
#if __LWIP_USE_PPP_NEW == 0
    pppInit();                                                          /*  ��ʼ�� point to point proto */
#endif                                                                  /*  !__LWIP_USE_PPP_NEW         */
#endif                                                                  /*  LW_CFG_LWIP_PPP             */
                                                                        /*  LW_CFG_LWIP_PPPOE           */
    
#if LWIP_SNMP > 0
    __netSnmpInit();                                                    /*  ��ʼ�� SNMP ������Ϣ        */
#endif                                                                  /*  LWIP_SNMP > 0               */
    
#if LW_CFG_SHELL_EN > 0
    __tshellNetInit();                                                  /*  ע����������                */
    __tshellNet6Init();                                                 /*  ע�� IPv6 ר������          */
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
    
    /*
     *  ������ع��߳�ʼ��.
     */
    __inetHostTableInit();                                              /*  ��ʼ�����ص�ַת����        */
    
#if LW_CFG_PROCFS_EN > 0
    __procFsNetInit();
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
}
#endif                                                                  /*  LW_CFG_NET_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/