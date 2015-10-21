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
** 文   件   名: arch_def.h
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 10 月 13 日
**
** 描        述: MIPS 相关定义.
*********************************************************************************************************/
#ifndef ARCH_DEF_H_
#define ARCH_DEF_H_

/*********************************************************************************************************
  MIPS 通用寄存器定义
*********************************************************************************************************/

#if defined(__ASSEMBLY__) || defined(ASSEMBLY)
#define ZERO            $0                                              /* wired zero                   */
#define AT              $at                                             /* assembler temp               */
#define V0              $2                                              /* return reg 0                 */
#define V1              $3                                              /* return reg 1                 */
#define A0              $4                                              /* arg reg 0                    */
#define A1              $5                                              /* arg reg 1                    */
#define A2              $6                                              /* arg reg 2                    */
#define A3              $7                                              /* arg reg 3                    */
#define T0              $8                                              /* caller saved 0               */
#define T1              $9                                              /* caller saved 1               */
#define T2              $10                                             /* caller saved 2               */
#define T3              $11                                             /* caller saved 3               */
#define T4              $12                                             /* caller saved 4               */
#define T5              $13                                             /* caller saved 5               */
#define T6              $14                                             /* caller saved 6               */
#define T7              $15                                             /* caller saved 7               */
#define S0              $16                                             /* callee saved 0               */
#define S1              $17                                             /* callee saved 1               */
#define S2              $18                                             /* callee saved 2               */
#define S3              $19                                             /* callee saved 3               */
#define S4              $20                                             /* callee saved 4               */
#define S5              $21                                             /* callee saved 5               */
#define S6              $22                                             /* callee saved 6               */
#define S7              $23                                             /* callee saved 7               */
#define T8              $24                                             /* caller saved 8               */
#define T9              $25                                             /* caller saved 9               */
#define K0              $26                                             /* kernel temp 0                */
#define K1              $27                                             /* kernel temp 1                */
#define GP              $28                                             /* global pointer               */
#define SP              $29                                             /* stack pointer                */
#define S8              $30                                             /* callee saved 8               */
#define FP              S8                                              /* callee saved 8               */
#define RA              $31                                             /* return address               */
#else
#define MIPS_ZERO       "$0"                                            /* wired zero                   */
#define MIPS_AT         "$at"                                           /* assembler temp               */
#define MIPS_V0         "$2"                                            /* return reg 0                 */
#define MIPS_V1         "$3"                                            /* return reg 1                 */
#define MIPS_A0         "$4"                                            /* arg reg 0                    */
#define MIPS_A1         "$5"                                            /* arg reg 1                    */
#define MIPS_A2         "$6"                                            /* arg reg 2                    */
#define MIPS_A3         "$7"                                            /* arg reg 3                    */
#define MIPS_T0         "$8"                                            /* caller saved 0               */
#define MIPS_T1         "$9"                                            /* caller saved 1               */
#define MIPS_T2         "$10"                                           /* caller saved 2               */
#define MIPS_T3         "$11"                                           /* caller saved 3               */
#define MIPS_T4         "$12"                                           /* caller saved 4               */
#define MIPS_T5         "$13"                                           /* caller saved 5               */
#define MIPS_T6         "$14"                                           /* caller saved 6               */
#define MIPS_T7         "$15"                                           /* caller saved 7               */
#define MIPS_S0         "$16"                                           /* callee saved 0               */
#define MIPS_S1         "$17"                                           /* callee saved 1               */
#define MIPS_S2         "$18"                                           /* callee saved 2               */
#define MIPS_S3         "$19"                                           /* callee saved 3               */
#define MIPS_S4         "$20"                                           /* callee saved 4               */
#define MIPS_S5         "$21"                                           /* callee saved 5               */
#define MIPS_S6         "$22"                                           /* callee saved 6               */
#define MIPS_S7         "$23"                                           /* callee saved 7               */
#define MIPS_T8         "$24"                                           /* caller saved 8               */
#define MIPS_T9         "$25"                                           /* caller saved 9               */
#define MIPS_K0         "$26"                                           /* kernel temp 0                */
#define MIPS_K1         "$27"                                           /* kernel temp 1                */
#define MIPS_GP         "$28"                                           /* global pointer               */
#define MIPS_SP         "$29"                                           /* stack pointer                */
#define MIPS_S8         "$30"                                           /* callee saved 8               */
#define MIPS_FP         MIPS_S8                                         /* callee saved 8               */
#define MIPS_RA         "$31"                                           /* return address               */
#endif

