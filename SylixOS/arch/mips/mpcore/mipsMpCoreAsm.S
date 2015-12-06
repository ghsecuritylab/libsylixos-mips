;/**********************************************************************************************************
;**
;**                                    中国软件开源组织
;**
;**                                   嵌入式实时操作系统
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------文件信息--------------------------------------------------------------------------------
;**
;** 文   件   名: mipsMpCoreAsm.S
;**
;** 创   建   人: Ryan.Xin (信金龙)
;**
;** 文件创建日期: 2015 年 09 月 01 日
;**
;** 描        述: MIPS 体系构架多核接口驱动.
;**
;** 参        考: http://www.ibm.com/developerworks/cn/linux/l-cn-spinlock_mips/index.html
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/mp/mp_cfg.h>

    FILE_BEGIN()

#if LW_CFG_SMP_EN > 0

    IMPORT_LABEL(bspMpInt)

    EXPORT_LABEL(mipsSpinLock)
    EXPORT_LABEL(mipsSpinTryLock)
    EXPORT_LABEL(mipsSpinUnlock)
    EXPORT_LABEL(archMpCur)
    EXPORT_LABEL(archMpInt)

    WEAK(archMpCur)
    WEAK(archMpInt)

;/*********************************************************************************************************
;  自旋锁
;*********************************************************************************************************/

FUNC_DEF(mipsSpinLock)
1:
    LL      T0 , 0(A0)
    BNEZ    T0 , 1b
    LI      T0 , 1
    SC      T0 , 0(A0)
    BEQZ    T0 , 1b
    SYNC
    JR      RA
    NOP
FUNC_END(mipsSpinLock)

FUNC_DEF(mipsSpinTryLock)
    LL      T0 , 0(A0)
    BNEZ    T0 , 1f
    LI      T0 , 1
    SC      T0 , 0(A0)
    BEQZ    T0 , 1f
    SYNC
    JR      RA
    MOV     V0 , ZERO
1:
    JR      RA
    LI      V0 , 1
FUNC_END(mipsSpinTryLock)

FUNC_DEF(mipsSpinUnlock)
    SYNC
    SW      ZERO , 0(A0)
    JR      RA
    NOP
FUNC_END(mipsSpinUnlock)

;/*********************************************************************************************************
;  获得当前核 ID TODO
;*********************************************************************************************************/

FUNC_DEF(archMpCur)
    JR      RA
    MOV     V0 , ZERO
FUNC_END(archMpCur)

;/*********************************************************************************************************
;  产生一次核间中断
;*********************************************************************************************************/

FUNC_DEF(archMpInt)
    SUBU    SP , 4
    SW      RA , 0(SP)
    JAL     bspMpInt
    NOP
    LW      RA , 0(SP)
    ADDU    SP , 4
    JR      RA
    NOP
FUNC_END(archMpInt)

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
