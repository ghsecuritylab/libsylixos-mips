;/*********************************************************************************************************
;**
;**                                    �й�������Դ��֯
;**
;**                                   Ƕ��ʽʵʱ����ϵͳ
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------�ļ���Ϣ--------------------------------------------------------------------------------
;**
;** ��   ��   ��: ppcLibAsm.S
;**
;** ��   ��   ��: Jiao.JinXing (������)
;**
;** �ļ���������: 2015 �� 12 �� 15 ��
;**
;** ��        ��: PowerPC ��ϵ�����ڲ���.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/cpu/cpu_cfg.h>
#include <config/kernel/cache_cfg.h>

    FILE_BEGIN()

    IMPORT_LABEL(__setjmpSetup)
    IMPORT_LABEL(__sigsetjmpSetup)
    IMPORT_LABEL(__longjmpSetup)
    IMPORT_LABEL(__siglongjmpSetup)
    IMPORT_LABEL(bspReboot)
    
    EXPORT_LABEL(setjmp)
    EXPORT_LABEL(sigsetjmp)
    EXPORT_LABEL(longjmp)
    EXPORT_LABEL(siglongjmp)
    
    EXPORT_LABEL(archFindLsb)
    EXPORT_LABEL(archFindMsb)

    EXPORT_LABEL(archIntDisable)
    EXPORT_LABEL(archIntEnable)
    EXPORT_LABEL(archIntEnableForce)
#if LW_CFG_CACHE_EN > 0
    EXPORT_LABEL(archPageCopy)
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
    EXPORT_LABEL(archReboot)

    WEAK(archIntDisable)
    WEAK(archIntEnable)
    WEAK(archIntEnableForce)

;/*********************************************************************************************************
;  PowerPC �ر����ж�
;*********************************************************************************************************/

FUNC_DEF(archIntDisable)
    MFMSR   R3
    ISYNC
    OR      R4 , R3 , R3
    ARCH_PPC_INT_MASK(R4, R4)
    ISYNC
    MTMSR   R4
    ISYNC
    BLR
    FUNC_END()
    
FUNC_DEF(archIntEnable)
    SYNC
    RLWINM  R4 , R3 , 0 , ARCH_PPC_MSR_BIT_EE , ARCH_PPC_MSR_BIT_EE
    CMPWI   CR0, R4 , 0
    BEQ     CR0, __archIntIsDisable
    MFMSR   R3
    ISYNC
    ORI     R3 , R3 , ARCH_PPC_MSR_EE
    ISYNC
    MTMSR   R3
    ISYNC
LINE_LABEL(__archIntIsDisable)
    BLR
    FUNC_END()
    
FUNC_DEF(archIntEnableForce)
    MFMSR   R3
    ISYNC
    ORI     R3 , R3 , ARCH_PPC_MSR_EE
    ISYNC
    MTMSR   R3
    ISYNC
    BLR
    FUNC_END()
    
;/*********************************************************************************************************
;  PowerPC ǰ��������
;*********************************************************************************************************/

FUNC_DEF(archFindLsb)
    NEG     R4 , R3
    AND     R3 , R3 , R4
    CNTLZW  R3 , R3                                                     ;/*  R3 = # leading zeros        */
    SUBFIC  R3 , R3 , 32                                                ;/*  R3 = 32 - R3                */
    BLR
    FUNC_END()

FUNC_DEF(archFindMsb)
    CNTLZW  R3 , R3                                                     ;/*  R3 = # leading zeros        */
    SUBFIC  R3 , R3 , 32                                                ;/*  R3 = 32 - R3                */
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  ҳ�濽�� (�������� linux/arch/powerpc/kernel/misc_32.S)
;*********************************************************************************************************/

#if LW_CFG_CACHE_EN > 0

#define PAGE_SZ             4096
#define L1_CACHE_BYTES      32
#define MAX_COPY_PREFETCH   1
;/* LOOP = (PAGE_SIZE / L1_CACHE_BYTES - MAX_COPY_PREFETCH) */
#define LOOP                127

#define COPY_16_BYTES       \
    LWZ     R6 , 4(R4);     \
    LWZ     R7 , 8(R4);     \
    LWZ     R8 , 12(R4);    \
    LWZU    R9 , 16(R4);    \
    STW     R6 , 4(R3);     \
    STW     R7 , 8(R3);     \
    STW     R8 , 12(R3);    \
    STWU    R9 , 16(R3)

FUNC_DEF(archPageCopy)
    ADDI    R3  , R3 , -4
    ADDI    R4  , R4 , -4

    LI      R5  , 4

#if MAX_COPY_PREFETCH > 1
    LI      R0  , MAX_COPY_PREFETCH
    LI      R11 , 4
    MTCTR   R0
11: DCBT    R11 , R4
    ADDI    R11 , R11 , L1_CACHE_BYTES
    BDNZ    11b
#else                                                                   /*  MAX_COPY_PREFETCH == 1       */
    DCBT    R5  , R4
    LI      R11 , L1_CACHE_BYTES + 4
#endif
    LI      R0  , LOOP
    CRCLR   4 * CR0 + EQ
