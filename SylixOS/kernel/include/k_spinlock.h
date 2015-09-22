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
** ��   ��   ��: k_spinlock.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 04 �� 07 ��
**
** ��        ��: ���������ͼ������������塣
**
** ע        ��: ������ spinlock ������Ҫ����ֲ arch ��㶨, ���� spinlock ����� cpu �ֶεĸ�ֵ, ���￼��
                 ���ܶ� CPU ��Ƴ��Ҷ����Լ���� spinlock ��Ч�ʵķ���, ������ͬ, �������￪�Ÿ� arch ��
                 ����, Ϊ����Ӧ��ͬ��ϵ�ṹ, ���� SL_ulCounter �ֶ���Ϊ�������. ��Ҫ arch ��֧��, ��Ȼ
                 arch ��Ҳ����ʹ�� SL_sltData ͬʱ��������������. (��ֲ�����֧�� spinlock ������)
                 ��ҳ���Ĳ�����ͬ, spinlock �漰 CACHE �Ĳ�����ȫ�ɲ���ϵͳ��ֲ�������.
                 ע��, ���е� spinlock ����ʱ, �����ں˲�û�г�ʼ�� CACHE ��, ���Բ��ܵ��ò���ϵͳ�ṩ��
                 CACHE ��������, ��Ҫ������ֲ���ڵ� CACHE ��������!
                 ͬʱ, spinlock Ԥ������ص� CPU �Ŷ�����, �����������ȼ�˳��, Ҳ����ʹ�� FIFO �ȴ�, ����
                 ȫȡ���� arch �����ֲ����.
** BUG:
2013.07.17  �����Ҫ���ڴ�����, ��֤ SMP ϵͳ spinlock ����ִ�а�ȫ.
*********************************************************************************************************/

#ifndef __K_SPINLOCK_H
#define __K_SPINLOCK_H

/*********************************************************************************************************
  ע��1: 
  
  1: �� spinlock �Ĳ���Ҫ��ʼ����(��Դ����������), ���򽫳��ֶ��������Σ��!
  
  2: ����Դ�Ļ�ȡҪ��˳��, ����ͻ���ֶ������!
  
     ����: 
           LW_SPINLOCK_LOCK_IRQ();
           __KERNEL_ENTER();
           
           ...
           
           LW_SPINLOCK_UNLOCK_IRQ();    ----- Σ��!(���� CPU ��������� spinlock ������Դ�ķ���Ȩ)
           ...                                     (Ȼ������ CPU ׼����ȡ�ں˷���Ȩʱ, ��������! )
           LW_SPINLOCK_LOCK_IRQ();      ----- Σ��!
           
           ...
           
           LW_SPINLOCK_UNLOCK_IRQ();
           __KERNEL_EXIT();
           
    ���ϴ���������Ҫ�ĳ�:
           
           LW_SPINLOCK_LOCK_IRQ();      ----- (1)
           __KERNEL_ENTER();            ----- (2)
           
           ...
           
           KN_INT_ENABLE();             ----- ��ȫ!
           ...
           KN_INT_DISABLE();            ----- ��ȫ!
           
           ...
           LW_SPINLOCK_UNLOCK_IRQ();    ----- (3)
           __KERNEL_EXIT();             ----- (4)
           
    �ڽ���(����)�ں˺�, �������׵��ͷ� spinlock, �������������˵ľ���, ֻ����ȷ���˳���ǰ���²���
    �ͷ�!
    
  ����ϵͳ�Զ�˴����Ϊ���ֻ�����Դ: 
  
    1: Ӧ��ģ����Ե� spinlock, ����: �ź���, ��Ϣ����, ... 
    2: �ں˺������ݽṹ spinlock, ����: �����������, �жϿ��ƵȵȺ�������. ...
    
    spinlock ��ȡ��˳��һ���Ǵ� 1 -> 2, ����������Ч��������Դ����.
    
  ע��2: 
  
    KN_INT_ENABLE();  KN_INT_DISABLE(); ����ֻ�ܲ��������ߵ� CPU, ����ǰ CPU, ֻ�ܽ�ֹ��ǰ CPU ����
    ��. �����ܽ�ֹ���� CPU ���ж�. (��ֹ��ǰ CPU ���жϿ��Ա�֤��ǰ CPU ������ռ)
    
    spinlock �����ռ��һ�� spinlock �ľ����߽�, ������ռ�� spinlock �ı����������У��������ڵò��� 
    spinlock ��ռ�߽�����������, ��������ߵ����ȼ����ڱ����ߵ����ȼ�, ���γ�һ�������ľ���, ��Ϊ��
    �����޷��õ����ж���Զ�����ͷ� spinlock. �����������ڲ��ܵõ�һ���������ͷŵ� spinlock ����Զ��
    �������
    �����жϴ�������Ҳ����ʹ�� spinlock�������ʹ�õ� spinlock �Ѿ���һ���̱߳���, �жϴ�����������
    ����������, �Ӷ��γ�����,
    
    �������ԭ�����ǿ����ṩһ�ָ���ȫ�� spinlock ���� ������� spinlock ����. ����˼����, �����
    ��ͬʱ�ṩһ����־�� ID �� Owner �ֶΣ���ĳ�� CPU �˻�ȡ����Owner �ֶξ͵��ڵ�ǰ�˵� ID������
    ����ʱ, �����ж� Owner �ֶ��Ƿ���������ߵĺ� ID, ������, ����������ļ������ֱ�Ӳ���������Դ��
    �����ٽ��� spin lock �Ĳ�����
    
    Ϊ�����ϵͳ��Ч��, ��Ҫ����ֲ���ж� Owner �ֶ�, ���������봦��.
    
  ע��3: 
    SylixOS ϵͳ�ṩ��һ�� QUICK �ӿ�, ��Ҫ˵������ LW_SPIN_UNLOCK_QUICK() ���᳢�Ե���, ��Ϊ�����ж�
    ������ spinlock �Ĺ�����, �����ᷢ������״̬Ǩ�ƵĲ���, ���߽������ĳ������Ͻ��������ĵ��ȳ���.
*********************************************************************************************************/
/*********************************************************************************************************
  �ü�
*********************************************************************************************************/
#if LW_CFG_SMP_EN > 0
/*********************************************************************************************************
  SMP ���� spinlock ����, ���е� spinlock �����ڲ���������Ӧ���ڴ����ϲ���.
*********************************************************************************************************/

