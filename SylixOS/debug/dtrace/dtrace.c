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
** ��   ��   ��: dtrace.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2011 �� 11 �� 18 ��
**
** ��        ��: SylixOS ���Ը�����, GDB server ����ʹ�ô˵��Խ���.

** BUG:
2012.09.05  �����賿, ������� dtrace �ĵȴ���ӿڻ���, ��ʼΪ GDB server �ı�дɨƽһ���ϰ�.
2014.05.23  �ڴ����ʱ, ��Ҫ���ȼ���ڴ����Ȩ��.
2014.08.10  API_DtraceThreadStepSet() ���ȶԵ����ϵ��ַ����һ��ҳ���ж�.
2014.09.02  ���� API_DtraceDelBreakInfo() �ӿ�, ��������ɾ���ϵ���Ϣ.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_GDB_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
/*********************************************************************************************************
  dtrace �ṹ
*********************************************************************************************************/
typedef struct {
    UINT                DMSG_uiIn;
    UINT                DMSG_uiOut;
    UINT                DMSG_uiNum;
    LW_DTRACE_MSG       DMSG_dmsgBuffer[LW_CFG_MAX_THREADS];
} LW_DTRACE_MSG_POOL;

typedef struct {
    LW_LIST_LINE        DTRACE_lineManage;                              /*  ��������                    */
    pid_t               DTRACE_pid;                                     /*  ����������                  */
    UINT                DTRACE_uiType;                                  /*  ��������                    */
    UINT                DTRACE_uiFlag;                                  /*  ����ѡ��                    */
    LW_DTRACE_MSG_POOL  DTRACE_dmsgpool;                                /*  ������ͣ�߳���Ϣ            */
    LW_OBJECT_HANDLE    DTRACE_ulDbger;                                 /*  ������                      */
} LW_DTRACE;
typedef LW_DTRACE      *PLW_DTRACE;

