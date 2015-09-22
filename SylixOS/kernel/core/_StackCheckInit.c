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
** ��   ��   ��: _StackCheckInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 04 �� 16 ��
**
** ��        ��: ��ջ����ʼ��

** BUG:
2013.09.17  �����̶߳�ջ�����鹦��.
2014.08.10  ��ջ���������� hook.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  �������
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
** ��������: _StackCheckInit
** ��������: ��ʼ����ջ����������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _StackCheckInit (VOID)
{
    REGISTER INT        i;
    
    _K_stkFreeFlag = (UCHAR)LW_CFG_STK_EMPTY_FLAG;
    for (i = 0; i < (sizeof(LW_STACK) - 1); i++) {
        _K_stkFreeFlag = (LW_STACK)((LW_STACK)(_K_stkFreeFlag << 8) + (UCHAR)LW_CFG_STK_EMPTY_FLAG);
    }
}
/*********************************************************************************************************
** ��������: _StackCheckGuard
** ��������: �̶߳�ջ������
** �䡡��  : ptcb      ������ƿ�
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  _StackCheckGuard (PLW_CLASS_TCB  ptcb)
{
    pid_t   pid;

    if ((ptcb->TCB_ulOption & LW_OPTION_THREAD_STK_CLR) &&
        (*ptcb->TCB_pstkStackGuard != _K_stkFreeFlag)) {
#if LW_CFG_MODULELOADER_EN > 0
        pid = __lw_vp_get_tcb_pid(ptcb);
#else
        pid = 0;
#endif
        __LW_STACK_OVERFLOW_HOOK(pid, ptcb->TCB_ulId);
        _DebugFormat(__ERRORMESSAGE_LEVEL, "thread %s id 0x%08lx stack may overflow.\r\n",
                     ptcb->TCB_cThreadName, ptcb->TCB_ulId);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/