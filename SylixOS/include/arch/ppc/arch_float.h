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
** 文   件   名: arch_float.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 11 月 26 日
**
** 描        述: PowerPC 浮点相关.
*********************************************************************************************************/

#ifndef __PPC_ARCH_FLOAT_H
#define __PPC_ARCH_FLOAT_H

/*********************************************************************************************************
  浮点数据寄存器的总数
*********************************************************************************************************/

#define FP_DREG_NR          32

/*********************************************************************************************************
  定义 ARCH_FPU_CTX 成员偏移
*********************************************************************************************************/

#define XFPR(n)             ((n) * 8)
#define XFPSCR              (FP_DREG_NR * 8)
#define XFPSCR_COPY         (XFPSCR + 4)

/*********************************************************************************************************
  汇编代码不包含以下内容
*********************************************************************************************************/

#if (!defined(__ASSEMBLY__)) && (!defined(ASSEMBLY))

/*********************************************************************************************************
  1: native-endian double  
     与处理器其他类型存储完全相同, 
                    
  2: mixed-endian double  
     混合大小端, (32位字内存储与处理器相同, 两个32位之间按照大端存储)
     
  PowerPC eabi 使用 native-endian
*********************************************************************************************************/

/*********************************************************************************************************
  线程浮点运算器上下文
*********************************************************************************************************/

typedef struct arch_fpu_ctx {                                           /* VFP 上下文           		*/
    double              FPUCTX_dfDreg[FP_DREG_NR];                      /* 32 个 double 寄存器          */
    UINT32              FPUCTX_uiFpscr;                                 /* 状态和控制寄存器             */
    UINT32              FPUCTX_uiFpscrCopy;                             /* 状态和控制寄存器的拷贝       */
} ARCH_FPU_CTX;

/*********************************************************************************************************
  float 格式 (使用 union 类型作为中间转换, 避免 GCC 3.x.x strict aliasing warning)
*********************************************************************************************************/

#define __ARCH_FLOAT_EXP_NAN           255                              /*  NaN 或者无穷大的 Exp 值     */

typedef struct __cpu_float_field {
    unsigned int        frac : 23;
    unsigned int        exp  :  8;
    unsigned int        sig  :  1;
} __CPU_FLOAT_FIELD;

typedef union __cpu_float {
    __CPU_FLOAT_FIELD   fltfield;                                       /*  float 位域字段              */
    float               flt;                                            /*  float 占位                  */
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
  double 格式
*********************************************************************************************************/

#define __ARCH_DOUBLE_EXP_NAN           2047                            /*  NaN 或者无穷大的 Exp 值     */
#define __ARCH_DOUBLE_INC_FLOAT_H          0                            /*  是否引用编译器 float.h 文件 */

/*********************************************************************************************************
  ppc-sylixos-eabi-gcc ... GNU
*********************************************************************************************************/

#if LW_CFG_DOUBLE_MIX_ENDIAN > 0
typedef struct __cpu_double_field {                                     /*  old mixed-endian            */
    unsigned int        frach : 20;
    unsigned int        exp   : 11;
    unsigned int        sig   :  1;
    
    unsigned int        fracl : 32;                                     /*  低 32 位放入高地址          */
} __CPU_DOUBLE_FIELD;
#else
typedef struct __cpu_double_field {                                     /*  native-endian               */
    unsigned int        fracl : 32;                                     /*  低 32 位放入低地址          */
    
    unsigned int        frach : 20;
    unsigned int        exp   : 11;
    unsigned int        sig   :  1;
} __CPU_DOUBLE_FIELD;
#endif                                                                  /*  __ARCH_DOUBLE_MIX_ENDIAN    */

typedef union __cpu_double {
    __CPU_DOUBLE_FIELD  dblfield;                                       /*  float 位域字段              */
    double              dbl;                                            /*  float 占位                  */
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

#endif

#endif                                                                  /*  __PPC_ARCH_FLOAT_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