/*********************************************************************************************************
  MIPS 协处理器 0 寄存器定义
*********************************************************************************************************/

#if defined(__ASSEMBLY__) || defined(ASSEMBLY)
#define CP0_INX         $0                                              /*  tlb index                   */
#define CP0_RAND        $1                                              /*  tlb random                  */
#define CP0_TLBLO       $2                                              /*  tlb entry low               */
#define CP0_TLBLO0      $2                                              /*  tlb entry low 0             */
#define CP0_TLBLO1      $3                                              /*  tlb entry low 1             */
#define CP0_CTXT        $4                                              /*  tlb context                 */
#define CP0_PAGEMASK    $5                                              /*  page mask                   */
#define CP0_WIRED       $6                                              /*  lb wired entries            */
#define CP0_HWRENA      $7
#define CP0_BADVADDR    $8                                              /*  bad virtual address         */
#define CP0_COUNT       $9                                              /*  count                       */
#define CP0_TLBHI       $10                                             /*  tlb entry hi                */
#define CP0_COMPARE     $11                                             /*  compare                     */
#define CP0_STATUS      $12                                             /*  status register             */
#define CP0_INTCTL      $12,1                                           /*  interrupts vector set       */
#define CP0_CAUSE       $13                                             /*  exception cause             */
#define CP0_EPC         $14                                             /*  exception pc                */
#define CP0_PRID        $15
#define CP0_EBASE       $15,1
#define CP0_CONFIG      $16
#define CP0_CONFIG1     $16,1
#define CP0_CONFIG2     $16,2
#define CP0_CONFIG3     $16,3
#define CP0_LLADDR      $17
#define CP0_WATCHLO     $18
#define CP0_WATCHHI     $19
#define CP0_ECC         $26
#define CP0_CACHEERR    $27
#define CP0_TAGLO       $28
#define CP0_TAGHI       $29
#define CP0_ERRPC       $30
#else
#define CP0_INX         "$0"                                            /*  tlb index                   */
#define CP0_RAND        "$1"                                            /*  tlb random                  */
#define CP0_TLBLO       "$2"                                            /*  tlb entry low               */
#define CP0_TLBLO0      "$2"                                            /*  tlb entry low 0             */
#define CP0_TLBLO1      "$3"                                            /*  tlb entry low 1             */
#define CP0_CTXT        "$4"                                            /*  tlb context                 */
#define CP0_PAGEMASK    "$5"                                            /*  page mask                   */
#define CP0_WIRED       "$6"                                            /*  lb wired entries            */
#define CP0_HWRENA      "$7"
#define CP0_BADVADDR    "$8"                                            /*  bad virtual address         */
#define CP0_COUNT       "$9"                                            /*  count                       */
#define CP0_TLBHI       "$10"                                           /*  tlb entry hi                */
#define CP0_COMPARE     "$11"                                           /*  compare                     */
#define CP0_STATUS      "$12"                                           /*  status register             */
#define CP0_INTCTL      "$12,1"                                         /*  interrupts vector set       */
#define CP0_CAUSE       "$13"                                           /*  exception cause             */
#define CP0_EPC         "$14"                                           /*  exception pc                */
#define CP0_PRID        "$15"
#define CP0_EBASE       "$15,1"
#define CP0_CONFIG      "$16"
#define CP0_CONFIG1     "$16,1"
#define CP0_CONFIG2     "$16,2"
#define CP0_CONFIG3     "$16,3"
#define CP0_LLADDR      "$17"
#define CP0_WATCHLO     "$18"
#define CP0_WATCHHI     "$19"
#define CP0_ECC         "$26"
#define CP0_CACHEERR    "$27"
#define CP0_TAGLO       "$28"
#define CP0_TAGHI       "$29"
#define CP0_ERRPC       "$30"
#endif

#define MIPS_EXEC_INS(ins)  __asm__ __volatile__ (ins)

/*********************************************************************************************************
  Define the address offset of the general registers
*********************************************************************************************************/