2:
    MTCTR   R0
1:
    DCBT    R11 , R4
    DCBZ    R5  , R3
    COPY_16_BYTES
#if L1_CACHE_BYTES >= 32
    COPY_16_BYTES
#if L1_CACHE_BYTES >= 64
    COPY_16_BYTES
    COPY_16_BYTES
#if L1_CACHE_BYTES >= 128
    COPY_16_BYTES
    COPY_16_BYTES
    COPY_16_BYTES
    COPY_16_BYTES
#endif
#endif
#endif
    BDNZ    1b
    BEQLR
    CRNOT   4 * CR0 + EQ , 4 * CR0 + EQ
    LI      R0  , MAX_COPY_PREFETCH
    LI      R11 , 4
    B       2b
    FUNC_END()

#endif                                                                  /*  LW_CFG_CACHE_EN > 0          */

;/*********************************************************************************************************
;  ע��: setjmp �� longjmp �����Ľṹ���߳������Ľṹ��ͬ
;
;  �����ĵ� <PowerPC-eabi_app.pdf>, һ�ε��ù���, Nonvolatile(����ʧ��)�ļĴ���������:
;  CR��CR2-CR4��  R14-R31  R1(SP)  F14-F31
;
;  setjmp �� longjmp �����Ľṹ�������:
;
;  �ߵ�ַ
;  +---------------+
;  |    CR         | (1 regs)
;  +---------------+
;  |    R14 - R31  | (18 regs)
;  +---------------+
;  |    SP(R1)     | (1 regs)
;  +---------------+
;  |    LR         | (1 regs)
;  +---------------+
;  �͵�ַ
;
;  F14-F31 ������, �� 21 ���Ĵ���
;
;*********************************************************************************************************/

;/*********************************************************************************************************
;  �������ú�����
;*********************************************************************************************************/

MACRO_DEF(CALL_SETUP, setup)
    SUBI    SP , SP , 16

    STW     R3 , 0(SP)

    STW     R4 , 4(SP)

    MFLR    R5
    ISYNC
    STW     R5 , 8(SP)
    SYNC

    BL      \setup

    LWZ     R5 , 8(SP)
    SYNC
    MTLR    R5
    ISYNC

    LWZ     R4 , 4(SP)

    LWZ     R3 , 0(SP)

    ADDI    SP , SP , 16
    MACRO_END()

;/*********************************************************************************************************
;  ����Ĵ�����
;*********************************************************************************************************/

MACRO_DEF(SAVE_NON_VOLATILE_REG)

    MFLR    R5
    ISYNC
    STW     R5 , 0(R3)
    SYNC

    STW     SP , 4(R3)

    STMW    R14, 8(R3)

    MFCR    R5
    ISYNC
    STW     R5 , 80(R3)
    SYNC
    MACRO_END()

;/*********************************************************************************************************
;  �ָ��Ĵ�����
;*********************************************************************************************************/

MACRO_DEF(RESTORE_NON_VOLATILE_REG)

    LWZ     R5 , 0(R3)
    SYNC
    MTLR    R5
    ISYNC

    LWZ     SP , 4(R3)

    LMW     R14, 8(R3)

    LWZ     R5 , 80(R3)
    SYNC
    MTCR    R5
    ISYNC
    MACRO_END()

;/*********************************************************************************************************
;  sigsetjmp (����Ϊ jmp_buf, mask_saved)
;*********************************************************************************************************/

FUNC_DEF(sigsetjmp)
    CALL_SETUP  __sigsetjmpSetup

    SAVE_NON_VOLATILE_REG

    XOR     R3 , R3 , R3
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  setjmp (����Ϊ jmp_buf)
;*********************************************************************************************************/

FUNC_DEF(setjmp)
    CALL_SETUP  __setjmpSetup

    SAVE_NON_VOLATILE_REG

    XOR     R3 , R3 , R3
    BLR
    FUNC_END()
    
;/*********************************************************************************************************
;  siglongjmp (����Ϊ jmp_buf, retval)
;*********************************************************************************************************/

FUNC_DEF(siglongjmp)
    CALL_SETUP  __siglongjmpSetup

    RESTORE_NON_VOLATILE_REG

    OR      R3 , R4 , R4
    CMPWI   CR0, R4 , 0
    BNE     CR0, siglongjmpnotzero
    LI      R3 , 1
LINE_LABEL(siglongjmpnotzero)
    BLR
    FUNC_END()

;/*********************************************************************************************************
;  longjmp (����Ϊ jmp_buf, retval)
;*********************************************************************************************************/

FUNC_DEF(longjmp)
    CALL_SETUP  __longjmpSetup

    RESTORE_NON_VOLATILE_REG

    OR      R3 , R4 , R4
    CMPWI   CR0, R4 , 0
    BNE     CR0, longjmpnotzero
    LI      R3 , 1
LINE_LABEL(longjmpnotzero)
    BLR
    FUNC_END()
    
;/*********************************************************************************************************
;  ϵͳ����
;*********************************************************************************************************/

FUNC_DEF(archReboot)
    B       bspReboot
    FUNC_END()
    
    FILE_END()
   
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/