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
** ��   ��   ��: ppc_support.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2015 �� 11 �� 26 ��
**
** ��        ��: PowerPC ��ϵ����֧��.
*********************************************************************************************************/

#ifndef __ARCH_PPC_SUPPORT_H
#define __ARCH_PPC_SUPPORT_H

#define __LW_SCHEDULER_BUG_TRACE_EN                                     /*  ���Զ�˵�����              */
#define __LW_KERNLOCK_BUG_TRACE_EN                                      /*  �����ں���                  */

/*********************************************************************************************************
  ������ͷ�ļ�
*********************************************************************************************************/

#include "arch/assembler.h"

/*********************************************************************************************************
  �洢������ (CPU ջ������)
*********************************************************************************************************/

#define CPU_STK_GROWTH              1                                   /*  1����ջ�Ӹߵ�ַ��͵�ַ     */
                                                                        /*  0����ջ�ӵ͵�ַ��ߵ�ַ     */
/*********************************************************************************************************
  arch �Ѿ��ṩ�Ľӿ�����:
*********************************************************************************************************/
/*********************************************************************************************************
  PowerPC ����������
*********************************************************************************************************/

VOID    archAssert(INT  iCond, CPCHAR  pcFunc, CPCHAR  pcFile, INT  iLine);

/*********************************************************************************************************
  PowerPC �������߳���������ؽӿ�
*********************************************************************************************************/

PLW_STACK       archTaskCtxCreate(PTHREAD_START_ROUTINE  pfuncTask,
                                  PVOID                  pvArg,
                                  PLW_STACK              pstkTop, 
                                  ULONG                  ulOpt);
VOID            archTaskCtxSetFp(PLW_STACK  pstkDest, PLW_STACK  pstkSrc);
ARCH_REG_CTX   *archTaskRegsGet(PLW_STACK  pstkTop, ARCH_REG_T *pregSp);
VOID            archTaskRegsSet(PLW_STACK  pstkTop, const ARCH_REG_CTX  *pregctx);

#if LW_CFG_DEVICE_EN > 0
VOID        archTaskCtxShow(INT  iFd, PLW_STACK  pstkTop);
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */

VOID        archTaskCtxStart(PLW_CLASS_CPU  pcpuSw);
VOID        archTaskCtxSwitch(PLW_CLASS_CPU  pcpuSw);

#if LW_CFG_COROUTINE_EN > 0
VOID        archCrtCtxSwitch(PLW_CLASS_CPU  pcpuSw);
#endif                                                                  /*  LW_CFG_COROUTINE_EN > 0     */

VOID        archIntCtxLoad(PLW_CLASS_CPU  pcpuSw);
VOID        archSigCtxLoad(PVOID  pvStack);

/*********************************************************************************************************
  PowerPC ���������Խӿ�
*********************************************************************************************************/

#if LW_CFG_GDB_EN > 0
VOID    archDbgBpInsert(addr_t   ulAddr, size_t stSize, ULONG  *pulIns, BOOL  bLocal);
VOID    archDbgAbInsert(addr_t   ulAddr, ULONG  *pulIns);
VOID    archDbgBpRemove(addr_t   ulAddr, size_t stSize, ULONG   ulIns, BOOL  bLocal);
VOID    archDbgBpPrefetch(addr_t ulAddr);
UINT    archDbgTrapType(addr_t   ulAddr, PVOID   pvArch);
VOID    archDbgBpAdjust(PVOID  pvDtrace, PVOID   pvtm);
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */

/*********************************************************************************************************
  PowerPC �������쳣�ӿ�
*********************************************************************************************************/

VOID    archIntHandle(ULONG  ulVector, BOOL  bPreemptive);              /*  bspIntHandle ��Ҫ���ô˺��� */

/*********************************************************************************************************
  PowerPC ͨ�ÿ�
*********************************************************************************************************/

INT     archFindLsb(UINT32 ui32);
INT     archFindMsb(UINT32 ui32);

/*********************************************************************************************************
  PowerPC ��������׼�ײ��
*********************************************************************************************************/

#define	KN_INT_DISABLE()            archIntDisable()
#define	KN_INT_ENABLE(intLevel)     archIntEnable(intLevel)
#define KN_INT_ENABLE_FORCE()       archIntEnableForce()

INTREG  archIntDisable(VOID);
VOID    archIntEnable(INTREG  iregInterLevel);
VOID    archIntEnableForce(VOID);

VOID    archPageCopy(PVOID pvTo, PVOID pvFrom);

#define KN_COPY_PAGE(to, from)      archPageCopy(to, from)

VOID    archReboot(INT  iRebootType, addr_t  ulStartAddress);

/*********************************************************************************************************
  PowerPC ����������
*********************************************************************************************************/

#define PPC_MACHINE_403         "403"
#define PPC_MACHINE_405         "405"
#define PPC_MACHINE_440         "440"

#define PPC_MACHINE_8XX         "8XX"
#define PPC_MACHINE_860         "860"