#define STK_OFFSET_RA   (0)
#define STK_OFFSET_FP   (STK_OFFSET_RA  + 4)
#define STK_OFFSET_GP   (STK_OFFSET_FP  + 4)
#define STK_OFFSET_T9   (STK_OFFSET_GP  + 4)
#define STK_OFFSET_T8   (STK_OFFSET_T9  + 4)
#define STK_OFFSET_S7   (STK_OFFSET_T8  + 4)
#define STK_OFFSET_S6   (STK_OFFSET_S7  + 4)
#define STK_OFFSET_S5   (STK_OFFSET_S6  + 4)
#define STK_OFFSET_S4   (STK_OFFSET_S5  + 4)
#define STK_OFFSET_S3   (STK_OFFSET_S4  + 4)
#define STK_OFFSET_S2   (STK_OFFSET_S3  + 4)
#define STK_OFFSET_S1   (STK_OFFSET_S2  + 4)
#define STK_OFFSET_S0   (STK_OFFSET_S1  + 4)
#define STK_OFFSET_T7   (STK_OFFSET_S0  + 4)
#define STK_OFFSET_T6   (STK_OFFSET_T7  + 4)
#define STK_OFFSET_T5   (STK_OFFSET_T6  + 4)
#define STK_OFFSET_T4   (STK_OFFSET_T5  + 4)
#define STK_OFFSET_T3   (STK_OFFSET_T4  + 4)
#define STK_OFFSET_T2   (STK_OFFSET_T3  + 4)
#define STK_OFFSET_T1   (STK_OFFSET_T2  + 4)
#define STK_OFFSET_T0   (STK_OFFSET_T1  + 4)
#define STK_OFFSET_A3   (STK_OFFSET_T0  + 4)
#define STK_OFFSET_A2   (STK_OFFSET_A3  + 4)
#define STK_OFFSET_A1   (STK_OFFSET_A2  + 4)
#define STK_OFFSET_A0   (STK_OFFSET_A1  + 4)
#define STK_OFFSET_V1   (STK_OFFSET_A0  + 4)
#define STK_OFFSET_V0   (STK_OFFSET_V1  + 4)
#define STK_OFFSET_AT   (STK_OFFSET_V0  + 4)
#define STK_OFFSET_SR   (STK_OFFSET_AT  + 4)
#define STK_OFFSET_EPC  (STK_OFFSET_SR  + 4)
#define STK_CTX_SIZE    (STK_OFFSET_EPC + 4)

/*********************************************************************************************************
  CP0 Status Register
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |C|C|C|C|R|F|R|M|P|B|T|S|M| | R |I|I|I|I|I|I|I|I|K|S|U|U|R|E|E|I|
 * |U|U|U|U|P|R|E|X|X|E|S|R|M| | s |M|M|M|M|M|M|M|M|X|X|X|M|s|R|X|E| Status
 * |3|2|1|0| | | | | |V| | |I| | v |7|6|5|4|3|2|1|0| | | | |v|L|L| |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_StatusCU      28                                              /* Coprocessor enable (R/W)     */
#define M_StatusCU      (0xf << S_StatusCU)
#define S_StatusCU3     31
#define M_StatusCU3     (0x1 << S_StatusCU3)
#define S_StatusCU2     30
#define M_StatusCU2     (0x1 << S_StatusCU2)
#define S_StatusCU1     29
#define M_StatusCU1     (0x1 << S_StatusCU1)
#define S_StatusCU0     28
#define M_StatusCU0     (0x1 << S_StatusCU0)
#define S_StatusRP      27                                              /* Enable reduced power
                                                                         * mode (R/W)*/
#define M_StatusRP      (0x1 << S_StatusRP)
#define S_StatusFR      26                                              /* Enable 64-bit FPRs
                                                                         * (MIPS64 only) (R/W) */
#define M_StatusFR      (0x1 << S_StatusFR)
#define S_StatusRE      25                                              /* Enable reverse endian (R/W)  */
#define M_StatusRE      (0x1 << S_StatusRE)
#define S_StatusMX      24                                              /* Enable access to MDMX
                                                                         * resources (MIPS64 only) (R/W)*/
#define M_StatusMX      (0x1 << S_StatusMX)
#define S_StatusPX      23                                              /* Enable access to 64-bit
                                                                         * instructions/data
                                                                         * (MIPS64 only) (R/W)          */
