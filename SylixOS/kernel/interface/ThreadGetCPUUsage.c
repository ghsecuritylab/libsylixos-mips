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
** ��   ��   ��: ThreadGetCPUUsage.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 07 �� 18 ��
**
** ��        ��: ����߳�CPU������

** BUG
2007.07.18  ���� _DebugHandle() ����
2009.12.14  �����ܵ��ں�ʹ�������ʲ���.
2012.08.28  ���� API_ThreadGetCPUUsageAll() ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_ThreadCPUUsageOn
** ��������: ���� CPU �����ʲ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0

LW_API
VOID  API_ThreadCPUUsageOn (VOID)
{
    __LW_TICK_CPUUSAGE_ENABLE();                                        /*  ���´򿪲���                */
}
/*********************************************************************************************************
** ��������: API_ThreadCPUUsageOff
** ��������: �ر� CPU �����ʲ���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
VOID  API_ThreadCPUUsageOff (VOID)
{
    __LW_TICK_CPUUSAGE_DISABLE();
}
/*********************************************************************************************************
** ��������: API_ThreadCPUUsageIsOn
** ��������: �鿴 CPU �����ʲ����Ƿ��
** �䡡��  : NONE
** �䡡��  : �Ƿ��
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API
BOOL  API_ThreadCPUUsageIsOn (VOID)
{
    return  (__LW_TICK_CPUUSAGE_ISENABLE());
}
/*********************************************************************************************************
** ��������: API_ThreadGetCPUUsage
** ��������: ����߳�CPU������
** �䡡��  : ulId                          Ҫ�����߳�ID
**           pucThreadUsage                ���ص�ָ���̵߳� CPU ������
**           pucCPUUsage                   CPU��������
**           pucKernelUsage                �ں����õ� CPU ������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
ULONG  API_ThreadGetCPUUsage (LW_OBJECT_HANDLE  ulId, 
                              UINT8            *pucThreadUsage,
                              UINT8            *pucCPUUsage,
                              UINT8            *pucKernelUsage)
{
             INTREG                iregInterLevel;
    REGISTER UINT16                usIndex;
    REGISTER PLW_CLASS_TCB         ptcb;
    REGISTER UINT8                 ucTemp;
	
    usIndex = _ObjectGetIndex(ulId);
    
    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (ERROR_KERNEL_IN_ISR);
    }
	
#if LW_CFG_ARG_CHK_EN > 0
    if (!_ObjectClassOK(ulId, _OBJECT_THREAD)) {                        /*  ��� ID ������Ч��          */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invaliate.\r\n");
        _ErrorHandle(ERROR_KERNEL_HANDLE_NULL);
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invaliate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
#endif

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "thread handle invaliate.\r\n");
        _ErrorHandle(ERROR_THREAD_NULL);
        return  (ERROR_THREAD_NULL);
    }
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
                                                                        /*  ����� 0 ����               */
    _K_ulCPUUsageTicks = (_K_ulCPUUsageTicks == 0) ? 1 : _K_ulCPUUsageTicks; 
    
    if (pucThreadUsage) {
        *pucThreadUsage = (UINT8)(((ptcb->TCB_ulCPUUsageTicks) * 100) / _K_ulCPUUsageTicks);
    }
    
    if (pucCPUUsage) {                                                  /*  �����߳�������              */
        ptcb = _K_ptcbTCBIdTable[0];
        ucTemp = (UINT8)(((ptcb->TCB_ulCPUUsageTicks) * 100) / _K_ulCPUUsageTicks);
        *pucCPUUsage = (UINT8)(100 - ucTemp);
    }
    
    if (pucKernelUsage) {                                               /*  �ں�ִ��ʱ��ռ��ʱ���������*/
        *pucKernelUsage = (UINT8)(((_K_ulCPUUsageKernelTicks) * 100) / _K_ulCPUUsageTicks);
    }
    
    __KERNEL_EXIT_IRQ(iregInterLevel);                                  /*  �˳��ں˲����ж�          */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadGetCPUUsageAll
** ��������: ��������߳�CPU������
** �䡡��  : ulId[]                        Ҫ�����߳�ID
**           pucThreadUsage[]              ���ص�ָ���̵߳� CPU ������
**           pucKernelUsage[]              �ں����õ� CPU ������
**           iSize                         �����С
** �䡡��  : ��õĸ���
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
INT  API_ThreadGetCPUUsageAll (LW_OBJECT_HANDLE  ulId[], 
                               UINT8             ucThreadUsage[],
                               UINT8             ucKernelUsage[],
                               INT               iSize)
{
    INT              i;
    INT              iIndex = 0;
    
    ULONG            ulCPUAllTicks;
    PLW_CLASS_TCB    ptcb;
    ULONG            ulCPUUsageTicks;
    ULONG            ulCPUUsageKernelTicks;
    
    REGISTER UINT    uiThreadUsage;
    REGISTER UINT    uiThreadUsageKernel;
    
    if (iSize == 0) {
        return  (iIndex);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    
    ulCPUAllTicks  = (_K_ulCPUUsageTicks == 0) ? 1 : _K_ulCPUUsageTicks;
    for (i = 0 ; i < LW_CFG_MAX_THREADS ; i++) {
        ptcb = _K_ptcbTCBIdTable[i];                                    /*  ��� TCB ���ƿ�             */
        if (ptcb == LW_NULL) {                                          /*  �̲߳�����                  */
            continue;
        }
        
        ulCPUUsageTicks       = ptcb->TCB_ulCPUUsageTicks;
        ulCPUUsageKernelTicks = ptcb->TCB_ulCPUUsageKernelTicks;
        
        /*
         *  �����߳��ܵ�������, ����һλС��
         */
        uiThreadUsage = (UINT)((ulCPUUsageTicks * 1000) / ulCPUAllTicks);
        if ((uiThreadUsage % 10) >= 5) {
            uiThreadUsage = (uiThreadUsage / 10) + 1;                   /*  ����                        */
        } else {
            uiThreadUsage = (uiThreadUsage / 10);                       /*  ����                        */
        }
        
        /*
         *  �����߳��ں��е�������, ����һλС��
         */
        uiThreadUsageKernel = (UINT)((ulCPUUsageKernelTicks * 1000) / ulCPUAllTicks);
        if ((uiThreadUsageKernel % 10) >= 5) {
            uiThreadUsageKernel = (uiThreadUsageKernel / 10) + 1;       /*  ����                        */
        } else {
            uiThreadUsageKernel = (uiThreadUsageKernel / 10);           /*  ����                        */
        }
        
        ulId[iIndex] = ptcb->TCB_ulId;
        ucThreadUsage[iIndex] = (UINT8)uiThreadUsage;
        ucKernelUsage[iIndex] = (UINT8)uiThreadUsageKernel;
        
        iIndex++;
        if (iIndex >= iSize) {
            break;
        }
    }
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (iIndex);
}

#endif                                                                  /*  LW_CFG_THREAD_CPU_USAGE...  */
/*********************************************************************************************************
  END
*********************************************************************************************************/