/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: mipsVfp64.c
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 12 月 01 日
**
** 描        述: MIPS 体系架构 VPU64 支持.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0
#include "../mipsFpu.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static MIPS_FPU_OP  _G_fpuopVfp64;
static INT          _G_iVfp64DNum;
/*********************************************************************************************************
  实现函数
*********************************************************************************************************/
extern UINT32   mipsVfp64GetFIR(VOID);
extern VOID     mipsVfp64Enable(VOID);
extern VOID     mipsVfp64Disable(VOID);
extern BOOL     mipsVfp64IsEnable(VOID);
extern VOID     mipsVfp64Save16(PVOID pvFpuCtx);
extern VOID     mipsVfp64Restore16(PVOID pvFpuCtx);
extern VOID     mipsVfp64Save32(PVOID pvFpuCtx);
extern VOID     mipsVfp64Restore32(PVOID pvFpuCtx);
/*********************************************************************************************************
** 函数名称: mipsVfp64CtxShow
** 功能描述: 显示 VFP 上下文
** 输　入  : iFd       输出文件描述符
**           pvFpuCtx  VFP 上下文
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  mipsVfp64CtxShow (INT iFd, PVOID pvFpuCtx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    LW_FPU_CONTEXT *pfpuCtx    = (LW_FPU_CONTEXT *)pvFpuCtx;
    ARCH_FPU_CTX   *pcpufpuCtx = &pfpuCtx->FPUCTX_fpuctxContext;
    INT             i;

    fdprintf(iFd, "FPCSR   = 0x%08x\n", pcpufpuCtx->FPUCTX_uiFpcsr);

    for (i = 0; i < _G_iVfp64DNum; i += 2) {
        fdprintf(iFd, "FPS[%02d] = 0x%08x  ", i,     pcpufpuCtx->FPUCTX_uiDreg[i]);
        fdprintf(iFd, "FPS[%02d] = 0x%08x\n", i + 1, pcpufpuCtx->FPUCTX_uiDreg[i + 1]);
    }
#endif
}
/*********************************************************************************************************
** 函数名称: mipsVfp64PrimaryInit
** 功能描述: 初始化并获取 VFP 控制器操作函数集
** 输　入  : pcMachineName 机器名
**           pcFpuName     浮点运算器名
** 输　出  : 操作函数集
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PMIPS_FPU_OP  mipsVfp64PrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    if (mipsVfp64GetFIR() == 0) {
        _G_iVfp64DNum = 32;
        _G_fpuopVfp64.MFPU_pfuncSave    = mipsVfp64Save16;
        _G_fpuopVfp64.MFPU_pfuncRestore = mipsVfp64Restore16;

    } else {
        _G_iVfp64DNum = 64;
        _G_fpuopVfp64.MFPU_pfuncSave    = mipsVfp64Save32;
        _G_fpuopVfp64.MFPU_pfuncRestore = mipsVfp64Restore32;
    }

    _G_fpuopVfp64.MFPU_pfuncEnable   = mipsVfp64Enable;
    _G_fpuopVfp64.MFPU_pfuncDisable  = mipsVfp64Disable;
    _G_fpuopVfp64.MFPU_pfuncIsEnable = mipsVfp64IsEnable;
    _G_fpuopVfp64.MFPU_pfuncCtxShow  = mipsVfp64CtxShow;

    return  (&_G_fpuopVfp64);
}
/*********************************************************************************************************
** 函数名称: mipsVfp64SecondaryInit
** 功能描述: 初始化 VFP 控制器
** 输　入  : pcMachineName 机器名
**           pcFpuName     浮点运算器名
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  mipsVfp64SecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcFpuName)
{
    (VOID)pcMachineName;
    (VOID)pcFpuName;
}

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
