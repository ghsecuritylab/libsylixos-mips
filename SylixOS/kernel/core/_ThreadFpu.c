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
** ��   ��   ��: _ThreadFpu.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2012 �� 09 �� 11 ��
**
** ��        ��: ����ϵͳ�߳� FPU ��ع��ܿ�.
**
** BUG:
2014.07.22  ���� _ThreadFpuSave() ��Ϊ CPU ֹͣʱ�������е��߳� FPU.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: _ThreadFpuSwith
** ��������: �߳� FPU �л� (�ڹر��ж�״̬�±�����)
** �䡡��  : bIntSwitch    �Ƿ�Ϊ _SchedInt() ���ж�״̬�µĵ��Ⱥ�������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

VOID  _ThreadFpuSwith (BOOL bIntSwitch)
{
    PLW_CLASS_TCB   ptcbCur;
    PLW_CLASS_TCB   ptcbHigh;
    
    LW_TCB_GET_CUR(ptcbCur);
    LW_TCB_GET_HIGH(ptcbHigh);
    
    if (_K_bInterFpuEn) {                                               /*  �ж�״̬֧�� FPU            */
        /*
         *  �����ں�֧�� FPU ����, �жϺ����ᱣ�浱ǰ����� FPU ������
         *  ����������жϻ����ĵ��Ⱥ���, ����Ҫ���浱ǰ���� FPU ������
         */
        if (bIntSwitch == LW_FALSE) {
            if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
                __ARCH_FPU_SAVE(ptcbCur->TCB_pvStackFP);                /*  ��Ҫ���浱ǰ FPU CTX        */
            }
        }
    } else {
        /*
         *  �����ж�״̬��֧�� FPU ����, �жϺ����в���� FPU ���������κβ���
         *  ���ﲻ���� _Sched() ���� _SchedInt() ����Ҫ���浱ǰ����� FPU ������
         */
        if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
            __ARCH_FPU_SAVE(ptcbCur->TCB_pvStackFP);                    /*  ��Ҫ���浱ǰ FPU CTX        */
        }
    }
        
        
    if (ptcbHigh->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
        __ARCH_FPU_RESTORE(ptcbHigh->TCB_pvStackFP);                    /*  ��Ҫ�ָ������� FPU CTX      */
        
    } else {
        __ARCH_FPU_DISABLE();                                           /*  ��������Ҫ FPU ֧��       */
    }
}

/*********************************************************************************************************
** ��������: _ThreadFpuSave
** ��������: �߳� FPU ���� (�ڹر��ж�״̬�±�����)
** �䡡��  : bIntSwitch    �Ƿ�Ϊ _SchedInt() ���ж�״̬�µĵ��Ⱥ�������
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _ThreadFpuSave (PLW_CLASS_TCB   ptcbCur, BOOL bIntSwitch)
{
    if (_K_bInterFpuEn) {                                               /*  �ж�״̬֧�� FPU            */
        /*
         *  �����ں�֧�� FPU ����, �жϺ����ᱣ�浱ǰ����� FPU ������
         *  ����������жϻ����ĵ��Ⱥ���, ����Ҫ���浱ǰ���� FPU ������
         */
        if (bIntSwitch == LW_FALSE) {
            if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
                __ARCH_FPU_SAVE(ptcbCur->TCB_pvStackFP);                /*  ��Ҫ���浱ǰ FPU CTX        */
            }
        }
    } else {
        /*
         *  �����ж�״̬��֧�� FPU ����, �жϺ����в���� FPU ���������κβ���
         *  ���ﲻ���� _Sched() ���� _SchedInt() ����Ҫ���浱ǰ����� FPU ������
         */
        if (ptcbCur->TCB_ulOption & LW_OPTION_THREAD_USED_FP) {
            __ARCH_FPU_SAVE(ptcbCur->TCB_pvStackFP);                    /*  ��Ҫ���浱ǰ FPU CTX        */
        }
    }
    
    __ARCH_FPU_DISABLE();                                               /*  ������Ҫ FPU ֧��           */
}

#endif
/*********************************************************************************************************
  END
*********************************************************************************************************/