VOID    _SmpSpinInit(spinlock_t *psl);

VOID    _SmpSpinLock(spinlock_t *psl);
BOOL    _SmpSpinTryLock(spinlock_t *psl);
INT     _SmpSpinUnlock(spinlock_t *psl);

VOID    _SmpSpinLockIgnIrq(spinlock_t *psl);
BOOL    _SmpSpinTryLockIgnIrq(spinlock_t *psl);
VOID    _SmpSpinUnlockIgnIrq(spinlock_t *psl);

VOID    _SmpSpinLockIrq(spinlock_t *psl, INTREG  *piregInterLevel);
BOOL    _SmpSpinTryLockIrq(spinlock_t *psl, INTREG  *piregInterLevel);
INT     _SmpSpinUnlockIrq(spinlock_t *psl, INTREG  iregInterLevel);
VOID    _SmpSpinUnlockIrqQuick(spinlock_t *psl, INTREG  iregInterLevel);

VOID    _SmpSpinLockTask(spinlock_t *psl);
BOOL    _SmpSpinTryLockTask(spinlock_t *psl);
INT     _SmpSpinUnlockTask(spinlock_t *psl);

#define LW_SPIN_INIT(psl)                   _SmpSpinInit(psl)

#define LW_SPIN_LOCK(psl)                   _SmpSpinLock(psl)
#define LW_SPIN_TRYLOCK(psl)                _SmpSpinTryLock(psl)
#define LW_SPIN_UNLOCK(psl)                 _SmpSpinUnlock(psl)

#define LW_SPIN_LOCK_IGNIRQ(psl)            _SmpSpinLockIgnIrq(psl)
#define LW_SPIN_TRYLOCK_IGNIRQ(psl)         _SmpSpinTryLockIgnIrq(psl)
#define LW_SPIN_UNLOCK_IGNIRQ(psl)          _SmpSpinUnlockIgnIrq(psl)

#define LW_SPIN_LOCK_IRQ(psl, pireg)        _SmpSpinLockIrq(psl, pireg)
#define LW_SPIN_TRYLOCK_IRQ(psl, pireg)     _SmpSpinTryLockIrq(psl, pireg)
#define LW_SPIN_UNLOCK_IRQ(psl, ireg)       _SmpSpinUnlockIrq(psl, ireg)

