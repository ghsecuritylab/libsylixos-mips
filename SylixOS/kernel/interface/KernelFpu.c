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
** ��   ��   ��: KernelFpu.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 12 �� 19 ��
**
** ��        ��: ��ʼ���ں˸���������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** ��������: API_KernelFpuPrimaryInit
** ��������: ��ʼ������������ (������ bsp ��ʼ���ص��е���)
** �䡡��  : pcMachineName ʹ�õĴ���������
**           pcFpuName     ʹ�õ� FPU ����.
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

LW_API
VOID  API_KernelFpuPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    INT     i, j;
    
    if (LW_SYS_STATUS_GET()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "kernel already running.\r\n");
        return;
    }

    archFpuPrimaryInit(pcMachineName, pcFpuName);                       /*  ��ʼ�� FPU ��Ԫ             */
    
    if (_K_bInterFpuEn) {                                               /*  �ж�״̬����ʹ�� FPU        */
        __ARCH_FPU_ENABLE();                                            /*  ������Ҫ�ڵ�ǰ FPU �������� */
                                                                        /*  ʹ�� FPU, ����ϵͳ�ں���Ҫ  */
    } else {
        __ARCH_FPU_DISABLE();
    }
    
    for (i = 0; i < LW_CFG_MAX_PROCESSORS; i++) {
        for (j = 0; j < LW_CFG_MAX_INTER_SRC; j++) {                    /*  ��ʼ��, ���ǲ�ʹ�� FPU      */
            __ARCH_FPU_CTX_INIT((PVOID)&(LW_CPU_GET(i)->CPU_fpuctxContext[j]));
        }
    }
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "FPU initilaized.\r\n");
}
/*********************************************************************************************************
** ��������: API_KernelFpuSecondaryInit
** ��������: ��ʼ������������ (������ bsp ��ʼ���ص��е���)
** �䡡��  : pcMachineName ʹ�õĴ���������
**           pcFpuName     ʹ�õ� FPU ����.
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0

LW_API
VOID  API_KernelFpuSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    archFpuSecondaryInit(pcMachineName, pcFpuName);
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "secondary FPU initilaized.\r\n");
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/