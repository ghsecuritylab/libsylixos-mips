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
** ��   ��   ��: ThreadWakeup.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 19 ��
**
** ��        ��: �̴߳�˯��ģʽ����

** BUG
2007.07.19  ���� _DebugHandle() ����
2008.03.29  ʹ���µĵȴ�����.
2008.03.30  ʹ���µľ���������.
2010.01.22  ֧�� SMP.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadWakeup
** ��������: �̴߳�˯��ģʽ����
** �䡡��  : 
**           ulId    �߳̾��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
ULONG  API_ThreadWakeup (LW_OBJECT_HANDLE  ulId)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER PLW_CLASS_PCB         ppcb;
	
    usIndex = _ObjectGetIndex(ulId);
	
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                         /*  ��� ID ������Ч��         */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                                /*  ����߳���Ч��             */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invalidate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    ppcb = _GetPcb(ptcb);
    
    if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {
        __DEL_FROM_WAKEUP_LINE(ptcb);                                   /*  �ӵȴ�����ɾ��              */
        ptcb->TCB_ulDelay = 0ul;
        
        if (ptcb->TCB_usStatus & LW_THREAD_STATUS_PEND_ANY) {
            ptcb->TCB_usStatus &= (~LW_THREAD_STATUS_PEND_ANY);
            ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_OUT;                 /*  �ȴ���ʱ                    */
        } else {
            ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_CLEAR;               /*  û�еȴ��¼�                */
        }
        
        if (__LW_THREAD_IS_READY(ptcb)) {                               /*  ����Ƿ����                */
            ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_OTHER;
            __ADD_TO_READY_RING(ptcb, ppcb);                            /*  ���������                  */
        }
        
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں˲����ж�          */
        
        MONITOR_EVT_LONG1(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_WAKEUP, ulId, LW_NULL);
        
        return  (ERROR_NONE);
    
    } else {
        __KERNEL_EXIT_IRQ(iregInterLevel);                              /*  �˳��ں˲����ж�          */
        
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread is not in SLEEP mode.\r\n");
        _ErrorHandle(ERROR_THREAD_NOT_SLEEP);
        return  (ERROR_THREAD_NOT_SLEEP);
    }
}

/*********************************************************************************************************
  END
*********************************************************************************************************/