#define PPC_MACHINE_603         "603"
#define PPC_MACHINE_EC603       "EC603"
#define PPC_MACHINE_604         "604"
#define PPC_MACHINE_750         "750"
#define PPC_MACHINE_MPC83XX     "MPC83XX"

#define PPC_MACHINE_970         "970"

#define PPC_MACHINE_E200        "E200"

#define PPC_MACHINE_E500        "E500"

/*********************************************************************************************************
  PowerPC ������ CACHE ����
*********************************************************************************************************/

#if LW_CFG_CACHE_EN > 0
VOID    archCacheReset(CPCHAR     pcMachineName);
VOID    archCacheInit(CACHE_MODE  uiInstruction, CACHE_MODE  uiData, CPCHAR  pcMachineName);

#define __ARCH_CACHE_INIT   archCacheInit
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

/*********************************************************************************************************
  PowerPC ������ MMU ���� (ppcMmu*() �ɹ��� bsp ʹ��)
*********************************************************************************************************/

#if LW_CFG_VMM_EN > 0
VOID    archMmuInit(CPCHAR  pcMachineName);

#define __ARCH_MMU_INIT     archMmuInit
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

/*********************************************************************************************************
  PowerPC �������������������
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
VOID    archSpinInit(spinlock_t  *psl);
VOID    archSpinDelay(VOID);
VOID    archSpinNotify(VOID);

#define __ARCH_SPIN_INIT    archSpinInit
#define __ARCH_SPIN_DELAY   archSpinDelay
#define __ARCH_SPIN_NOTIFY  archSpinNotify

INT     archSpinLock(spinlock_t  *psl);
INT     archSpinTryLock(spinlock_t  *psl);
INT     archSpinUnlock(spinlock_t  *psl);

#define __ARCH_SPIN_LOCK    archSpinLock
#define __ARCH_SPIN_TRYLOCK archSpinTryLock
#define __ARCH_SPIN_UNLOCK  archSpinUnlock

INT     archSpinLockRaw(spinlock_t  *psl);
INT     archSpinTryLockRaw(spinlock_t  *psl);
INT     archSpinUnlockRaw(spinlock_t  *psl);

#define __ARCH_SPIN_LOCK_RAW    archSpinLockRaw
#define __ARCH_SPIN_TRYLOCK_RAW archSpinTryLockRaw
#define __ARCH_SPIN_UNLOCK_RAW  archSpinUnlockRaw

ULONG   archMpCur(VOID);
VOID    archMpInt(ULONG  ulCPUId);
#endif                                                                  /*  LW_CFG_SMP_EN               */

/*********************************************************************************************************
  PowerPC �ڴ�����
*********************************************************************************************************/

#define KN_BARRIER()            __asm__ __volatile__ ("" : : : "memory")
#define KN_SYNC()               __asm__ __volatile__ ("sync" : : : "memory")

#define KN_MB()                 KN_SYNC()
#define KN_RMB()                KN_SYNC()
#define KN_WMB()                KN_SYNC()

#if LW_CFG_SMP_EN > 0
#define KN_SMP_MB()             __asm__ __volatile__ ("sync" : : : "memory")
#define KN_SMP_RMB()            __asm__ __volatile__ ("sync" : : : "memory")
#define KN_SMP_WMB()            __asm__ __volatile__ ("sync" : : : "memory")
#else
#define KN_SMP_MB()             KN_BARRIER()
#define KN_SMP_RMB()            KN_BARRIER()
#define KN_SMP_WMB()            KN_BARRIER()
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */

/*********************************************************************************************************
  PowerPC ����������������
*********************************************************************************************************/

#define PPC_FPU_NONE        "none"
#define PPC_FPU_VFP    		"vfp"

#if LW_CFG_CPU_FPU_EN > 0
VOID    archFpuPrimaryInit(CPCHAR  pcMachineName, CPCHAR  pcFpuName);
#if LW_CFG_SMP_EN > 0
VOID    archFpuSecondaryInit(CPCHAR  pcMachineName, CPCHAR  pcFpuName);
#endif                                                                  /*  LW_CFG_SMP_EN               */

VOID    archFpuCtxInit(PVOID pvFpuCtx);
VOID    archFpuEnable(VOID);
VOID    archFpuDisable(VOID);
VOID    archFpuSave(PVOID pvFpuCtx);
VOID    archFpuRestore(PVOID pvFpuCtx);

#define __ARCH_FPU_CTX_INIT     archFpuCtxInit
#define __ARCH_FPU_ENABLE       archFpuEnable
#define __ARCH_FPU_DISABLE      archFpuDisable
#define __ARCH_FPU_SAVE         archFpuSave
#define __ARCH_FPU_RESTORE      archFpuRestore

#if LW_CFG_DEVICE_EN > 0
VOID    archFpuCtxShow(INT  iFd, PVOID pvFpuCtx);
#define __ARCH_FPU_CTX_SHOW     archFpuCtxShow
#endif                                                                  /*  #if LW_CFG_DEVICE_EN        */