#define DPOOL_IN        pdtrace->DTRACE_dmsgpool.DMSG_uiIn
#define DPOOL_OUT       pdtrace->DTRACE_dmsgpool.DMSG_uiOut
#define DPOOL_NUM       pdtrace->DTRACE_dmsgpool.DMSG_uiNum
#define DPOOL_MSG       pdtrace->DTRACE_dmsgpool.DMSG_dmsgBuffer
/*********************************************************************************************************
  dtrace �ṹ
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineDtraceHeader = LW_NULL;
/*********************************************************************************************************
** ��������: __dtraceWriteMsg
** ��������: ����һ��������Ϣ
** �䡡��  : pdtrace       dtrace �ڵ�
**           dmsg          ������Ϣ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __dtraceWriteMsg (PLW_DTRACE  pdtrace, const PLW_DTRACE_MSG  pdmsg)
{
    if (DPOOL_NUM == LW_CFG_MAX_THREADS) {
        return  (PX_ERROR);
    }
    
    DPOOL_MSG[DPOOL_IN] = *pdmsg;
    DPOOL_IN++;
    DPOOL_NUM++;
    
    if (DPOOL_IN >= LW_CFG_MAX_THREADS) {
        DPOOL_IN =  0;
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __dtraceReadMsg
** ��������: ��ȡһ��������Ϣ
** �䡡��  : pdtrace       dtrace �ڵ�
**           dmsg          ������Ϣ
**           bPeek         �Ƿ���Ҫ��������
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __dtraceReadMsg (PLW_DTRACE  pdtrace, PLW_DTRACE_MSG  pdmsg, BOOL  bPeek)
{
    if (DPOOL_NUM == 0) {
        return  (PX_ERROR);
    }
    
    while (DPOOL_MSG[DPOOL_OUT].DTM_ulThread == LW_OBJECT_HANDLE_INVALID) {
        DPOOL_OUT++;
        if (DPOOL_OUT >= LW_CFG_MAX_THREADS) {
            DPOOL_OUT =  0;
        }
    }
    
    *pdmsg = DPOOL_MSG[DPOOL_OUT];
    
    if (bPeek == LW_FALSE) {
        DPOOL_MSG[DPOOL_OUT].DTM_ulThread = LW_OBJECT_HANDLE_INVALID;
        DPOOL_OUT++;
        DPOOL_NUM--;
        
        if (DPOOL_OUT >= LW_CFG_MAX_THREADS) {
            DPOOL_OUT =  0;
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __dtraceReadMsgEx
** ��������: ��ȡһ��������Ϣ
** �䡡��  : pdtrace       dtrace �ڵ�
**           dmsg          ������Ϣ
**           bPeek         �Ƿ���Ҫ��������
**           ulThread      ָ�����߳� ID
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __dtraceReadMsgEx (PLW_DTRACE       pdtrace, 
                               PLW_DTRACE_MSG   pdmsg, 
                               BOOL             bPeek, 
                               LW_OBJECT_HANDLE ulThread)
{
    UINT    uiI = DPOOL_OUT;

    if (DPOOL_NUM == 0) {
        return  (PX_ERROR);
    }
    
    while (DPOOL_MSG[uiI].DTM_ulThread != ulThread) {
        uiI++;
        if (uiI >= LW_CFG_MAX_THREADS) {
            uiI =  0;
        }
        if (uiI == DPOOL_IN) {
            return  (PX_ERROR);                                         /*  û�ж�Ӧ�߳���Ϣ            */
        }
    }
    
    *pdmsg = DPOOL_MSG[uiI];
    
    if (bPeek == LW_FALSE) {
        DPOOL_MSG[uiI].DTM_ulThread = LW_OBJECT_HANDLE_INVALID;
        DPOOL_NUM--;
        
        if (DPOOL_OUT == uiI) {
            DPOOL_OUT++;
            if (DPOOL_OUT >= LW_CFG_MAX_THREADS) {
                DPOOL_OUT =  0;
            }
        }
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __dtraceThread
** ��������: ��ȡһ���̵߳ľ��
** �䡡��  : pdtrace       dtrace �ڵ�
**           dmsg          ������Ϣ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __dtraceThreadCb (LW_OBJECT_HANDLE  ulId, 
                               LW_OBJECT_HANDLE  ulThread[], 
                               UINT              uiTableNum, 
                               UINT             *uiNum)
{
    if (*uiNum < uiTableNum) {
        ulThread[*uiNum] = ulId;
        (*uiNum)++;
    }
}
/*********************************************************************************************************
** ��������: __dtraceMemVerify
** ��������: ����ڴ�����
** �䡡��  : pdtrace       dtrace �ڵ�
**           ulAddr        �ڴ��ַ
**           bWrite        �Ƿ�Ϊд, ����Ϊ��
** �䡡��  : �Ƿ���������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  __dtraceMemVerify (PVOID  pvDtrace, addr_t  ulAddr, BOOL  bWrite)
{
#if LW_CFG_VMM_EN > 0
    ULONG  ulFlags;
    if (API_VmmGetFlag((PVOID)ulAddr, &ulFlags)) {
        return  (LW_FALSE);
    }
    if (bWrite && !(ulFlags & LW_VMM_FLAG_WRITABLE)) {
        return  (LW_FALSE);
    }
    if (bWrite && !(ulFlags & LW_VMM_FLAG_ACCESS)) {
        return  (LW_FALSE);
    }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

    return  (LW_TRUE);
}
/*********************************************************************************************************
** ��������: API_DtraceBreakTrap
** ��������: ��һ���̴߳����ϵ�
** �䡡��  : ulAddr        ������ַ
**           uiBpType      �������� (LW_TRAP_INVAL / LW_TRAP_BRKPT / LW_TRAP_ABORT) 
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_DtraceBreakTrap (addr_t  ulAddr, UINT  uiBpType)
{
#if LW_CFG_MODULELOADER_EN > 0
    LW_OBJECT_HANDLE    ulDbger;
    PLW_LIST_LINE       plineTemp;
    LW_LD_VPROC        *pvproc;
    PLW_CLASS_TCB       ptcbCur;
    PLW_DTRACE          pdtrace;
    LW_DTRACE_MSG       dtm;
    union sigval        sigvalue;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pvproc = __LW_VP_GET_TCB_PROC(ptcbCur);
    if (!pvproc || (pvproc->VP_pid <= 0)) {
        return  (PX_ERROR);
    }
    
    dtm.DTM_ulAddr   = ulAddr;
    dtm.DTM_uiType   = uiBpType;                                        /*  ��� trap ����              */
    dtm.DTM_ulThread = ptcbCur->TCB_ulId;
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    for (plineTemp  = _G_plineDtraceHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ���Ҷ�Ӧ�ĵ�����            */
        pdtrace = _LIST_ENTRY(plineTemp, LW_DTRACE, DTRACE_lineManage);
        if (pdtrace->DTRACE_pid == pvproc->VP_pid) {
            ulDbger = pdtrace->DTRACE_ulDbger;
            __dtraceWriteMsg(pdtrace, &dtm);
            break;
        }
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    if (plineTemp == LW_NULL) {                                         /*  �˽��̲����ڵ�����          */
        return  (PX_ERROR);
    
    } else {
        _DebugHandle(__LOGMESSAGE_LEVEL, "dtrace trap.\r\n");
        sigvalue.sival_int = dtm.DTM_uiType;
        sigTrap(ulDbger, sigvalue);                                     /*  ֪ͨ�������߳�              */
        return  (ERROR_NONE);
    }
#else
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
}
/*********************************************************************************************************
** ��������: API_DtraceAbortTrap
** ��������: ��һ���̴߳����ǿɿ��쳣
** �䡡��  : ulAddr        ������ַ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_DtraceAbortTrap (addr_t  ulAddr)
{
#if LW_CFG_MODULELOADER_EN > 0
    ULONG               ulIns;
    PLW_LIST_LINE       plineTemp;
    LW_LD_VPROC        *pvproc;
    PLW_CLASS_TCB       ptcbCur;
    PLW_DTRACE          pdtrace;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pvproc = __LW_VP_GET_TCB_PROC(ptcbCur);
    if (!pvproc || (pvproc->VP_pid <= 0)) {
        return  (PX_ERROR);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    for (plineTemp  = _G_plineDtraceHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ���Ҷ�Ӧ�ĵ�����            */
        pdtrace = _LIST_ENTRY(plineTemp, LW_DTRACE, DTRACE_lineManage);
        if (pdtrace->DTRACE_pid == pvproc->VP_pid) {
            break;
        }
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    if (plineTemp == LW_NULL) {                                         /*  �˽��̲����ڵ�����          */
        return  (PX_ERROR);
    
    } else {
        archDbgAbInsert(ulAddr, &ulIns);                                /*  ����һ���ϵ�                */
        return  (ERROR_NONE);
    }
#else
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
}
/*********************************************************************************************************
** ��������: API_DtraceChildSig
** ��������: �������߳�֪ͨ
** �䡡��  : pid           ���̺�
**           psigevent     �ź���Ϣ
**           psiginfo      �ź���Ϣ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
INT  API_DtraceChildSig (pid_t pid, struct sigevent *psigevent, struct siginfo *psiginfo)
{
#if LW_CFG_MODULELOADER_EN > 0
    PLW_LIST_LINE       plineTemp;
    PLW_DTRACE          pdtrace;
    LW_OBJECT_HANDLE    ulDbger;
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    for (plineTemp  = _G_plineDtraceHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ���Ҷ�Ӧ�ĵ�����            */
        pdtrace = _LIST_ENTRY(plineTemp, LW_DTRACE, DTRACE_lineManage);
        if (pdtrace->DTRACE_pid == pid) {
            ulDbger = pdtrace->DTRACE_ulDbger;
            break;
        }
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    if (plineTemp == LW_NULL) {                                         /*  �˽��̲����ڵ�����          */
        return  (PX_ERROR);
    
    } else {
        _doSigEventEx(ulDbger, psigevent, psiginfo);                    /*  ���� SIGCHLD �ź�           */
        return  (ERROR_NONE);
    }
