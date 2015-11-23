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
** 文   件   名: armContext.c
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 09 月 01 日
**
** 描        述: MIPS 体系构架上下文处理.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "arch/mips/common/cp0/mipsCp0.h"
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
    ARCH_REG_CTX      *pregctx;
    ARCH_FP_CTX       *pfpctx;
    UINT32             uiCP0Status;
    UINT32             uiGP;

    uiCP0Status  = mipsCp0StatusRead();

    uiCP0Status |= M_StatusIM6 |
                   M_StatusIM5 |
                   M_StatusIM4 |
                   M_StatusIM3 |
                   M_StatusIM2 |
                   M_StatusIE;

    MIPS_EXEC_INS("addi   %0, " MIPS_GP ", 0" : "=r"(uiGP));

    if ((addr_t)pstkTop & 0x7) {                                        /*  保证出栈后 CPU SP 8 字节对齐*/
        pstkTop = (PLW_STACK)((addr_t)pstkTop - 4);                     /*  向低地址推进 4 字节         */
    }

    pfpctx  = (ARCH_FP_CTX  *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX));
    pregctx = (ARCH_REG_CTX *)((PCHAR)pstkTop - sizeof(ARCH_FP_CTX) - sizeof(ARCH_REG_CTX));

    pfpctx->FP_uiFP = (ARCH_REG_T)LW_NULL;
    pfpctx->FP_uiRA = (ARCH_REG_T)LW_NULL;

    pregctx->REG_uiCP0Status = uiCP0Status;

    pregctx->REG_uiEPC = (ARCH_REG_T)pfuncTask;

    pregctx->REG_uiAT = (ARCH_REG_T)0xAF;
    pregctx->REG_uiV0 = (ARCH_REG_T)0x0;
    pregctx->REG_uiV1 = (ARCH_REG_T)0x1;
    pregctx->REG_uiA0 = (ARCH_REG_T)pvArg;
    pregctx->REG_uiA1 = (ARCH_REG_T)0xA1;
    pregctx->REG_uiA2 = (ARCH_REG_T)0xA2;
    pregctx->REG_uiA3 = (ARCH_REG_T)0xA3;
    pregctx->REG_uiT0 = (ARCH_REG_T)0xF0;
    pregctx->REG_uiT1 = (ARCH_REG_T)0xF1;
    pregctx->REG_uiT2 = (ARCH_REG_T)0xF2;
    pregctx->REG_uiT3 = (ARCH_REG_T)0xF3;
    pregctx->REG_uiT4 = (ARCH_REG_T)0xF4;
    pregctx->REG_uiT5 = (ARCH_REG_T)0xF5;
    pregctx->REG_uiT6 = (ARCH_REG_T)0xF6;
    pregctx->REG_uiT7 = (ARCH_REG_T)0xF7;
    pregctx->REG_uiS0 = (ARCH_REG_T)0x80;
    pregctx->REG_uiS1 = (ARCH_REG_T)0x81;
    pregctx->REG_uiS2 = (ARCH_REG_T)0x82;
    pregctx->REG_uiS3 = (ARCH_REG_T)0x83;
    pregctx->REG_uiS4 = (ARCH_REG_T)0x84;
    pregctx->REG_uiS5 = (ARCH_REG_T)0x85;
    pregctx->REG_uiS6 = (ARCH_REG_T)0x86;
    pregctx->REG_uiS7 = (ARCH_REG_T)0x87;
    pregctx->REG_uiT8 = (ARCH_REG_T)0xF8;
    pregctx->REG_uiT9 = (ARCH_REG_T)0xF9;
    pregctx->REG_uiGP = (ARCH_REG_T)uiGP;
    pregctx->REG_uiFP = (ARCH_REG_T)pfpctx->FP_uiFP;
    pregctx->REG_uiRA = (ARCH_REG_T)pfuncTask;

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
    pfpctx->FP_uiFP = pregctxSrc->REG_uiFP;
    pfpctx->FP_uiRA = pregctxSrc->REG_uiRA;

    pregctxDest->REG_uiFP = (ARCH_REG_T)&pfpctx->FP_uiRA;
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