INT     archFpuUndHandle(PLW_CLASS_TCB  ptcbCur);
#endif                                                                  /*  LW_CFG_CPU_FPU_EN           */

/*********************************************************************************************************
  bsp ��Ҫ�ṩ�Ľӿ�����:
*********************************************************************************************************/
/*********************************************************************************************************
  PowerPC �������ж������ж�
*********************************************************************************************************/

VOID    bspIntInit(VOID);
VOID    bspIntHandle(VOID);

VOID    bspIntVectorEnable(ULONG  ulVector);
VOID    bspIntVectorDisable(ULONG  ulVector);
BOOL    bspIntVectorIsEnable(ULONG  ulVector);

#define __ARCH_INT_VECTOR_ENABLE    bspIntVectorEnable
#define __ARCH_INT_VECTOR_DISABLE   bspIntVectorDisable
#define __ARCH_INT_VECTOR_ISENABLE  bspIntVectorIsEnable

/*********************************************************************************************************
  CPU ��ʱ��ʱ��
*********************************************************************************************************/

VOID    bspTickInit(VOID);
VOID    bspDelayUs(ULONG ulUs);
VOID    bspDelayNs(ULONG ulNs);

#if LW_CFG_TIME_HIGH_RESOLUTION_EN > 0
VOID    bspTickHighResolution(struct timespec *ptv);
#endif                                                                  /*  LW_CFG_TIME_HIGH_...        */

/*********************************************************************************************************
  �ں˹ؼ�λ�ûص�����
*********************************************************************************************************/

VOID    bspTaskCreateHook(LW_OBJECT_ID  ulId);
VOID    bspTaskDeleteHook(LW_OBJECT_ID  ulId, PVOID  pvReturnVal, PLW_CLASS_TCB  ptcb);
VOID    bspTaskSwapHook(LW_OBJECT_HANDLE  hOldThread, LW_OBJECT_HANDLE  hNewThread);
VOID    bspTaskIdleHook(VOID);
VOID    bspTickHook(INT64  i64Tick);
VOID    bspWdTimerHook(LW_OBJECT_ID  ulId);
VOID    bspTCBInitHook(LW_OBJECT_ID  ulId, PLW_CLASS_TCB   ptcb);
VOID    bspKernelInitHook(VOID);

/*********************************************************************************************************
  ϵͳ�������� (ulStartAddress �������ڵ���, BSP �ɺ���)
*********************************************************************************************************/

VOID    bspReboot(INT  iRebootType, addr_t  ulStartAddress);

/*********************************************************************************************************
  ϵͳ�ؼ���Ϣ��ӡ (��ӡ��Ϣ���������κβ���ϵͳ api)
*********************************************************************************************************/

VOID    bspDebugMsg(CPCHAR pcMsg);

/*********************************************************************************************************
  BSP ��Ϣ
*********************************************************************************************************/

CPCHAR  bspInfoCpu(VOID);
CPCHAR  bspInfoCache(VOID);
CPCHAR  bspInfoPacket(VOID);
CPCHAR  bspInfoVersion(VOID);
ULONG   bspInfoHwcap(VOID);
addr_t  bspInfoRomBase(VOID);
size_t  bspInfoRomSize(VOID);
addr_t  bspInfoRamBase(VOID);
size_t  bspInfoRamSize(VOID);

/*********************************************************************************************************
  PowerPC ������ CACHE ����
*********************************************************************************************************/

#if LW_CFG_PPC_CACHE_L2 > 0

#endif                                                                  /*  LW_CFG_PPC_CACHE_L2 > 0     */

/*********************************************************************************************************
  PowerPC ������ MMU ����
*********************************************************************************************************/

#if LW_CFG_VMM_EN > 0
ULONG   bspMmuPgdMaxNum(VOID);
ULONG   bspMmuPteMaxNum(VOID);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

/*********************************************************************************************************
  PowerPC ��������˲���
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
VOID    bspMpInt(ULONG  ulCPUId);
VOID    bspCpuUp(ULONG  ulCPUId);                                       /*  ����һ�� CPU                */

#if LW_CFG_SMP_CPU_DOWN_EN > 0
VOID    bspCpuDown(ULONG  ulCPUId);                                     /*  ֹͣһ�� CPU                */
#endif                                                                  /*  LW_CFG_SMP_CPU_DOWN_EN > 0  */
#endif                                                                  /*  LW_CFG_SMP_EN               */

/*********************************************************************************************************
  PowerPC ������ CPU ���� (������Ƶ)
*********************************************************************************************************/

#if LW_CFG_POWERM_EN > 0
VOID    bspSuspend(VOID);                                               /*  ϵͳ����                    */
VOID    bspCpuPowerSet(UINT  uiPowerLevel);                             /*  ���� CPU ��Ƶ�ȼ�           */
VOID    bspCpuPowerGet(UINT *puiPowerLevel);                            /*  ��ȡ CPU ��Ƶ�ȼ�           */
#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */

#endif                                                                  /*  __ARCH_PPC_SUPPORT_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/