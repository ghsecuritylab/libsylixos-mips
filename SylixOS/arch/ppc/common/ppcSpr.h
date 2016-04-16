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
** ��   ��   ��: ppcSpr.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 17 ��
**
** ��        ��: PowerPC ��ϵ�������⹦�ܼĴ����ӿ�.
*********************************************************************************************************/

#ifndef __ARCH_PPCSPR_H
#define __ARCH_PPCSPR_H

extern UINT32  ppcGetMSR(VOID);
extern UINT32  ppcGetDAR(VOID);

extern VOID    ppcSetDEC(UINT32);
extern UINT32  ppcGetDEC(VOID);

#endif                                                                  /*  __ARCH_PPCSPR_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/