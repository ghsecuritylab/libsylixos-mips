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
** ��   ��   ��: TimeSleep.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 12 �� 14 ��
**
** ��        ��: �߳�˯�ߺ�����

** BUG
2008.03.29  ʹ���µĵȴ�����.
2008.03.30  ʹ���µľ���������.
2008.04.12  ������źŵ�֧��.
2008.04.13  ���ںܶ��ں˻�����Ҫ��ȷ���ӳ�, ������������ API_TimeSleep �� posix sleep.
            API_TimeSleep �ǲ��ܱ��źŻ��ѵ�, �� posix sleep �ǿ��Ա��źŻ��ѵ�.
2009.10.12  ����� SMP ��˵�֧��.
2012.03.20  ���ٶ� _K_ptcbTCBCur ������, �������þֲ�����, ���ٶԵ�ǰ CPU ID ��ȡ�Ĵ���.
2013.07.10  ȥ�� API_TimeUSleep API.
            API_TimeMSleep ���ٱ�֤һ�� tick �ӳ�.
2015.02.04  nanosleep() �������һ�� tick ��ʹ�� bspDelayNs() �����ӳ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  s_internal.h ��Ҳ����ض���
*********************************************************************************************************/
#if LW_CFG_THREAD_CANCEL_EN > 0
#define __THREAD_CANCEL_POINT()         API_ThreadTestCancel()
#else
#define __THREAD_CANCEL_POINT()
#endif                                                                  /*  LW_CFG_THREAD_CANCEL_EN     */
/*********************************************************************************************************
** ��������: API_TimeSleep
** ��������: �߳�˯�ߺ��� (�� API ���ܱ��źŻ���)
** �䡡��  : ulTick            ˯�ߵ�ʱ��
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API
VOID  API_TimeSleep (ULONG    ulTick)
{
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
	REGISTER PLW_CLASS_PCB         ppcb;
	REGISTER ULONG                 ulKernelTime;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return;
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_SLEEP, 
                      ptcbCur->TCB_ulId, ulTick, LW_NULL);
    
__wait_again:
    if (!ulTick) {                                                      /*  �������ӳ�                  */
        return;
    }

    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�                    */

    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  �Ӿ�������ɾ��              */
    
    ptcbCur->TCB_ulDelay = ulTick;
    __ADD_TO_WAKEUP_LINE(ptcbCur);                                      /*  ����ȴ�ɨ����              */
    
    __KERNEL_TIME_GET_NO_SPINLOCK(ulKernelTime, ULONG);                 /*  ��¼ϵͳʱ��                */
    
    if (__KERNEL_EXIT_IRQ(iregInterLevel)) {                            /*  ���źż���                  */
        ulTick = _sigTimeoutRecalc(ulKernelTime, ulTick);               /*  ���¼���ȴ�ʱ��            */
        goto __wait_again;                                              /*  �����ȴ�                    */
    }
}
/*********************************************************************************************************
** ��������: API_TimeSleepEx
** ��������: �߳�˯�ߺ��� (����Ϊ TICK HZ)
** �䡡��  : ulTick            ˯�ߵ�ʱ��
**           bSigRet           �Ƿ������źŻ���
** �䡡��  : ERROR_NONE or EINTR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API
ULONG  API_TimeSleepEx (ULONG  ulTick, BOOL  bSigRet)
{
             INTREG                iregInterLevel;
             
             PLW_CLASS_TCB         ptcbCur;
	REGISTER PLW_CLASS_PCB         ppcb;
	REGISTER ULONG                 ulKernelTime;
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_SLEEP, 
                      ptcbCur->TCB_ulId, ulTick, LW_NULL);
    
__wait_again:
    if (!ulTick) {                                                      /*  �������ӳ�                  */
        return  (ERROR_NONE);
    }

    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�                    */

    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  �Ӿ�������ɾ��              */
    
    ptcbCur->TCB_ulDelay = ulTick;
    __ADD_TO_WAKEUP_LINE(ptcbCur);                                      /*  ����ȴ�ɨ����              */
    
    __KERNEL_TIME_GET_NO_SPINLOCK(ulKernelTime, ULONG);                 /*  ��¼ϵͳʱ��                */
    
    if (__KERNEL_EXIT_IRQ(iregInterLevel)) {                            /*  ���źż���                  */
        if (bSigRet) {
            _ErrorHandle(EINTR);
            return  (EINTR);
        }
        ulTick = _sigTimeoutRecalc(ulKernelTime, ulTick);               /*  ���¼���ȴ�ʱ��            */
        goto __wait_again;                                              /*  �����ȴ�                    */
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_TimeSSleep
** ��������: �߳�˯�ߺ��� (����Ϊ TICK HZ)
** �䡡��  : ulSeconds            ˯�ߵ�ʱ�� (��)
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
VOID    API_TimeSSleep (ULONG   ulSeconds)
{
    API_TimeSleep(ulSeconds * LW_TICK_HZ);
}
/*********************************************************************************************************
** ��������: API_TimeMSleep
** ��������: �߳�˯�ߺ��� (����Ϊ TICK HZ)
** �䡡��  : ulMSeconds            ˯�ߵ�ʱ�� (����)
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
VOID    API_TimeMSleep (ULONG   ulMSeconds)
{
    REGISTER ULONG      ulTicks;
    
    if (ulMSeconds == 0) {
        return;
    }
    
    ulTicks = LW_MSECOND_TO_TICK_1(ulMSeconds);
    
    API_TimeSleep(ulTicks);
}
/*********************************************************************************************************
** ��������: __timeGetHighResolution
** ��������: ��õ�ǰ�߾���ʱ�� (ʹ�� CLOCK_MONOTONIC)
** �䡡��  : ptv       �߾���ʱ��
** ȫ�ֱ���: NONE
** ����ģ��: 
*********************************************************************************************************/
static VOID  __timeGetHighResolution (struct timespec  *ptv)
{
    INTREG  iregInterLevel;
    
    LW_SPIN_LOCK_QUICK(&_K_slKernel, &iregInterLevel);
    *ptv = _K_tvTODMono;
#if LW_CFG_TIME_HIGH_RESOLUTION_EN > 0
    bspTickHighResolution(ptv);                                         /*  �߾���ʱ��ֱ��ʼ���        */
#endif                                                                  /*  LW_CFG_TIME_HIGH_RESOLUT... */
    LW_SPIN_UNLOCK_QUICK(&_K_slKernel, iregInterLevel);
}
/*********************************************************************************************************
** ��������: __timePassSpec
** ��������: ƽ���ȴ�ָ���Ķ�ʱ��
** �䡡��  : ptv       ��ʱ��
** ȫ�ֱ���: NONE
** ����ģ��: 
*********************************************************************************************************/
static VOID  __timePassSpec (const struct timespec  *ptv)
{
#if LW_CFG_TIME_HIGH_RESOLUTION_EN > 0
    struct timespec  tvEnd, tvNow;
    
    __timeGetHighResolution(&tvEnd);
    __timespecAdd(&tvEnd, ptv);
    do {
        __timeGetHighResolution(&tvNow);
    } while (__timespecLeftTime(&tvNow, &tvEnd));
#else
    bspDelayNs((ULONG)lNsec);                                           /*  ʹ�� BSP ���ӳٺ���         */
#endif                                                                  /*  LW_CFG_TIME_HIGH_RESOLUT... */
}
/*********************************************************************************************************
** ��������: nanosleep 
** ��������: ʹ���ô˺������߳�˯��һ��ָ����ʱ��, ˯�߹����п��ܱ��źŻ���. (POSIX)
** �䡡��  : rqtp         ˯�ߵ�ʱ��
**           rmtp         ����ʣ��ʱ��Ľṹ.
** �䡡��  : ERROR_NONE  or  PX_ERROR

             error == EINTR    ��ʾ���źż���.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
INT  nanosleep (const struct timespec  *rqtp, struct timespec  *rmtp)
{
             INTREG             iregInterLevel;
             
             PLW_CLASS_TCB      ptcbCur;
    REGISTER PLW_CLASS_PCB      ppcb;
	REGISTER ULONG              ulKernelTime;
	REGISTER INT                iRetVal;
	         INT                iSchedRet;
	
	REGISTER ULONG              ulError;
             ULONG              ulTick;
             
             struct timespec    tvStart;
             struct timespec    tvTemp;
    
    if ((!rqtp)              ||
        (rqtp->tv_nsec <  0) ||
        (rqtp->tv_nsec >= __TIMEVAL_NSEC_MAX)) {                        /*  ʱ���ʽ����                */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    ulTick = __timespecToTick((struct timespec *)rqtp);
    if (!ulTick) {                                                      /*  ����һ�� tick               */
        __timePassSpec(rqtp);                                           /*  ƽ���ȹ�                    */
        if (rmtp) {
            rmtp->tv_sec  = 0;                                          /*  ������ʱ����              */
            rmtp->tv_nsec = 0;
        }
        return  (ERROR_NONE);
    }
    
    __timeGetHighResolution(&tvStart);                                  /*  ��¼��ʼ��ʱ��              */
    
    __THREAD_CANCEL_POINT();                                            /*  ����ȡ����                  */
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);                                       /*  ��ǰ������ƿ�              */
    
    MONITOR_EVT_LONG2(MONITOR_EVENT_ID_THREAD, MONITOR_EVENT_THREAD_SLEEP, 
                      ptcbCur->TCB_ulId, ulTick, LW_NULL);
    
