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
** ��   ��   ��: arch_float.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 05 �� 04 ��
**
** ��        ��: ARM �������.
*********************************************************************************************************/

#ifndef __ARM_ARCH_FLOAT_H
#define __ARM_ARCH_FLOAT_H

/*********************************************************************************************************
  1: native-endian double  
     �봦�����������ʹ洢��ȫ��ͬ, 
                    
  2: mixed-endian double  
     ��ϴ�С��, (32λ���ڴ洢�봦������ͬ, ����32λ֮�䰴�մ�˴洢)
     
  arm eabi ʹ�� native-endian
*********************************************************************************************************/

/*********************************************************************************************************
  �̸߳���������������
  
  ע��: VFPv2 ֧�� 16 �� double �Ĵ���, VFPv3 ֧�� 32 �� double �ͼĴ�����
  
  VFPv2: (dreg is s)
    +-----------+
 	| freg[31]  |    + 0x98 <-- (r0 + 152)
 	|  ...      |
 	| freg[2]   |    + 0x24
 	| freg[1]   |    + 0x20
 	| freg[0]   |    + 0x1C  <-- (r0 + 28)
 	| mfvfr1    |    + 0x18
 	| mfvfr0    |    + 0x14
 	| fpinst2   |    + 0x10
 	| fpinst    |    + 0x0C
 	| fpexc     |    + 0x08
 	| fpscr     |    + 0x04
 	| fpsid     | <-- cpu_fpu_context ( = r0 )
 	+-----------+
 	
  VFPv3: (dreg is s)
    +-----------+
 	| freg[63]  |    + 0x118 <-- (r0 + 280)
 	|  ...      |
 	| freg[2]   |    + 0x24
 	| freg[1]   |    + 0x20
 	| freg[0]   |    + 0x1C  <-- (r0 + 28)
 	| mfvfr1    |    + 0x18
 	| mfvfr0    |    + 0x14
 	| fpinst2   |    + 0x10
 	| fpinst    |    + 0x0C
 	| fpexc     |    + 0x08
 	| fpscr     |    + 0x04
 	| fpsid     | <-- cpu_fpu_context ( = r0 )
 	+-----------+
*********************************************************************************************************/

typedef struct arch_cpu_fpu_context {                                   /* VFPv2/VFPv3 ������           */
    UINT32              FPUCTX_uiFpsid;                                 /* system ID register           */
    UINT32              FPUCTX_uiFpscr;                                 /* status and control register  */
    UINT32              FPUCTX_uiFpexc;                                 /* exception register           */
    UINT32              FPUCTX_uiFpinst;                                /* instruction register         */
    UINT32              FPUCTX_uiFpinst2;                               /* instruction register         */
    UINT32              FPUCTX_uiMfvfr0;                                /* media and VFP feature Reg    */
    UINT32              FPUCTX_uiMfvfr1;                                /* media and VFP feature Reg    */
    UINT32              FPUCTX_uiDreg[32 * 2];                          /* general purpose Reg  D0 ~ D32*/
                                                                        /* equ -> S0 ~ S64              */
} ARCH_CPU_FPU_CONTEXT;

/*********************************************************************************************************
  float ��ʽ (ʹ�� union ������Ϊ�м�ת��, ���� GCC 3.x.x strict aliasing warning)
*********************************************************************************************************/

#define __ARCH_FLOAT_EXP_NAN           255                              /*  NaN ���������� Exp ֵ     */

typedef struct __cpu_float_field {
    unsigned int        frac : 23;
    unsigned int        exp  :  8;
    unsigned int        sig  :  1;
} __CPU_FLOAT_FIELD;

typedef union __cpu_float {
    __CPU_FLOAT_FIELD   fltfield;                                       /*  float λ���ֶ�              */
    float               flt;                                            /*  float ռλ                  */
} __CPU_FLOAT;

static LW_INLINE INT  __ARCH_FLOAT_ISNAN (float  x)
{
    __CPU_FLOAT     cpuflt;
    
    cpuflt.flt = x;

    return  ((cpuflt.fltfield.exp == __ARCH_FLOAT_EXP_NAN) && (cpuflt.fltfield.frac != 0));
}

static LW_INLINE INT  __ARCH_FLOAT_ISINF (float  x)
{
    __CPU_FLOAT     cpuflt;
    
    cpuflt.flt = x;
    
    return  ((cpuflt.fltfield.exp == __ARCH_FLOAT_EXP_NAN) && (cpuflt.fltfield.frac == 0));
}

/*********************************************************************************************************
  double ��ʽ
*********************************************************************************************************/

#define __ARCH_DOUBLE_EXP_NAN           2047                            /*  NaN ���������� Exp ֵ     */
#define __ARCH_DOUBLE_INC_FLOAT_H          0                            /*  �Ƿ����ñ����� float.h �ļ� */

/*********************************************************************************************************
  arm-none-eabi-gcc ... GNU
*********************************************************************************************************/

#if LW_CFG_DOUBLE_MIX_ENDIAN > 0
typedef struct __cpu_double_field {                                     /*  old mixed-endian            */
    unsigned int        frach : 20;
    unsigned int        exp   : 11;
    unsigned int        sig   :  1;
    
    unsigned int        fracl : 32;                                     /*  �� 32 λ����ߵ�ַ          */
} __CPU_DOUBLE_FIELD;
#else
typedef struct __cpu_double_field {                                     /*  native-endian               */
    unsigned int        fracl : 32;                                     /*  �� 32 λ����͵�ַ          */
    
    unsigned int        frach : 20;
    unsigned int        exp   : 11;
    unsigned int        sig   :  1;
} __CPU_DOUBLE_FIELD;
#endif                                                                  /*  __ARCH_DOUBLE_MIX_ENDIAN    */

typedef union __cpu_double {
    __CPU_DOUBLE_FIELD  dblfield;                                       /*  float λ���ֶ�              */
    double              dbl;                                            /*  float ռλ                  */
} __CPU_DOUBLE;

static LW_INLINE INT  __ARCH_DOUBLE_ISNAN (double  x)
{
    __CPU_DOUBLE     dblflt;
    
    dblflt.dbl = x;
    
    return  ((dblflt.dblfield.exp == __ARCH_DOUBLE_EXP_NAN) && 
             ((dblflt.dblfield.fracl != 0) && 
              (dblflt.dblfield.frach != 0)));
}

static LW_INLINE INT  __ARCH_DOUBLE_ISINF (double  x)
{
    __CPU_DOUBLE     dblflt;
    
    dblflt.dbl = x;
    
    return  ((dblflt.dblfield.exp == __ARCH_DOUBLE_EXP_NAN) && 
             ((dblflt.dblfield.fracl == 0) || 
              (dblflt.dblfield.frach == 0)));
}

#endif                                                                  /*  __ARM_ARCH_FLOAT_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/