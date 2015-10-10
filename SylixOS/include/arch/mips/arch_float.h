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
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 09 月 01 日
**
** 描        述: MIPS 浮点相关.
*********************************************************************************************************/

#ifndef __MIPS_ARCH_FLOAT_H
#define __MIPS_ARCH_FLOAT_H

/*********************************************************************************************************
  MIPS floating point coprocessor register definitions
*********************************************************************************************************/

#define FP0     $f0                                                     /* return reg 0                 */
#define FP1     $f1                                                     /* return reg 1                 */
#define FP2     $f2                                                     /* return reg 2                 */
#define FP3     $f3                                                     /* return reg 3                 */
#define FP4     $f4                                                     /* caller saved 0               */
#define FP5     $f5                                                     /* caller saved 1               */
#define FP6     $f6                                                     /* caller saved 2               */
#define FP7     $f7                                                     /* caller saved 3               */
#define FP8     $f8                                                     /* caller saved 4               */
#define FP9     $f9                                                     /* caller saved 5               */
#define FP10    $f10                                                    /* caller saved 6               */
#define FP11    $f11                                                    /* caller saved 7               */
#define FP12    $f12                                                    /* arg reg 0                    */
#define FP13    $f13                                                    /* arg reg 1                    */
#define FP14    $f14                                                    /* arg reg 2                    */
#define FP15    $f15                                                    /* arg reg 3                    */
#define FP16    $f16                                                    /* caller saved 8               */
#define FP17    $f17                                                    /* caller saved 9               */
#define FP18    $f18                                                    /* caller saved 10              */
#define FP19    $f19                                                    /* caller saved 11              */
#define FP20    $f20                                                    /* callee saved 0               */
#define FP21    $f21                                                    /* callee saved 1               */
#define FP22    $f22                                                    /* callee saved 2               */
#define FP23    $f23                                                    /* callee saved 3               */
#define FP24    $f24                                                    /* callee saved 4               */
#define FP25    $f25                                                    /* callee saved 5               */
#define FP26    $f26                                                    /* callee saved 6               */
#define FP27    $f27                                                    /* callee saved 7               */
#define FP28    $f28                                                    /* callee saved 8               */
#define FP29    $f29                                                    /* callee saved 9               */
#define FP30    $f30                                                    /* callee saved 10              */
#define FP31    $f31                                                    /* callee saved 11              */

#define CP1_IR       $0                                                 /* implementation/revision reg  */
#define CP1_STATUS   $31                                                /* control/status reg           */

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
  arm-none-eabi-gcc ... GNU
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

#endif                                                                  /*  __MIPS_ARCH_FLOAT_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
