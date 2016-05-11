/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: ppcExc.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 12 月 15 日
**
** 描        述: PowerPC 体系构架异常处理.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include "ppcSpr.h"
#if LW_CFG_VMM_EN > 0
#include "arch/ppc/mm/mmu/common/ppcMmu.h"
#endif
/*********************************************************************************************************
  向量使能与禁能锁
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
#define VECTOR_OP_LOCK()    LW_SPIN_LOCK_IGNIRQ(&_K_slVectorTable)
#define VECTOR_OP_UNLOCK()  LW_SPIN_UNLOCK_IGNIRQ(&_K_slVectorTable)
#else
#define VECTOR_OP_LOCK()
#define VECTOR_OP_UNLOCK()
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
/*********************************************************************************************************
  定义 Decrementer 相关全局变量
*********************************************************************************************************/
static ULONG    _G_ulDecVector;
static BOOL     _G_bDecPreemptive;
static UINT32   _G_uiDecValue;
static BOOL     _G_bDecInited = LW_FALSE;
/*********************************************************************************************************
** 函数名称: archIntHandle
** 功能描述: bspIntHandle 需要调用此函数处理中断 (关闭中断情况被调用)
** 输　入  : ulVector         中断向量
**           bPreemptive      中断是否可抢占
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archIntHandle (ULONG  ulVector, BOOL  bPreemptive)
{
    REGISTER irqreturn_t irqret;

    if (_Inter_Vector_Invalid(ulVector)) {
        return;                                                         /*  向量号不正确                */
    }

    if (LW_IVEC_GET_FLAG(ulVector) & LW_IRQ_FLAG_PREEMPTIVE) {
        bPreemptive = LW_TRUE;
    }

    if (bPreemptive) {
        VECTOR_OP_LOCK();
        __ARCH_INT_VECTOR_DISABLE(ulVector);                            /*  屏蔽 vector 中断            */
        VECTOR_OP_UNLOCK();
        KN_INT_ENABLE_FORCE();                                          /*  允许中断                    */
    }

    irqret = API_InterVectorIsr(ulVector);                              /*  调用中断服务程序            */
    
    KN_INT_DISABLE();                                                   /*  禁能中断                    */
    
    if (bPreemptive) {
        if (irqret != LW_IRQ_HANDLED_DISV) {
            VECTOR_OP_LOCK();
            __ARCH_INT_VECTOR_ENABLE(ulVector);                         /*  允许 vector 中断            */
            VECTOR_OP_UNLOCK();
        }
    
    } else if (irqret == LW_IRQ_HANDLED_DISV) {
        VECTOR_OP_LOCK();
        __ARCH_INT_VECTOR_DISABLE(ulVector);                            /*  屏蔽 vector 中断            */
        VECTOR_OP_UNLOCK();
    }
}
/*********************************************************************************************************
** 函数名称: archDataStorageExceptionHandle
** 功能描述: 数据存储异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archDataStorageExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    ulAbortAddr          = ppcGetDAR();
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_VMM_EN > 0
    UINT32  uiDSISR = ppcMmuGetDSISR();

    /*
     * See << programming_environment_manual >> Figure 7-16
     */

    if (uiDSISR & (0x1 << (31 - 1))) {
        /*
         * Page fault (no PTE found)
         */
        abtInfo.VMABT_uiType   = ppcMmuPteMissHandle(ulAbortAddr);
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;

    } else if (uiDSISR & (0x1 << (31 - 4))) {
        /*
         * Page protection violation
         */
        if (uiDSISR & (0x1 << (31 - 6))) {
            /*
             * If the access is a store
             */
            abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
            abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_WRITE;

        } else {
            /*
             * 不允许读
             */
            abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
            abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;
        }

    } else {
        /*
         * dcbt/dcbtst Instruction
         */
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    }
#endif

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** 函数名称: archInstructionStorageExceptionHandle
** 功能描述: 指令访问异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archInstructionStorageExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    ulAbortAddr          = ppcGetDAR();
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_VMM_EN > 0
    /*
     * See << programming_environment_manual >> Figure 7-16
     */

    UINT32  uiSRR1 = ppcMmuGetSRR1();

    if (uiSRR1 & (0x1 << (31 - 1))) {
        /*
         * Page fault (no PTE found)
         */
        abtInfo.VMABT_uiType   = ppcMmuPteMissHandle(ulAbortAddr);
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;

    } else if (uiSRR1 & (0x1 << (31 - 4))) {
        /*
         * Page protection violation
         * 不允许预取
         */
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_READ;

    } else if (uiSRR1 & (0x1 << (31 - 3))) {
        /*
         * If the segment is designated as no-execute
         */
        abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_PERM;
        abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;

    } else {
        /*
         * dcbt/dcbtst Instruction
         */
        abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    }