#if CPU_STK_GROWTH == 0
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
    UINT32             uiCP0Status;

    fdprintf(iFd, "\n");

    fdprintf(iFd, "RA  = 0x%08x  ", pstkTop[STK_OFFSET_RA / sizeof(ARCH_REG_T)]);
    fdprintf(iFd, "FP  = 0x%08x  ", pstkTop[STK_OFFSET_FP / sizeof(ARCH_REG_T)]);
    fdprintf(iFd, "GP  = 0x%08x\n", pstkTop[STK_OFFSET_GP / sizeof(ARCH_REG_T)]);

    fdprintf(iFd, "S7  = 0x%08x  ", pstkTop[STK_OFFSET_S7 / sizeof(ARCH_REG_T)]);
    fdprintf(iFd, "S6  = 0x%08x\n", pstkTop[STK_OFFSET_S6 / sizeof(ARCH_REG_T)]);

    fdprintf(iFd, "S5  = 0x%08x  ", pstkTop[STK_OFFSET_S4 / sizeof(ARCH_REG_T)]);
    fdprintf(iFd, "S4  = 0x%08x\n", pstkTop[STK_OFFSET_S4 / sizeof(ARCH_REG_T)]);

    fdprintf(iFd, "S3  = 0x%08x  ", pstkTop[STK_OFFSET_S3 / sizeof(ARCH_REG_T)]);
    fdprintf(iFd, "S2  = 0x%08x\n", pstkTop[STK_OFFSET_S2 / sizeof(ARCH_REG_T)]);

    fdprintf(iFd, "S1  = 0x%08x  ", pstkTop[STK_OFFSET_S1 / sizeof(ARCH_REG_T)]);
    fdprintf(iFd, "S0  = 0x%08x\n", pstkTop[STK_OFFSET_S0 / sizeof(ARCH_REG_T)]);

    fdprintf(iFd, "T9  = 0x%08x  ", pstkTop[STK_OFFSET_T9 / sizeof(ARCH_REG_T)]);
    fdprintf(iFd, "T8  = 0x%08x\n", pstkTop[STK_OFFSET_T8 / sizeof(ARCH_REG_T)]);

    fdprintf(iFd, "T7  = 0x%08x  ", pstkTop[STK_OFFSET_T7 / sizeof(ARCH_REG_T)]);
    fdprintf(iFd, "T6  = 0x%08x\n", pstkTop[STK_OFFSET_T6 / sizeof(ARCH_REG_T)]);

    fdprintf(iFd, "T5  = 0x%08x  ", pstkTop[STK_OFFSET_T5 / sizeof(ARCH_REG_T)]);
    fdprintf(iFd, "T4  = 0x%08x\n", pstkTop[STK_OFFSET_T4 / sizeof(ARCH_REG_T)]);

    fdprintf(iFd, "T3  = 0x%08x  ", pstkTop[STK_OFFSET_T3 / sizeof(ARCH_REG_T)]);
    fdprintf(iFd, "T2  = 0x%08x\n", pstkTop[STK_OFFSET_T2 / sizeof(ARCH_REG_T)]);

    fdprintf(iFd, "T1  = 0x%08x  ", pstkTop[STK_OFFSET_T1 / sizeof(ARCH_REG_T)]);
    fdprintf(iFd, "T0  = 0x%08x\n", pstkTop[STK_OFFSET_T0 / sizeof(ARCH_REG_T)]);

    fdprintf(iFd, "A3  = 0x%08x  ", pstkTop[STK_OFFSET_A3 / sizeof(ARCH_REG_T)]);
    fdprintf(iFd, "A2  = 0x%08x\n", pstkTop[STK_OFFSET_A2 / sizeof(ARCH_REG_T)]);

    fdprintf(iFd, "A1  = 0x%08x  ", pstkTop[STK_OFFSET_A1 / sizeof(ARCH_REG_T)]);
    fdprintf(iFd, "A0  = 0x%08x\n", pstkTop[STK_OFFSET_A0 / sizeof(ARCH_REG_T)]);

    fdprintf(iFd, "V1  = 0x%08x  ", pstkTop[STK_OFFSET_V1 / sizeof(ARCH_REG_T)]);
    fdprintf(iFd, "V0  = 0x%08x\n", pstkTop[STK_OFFSET_V0 / sizeof(ARCH_REG_T)]);

    fdprintf(iFd, "AT  = 0x%08x  ", pstkTop[STK_OFFSET_AT / sizeof(ARCH_REG_T)]);
    fdprintf(iFd, "EPC = 0x%08x  ", pstkTop[STK_OFFSET_EPC / sizeof(ARCH_REG_T)]);

    fdprintf(iFd, "SP  = 0x%08x\n", (ARCH_REG_T)pstkTop);
    uiCP0Status = pstkTop[STK_OFFSET_SR / sizeof(ARCH_REG_T)];
    fdprintf(iFd, "SR  = 0x%08x\n", uiCP0Status);
    fdprintf(iFd, "CP0 Status Register:\n");
    fdprintf(iFd, "CU3 = %4d  ", (uiCP0Status & M_StatusCU3) >> S_StatusCU3);
    fdprintf(iFd, "CU2 = %4d\n", (uiCP0Status & M_StatusCU2) >> S_StatusCU2);
    fdprintf(iFd, "CU1 = %4d  ", (uiCP0Status & M_StatusCU1) >> S_StatusCU1);
    fdprintf(iFd, "CU0 = %4d\n", (uiCP0Status & M_StatusCU0) >> S_StatusCU0);
    fdprintf(iFd, "RP  = %4d  ", (uiCP0Status & M_StatusRP)  >> S_StatusRP);
    fdprintf(iFd, "FR  = %4d\n", (uiCP0Status & M_StatusFR)  >> S_StatusFR);
    fdprintf(iFd, "RE  = %4d  ", (uiCP0Status & M_StatusRE)  >> S_StatusRE);
    fdprintf(iFd, "MX  = %4d\n", (uiCP0Status & M_StatusMX)  >> S_StatusMX);
    fdprintf(iFd, "PX  = %4d  ", (uiCP0Status & M_StatusPX)  >> S_StatusPX);
    fdprintf(iFd, "BEV = %4d\n", (uiCP0Status & M_StatusBEV) >> S_StatusBEV);
    fdprintf(iFd, "TS  = %4d  ", (uiCP0Status & M_StatusTS)  >> S_StatusTS);
    fdprintf(iFd, "SR  = %4d\n", (uiCP0Status & M_StatusSR)  >> S_StatusSR);
    fdprintf(iFd, "NMI = %4d  ", (uiCP0Status & M_StatusNMI) >> S_StatusNMI);
    fdprintf(iFd, "IM7 = %4d\n", (uiCP0Status & M_StatusIM7) >> S_StatusIM7);
    fdprintf(iFd, "IM6 = %4d  ", (uiCP0Status & M_StatusIM6) >> S_StatusIM6);
    fdprintf(iFd, "IM5 = %4d\n", (uiCP0Status & M_StatusIM5) >> S_StatusIM5);
    fdprintf(iFd, "IM4 = %4d  ", (uiCP0Status & M_StatusIM4) >> S_StatusIM4);
    fdprintf(iFd, "IM3 = %4d\n", (uiCP0Status & M_StatusIM3) >> S_StatusIM3);
    fdprintf(iFd, "IM2 = %4d  ", (uiCP0Status & M_StatusIM2) >> S_StatusIM2);
    fdprintf(iFd, "IM1 = %4d\n", (uiCP0Status & M_StatusIM1) >> S_StatusIM1);
    fdprintf(iFd, "IM0 = %4d  ", (uiCP0Status & M_StatusIM0) >> S_StatusIM0);
    fdprintf(iFd, "KX  = %4d\n", (uiCP0Status & M_StatusKX)  >> S_StatusKX);
    fdprintf(iFd, "SX  = %4d  ", (uiCP0Status & M_StatusSX)  >> S_StatusSX);
    fdprintf(iFd, "UX  = %4d\n", (uiCP0Status & M_StatusUX)  >> S_StatusUX);
    fdprintf(iFd, "KSU = %4d  ", (uiCP0Status & M_StatusKSU) >> S_StatusKSU);
    fdprintf(iFd, "UM  = %4d\n", (uiCP0Status & M_StatusUM)  >> S_StatusUM);
    fdprintf(iFd, "SM  = %4d  ", (uiCP0Status & M_StatusSM)  >> S_StatusSM);
    fdprintf(iFd, "ERL = %4d\n", (uiCP0Status & M_StatusERL) >> S_StatusERL);
    fdprintf(iFd, "EXL = %4d  ", (uiCP0Status & M_StatusEXL) >> S_StatusEXL);
    fdprintf(iFd, "IE  = %4d\n", (uiCP0Status & M_StatusIE)  >> S_StatusIE);
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
  END
*********************************************************************************************************/
