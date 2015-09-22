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
** ��   ��   ��: _ThreadShell.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 13 ��
**
** ��        ��: ����ϵͳ�̵߳���ǡ�

** BUG
2007.11.04  �� 0xFFFFFFFF ��Ϊ __ARCH_ULONG_MAX.
2008.01.16  API_ThreadDelete() -> API_ThreadForceDelete();
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _BuidTCB
** ��������: ����һ��TCB
** �䡡��  : pvThreadStartAddress              �̴߳������ʼ��ַ
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PVOID  _ThreadShell (PVOID  pvThreadStartAddress)
{
    PLW_CLASS_TCB       ptcbCur;
    PVOID               pvReturnVal;
    LW_OBJECT_HANDLE    ulId;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
    pvReturnVal = ((PTHREAD_START_ROUTINE)pvThreadStartAddress)
                  (ptcbCur->TCB_pvArg);                                 /*  ִ���߳�                    */

    ulId = ptcbCur->TCB_ulId;

#if LW_CFG_THREAD_DEL_EN > 0
    API_ThreadForceDelete(&ulId, pvReturnVal);                          /*  ɾ���߳�                    */
#endif

#if LW_CFG_THREAD_SUSPEND_EN > 0
    API_ThreadSuspend(ulId);                                            /*  �����߳�                    */
#endif

    for (;;) {
        API_TimeSleep(__ARCH_ULONG_MAX);                                /*  ˯��                        */
    }
    
    return  (LW_NULL);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/