#define M_StatusPX      (0x1 << S_StatusPX)
#define S_StatusBEV     22                                              /* Enable Boot Exception
                                                                         * Vectors (R/W)                */
#define M_StatusBEV     (0x1 << S_StatusBEV)
#define S_StatusTS      21                                              /* Denote TLB shutdown (R/W)    */
#define M_StatusTS      (0x1 << S_StatusTS)
#define S_StatusSR      20                                              /* Denote soft reset (R/W)      */
#define M_StatusSR      (0x1 << S_StatusSR)
#define S_StatusNMI     19
#define M_StatusNMI     (0x1 << S_StatusNMI)                            /* Denote NMI (R/W)             */
#define S_StatusIM      8                                               /* Interrupt mask (R/W)         */
#define M_StatusIM      (0xff << S_StatusIM)
#define S_StatusIM7     15
#define M_StatusIM7     (0x1 << S_StatusIM7)
#define S_StatusIM6     14
#define M_StatusIM6     (0x1 << S_StatusIM6)
#define S_StatusIM5     13
#define M_StatusIM5     (0x1 << S_StatusIM5)
#define S_StatusIM4     12
#define M_StatusIM4     (0x1 << S_StatusIM4)
#define S_StatusIM3     11
#define M_StatusIM3     (0x1 << S_StatusIM3)
#define S_StatusIM2     10
#define M_StatusIM2     (0x1 << S_StatusIM2)
#define S_StatusIM1     9
#define M_StatusIM1     (0x1 << S_StatusIM1)
#define S_StatusIM0     8
#define M_StatusIM0     (0x1 << S_StatusIM0)
#define S_StatusKX      7                                               /* Enable access to extended    */
                                                                        /* kernel addresses             */
                                                                        /* (MIPS64 only) (R/W)          */
#define M_StatusKX      (0x1 << S_StatusKX)
#define S_StatusSX      6                                               /* Enable access to extended    */
                                                                        /* supervisor addresses         */
                                                                        /* (MIPS64 only) (R/W)          */
#define M_StatusSX      (0x1 << S_StatusSX)
#define S_StatusUX      5                                               /* Enable access to extended    */
                                                                        /* user addresses (MIPS64 only) */
#define M_StatusUX      (0x1 << S_StatusUX)
#define S_StatusKSU     3                                               /* Two-bit current mode (R/W)   */
#define M_StatusKSU     (0x3 << S_StatusKSU)
#define S_StatusUM      4                                               /* User mode if supervisor mode */
                                                                        /* not implemented (R/W)        */
#define M_StatusUM      (0x1 << S_StatusUM)
#define S_StatusSM      3                                               /* Supervisor mode (R/W)        */
#define M_StatusSM      (0x1 << S_StatusSM)
#define S_StatusERL     2                                               /* Denotes error level (R/W)    */
#define M_StatusERL     (0x1 << S_StatusERL)
#define S_StatusEXL     1                                               /* Denotes exception level (R/W)*/
#define M_StatusEXL     (0x1 << S_StatusEXL)
#define S_StatusIE      0                                               /* Enables interrupts (R/W)     */
#define M_StatusIE      (0x1 << S_StatusIE)

