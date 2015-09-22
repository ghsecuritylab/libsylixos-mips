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
** ��   ��   ��: SemaphoreMStatus.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 21 ��
**
** ��        ��: ��ѯ�����ź���״̬

** BUG
2007.07.21  ���� _DebugHandle() ���ܡ�
2008.03.04  �޸Ĵ����ʽ��
2008.05.18  ʹ�� __KERNEL_ENTER() ���� ThreadLock();
2009.04.08  ����� SMP ��˵�֧��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_SemaphoreMStatus
** ��������: ��ѯ�����ź���״̬
** �䡡��  : 
**           ulId                   �¼����
**           pbValue                �¼�����ֵ         ����ΪNULL
**           pulOption              �¼�ѡ��ָ��       ����ΪNULL
**           pulThreadBlockNum      ���������߳�����   ����ΪNULL
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if (LW_CFG_SEMM_EN > 0) && (LW_CFG_MAX_EVENTS > 0)

LW_API  
ULONG  API_SemaphoreMStatus (LW_OBJECT_HANDLE   ulId,
                             BOOL              *pbValue,
                             ULONG             *pulOption,
                             ULONG             *pulThreadBlockNum)
{
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_EVENT       pevent;
    
    usIndex = _ObjectGetIndex(ulId);
    
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_SEM_M)) {                         /*  �����Ƿ���ȷ                */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    if (_Event_Index_Invalid(usIndex)) {                                /*  �±��Ƿ�����ȷ              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
#endif
    pevent = &_K_eventBuffer[usIndex];
    
    LW_SPIN_LOCK(&pevent->EVENT_slLock);                                /*  ���� spinlock               */
    
    if (_Event_Type_Invalid(usIndex, LW_TYPE_EVENT_MUTEX)) {
        LW_SPIN_UNLOCK(&pevent->EVENT_slLock);                          /*  �� spinlock               */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "semaphore handle invalidate.\r\n");
        _ErrorHandle(ERROR_EVENT_TYPE);
        return  (ERROR_EVENT_TYPE);
    }
    
    if (pbValue) {
        *pbValue = (BOOL)pevent->EVENT_ulCounter;
    }
    if (pulOption) {
        *pulOption = pevent->EVENT_ulOption;
    }
    if (pulThreadBlockNum) {
        *pulThreadBlockNum = _EventWaitNum(pevent);
    }
    
    LW_SPIN_UNLOCK(&pevent->EVENT_slLock);                              /*  �� spinlock               */
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_SEMM_EN > 0)        */
                                                                        /*  (LW_CFG_MAX_EVENTS > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/