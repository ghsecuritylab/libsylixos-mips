/*********************************************************************************************************
**
**                                    �й�������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: ppcFpu.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 12 �� 15 ��
**
** ��        ��: PowerPC ��ϵ�ܹ�Ӳ������������ (VFP).
*********************************************************************************************************/

#ifndef __ARCH_PPCFPU_H
#define __ARCH_PPCFPU_H

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

/*********************************************************************************************************
  PowerPC fpu ��������
*********************************************************************************************************/

typedef struct {
    ULONGFUNCPTR    PFPU_pfuncHwSid;
    VOIDFUNCPTR     PFPU_pfuncEnable;
    VOIDFUNCPTR     PFPU_pfuncDisable;
    BOOLFUNCPTR     PFPU_pfuncIsEnable;
    VOIDFUNCPTR     PFPU_pfuncSave;
    VOIDFUNCPTR     PFPU_pfuncRestore;
    VOIDFUNCPTR     PFPU_pfuncCtxShow;
} PPC_FPU_OP;
typedef PPC_FPU_OP *PPPC_FPU_OP;

/*********************************************************************************************************
  PowerPC fpu ��������
*********************************************************************************************************/

#define PPC_VFP_HW_SID(op)              op->PFPU_pfuncHwSid()
#define PPC_VFP_ENABLE(op)              op->PFPU_pfuncEnable()
#define PPC_VFP_DISABLE(op)             op->PFPU_pfuncDisable()
#define PPC_VFP_ISENABLE(op)            op->PFPU_pfuncIsEnable()
#define PPC_VFP_SAVE(op, ctx)           op->PFPU_pfuncSave((ctx))
#define PPC_VFP_RESTORE(op, ctx)        op->PFPU_pfuncRestore((ctx))
#define PPC_VFP_CTXSHOW(op, fd, ctx)    op->PFPU_pfuncCtxShow((fd), (ctx))

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#endif                                                                  /*  __ARCH_PPCFPU_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/