/*********************************************************************************************************
 *  CP0 Cause Register
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |B| | C |       |I|W|           |I|I|I|I|I|I|I|I| |         | R |
 * |D| | E | Rsvd  |V|P|    Rsvd   |P|P|P|P|P|P|P|P| | ExcCode | s | Cause
 * | | |   |       | | |           |7|6|5|4|3|2|1|0| |         | v |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_CauseBD       31
#define M_CauseBD       (0x1 << S_CauseBD)
#define S_CauseCE       28
#define M_CauseCE       (0x3<< S_CauseCE)
#define S_CauseIV       23
#define M_CauseIV       (0x1 << S_CauseIV)
#define S_CauseWP       22
#define M_CauseWP       (0x1 << S_CauseWP)
#define S_CauseIP       8
#define M_CauseIP       (0xff << S_CauseIP)
#define S_CauseIPEXT    10
#define M_CauseIPEXT    (0x3f << S_CauseIPEXT)
#define S_CauseIP7      15
#define M_CauseIP7      (0x1 << S_CauseIP7)
#define S_CauseIP6      14
#define M_CauseIP6      (0x1 << S_CauseIP6)
#define S_CauseIP5      13
#define M_CauseIP5      (0x1 << S_CauseIP5)
#define S_CauseIP4      12
#define M_CauseIP4      (0x1 << S_CauseIP4)
#define S_CauseIP3      11
#define M_CauseIP3      (0x1 << S_CauseIP3)
#define S_CauseIP2      10
#define M_CauseIP2      (0x1 << S_CauseIP2)
#define S_CauseIP1      9
#define M_CauseIP1      (0x1 << S_CauseIP1)
#define S_CauseIP0      8
#define M_CauseIP0      (0x1 << S_CauseIP0)
#define S_CauseExcCode  2
#define M_CauseExcCode  (0x1f << S_CauseExcCode)

/*********************************************************************************************************
  Values in the ExcCode field
*********************************************************************************************************/
#define EX_INT          0                                               /* Interrupt                    */
#define EXC_INT         (EX_INT << S_CauseExcCode)
#define EX_MOD          1                                               /* TLB modified                 */
#define EXC_MOD         (EX_MOD << S_CauseExcCode)
#define EX_TLBL         2                                               /* TLB exception(load or ifetch)*/
#define EXC_TLBL        (EX_TLBL << S_CauseExcCode)
#define EX_TLBS         3                                               /* TLB exception (store)        */
#define EXC_TLBS        (EX_TLBS << S_CauseExcCode)
#define EX_ADEL         4                                               /* Address error(load or ifetch)*/
#define EXC_ADEL        (EX_ADEL << S_CauseExcCode)
#define EX_ADES         5                                               /* Address error (store)        */
#define EXC_ADES        (EX_ADES << S_CauseExcCode)
#define EX_IBE          6                                               /* Instruction Bus Error        */
#define EXC_IBE         (EX_IBE << S_CauseExcCode)
#define EX_DBE          7                                               /* Data Bus Error               */
#define EXC_DBE         (EX_DBE << S_CauseExcCode)
#define EX_SYS          8                                               /* Syscall                      */
#define EXC_SYS         (EX_SYS << S_CauseExcCode)
#define EX_SYSCALL      EX_SYS
#define EXC_SYSCALL     EXC_SYS
#define EX_BP           9                                               /* Breakpoint                   */
#define EXC_BP          (EX_BP << S_CauseExcCode)
#define EX_BREAK        EX_BP
#define EXC_BREAK       EXC_BP
#define EX_RI           10                                              /* Reserved instruction         */
#define EXC_RI          (EX_RI << S_CauseExcCode)
#define EX_CPU          11                                              /* CoProcessor Unusable         */
#define EXC_CPU         (EX_CPU << S_CauseExcCode)
#define EX_OV           12                                              /* OVerflow                     */
#define EXC_OV          (EX_OV << S_CauseExcCode)
#define EX_TR           13                                              /* Trap instruction             */
#define EXC_TR          (EX_TR << S_CauseExcCode)
#define EX_TRAP         EX_TR
#define EXC_TRAP        EXC_TR
#define EX_FPE          15                                              /* floating point exception     */
#define EXC_FPE         (EX_FPE << S_CauseExcCode)
#define EX_C2E          18                                              /* COP2 exception               */
#define EXC_C2E         (EX_C2E << S_CauseExcCode)
#define EX_MDMX         22                                              /* MDMX exception               */
#define EXC_MDMX        (EX_MDMX << S_CauseExcCode)
#define EX_WATCH        23                                              /* Watch exception              */
#define EXC_WATCH       (EX_WATCH << S_CauseExcCode)
#define EX_MCHECK       24                                              /* Machine check exception      */
#define EXC_MCHECK      (EX_MCHECK << S_CauseExcCode)
#define EX_CacheErr     30                                              /* Cache error caused re-entry  */
                                                                        /* to Debug Mode                */
#define EXC_CacheErr    (EX_CacheErr << S_CauseExcCode)

/*********************************************************************************************************
 *  CP0 Config Register
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |M|                             |B| A |  A  |               | K | Config
 * | | Reserved for Implementations|E| T |  R  |    Reserved   | 0 |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_ConfigMore    31                                              /* Additional config registers  */
                                                                        /* present (R)                  */
#define M_ConfigMore    (0x1 << S_ConfigMore)
#define S_ConfigImpl    16                                              /* Implementation-specific      */
                                                                        /* fields                       */
