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
** 文   件   名: ppcContext.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 12 月 15 日
**
** 描        述: PowerPC 体系构架上下文处理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "ppcSpr.h"
/*********************************************************************************************************
** 函数名称: archTaskCtxCreate
** 功能描述: 创建任务上下文
** 输　入  : pfuncTask      任务入口
**           pvArg          入口参数
**           pstkTop        初始化堆栈起点
**           ulOpt          任务创建选项
** 输　出  : 初始化堆栈结束点
** 全局变量: 
** 调用模块: 
** 注  意  : 堆栈从高地址向低地址增长.
*********************************************************************************************************/
PLW_STACK  archTaskCtxCreate (PTHREAD_START_ROUTINE  pfuncTask,
                              PVOID                  pvArg,
                              PLW_STACK              pstkTop, 
                              ULONG                  ulOpt)
{
    ARCH_REG_CTX       *pregctx;
    ARCH_FP_CTX        *pfpctx;
    ARCH_REG_T          uiMsr;
    ARCH_REG_T          uiSrr1;

    uiMsr   =  ppcGetMSR();                                             /*  获得当前 MSR 的值           */
#if LW_CFG_VMM_EN > 0
#if defined(ARCH_PPC_MSR_DR)
    uiSrr1  = (uiMsr | ARCH_PPC_MSR_EE |
               ARCH_PPC_MSR_IR | ARCH_PPC_MSR_DR);                      /*  使能中断和 MMU              */
#elif defined(ARCH_PPC_MSR_DS)
    uiSrr1  = (uiMsr | ARCH_PPC_MSR_EE |
               ARCH_PPC_MSR_IS | ARCH_PPC_MSR_DS);                      /*  使能中断和 MMU              */
#else
#error "bit to enable mmu is not defined"
#endif                                                                  /*  defined(ARCH_PPC_MSR_DR)    */
#else
    uiSrr1  = (uiMsr | ARCH_PPC_MSR_EE);                                /*  使能中断                    */
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    uiSrr1 &= ~ARCH_PPC_MSR_FP;                                         /*  禁能 FPU                    */
    
    if ((addr_t)pstkTop & 0x7) {                                        /*  保证出栈后 CPU SP 8 字节对齐*/
        pstkTop = (PLW_STACK)((addr_t)pstkTop - 4);                     /*  向低地址推进 4 字节         */
    }

    pfpctx  = (ARCH_FP_CTX  *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));
    pregctx = (ARCH_REG_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX) - sizeof(ARCH_REG_CTX));

    pfpctx->FP_uiFp     = (ARCH_REG_T)LW_NULL;
    pfpctx->FP_uiLr     = (ARCH_REG_T)LW_NULL;

    pregctx->REG_uiR0   = 0;
    pregctx->REG_uiSp   = (ARCH_REG_T)pfpctx;                           /*  R1 == SP 完全出栈后的 SP    */
    pregctx->REG_uiR2   = 2;
    pregctx->REG_uiR3   = (ARCH_REG_T)pvArg;                            /*  R3 用于参数传递             */
    pregctx->REG_uiR4   = 4;
    pregctx->REG_uiR5   = 5;
    pregctx->REG_uiR6   = 6;
    pregctx->REG_uiR7   = 7;
    pregctx->REG_uiR8   = 8;
    pregctx->REG_uiR9   = 9;
    pregctx->REG_uiR10  = 10;
    pregctx->REG_uiR11  = 11;
    pregctx->REG_uiR12  = 12;
    pregctx->REG_uiR13  = 13;
    pregctx->REG_uiR14  = 14;
    pregctx->REG_uiR15  = 15;
    pregctx->REG_uiR16  = 16;
    pregctx->REG_uiR17  = 17;
    pregctx->REG_uiR18  = 18;
    pregctx->REG_uiR19  = 19;
    pregctx->REG_uiR20  = 20;
    pregctx->REG_uiR21  = 21;
    pregctx->REG_uiR22  = 22;
    pregctx->REG_uiR23  = 23;
    pregctx->REG_uiR24  = 24;
    pregctx->REG_uiR25  = 25;
    pregctx->REG_uiR26  = 26;
    pregctx->REG_uiR27  = 27;
    pregctx->REG_uiR28  = 28;
    pregctx->REG_uiR29  = 29;
    pregctx->REG_uiR30  = 30;
    pregctx->REG_uiFp   = pfpctx->FP_uiFp;

    pregctx->REG_uiLr   = (ARCH_REG_T)pfuncTask;
    pregctx->REG_uiSrr0 = (ARCH_REG_T)pfuncTask;                        /*  RFI 返回的 地址             */
    pregctx->REG_uiSrr1 = uiSrr1;                                       /*  RFI 后的 MSR                */
    pregctx->REG_uiMsr  = uiMsr;                                        /*  现时的 MSR                  */
    pregctx->REG_uiCtr  = 0;
    pregctx->REG_uiXer  = 0;
    pregctx->REG_uiCr   = 0;

    return  ((PLW_STACK)pregctx);
}
/*********************************************************************************************************
** 函数名称: archTaskCtxSetFp
** 功能描述: 设置任务上下文栈帧 (用于 backtrace 回溯, 详情请见 backtrace 相关文件)
** 输　入  : pstkDest  目的 stack frame
**           pstkSrc   源端 stack frame
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archTaskCtxSetFp (PLW_STACK  pstkDest, PLW_STACK  pstkSrc)
{
    ARCH_REG_CTX      *pregctxDest = (ARCH_REG_CTX *)pstkDest;
    ARCH_REG_CTX      *pregctxSrc  = (ARCH_REG_CTX *)pstkSrc;
    ARCH_FP_CTX       *pfpctx      = (ARCH_FP_CTX *)((PCHAR)pregctxDest + sizeof(ARCH_REG_CTX));

    /*
     *  在 ARCH_FP_CTX 区域内, 模拟了一次
     *  push {fp, lr}
     *  add  fp, sp, #4
     */
    pfpctx->FP_uiFp = pregctxSrc->REG_uiFp;
    pfpctx->FP_uiLr = pregctxSrc->REG_uiLr;

    pregctxDest->REG_uiFp = (ARCH_REG_T)&pfpctx->FP_uiLr;
}
/*********************************************************************************************************
** 函数名称: archTaskRegsGet
** 功能描述: 通过栈顶指针获取寄存器表 (满栈结构)
** 输　入  : pstkTop        堆栈顶点
**           pregSp         SP 指针
** 输　出  : 寄存器结构
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
ARCH_REG_CTX  *archTaskRegsGet (PLW_STACK  pstkTop, ARCH_REG_T *pregSp)
{
    ARCH_REG_T  regSp = (ARCH_REG_T)pstkTop;
    
#if	CPU_STK_GROWTH == 0
    regSp -= sizeof(ARCH_REG_CTX);
#else
    regSp += sizeof(ARCH_REG_CTX);
#endif

    *pregSp = regSp;
    
    return  ((ARCH_REG_CTX *)pstkTop);
}
/*********************************************************************************************************
** 函数名称: archTaskRegsSet
** 功能描述: 通过栈顶指针获取寄存器表 (满栈结构)
** 输　入  : pstkTop        堆栈顶点
**           pregctx        寄存器表
** 输　出  : 寄存器结构
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  archTaskRegsSet (PLW_STACK  pstkTop, const ARCH_REG_CTX  *pregctx)
{
    *(ARCH_REG_CTX *)pstkTop = *pregctx;
}
/*********************************************************************************************************
** 函数名称: archTaskCtxShow
** 功能描述: 打印任务上下文
** 输　入  : iFd        文件描述符
			 pstkTop    堆栈栈顶
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

VOID  archTaskCtxShow (INT  iFd, PLW_STACK  pstkTop)
{
    ARCH_REG_CTX       *pregctx = (ARCH_REG_CTX *)pstkTop;

    fdprintf(iFd, "\n");

    fdprintf(iFd, "R0  = 0x%08x  ", pregctx->REG_uiR0);
    fdprintf(iFd, "R1  = 0x%08x\n", pregctx->REG_uiSp);                 /*  R1 == SP 异常时的 SP        */

    fdprintf(iFd, "R2  = 0x%08x  ", pregctx->REG_uiR2);
    fdprintf(iFd, "R3  = 0x%08x\n", pregctx->REG_uiR3);

    fdprintf(iFd, "R4  = 0x%08x  ", pregctx->REG_uiR4);
    fdprintf(iFd, "R5  = 0x%08x\n", pregctx->REG_uiR5);

    fdprintf(iFd, "R6  = 0x%08x  ", pregctx->REG_uiR6);
    fdprintf(iFd, "R7  = 0x%08x\n", pregctx->REG_uiR7);

    fdprintf(iFd, "R8  = 0x%08x  ", pregctx->REG_uiR8);
    fdprintf(iFd, "R9  = 0x%08x\n", pregctx->REG_uiR9);

    fdprintf(iFd, "R10 = 0x%08x  ", pregctx->REG_uiR10);
    fdprintf(iFd, "R11 = 0x%08x\n", pregctx->REG_uiR11);

    fdprintf(iFd, "R12 = 0x%08x  ", pregctx->REG_uiR12);
    fdprintf(iFd, "R13 = 0x%08x\n", pregctx->REG_uiR13);

    fdprintf(iFd, "R14 = 0x%08x  ", pregctx->REG_uiR14);
    fdprintf(iFd, "R15 = 0x%08x\n", pregctx->REG_uiR15);

    fdprintf(iFd, "R16 = 0x%08x  ", pregctx->REG_uiR16);
    fdprintf(iFd, "R17 = 0x%08x\n", pregctx->REG_uiR17);

    fdprintf(iFd, "R18 = 0x%08x  ", pregctx->REG_uiR18);
    fdprintf(iFd, "R19 = 0x%08x\n", pregctx->REG_uiR19);

    fdprintf(iFd, "R20 = 0x%08x  ", pregctx->REG_uiR20);
    fdprintf(iFd, "R21 = 0x%08x\n", pregctx->REG_uiR21);

    fdprintf(iFd, "R22 = 0x%08x  ", pregctx->REG_uiR22);
    fdprintf(iFd, "R23 = 0x%08x\n", pregctx->REG_uiR23);

    fdprintf(iFd, "R24 = 0x%08x  ", pregctx->REG_uiR24);
    fdprintf(iFd, "R25 = 0x%08x\n", pregctx->REG_uiR25);

    fdprintf(iFd, "R26 = 0x%08x  ", pregctx->REG_uiR26);
    fdprintf(iFd, "R27 = 0x%08x\n", pregctx->REG_uiR27);

    fdprintf(iFd, "R28 = 0x%08x  ", pregctx->REG_uiR28);
    fdprintf(iFd, "R29 = 0x%08x\n", pregctx->REG_uiR29);

    fdprintf(iFd, "R30 = 0x%08x  ", pregctx->REG_uiR30);
    fdprintf(iFd, "R31 = 0x%08x\n", pregctx->REG_uiFp);                 /*  R31 == FP                   */

    fdprintf(iFd, "FP  = 0x%08x  ", pregctx->REG_uiFp);
    fdprintf(iFd, "SP  = 0x%08x\n", (ARCH_REG_T)pstkTop);               /*  异常压栈后的 SP             */

    fdprintf(iFd, "LR  = 0x%08x  ", pregctx->REG_uiLr);
    fdprintf(iFd, "PC  = 0x%08x\n", pregctx->REG_uiSrr0);               /*  异常时的 PC                 */

    fdprintf(iFd, "MSR = 0x%08x  ", pregctx->REG_uiSrr1);               /*  异常时的 MSR                */
    fdprintf(iFd, "CR  = 0x%08x\n", pregctx->REG_uiCr);

    fdprintf(iFd, "CTR = 0x%08x  ", pregctx->REG_uiCtr);
    fdprintf(iFd, "XER = 0x%08x\n", pregctx->REG_uiXer);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