#endif

    if (abtInfo.VMABT_uiType) {
        API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
    }
}
/*********************************************************************************************************
** 函数名称: archAlignmentExceptionHandle
** 功能描述: 非对齐异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archAlignmentExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    addr_t          ulAbortAddr;
    LW_VMM_ABORT    abtInfo;

    ulAbortAddr          = ppcGetDAR();
    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_BUS;

    LW_TCB_GET_CUR(ptcbCur);

    API_VmmAbortIsr(ulRetAddr, ulAbortAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archProgramExceptionHandle
** 功能描述: 程序异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archProgramExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

#if LW_CFG_GDB_EN > 0
    UINT    uiBpType = archDbgTrapType(ulRetAddr, (PVOID)LW_NULL);      /*  断点指令探测                */
    if (uiBpType) {
        if (API_DtraceBreakTrap(ulRetAddr, uiBpType) == ERROR_NONE) {   /*  进入调试接口断点处理        */
            return;
        }
    }
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType   = LW_VMM_ABORT_TYPE_UNDEF;
    abtInfo.VMABT_uiMethod = LW_VMM_ABORT_METHOD_EXEC;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archFpuUnavailableExceptionHandle
** 功能描述: FPU 不可用异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archFpuUnavailableExceptionHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

#if LW_CFG_CPU_FPU_EN > 0
    if (archFpuUndHandle(ptcbCur) == ERROR_NONE) {                      /*  进行 FPU 指令探测           */
        return;
    }
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_FPE;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archSystemCallHandle
** 功能描述: 系统调用处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archSystemCallHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_SYS;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archTraceHandle
** 功能描述: Trace 处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archTraceHandle (addr_t  ulRetAddr)
{
    PLW_CLASS_TCB   ptcbCur;
    LW_VMM_ABORT    abtInfo;

    LW_TCB_GET_CUR(ptcbCur);

    abtInfo.VMABT_uiType = LW_VMM_ABORT_TYPE_TERMINAL;
    API_VmmAbortIsr(ulRetAddr, ulRetAddr, &abtInfo, ptcbCur);
}
/*********************************************************************************************************
** 函数名称: archDecrementerInterruptHandle
** 功能描述: Decrementer 中断处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  : 此函数退出时必须为中断关闭状态.
*********************************************************************************************************/
VOID  archDecrementerInterruptHandle (VOID)
{
    if (_G_bDecInited) {
        ppcSetDEC(_G_uiDecValue);

        archIntHandle(_G_ulDecVector, _G_bDecPreemptive);
    } else {
        ppcSetDEC(0x7FFFFFFF);
    }
}
/*********************************************************************************************************
** 函数名称: archDecrementerInit
** 功能描述: 初始化 Decrementer
** 输　入  : ulVector          Decrementer 中断向量
**           bPreemptive       是否可抢占
**           uiDecValue        Decrementer 值
** 输　出  : NONE
** 全局变量:
** 调用模块:
** 注  意  :
*********************************************************************************************************/
VOID  archDecrementerInit (ULONG    ulVector,
                           BOOL     bPreemptive,
                           UINT32   uiDecValue)
{
    _G_ulDecVector    = ulVector;
    _G_bDecPreemptive = bPreemptive;
    _G_uiDecValue     = uiDecValue;

    ppcSetDEC(uiDecValue);

    _G_bDecInited = LW_TRUE;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