#define M_ConfigImpl    (0x7fff << S_ConfigImpl)
#define S_ConfigBE      15                                              /* Denotes big-endian           */
                                                                        /* operation (R)                */
#define M_ConfigBE      (0x1 << S_ConfigBE)
#define S_ConfigAT      13                                              /* Architecture type (R)        */
#define M_ConfigAT      (0x3 << S_ConfigAT)
#define S_ConfigAR      10                                              /* Architecture revision (R)    */
#define M_ConfigAR      (0x7 << S_ConfigAR)
#define S_ConfigMT      7                                               /* MMU Type (R)                 */
#define M_ConfigMT      (0x7 << S_ConfigMT)
#define S_ConfigK0      0                                               /* Kseg0 coherency              */
                                                                        /* algorithm (R/W)              */
#define M_ConfigK0      (0x7 << S_ConfigK0)

/*********************************************************************************************************
  Bits in the CP0 config register
  Generic bits
*********************************************************************************************************/

#define MIPS_CACHABLE_NO_WA          0
#define MIPS_CACHABLE_WA             1
#define MIPS_UNCACHED                2
#define MIPS_CACHABLE_NONCOHERENT    3
#define MIPS_CACHABLE_CE             4
#define MIPS_CACHABLE_COW            5
#define MIPS_CACHABLE_CUW            6
#define MIPS_CACHABLE_ACCELERATED    7

/*********************************************************************************************************
 *  CP0 Config1 Register
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |M|  MMU Size |  IS |  IL |  IA |  DS |  DL |  DA |C|M|P|W|C|E|F| Config1
 * | |           |     |     |     |     |     |     |2|D|C|R|A|P|P|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_Config1More       31                                          /*  Additional Config registers */
                                                                        /*  present (R)                 */
#define M_Config1More       (0x1 << S_Config1More)
#define S_Config1MMUSize    25                                          /*  Number of MMU entries-1 (R) */
#define M_Config1MMUSize    (0x3f << S_Config1MMUSize)
#define S_Config1IS         22                                          /*  Icache sets per way (R)     */
#define M_Config1IS         (0x7 << S_Config1IS)
#define S_Config1IL         19                                          /*  Icache line size (R)        */
#define M_Config1IL         (0x7 << S_Config1IL)
#define S_Config1IA         16                                          /*  Icache associativity-1 (R)  */
#define M_Config1IA         (0x7 << S_Config1IA)
#define S_Config1DS         13                                          /*  Dcache sets per way (R)     */
#define M_Config1DS         (0x7 << S_Config1DS)
#define S_Config1DL         10                                          /*  Dcache line size (R)        */
#define M_Config1DL         (0x7 << S_Config1DL)
#define S_Config1DA         7                                           /*  Dcache associativity (R)    */
#define M_Config1DA         (0x7 << S_Config1DA)
#define S_Config1C2         6                                           /*  Coprocessor 2 present (R)   */
#define M_Config1C2         (0x1 << S_Config1C2)
#define S_Config1MD         5                                           /*  Denotes MDMX present (R)    */
#define M_Config1MD         (0x1 << S_Config1MD)
#define S_Config1PC         4                                           /*  Denotes performance         */
                                                                        /*  counters present (R)        */
#define M_Config1PC         (0x1 << S_Config1PC)
#define S_Config1WR         3                                           /*  Denotes watch registers     */
                                                                        /*  present (R)                 */
#define M_Config1WR         (0x1 << S_Config1WR)
#define S_Config1CA         2                                           /*  Denotes MIPS-16 present (R) */
#define M_Config1CA         (0x1 << S_Config1CA)
#define S_Config1EP         1                                           /*  Denotes EJTAG present (R)   */
#define M_Config1EP         (0x1 << S_Config1EP)
#define S_Config1FP         0                                           /*  Denotes floating point      */
                                                                        /*  present (R)                 */
#define M_Config1FP         (0x1 << S_Config1FP)

/*********************************************************************************************************
 *  CP0 Config2 Register
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |M|           |     |     |     |     |     |     | | | | | |S|T| Config2
 * | |           |     |     |     |     |     |     | | | | | |M|L|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_Config2More       31                                          /*  Additional Config registers */
                                                                        /*  present (R)                 */
