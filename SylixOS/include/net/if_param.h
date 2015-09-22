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
** ��   ��   ��: if_param.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2015 �� 09 �� 20 ��
**
** ��        ��: ����ӿ����ò�����ȡ.
*********************************************************************************************************/

#ifndef __IF_PARAM_H
#define __IF_PARAM_H

#include "lwip/inet.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

/*********************************************************************************************************
  ��������ļ���ʽ���� /etc/ifparam.ini

  [dm9000a]
  enable=1
  ipaddr=192.168.1.2
  netmask=255.255.255.0
  gateway=192.168.1.1
  default=1
  mac=00:11:22:33:44:55

  resolver ��������ļ����� /etc/resolv.conf

  nameserver x.x.x.x
*********************************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

LW_API void  *if_param_load(const char *name);
LW_API void   if_param_unload(void *pifparam);
LW_API int    if_param_getenable(void *pifparam, int *enable);
LW_API int    if_param_getdefault(void *pifparam, int *def);
LW_API int    if_param_getipaddr(void *pifparam, ip_addr_t *ipaddr);
LW_API int    if_param_getnetmask(void *pifparam, ip_addr_t *mask);
LW_API int    if_param_getgw(void *pifparam, ip_addr_t *gw);
LW_API int    if_param_getmac(void *pifparam, char *mac, size_t  sz);
LW_API void   if_param_syncdns(void);

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  LW_CFG_NET_EN               */
#endif                                                                  /*  __IF_ETHER_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
