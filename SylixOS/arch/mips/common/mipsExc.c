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
** 文   件   名: mipsExc.c
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 09 月 01 日
**
** 描        述: MIPS 体系构架异常处理.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "dtrace.h"
#include "arch/mips/common/cp0/mipsCp0.h"
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
** 函数名称: archCacheErrorHandle
** 功能描述: Cache 错误处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archCacheErrorHandle (addr_t  ulRetAddr)
{
    /*
     * TODO
     */
    while (1) {

    }
}
/*********************************************************************************************************
** 函数名称: archExceptionHandle
** 功能描述: 通用异常处理
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  archExceptionHandle (addr_t  ulRetAddr)
{
    REGISTER UINT32  uiCause   = mipsCp0CauseRead();
    REGISTER UINT32  uiExcCode = ((uiCause & M_CauseExcCode) >> S_CauseExcCode);
    PLW_CLASS_TCB    ptcbCur;

    LW_TCB_GET_CUR(ptcbCur);

    switch (uiExcCode) {
    case EX_MOD:                                                        /* TLB modified                 */
        API_VmmAbortIsr(ulRetAddr, mipsCp0BadVAddrRead(), LW_VMM_ABORT_TYPE_WRITE, ptcbCur);
        break;

    case EX_TLBL:                                                       /* TLB exception(load or ifetch)*/
    case EX_TLBS:                                                       /* TLB exception (store)        */
        API_VmmAbortIsr(ulRetAddr, mipsCp0BadVAddrRead(), LW_VMM_ABORT_TYPE_MAP, ptcbCur);
        break;

    case EX_ADEL:                                                       /* Address error(load or ifetch)*/
    case EX_ADES:                                                       /* Address error (store)        */
        API_VmmAbortIsr(ulRetAddr, mipsCp0BadVAddrRead(), LW_VMM_ABORT_TYPE_TERMINAL, ptcbCur);
        break;

    case EX_IBE:                                                        /* Instruction Bus Error        */
    case EX_DBE:                                                        /* Data Bus Error               */
        API_VmmAbortIsr(ulRetAddr, mipsCp0BadVAddrRead(), LW_VMM_ABORT_TYPE_BUS, ptcbCur);
        break;

    case EX_SYS:                                                        /* Syscall                      */
        API_VmmAbortIsr(ulRetAddr, mipsCp0BadVAddrRead(), LW_VMM_ABORT_TYPE_SYS, ptcbCur);
        break;

    case EX_BP:                                                         /* Breakpoint                   */
    case EX_TR:                                                         /* Trap instruction             */
        API_VmmAbortIsr(ulRetAddr, mipsCp0BadVAddrRead(), LW_VMM_ABORT_TYPE_BREAK, ptcbCur);
        break;

    case EX_RI:                                                         /* Reserved instruction         */
        API_VmmAbortIsr(ulRetAddr, mipsCp0BadVAddrRead(), LW_VMM_ABORT_TYPE_UNDEF, ptcbCur);
        break;

    case EX_FPE:                                                        /* floating point exception     */
        API_VmmAbortIsr(ulRetAddr, mipsCp0BadVAddrRead(), LW_VMM_ABORT_TYPE_FPE, ptcbCur);
        break;

    case EX_CPU:                                                        /* CoProcessor Unusable         */
    case EX_OV:                                                         /* OVerflow                     */
    case EX_C2E:                                                        /* COP2 exception               */
    case EX_MDMX:                                                       /* MDMX exception               */
    case EX_WATCH:                                                      /* Watch exception              */
    case EX_MCHECK:                                                     /* Machine check exception      */
    case EX_CacheErr:                                                   /* Cache error caused re-entry  */
                                                                        /* to Debug Mode                */
        API_VmmAbortIsr(ulRetAddr, mipsCp0BadVAddrRead(), LW_VMM_ABORT_TYPE_TERMINAL, ptcbCur);
        break;

    default:
        _DebugFormat(__ERRORMESSAGE_LEVEL, "Unknow exception: %d\r\n", uiExcCode);
        while (1) {
        }
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