#else
    return  (PX_ERROR);
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
}
/*********************************************************************************************************
** ��������: API_DtraceCreate
** ��������: ����һ�� dtrace ���Խڵ�
** �䡡��  : uiType            ��������
**           uiFlag            ����ѡ��
**           ulDbger           �����������߳�
** �䡡��  : dtrace �ڵ�
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
PVOID  API_DtraceCreate (UINT  uiType, UINT  uiFlag, LW_OBJECT_HANDLE  ulDbger)
{
    PLW_DTRACE  pdtrace;
    
    pdtrace = (PLW_DTRACE)__SHEAP_ALLOC(sizeof(LW_DTRACE));
    if (pdtrace == LW_NULL) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);
    }
    lib_bzero(pdtrace, sizeof(LW_DTRACE));
    
    pdtrace->DTRACE_pid     = (pid_t)PX_ERROR;
    pdtrace->DTRACE_uiType  = uiType;
    pdtrace->DTRACE_uiFlag  = uiFlag;
    pdtrace->DTRACE_ulDbger = ulDbger;
    
    __KERNEL_ENTER();
    _List_Line_Add_Ahead(&pdtrace->DTRACE_lineManage, &_G_plineDtraceHeader);
    __KERNEL_EXIT();
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "dtrace create.\r\n");
    return  ((PVOID)pdtrace);
}
/*********************************************************************************************************
** ��������: API_DtraceDelete
** ��������: ɾ��һ�� dtrace ���Խڵ�
** �䡡��  : pvDtrace      dtrace �ڵ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceDelete (PVOID  pvDtrace)
{
    PLW_DTRACE  pdtrace = (PLW_DTRACE)pvDtrace;
    
    if (!pdtrace) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    __KERNEL_ENTER();
    _List_Line_Del(&pdtrace->DTRACE_lineManage, &_G_plineDtraceHeader);
    __KERNEL_EXIT();
    
    __SHEAP_FREE(pdtrace);
    
    _DebugHandle(__LOGMESSAGE_LEVEL, "dtrace delete.\r\n");
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DtraceSetPid
** ��������: ���� dtrace ���ٽ���
** �䡡��  : pvDtrace          dtrace �ڵ�
**           pid               �����Խ��̺�
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceSetPid (PVOID  pvDtrace, pid_t  pid)
{
    PLW_DTRACE  pdtrace = (PLW_DTRACE)pvDtrace;
    
    if (!pdtrace) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    pdtrace->DTRACE_pid = pid;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DtraceGetRegs
** ��������: ��ȡָ�������̵߳ļĴ���������
** �䡡��  : pvDtrace      dtrace �ڵ�
**           ulThread      �߳̾��
**           pregctx       �Ĵ�����
**           pregSp        ��ջָ��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceGetRegs (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, 
                          ARCH_REG_CTX  *pregctx, ARCH_REG_T *pregSp)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
    REGISTER ARCH_REG_T     regSp;
    
    PLW_DTRACE  pdtrace = (PLW_DTRACE)pvDtrace;
    
    if (!pdtrace || !pregctx) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    usIndex = _ObjectGetIndex(ulThread);
    
    if (!_ObjectClassOK(ulThread, _OBJECT_THREAD)) {                    /*  ��� ID ������Ч��          */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        return  (ERROR_THREAD_NULL);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    *pregctx = *(ARCH_REG_CTX *)ptcb->TCB_pstkStackNow;
    regSp    =  (ARCH_REG_T)ptcb->TCB_pstkStackNow;
    