#define LW_SPIN_LOCK_QUICK(psl, pireg)      _SmpSpinLockIrq(psl, pireg)
#define LW_SPIN_UNLOCK_QUICK(psl, ireg)     _SmpSpinUnlockIrqQuick(psl, ireg)

#define LW_SPIN_LOCK_TASK(psl)              _SmpSpinLockTask(psl)
#define LW_SPIN_TRYLOCK_TASK(psl)           _SmpSpinTryLockTask(psl)
#define LW_SPIN_UNLOCK_TASK(psl)            _SmpSpinUnlockTask(psl)

/*********************************************************************************************************
  SMP �������л���ɺ�ר���ͷź���
*********************************************************************************************************/

struct __lw_tcb;
VOID  _SmpSpinUnlockSched(spinlock_t *psl, struct __lw_tcb *ptcbOwner);
#define LW_SPIN_UNLOCK_SCHED(psl, ptcb)     _SmpSpinUnlockSched(psl, ptcb)

#else
/*********************************************************************************************************
  ��������ϵͳ
*********************************************************************************************************/

VOID    _UpSpinInit(spinlock_t *psl);

VOID    _UpSpinLock(spinlock_t *psl);
BOOL    _UpSpinTryLock(spinlock_t *psl);
INT     _UpSpinUnlock(spinlock_t *psl);

VOID    _UpSpinLockIgnIrq(spinlock_t *psl);
BOOL    _UpSpinTryLockIgnIrq(spinlock_t *psl);
VOID    _UpSpinUnlockIgnIrq(spinlock_t *psl);

VOID    _UpSpinLockIrq(spinlock_t *psl, INTREG  *piregInterLevel);
BOOL    _UpSpinTryLockIrq(spinlock_t *psl, INTREG  *piregInterLevel);
INT     _UpSpinUnlockIrq(spinlock_t *psl, INTREG  iregInterLevel);
VOID    _UpSpinUnlockIrqQuick(spinlock_t *psl, INTREG  iregInterLevel);

#define LW_SPIN_INIT(psl)                   _UpSpinInit(psl)

#define LW_SPIN_LOCK(psl)                   _UpSpinLock(psl)
#define LW_SPIN_TRYLOCK(psl)                _UpSpinTryLock(psl)
#define LW_SPIN_UNLOCK(psl)                 _UpSpinUnlock(psl)

#define LW_SPIN_LOCK_IGNIRQ(psl)            _UpSpinLockIgnIrq(psl)
#define LW_SPIN_TRYLOCK_IGNIRQ(psl)         _UpSpinTryLockIgnIrq(psl)
#define LW_SPIN_UNLOCK_IGNIRQ(psl)          _UpSpinUnlockIgnIrq(psl)

#define LW_SPIN_LOCK_IRQ(psl, pireg)        _UpSpinLockIrq(psl, pireg)
#define LW_SPIN_TRYLOCK_IRQ(psl, pireg)     _UpSpinTryLockIrq(psl, pireg)
#define LW_SPIN_UNLOCK_IRQ(psl, ireg)       _UpSpinUnlockIrq(psl, ireg)

#define LW_SPIN_LOCK_QUICK(psl, pireg)      _UpSpinLockIrq(psl, pireg)
#define LW_SPIN_UNLOCK_QUICK(psl, ireg)     _UpSpinUnlockIrqQuick(psl, ireg)

#define LW_SPIN_LOCK_TASK(psl)              _UpSpinLock(psl)
#define LW_SPIN_TRYLOCK_TASK(psl)           _UpSpinTryLock(psl)
#define LW_SPIN_UNLOCK_TASK(psl)            _UpSpinUnlock(psl)

/*********************************************************************************************************
  ���������������л���ɺ�ר���ͷź���
*********************************************************************************************************/

struct __lw_tcb;
VOID  _UpSpinUnlockSched(spinlock_t *psl, struct __lw_tcb *ptcbOwner);
#define LW_SPIN_UNLOCK_SCHED(psl, ptcb)     _UpSpinUnlockSched(psl, ptcb)

#endif                                                                  /*  LW_CFG_SMP_EN               */
#endif                                                                  /*  __K_SPINLOCK_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/