#define M_Config2More       (0x1 << S_Config2More)
#define S_Config2SM         1                                           /*  Denotes SmartMIPS ASE       */
                                                                        /*  present (R)                 */
#define M_Config2SM         (0x1 << S_Config2SM)
#define S_Config2TL         0                                           /*  Denotes Tracing Logic       */
                                                                        /*  present (R)                 */
#define M_Config2TL         (0x1 << S_Config2TL)


/*********************************************************************************************************
 *  CP0 cache Register
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |IPTI |IPPCI|                               |    VS   |         |IntCtl
 * |     |     |            0                  |         |    0    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_IntCtlIPPI    29
#define M_IntCtlIPPI    (0x7 << S_IntCtlIPPI)
#define S_IntCtlIPPCI   26
#define M_IntCtlIPPCI   (0x7 << S_IntCtlIPPCI)
#define S_IntCtlVS      5
#define M_IntCtlVS      (0x1f << S_IntCtlVS)

/*********************************************************************************************************
 *  CP0 CacheErr Register
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |T|L|E|E|E|E|E|E|E|E|                                           |CacheErr
 * |P|E|D|T|S|E|B|I|1|0|                                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_CcaheType     31                                              /*  reference type              */
#define M_CcaheType     (0x1 << S_CcaheType)                            /*  0=Instr, 1=Data             */
#define S_CcaheLevel    30                                              /*  reference level             */
#define M_CcaheLevel    (0x1 << S_CcaheLevel)                           /*  0=Primary, 1=Secondary      */
#define S_CcaheData     29                                              /*  data field                  */
#define M_CcaheData     (0x1 << S_CcaheData)                            /*  0=No error, 1=Error         */
#define S_CcaheTag      28                                              /*  Tag field                   */
#define M_CcaheTag      (0x1 << S_CcaheTag)                             /*  0=No error, 1=Error         */
#define S_CcaheBus      27                                              /*  error on bus                */
#define M_CcaheBus      (0x1 << S_CcaheBus)                             /*  0=No, 1=Yes                 */
#define S_CcaheECC      26                                              /*  ECC error                   */
#define M_CcaheECC      (0x1 << S_CcaheECC)                             /*  0=No, 1=Yes                 */
#define S_CcaheBoth     25                                              /*  Data & Instruction error    */
#define M_CcaheBoth     (0x1 << S_CcaheBoth)                            /*  0=No, 1=Yes                 */
#define S_CcaheEI       24
#define M_CcaheEI       (0x1 << S_CcaheEI)
#define S_CcaheE1       23
#define M_CcaheE1       (0x1 << S_CcaheE1)
#define S_CcaheE0       22
#define M_CcaheE0       (0x1 << S_CcaheE0)



/*********************************************************************************************************
  Virtual Address Definitions
*********************************************************************************************************/

#define A_K0BASE        0x80000000
#define A_K1BASE        0xa0000000
#define A_K2BASE        0xc0000000
#define A_K3BASE        0xe0000000
#define M_KMAPPED       0x40000000                                      /* KnSEG address is mapped      */
                                                                        /* if bit is one                */
/*********************************************************************************************************
  Cache Instruction Operation Codes
*********************************************************************************************************/

#define K_CachePriI     0                                               /* Primary Icache               */
#define K_CachePriD     1                                               /* Primary Dcache               */
#define K_CachePriU     1                                               /* Unified primary              */
#define K_CacheTerU     2                                               /* Unified Tertiary             */
#define K_CacheSecU     3                                               /* Unified secondary            */

/*********************************************************************************************************
  Cache Operations available on all MIPS processors
*********************************************************************************************************/

#define INDEX_INVALIDATE_I      0x00
#define INDEX_WRITEBACK_INV_D   0x01
#define INDEX_LOAD_TAG_I        0x04
#define INDEX_LOAD_TAG_D        0x05
#define INDEX_STORE_TAG_I       0x08
#define INDEX_STORE_TAG_D       0x09
#define HIT_INVALIDATE_I        0x10
#define HIT_INVALIDATE_D        0x11
#define HIT_WRITEBACK_INV_D     0x15
#define HIT_WRITEBACK_I         0x18
#define HIT_WRITEBACK_D         0x19


#endif                                                                  /*  ARCH_DEF_H_                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