#if	CPU_STK_GROWTH == 0
    regSp -= sizeof(ARCH_REG_CTX);
#else
    regSp += sizeof(ARCH_REG_CTX);
#endif

    *pregSp = regSp;
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DtraceSetRegs
** ��������: ����ָ�������̵߳ļĴ���������
** �䡡��  : pvDtrace      dtrace �ڵ�
**           ulThread      �߳̾��
**           pregctx       �Ĵ�����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceSetRegs (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, const ARCH_REG_CTX  *pregctx)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
    
    PLW_DTRACE  pdtrace = (PLW_DTRACE)pvDtrace;
    
    if (!pdtrace || !pregctx) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    usIndex = _ObjectGetIndex(ulThread);
    
    if (!_ObjectClassOK(ulThread, _OBJECT_THREAD)) {                    /*  ��� ID ������Ч��          */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }
    
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        return  (ERROR_THREAD_NULL);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        return  (ERROR_THREAD_NULL);
    }
    
    ptcb = _K_ptcbTCBIdTable[usIndex];
    
    *(ARCH_REG_CTX *)ptcb->TCB_pstkStackNow = *pregctx;
    
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DtraceGetMems
** ��������: �����ڴ�
** �䡡��  : pvDtrace      dtrace �ڵ�
**           ulAddr        �����ڴ���ʼ��ַ
**           pvBuffer      ���ջ���
**           stSize        �����Ĵ�С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �ڴ��������Ӧ�� PAGESIZE ��.
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceGetMems (PVOID  pvDtrace, addr_t  ulAddr, PVOID  pvBuffer, size_t  stSize)
{
    if (!pvBuffer) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if ((__dtraceMemVerify(pvDtrace, ulAddr,              LW_FALSE) == LW_FALSE) ||
        (__dtraceMemVerify(pvDtrace, ulAddr + stSize - 1, LW_FALSE) == LW_FALSE)) {
        _ErrorHandle(EACCES);
        return  (EACCES);
    }
    
    lib_memcpy(pvBuffer, (const PVOID)ulAddr, stSize);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DtraceSetMems
** ��������: д���ڴ�
** �䡡��  : pvDtrace      dtrace �ڵ�
**           ulAddr        д���ڴ���ʼ��ַ
**           pvBuffer      д������
**           stSize        д��Ĵ�С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : �ڴ��������Ӧ�� PAGESIZE ��.
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceSetMems (PVOID  pvDtrace, addr_t  ulAddr, const PVOID  pvBuffer, size_t  stSize)
{
    if (!pvBuffer) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if ((__dtraceMemVerify(pvDtrace, ulAddr,              LW_TRUE) == LW_FALSE) ||
        (__dtraceMemVerify(pvDtrace, ulAddr + stSize - 1, LW_TRUE) == LW_FALSE)) {
        _ErrorHandle(EACCES);
        return  (EACCES);
    }
    
    lib_memcpy((PVOID)ulAddr, pvBuffer, stSize);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DtraceBreakpointInsert
** ��������: ����һ���ϵ�
** �䡡��  : pvDtrace      dtrace �ڵ�
**           ulAddr        �ϵ��ַ
**           stSize        �ϵ㳤��
**           pulIns        ���ضϵ��ַ֮ǰ��ָ��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceBreakpointInsert (PVOID  pvDtrace, addr_t  ulAddr, size_t stSize, ULONG  *pulIns)
{
    PLW_DTRACE    pdtrace = (PLW_DTRACE)pvDtrace;
    
    if (!(pdtrace->DTRACE_uiFlag & LW_DTRACE_F_KBP)) {                  /*  �ں˶ϵ����                */
#if LW_CFG_VMM_EN > 0
        if (!API_VmmVirtualIsInside(ulAddr)) {
            _ErrorHandle(ERROR_KERNEL_MEMORY);
            return  (ERROR_KERNEL_MEMORY);
        }
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    }
    
    archDbgBpInsert(ulAddr, stSize, pulIns);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DtraceBreakpointRemove
** ��������: ɾ��һ���ϵ�
** �䡡��  : pvDtrace      dtrace �ڵ�
**           ulAddr        �ϵ��ַ
**           stSize        �ϵ㳤��
**           ulIns         �ϵ�֮ǰ��ָ��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceBreakpointRemove (PVOID  pvDtrace, addr_t  ulAddr, size_t stSize, ULONG  ulIns)
{
    archDbgBpRemove(ulAddr, stSize, ulIns);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DtraceWatchpointInsert
** ��������: ����һ�����ݹ۲��
** �䡡��  : pvDtrace      dtrace �ڵ�
**           ulAddr        ���ݵ�ַ
**           stSize        ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceWatchpointInsert (PVOID  pvDtrace, addr_t  ulAddr, size_t stSize)
{
    _ErrorHandle(ENOSYS);
    return  (ENOSYS);
}
/*********************************************************************************************************
** ��������: API_DtraceWatchpointRemove
** ��������: ɾ��һ�����ݹ۲��
** �䡡��  : pvDtrace      dtrace �ڵ�
**           ulAddr        ���ݵ�ַ
**           stSize        ����
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceWatchpointRemove (PVOID  pvDtrace, addr_t  ulAddr, size_t stSize)
{
    _ErrorHandle(ENOSYS);
    return  (ENOSYS);
}
/*********************************************************************************************************
** ��������: API_DtraceStopThread
** ��������: ֹͣһ���߳�
** �䡡��  : pvDtrace      dtrace �ڵ�
**           ulThread      �߳̾��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceStopThread (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread)
{
    return  (API_ThreadStop(ulThread));
}
/*********************************************************************************************************
** ��������: API_DtraceContinueThread
** ��������: �ָ�һ���Ѿ���ֹͣ���߳�
** �䡡��  : pvDtrace      dtrace �ڵ�
**           ulThread      �߳̾��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceContinueThread (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread)
{
    return  (API_ThreadContinue(ulThread));
}
/*********************************************************************************************************
** ��������: API_DtraceStopProcess
** ��������: ֹͣ dtrace ��Ӧ�ĵ��Խ���
** �䡡��  : pvDtrace      dtrace �ڵ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceStopProcess (PVOID  pvDtrace)
{
    PLW_DTRACE    pdtrace = (PLW_DTRACE)pvDtrace;
    LW_LD_VPROC  *pvproc  = vprocGet(pdtrace->DTRACE_pid);
    
    if (pvproc) {
        vprocThreadTraversal(pvproc, (VOIDFUNCPTR)API_ThreadStop, 0, 0, 0, 0, 0, 0);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DtraceContinueProcess
** ��������: �ָ� dtrace ��Ӧ�ĵ��Խ���
** �䡡��  : pvDtrace      dtrace �ڵ�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceContinueProcess (PVOID  pvDtrace)
{
    PLW_DTRACE    pdtrace = (PLW_DTRACE)pvDtrace;
    LW_LD_VPROC  *pvproc  = vprocGet(pdtrace->DTRACE_pid);
    
    if (pvproc) {
        vprocThreadTraversal(pvproc, (VOIDFUNCPTR)API_ThreadContinue, 0, 0, 0, 0, 0, 0);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DtraceGetBreakInfo
** ��������: ��õ�ǰ�ϵ��߳���Ϣ
** �䡡��  : pvDtrace      dtrace �ڵ�
**           pdtm          ��ȡ����Ϣ
**           ulThread      ָ�����߳̾��
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceGetBreakInfo (PVOID  pvDtrace, PLW_DTRACE_MSG  pdtm, LW_OBJECT_HANDLE  ulThread)
{
    INT         iError;
    PLW_DTRACE  pdtrace = (PLW_DTRACE)pvDtrace;
    
    if (!pdtrace || !pdtm) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (ulThread == LW_OBJECT_HANDLE_INVALID) {
        iError = __dtraceReadMsg(pdtrace, pdtm, LW_FALSE);
    } else {
        iError = __dtraceReadMsgEx(pdtrace, pdtm, LW_FALSE, ulThread);
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    if (iError) {
        _ErrorHandle(ENOMSG);
        return  (ENOMSG);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DtraceDelBreakInfo
** ��������: ɾ��ָ���̵߳Ķϵ���Ϣ(�����ػ���һ���ϵ���Ϣ).
** �䡡��  : pvDtrace      dtrace �ڵ�
**           ulThread      ָ�����߳̾�� (����Ϊ��Ч�߳̾��)
**           ulBreakAddr   �ϵ��ַ
**           bContinue     �Ƴ����Ƿ����ִ���߳�
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceDelBreakInfo (PVOID             pvDtrace, 
                               LW_OBJECT_HANDLE  ulThread, 
                               addr_t            ulBreakAddr,
                               BOOL              bContinue)
{
    INT             iError  = PX_ERROR;
    PLW_DTRACE      pdtrace = (PLW_DTRACE)pvDtrace;
    LW_DTRACE_MSG   dtm;
    
    if (!pdtrace) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (ulThread) {
        if (__dtraceReadMsgEx(pdtrace, &dtm, LW_TRUE, ulThread) == ERROR_NONE) {
            if (ulBreakAddr == dtm.DTM_ulAddr) {
                __dtraceReadMsgEx(pdtrace, &dtm, LW_FALSE, ulThread);
                if (bContinue) {
                    API_ThreadContinue(ulThread);
                }
                iError = ERROR_NONE;
            }
        }
    }
    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */
    
    if (iError) {
        _ErrorHandle(ENOMSG);
        return  (ENOMSG);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DtraceProcessThread
** ��������: ��ý����������̵߳ľ��
** �䡡��  : pvDtrace      dtrace �ڵ�
**           ulThread      �߳̾���б�����
**           uiTableNum    �б���С
**           puiThreadNum  ʵ�ʻ�ȡ���߳���Ŀ
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceProcessThread (PVOID  pvDtrace, LW_OBJECT_HANDLE ulThread[], 
                                UINT   uiTableNum, UINT *puiThreadNum)
{
    PLW_DTRACE    pdtrace = (PLW_DTRACE)pvDtrace;
    LW_LD_VPROC  *pvproc  = vprocGet(pdtrace->DTRACE_pid);
    UINT          uiNum   = 0;
    
    if (!ulThread || !uiTableNum || !puiThreadNum) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    if (pvproc) {
        vprocThreadTraversal(pvproc, __dtraceThreadCb, 
                             (PVOID)ulThread, (PVOID)uiTableNum, (PVOID)&uiNum, 
                             0, 0, 0);
    }
    
    *puiThreadNum = uiNum;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DtraceThreadExtraInfo
** ��������: ����̶߳�����Ϣ
** �䡡��  : pvDtrace      dtrace �ڵ�
**           ulThread      �߳̾��
**           pcExtraInfo   ������Ϣ����
**           siSize        �����С
** �䡡��  : ERROR
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
ULONG  API_DtraceThreadExtraInfo (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread,
                                  PCHAR  pcExtraInfo, size_t  stSize)
{
    ULONG              ulError;
    LW_CLASS_TCB_DESC  tcbdesc;
    
    PCHAR              pcPendType = LW_NULL;
    PCHAR              pcFpu      = LW_NULL;
    size_t             stFreeByteSize = 0;
    
    if (!pcExtraInfo || !stSize) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    ulError = API_ThreadDesc(ulThread, &tcbdesc);
    if (ulError) {
        return  (ulError);
    }
    
    API_ThreadStackCheck(ulThread, &stFreeByteSize, LW_NULL, LW_NULL);
    
    if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_SEM) {                 /*  �ȴ��ź���                  */
        pcPendType = "SEM";
    
    } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_MSGQUEUE) {     /*  �ȴ���Ϣ����                */
        pcPendType = "MSGQ";
    
    } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_JOIN) {         /*  �ȴ������߳�                */
        pcPendType = "JOIN";
        
    } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_SUSPEND) {      /*  ����                        */
        pcPendType = "SUSP";
    
    } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_EVENTSET) {     /*  �ȴ��¼���                  */
        pcPendType = "ENTS";
    
    } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_SIGNAL) {       /*  �ȴ��ź�                    */
        pcPendType = "WSIG";
    
    } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_INIT) {         /*  ��ʼ����                    */
        pcPendType = "INIT";
    
    } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_WDEATH) {       /*  ����״̬                    */
        pcPendType = "WDEA";
    
    } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_DELAY) {        /*  ˯��                        */
        pcPendType = "SLP";
    
    } else if (tcbdesc.TCBD_usStatus & LW_THREAD_STATUS_WSTAT) {        /*  �ȴ�״̬ת��                */
        pcPendType = "WSTAT";
    
    } else {
        pcPendType = "RDY";                                             /*  ����̬                      */
    }
    
    if (tcbdesc.TCBD_ulOption & LW_OPTION_THREAD_USED_FP) {
        pcFpu = "USE";
    } else {
        pcFpu = "NO";
    }
    
    snprintf(pcExtraInfo, stSize, "%s,prio:%d,stat:%s,errno:%ld,wake:%ld,fpu:%s,cpu:%ld,stackfree:%zd",
             tcbdesc.TCBD_cThreadName,
             tcbdesc.TCBD_ucPriority,
             pcPendType,
             tcbdesc.TCBD_ulLastError,
             tcbdesc.TCBD_ulWakeupLeft,
             pcFpu,
             tcbdesc.TCBD_ulCPUId,
             stFreeByteSize);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DtraceThreadStepSet