__wait_again:
    iregInterLevel = __KERNEL_ENTER_IRQ();                              /*  �����ں�                    */
    
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  �Ӿ�������ɾ��              */
    
    ptcbCur->TCB_ulDelay = ulTick;
    __ADD_TO_WAKEUP_LINE(ptcbCur);                                      /*  ����ȴ�ɨ����              */
    
    __KERNEL_TIME_GET_NO_SPINLOCK(ulKernelTime, ULONG);                 /*  ��¼ϵͳʱ��                */
    
    iSchedRet = __KERNEL_EXIT_IRQ(iregInterLevel);                      /*  ����������                  */
    if (iSchedRet == LW_SIGNAL_EINTR) {
        iRetVal = PX_ERROR;                                             /*  ���źż���                  */
        ulError = EINTR;
    
    } else {
        if (iSchedRet == LW_SIGNAL_RESTART) {                           /*  �ź�Ҫ�������ȴ�            */
            ulTick = _sigTimeoutRecalc(ulKernelTime, ulTick);           /*  ���¼���ȴ�ʱ��            */
            if (ulTick != 0ul) {
                goto    __wait_again;                                   /*  ���µȴ�ʣ��� tick         */
            }
        }
        iRetVal = ERROR_NONE;                                           /*  ��Ȼ����                    */
        ulError = ERROR_NONE;
    }
    
    if (ulError ==  ERROR_NONE) {                                       /*  tick �Ѿ��ӳٽ���           */
        __timeGetHighResolution(&tvTemp);
        __timespecSub(&tvTemp, &tvStart);                               /*  �����Ѿ��ӳٵ�ʱ��          */
        if (__timespecLeftTime(&tvTemp, rqtp)) {                        /*  ����ʣ��ʱ����Ҫ�ӳ�        */
            struct timespec  tvNeed = *rqtp;
            __timespecSub(&tvNeed, &tvTemp);
            __timePassSpec(&tvNeed);                                    /*  ƽ���ȹ�                    */
        }
        if (rmtp) {
            rmtp->tv_sec  = 0;                                          /*  ������ʱ����              */
            rmtp->tv_nsec = 0;
        }
    } else {                                                            /*  �źŻ���                    */
        if (rmtp) {
            *rmtp = *rqtp;
            __timeGetHighResolution(&tvTemp);
            __timespecSub(&tvTemp, &tvStart);                           /*  �����Ѿ��ӳٵ�ʱ��          */
            if (__timespecLeftTime(&tvTemp, rmtp)) {                    /*  û���ӳٹ�                  */
                __timespecSub(rmtp, &tvTemp);                           /*  ����û���ӳٹ���ʱ��        */
            }
        }
    }
             
    _ErrorHandle(ulError);                                              /*  ���� errno ֵ               */
    return  (iRetVal);
}
/*********************************************************************************************************
** ��������: usleep 
** ��������: ʹ���ô˺������߳�˯��һ��ָ����ʱ��(us Ϊ��λ), ˯�߹����п��ܱ��źŻ���. (POSIX)
** �䡡��  : usecondTime       ʱ�� (us)
** �䡡��  : ERROR_NONE  or  PX_ERROR
             error == EINTR    ��ʾ���źż���.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
INT  usleep (usecond_t   usecondTime)
{
    struct timespec ts;

    ts.tv_sec  = (time_t)(usecondTime / 1000000);
    ts.tv_nsec = (LONG)(usecondTime % 1000000) * 1000ul;
    
    return  (nanosleep(&ts, LW_NULL));
}
/*********************************************************************************************************
** ��������: sleep 
** ��������: sleep() ����Ŀǰ���߳���ͣ, ֱ���ﵽ���� uiSeconds ��ָ����ʱ��, ���Ǳ��ź�������. (POSIX)
** �䡡��  : uiSeconds         ˯�ߵ�����
** �䡡��  : ��������ͣ������ uiSeconds ��ָ����ʱ���򷵻� 0, �����ź��ж��򷵻�ʣ������.
**           error == EINTR    ��ʾ���źż���.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
UINT  sleep (UINT    uiSeconds)
{
    struct timespec  rqtp;
    struct timespec  rmtp;
    
    rqtp.tv_sec  = (time_t)uiSeconds;
    rqtp.tv_nsec = 0;
    
    nanosleep(&rqtp, &rmtp);
    
    return  ((UINT)rmtp.tv_sec);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/