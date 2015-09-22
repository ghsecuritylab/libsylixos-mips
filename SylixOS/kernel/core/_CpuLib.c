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
** ��   ��   ��: _CpuLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 01 �� 10 ��
**
** ��        ��: CPU ������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _CpuActive
** ��������: �� CPU ����Ϊ����״̬ (�����ں��ҹر��ж�״̬�±�����)
** �䡡��  : pcpu      CPU �ṹ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ���뱣֤ pcpu ��ǰִ���߳���һ����Ч�� TCB ���� _K_tcbDummyKernel ��������.
*********************************************************************************************************/
INT  _CpuActive (PLW_CLASS_CPU   pcpu)
{
    if (LW_CPU_IS_ACTIVE(pcpu)) {
        return  (PX_ERROR);
    }
    
    pcpu->CPU_ulStatus |= LW_CPU_STATUS_ACTIVE;
    KN_SMP_MB();
    
    _CandTableUpdate(pcpu);                                             /*  ���º�ѡ�߳�                */

    pcpu->CPU_ptcbTCBCur  = LW_CAND_TCB(pcpu);
    pcpu->CPU_ptcbTCBHigh = LW_CAND_TCB(pcpu);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _CpuInactive
** ��������: �� CPU ����Ϊ�Ǽ���״̬ (�����ں��ҹر��ж�״̬�±�����)
** �䡡��  : pcpu      CPU �ṹ
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  _CpuInactive (PLW_CLASS_CPU   pcpu)
{
    INT             i;
    ULONG           ulCPUId = pcpu->CPU_ulCPUId;
    PLW_CLASS_CPU   pcpuOther;
    PLW_CLASS_TCB   ptcb;
    PLW_CLASS_TCB   ptcbCand;

    if (!LW_CPU_IS_ACTIVE(pcpu)) {
        return  (PX_ERROR);
    }
    
    ptcb = LW_CAND_TCB(pcpu);
    
    pcpu->CPU_ulStatus &= ~LW_CPU_STATUS_ACTIVE;
    KN_SMP_MB();
    
    _CandTableRemove(pcpu);                                             /*  �Ƴ���ѡִ���߳�            */
    LW_CAND_ROT(pcpu) = LW_FALSE;                                       /*  ������ȼ����Ʊ�־          */

    pcpu->CPU_ptcbTCBCur  = LW_NULL;
    pcpu->CPU_ptcbTCBHigh = LW_NULL;
    
    for (i = 0; i < LW_NCPUS; i++) {                                    /*  �������� CPU ����           */
        if (ulCPUId != i) {
            pcpuOther = LW_CPU_GET(i);
            if (!LW_CPU_IS_ACTIVE(pcpuOther)) {                         /*  CPU �����Ǽ���״̬          */
                continue;
            }
            ptcbCand  = LW_CAND_TCB(pcpuOther);
            if (LW_PRIO_IS_HIGH(ptcb->TCB_ucPriority,
                                ptcbCand->TCB_ucPriority)) {            /*  ��ǰ�˳����������ȼ���      */
                LW_CAND_ROT(pcpuOther) = LW_TRUE;
            }
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: _CpuGetNesting
** ��������: ��ȡ CPU ��ǰ�ж�Ƕ��ֵ
** �䡡��  : NONE
** �䡡��  : �ж�Ƕ��ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  _CpuGetNesting (VOID)
{
#if LW_CFG_SMP_EN > 0
    INTREG          iregInterLevel;
    ULONG           ulNesting;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    ulNesting      = LW_CPU_GET_CUR()->CPU_ulInterNesting;
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */
    
    return  (ulNesting);
#else
    return  (LW_CPU_GET_CUR()->CPU_ulInterNesting);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
}
/*********************************************************************************************************
** ��������: _CpuGetMaxNesting
** ��������: ��ȡ CPU ����ж�Ƕ��ֵ
** �䡡��  : NONE
** �䡡��  : �ж�Ƕ��ֵ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
ULONG  _CpuGetMaxNesting (VOID)
{
#if LW_CFG_SMP_EN > 0
    INTREG          iregInterLevel;
    ULONG           ulNesting;
    
    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */
    ulNesting      = LW_CPU_GET_CUR()->CPU_ulInterNestingMax;
    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */
    
    return  (ulNesting);
#else
    return  (LW_CPU_GET_CUR()->CPU_ulInterNestingMax);
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/