** ��������: �����̵߳����ϵ��ַ
** �䡡��  : pvDtrace      dtrace �ڵ�
**           ulThread      �߳̾��
**           ulAddr        �����ϵ��ַ��PX_ERROR��ʾ���õ���
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
ULONG API_DtraceThreadStepSet (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, addr_t  ulAddr)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
             PLW_DTRACE     pdtrace = (PLW_DTRACE)pvDtrace;

    if (!pdtrace) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }

    if (ulAddr != (addr_t)PX_ERROR) {
        archDbgBpPrefetch(ulAddr);                                      /*  Ԥ�Ȳ������ҳ���ж�        */
    }

    usIndex = _ObjectGetIndex(ulThread);

    if (!_ObjectClassOK(ulThread, _OBJECT_THREAD)) {                    /*  ��� ID ������Ч��          */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }

    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        return  (ERROR_THREAD_NULL);
    }

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        return  (ERROR_THREAD_NULL);
    }

    ptcb = _K_ptcbTCBIdTable[usIndex];

    ptcb->TCB_ulStepAddr = ulAddr;                                      /*  ���õ����ϵ��ַ            */

    __KERNEL_EXIT();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DtraceThreadStepGet
** ��������: ��ȡ�̵߳����ϵ��ַ
** �䡡��  : pvDtrace      dtrace �ڵ�
**           ulThread      �߳̾��
** �䡡��  : pulAddr       �����ϵ��ַ
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
ULONG API_DtraceThreadStepGet (PVOID  pvDtrace, LW_OBJECT_HANDLE  ulThread, addr_t  *pulAddr)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;
             PLW_DTRACE     pdtrace = (PLW_DTRACE)pvDtrace;

    if (!pdtrace || !pulAddr) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }

    usIndex = _ObjectGetIndex(ulThread);

    if (!_ObjectClassOK(ulThread, _OBJECT_THREAD)) {                    /*  ��� ID ������Ч��          */
        return  (ERROR_KERNEL_HANDLE_NULL);
    }

    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        return  (ERROR_THREAD_NULL);
    }

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_Thread_Invalid(usIndex)) {
        __KERNEL_EXIT();                                                /*  �˳��ں�                    */
        return  (ERROR_THREAD_NULL);
    }

    ptcb = _K_ptcbTCBIdTable[usIndex];

    *pulAddr = ptcb->TCB_ulStepAddr;                                    /*  ���ص����ϵ��ַ            */

    __KERNEL_EXIT();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DtraceSchedHook
