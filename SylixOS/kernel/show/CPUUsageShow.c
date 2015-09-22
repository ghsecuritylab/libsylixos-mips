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
** ��   ��   ��: CPUUsageShow.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 03 �� 30 ��
**
** ��        ��: ��ʾ���е��̵߳� CPU ռ������Ϣ.

** BUG:
2009.09.18  ���̹��жϵ�ʱ��.
2009.09.18  ʹ���µ��㷨, ����ļ����ж�����ʱ��.
2009.11.07  ���� -n ѡ��, ������������ٴ�.
2009.12.14  ������ں�ռ��ʱ�����ʾ.
2012.08.28  ʹ�� API ����������.
2012.12.11  tid ��ʾ 7 λ�Ϳ�����
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_FIO_LIB_EN > 0 && LW_CFG_THREAD_CPU_USAGE_CHK_EN > 0
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static const CHAR   _G_cCPUUsageInfoHdr[] = "\n\
       NAME        TID   PRI CPU  KERN\n\
---------------- ------- --- ---- ----\n";
/*********************************************************************************************************
** ��������: API_CPUUsageShow
** ��������: ��ʾ���е��̵߳ĵ� CPU ռ������Ϣ
** �䡡��  : 
**           iWaitSec      ÿ�μ�������
**           iTimes        �����ٴβ����� 10s
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
                                           
                                       (�������ж��е���)
*********************************************************************************************************/
LW_API  
VOID    API_CPUUsageShow (INT  iWaitSec, INT  iTimes)
{
    REGISTER INT              i;
             INT              iCounter;
             
             INT              iThreadNum;
             LW_OBJECT_HANDLE ulId[LW_CFG_MAX_THREADS];
             UINT8            ucThreadUsage[LW_CFG_MAX_THREADS];
             UINT8            ucThreadKernel[LW_CFG_MAX_THREADS];
    
             ULONG            iOptionNoAbort;
             ULONG            iOption;
    
    if (LW_CPU_GET_CUR_NESTING()) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return;
    }
    
    ioctl(STD_IN, FIOGETOPTIONS, &iOption);
    iOptionNoAbort = (iOption & ~OPT_ABORT);
    ioctl(STD_IN, FIOSETOPTIONS, iOptionNoAbort);                       /*  ������ control-C ����       */
            
    for (iCounter = 0; iCounter < iTimes; iCounter++) {
    
        API_ThreadCPUUsageRefresh();                                    /*  ˢ��ͳ�Ʊ���                */
        API_ThreadCPUUsageOn();
        printf("CPU usage checking, please wait...\n");
        
        {
            /*
             *  ���������ӳ�ʱ���� control-C ����.
             */
            ioctl(STD_IN, FIOSETOPTIONS, iOption);                      /*  �ظ�ԭ��״̬                */
            
            iWaitSec = (iWaitSec >  0) ? iWaitSec : 1;
            iWaitSec = (iWaitSec < 10) ? iWaitSec : 10;
            API_TimeSleep((ULONG)iWaitSec * LW_TICK_HZ);
            
            ioctl(STD_IN, FIOSETOPTIONS, iOptionNoAbort);               /*  ������ control-C ����       */
        }
        
        API_ThreadCPUUsageOff();
        printf("CPU usage show >>\n");
        printf(_G_cCPUUsageInfoHdr);                                    /*  ��ӡ��ӭ��Ϣ                */
        
        iThreadNum = API_ThreadGetCPUUsageAll(ulId, ucThreadUsage, ucThreadKernel, LW_CFG_MAX_THREADS);
        if (iThreadNum <= 0) {
            continue;
        }
        
        for (i = iThreadNum - 1; i >= 0; i--) {                         /*  ������ʾ                    */
            CHAR    cName[LW_CFG_OBJECT_NAME_SIZE];
            UINT8   ucPriority;
            
            if ((API_ThreadGetName(ulId[i], cName) == ERROR_NONE) &&
                (API_ThreadGetPriority(ulId[i], &ucPriority) == ERROR_NONE)) {
                
                printf("%-16s %7lx %3d %3d%% %3d%%\n", 
                       cName,                                           /*  �߳���                      */
                       ulId[i],                                         /*  �̴߳������                */
                       ucPriority,                                      /*  ���ȼ�                      */
                       ucThreadUsage[i],
                       ucThreadKernel[i]);
            }
        }
    }
    ioctl(STD_IN, FIOSETOPTIONS, iOption);                              /*  �ظ�ԭ��״̬                */
    
    printf("\n");
}
#endif                                                                  /*  LW_CFG_FIO_LIB_EN > 0       */
                                                                        /*  LW_CFG_THREAD_CPU_...       */
/*********************************************************************************************************
  END
*********************************************************************************************************/