** ��������: �߳��л�HOOK�����������л��ϵ���Ϣ
**           ulThreadOld      �г��߳�
**           ulThreadNew      �����߳�
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
ULONG API_DtraceSchedHook (LW_OBJECT_HANDLE  ulThreadOld, LW_OBJECT_HANDLE  ulThreadNew)
{
    REGISTER UINT16         usIndex;
    REGISTER PLW_CLASS_TCB  ptcb;

    usIndex = _ObjectGetIndex(ulThreadOld);
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        return  (ERROR_THREAD_NULL);
    }

    ptcb = __GET_TCB_FROM_INDEX(usIndex);
    if (ptcb && (ptcb->TCB_ulStepAddr != (addr_t)PX_ERROR)) {           /*  ȷ���г��߳���Ч            */
        archDbgBpRemove(ptcb->TCB_ulStepAddr, sizeof(ULONG), ptcb->TCB_ulStepInst);
    }

    usIndex = _ObjectGetIndex(ulThreadNew);
    if (_Thread_Index_Invalid(usIndex)) {                               /*  ����߳���Ч��              */
        return  (ERROR_THREAD_NULL);
    }

    ptcb = __GET_TCB_FROM_INDEX(usIndex);
    if (ptcb->TCB_ulStepAddr != (addr_t)PX_ERROR) {
        archDbgBpInsert(ptcb->TCB_ulStepAddr, sizeof(ULONG), &ptcb->TCB_ulStepInst);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/