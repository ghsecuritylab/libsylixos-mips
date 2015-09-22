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
** 文   件   名: assembler.h
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2015 年 09 月 01 日
**
** 描        述: MIPS 汇编相关.
*********************************************************************************************************/

#ifndef __ASMMIPS_ASSEMBLER_H
#define __ASMMIPS_ASSEMBLER_H

/*********************************************************************************************************
  定义MIPS架构是大/小端存储
*********************************************************************************************************/

#if defined(MIPSEB) || defined(__MIPSEB__)
#undef __SYLIXOS_MIPS_BYTE_ORDER
#define __SYLIXOS_MIPS_BYTE_ORDER             _BIG_ENDIAN
#elif defined(MIPSEL) || defined(__MIPSEL__)
#undef __SYLIXOS_MIPS_BYTE_ORDER
#define __SYLIXOS_MIPS_BYTE_ORDER     _LITTLE_ENDIAN
#else
#warning "One of MIPSEL or MIPSEB must be defined"
#endif

/*********************************************************************************************************
  定MIPS32/MIPS64，暂时先不用
*********************************************************************************************************/

#define __SYLIXOS_CPU_MIPS_32BIT ((__SYLIXOS_CPU==__SYLIXOS_MIPS32) || \
                (__SYLIXOS_CPU==__SYLIXOS_MIPSI2) || \
                (__SYLIXOS_CPU==__SYLIXOS_MIPSI32) || \
                (__SYLIXOS_CPU==__SYLIXOS_MIPSI32R2))

#define __SYLIXOS_CPU_MIPS_64BIT ((__SYLIXOS_CPU==__SYLIXOS_MIPS64) || \
                (__SYLIXOS_CPU==__SYLIXOS_MIPSI3) || \
                (__SYLIXOS_CPU==__SYLIXOS_MIPSI64) || \
                (__SYLIXOS_CPU==__SYLIXOS_MIPSI64R2))

/*********************************************************************************************************
  mips架构下的特殊code定义
*********************************************************************************************************/

#if defined(__ASSEMBLY__) || defined(ASSEMBLY)

/*********************************************************************************************************
  汇编定定义
*********************************************************************************************************/

/*********************************************************************************************************
  EXPORT - export definition of symbol
*********************************************************************************************************/

#define EXPORT_LABEL(label)       .global label

/*********************************************************************************************************
  IMPORT - import definition of symbol
*********************************************************************************************************/

#define IMPORT_LABEL(label)       .extern label

#define FUNC_LABEL(func)          func:
#define LINE_LABEL(line)          line:

/*********************************************************************************************************
  FUNC_DEF - declare leaf routine
*********************************************************************************************************/
#define FUNC_DEF(name) \
        .text; \
        .balign 4; \
        .type symbol, @function; \
        .ent  name; \
name:

/*********************************************************************************************************
  FUNC_END - mark end of function
*********************************************************************************************************/
#define FUNC_END(name) \
        .size name,.-name; \
        .end  name

#define MACRO_DEF(mfunc...) \
        .macro  mfunc

#define MACRO_END() \
        .endm

#define FILE_BEGIN() \
        .set noreorder; \
        .balign 4;

#define FILE_END()

#define SECTION(sec) \
        .section sec

#define WEAK(name) \
        .weakext name; \
        .balign 4;

/*********************************************************************************************************
  FEXPORT - export definition of a function symbol
*********************************************************************************************************/

#define FEXPORT(name) \
        .globl  name; \
        .type   name, @name; \
name:

/*********************************************************************************************************
  ABS - export absolute symbol
*********************************************************************************************************/

#define ABS(symbol,value) \
        .globl  symbol; \
symbol  =  value

#define PANIC(msg) \
        .set    push; \
        .set    reorder; \
        PTR_LA  a0, 8f; \
        jal panic; \
9:      b   9b; \
        .set    pop; \
        TEXT(msg)

#define TEXT(msg) \
        .pushsection .data; \
8:      .asciiz msg; \
        .popsection;

/*********************************************************************************************************
  Build text tables
*********************************************************************************************************/

#define TTABLE(string) \
        .pushsection .text; \
        .word   1f; \
        .popsection \
        .pushsection .data; \
1:      .asciiz string; \
        .popsection

/*********************************************************************************************************
  Stack alignment
*********************************************************************************************************/

#define ALSZ    (7)
#define ALMASK  (~7)

/*********************************************************************************************************
  Size of a register
*********************************************************************************************************/

#define REG_SIZE   4

/*********************************************************************************************************
  size define
*********************************************************************************************************/

#ifndef LW_CFG_KB_SIZE
#define LW_CFG_KB_SIZE  (1024)
#define LW_CFG_MB_SIZE  (1024 * LW_CFG_KB_SIZE)
#define LW_CFG_GB_SIZE  (1024 * LW_CFG_MB_SIZE)
#endif

/*********************************************************************************************************
  Use the following macros in assemblercode to load/store registers,pointers etc.
*********************************************************************************************************/

#define REG_S       sw
#define REG_L       lw
#define REG_SUBU    subu
#define REG_ADDU    addu

/*********************************************************************************************************
  How to add/sub/load/store/shift C int variables.
*********************************************************************************************************/

#define INT_ADD     add
#define INT_ADDU    addu
#define INT_ADDI    addi
#define INT_ADDIU   addiu
#define INT_SUB     sub
#define INT_SUBU    subu
#define INT_L       lw
#define INT_S       sw
#define INT_SLL     sll
#define INT_SLLV    sllv
#define INT_SRL     srl
#define INT_SRLV    srlv
#define INT_SRA     sra
#define INT_SRAV    srav

/*********************************************************************************************************
  How to add/sub/load/store/shift C long variables.
*********************************************************************************************************/

#define LONG_ADD    add
#define LONG_ADDU   addu
#define LONG_ADDI   addi
#define LONG_ADDIU  addiu
#define LONG_SUB    sub
#define LONG_SUBU   subu
#define LONG_L      lw
#define LONG_S      sw
#define LONG_SP     swp
#define LONG_SLL    sll
#define LONG_SLLV   sllv
#define LONG_SRL    srl
#define LONG_SRLV   srlv
#define LONG_SRA    sra
#define LONG_SRAV   srav

#define LONG        .word
#define LONGSIZE    4
#define LONGMASK    3
#define LONGLOG     2

/*********************************************************************************************************
  How to add/sub/load/store/shift pointers.
*********************************************************************************************************/

#define PTR_ADD     add
#define PTR_ADDU    addu
#define PTR_ADDI    addi
#define PTR_ADDIU   addiu
#define PTR_SUB     sub
#define PTR_SUBU    subu
#define PTR_L       lw
#define PTR_S       sw
#define PTR_LA      la
#define PTR_LI      li
#define PTR_SLL     sll
#define PTR_SLLV    sllv
#define PTR_SRL     srl
#define PTR_SRLV    srlv
#define PTR_SRA     sra
#define PTR_SRAV    srav

#define PTR_SCALESHIFT  2

#define PTR     .word
#define PTRSIZE     4
#define PTRLOG      2

/*********************************************************************************************************
  Some cp0 registers were extended to 64bit for MIPS III.
*********************************************************************************************************/

#define MFC0        mfc0
#define MTC0        mtc0

#define SSNOP       sll zero, zero, 1
#define NOPS        SSNOP; SSNOP; SSNOP; SSNOP

/*********************************************************************************************************
  MIPS 通用寄存器定义
*********************************************************************************************************/

#define zero            $0                                              /* wired zero                   */
#define AT              $at                                             /* assembler temp               */
#define v0              $2                                              /* return reg 0                 */
#define v1              $3                                              /* return reg 1                 */
#define a0              $4                                              /* arg reg 0                    */
#define a1              $5                                              /* arg reg 1                    */
#define a2              $6                                              /* arg reg 2                    */
#define a3              $7                                              /* arg reg 3                    */
#define t0              $8                                              /* caller saved 0               */
#define t1              $9                                              /* caller saved 1               */
#define t2              $10                                             /* caller saved 2               */
#define t3              $11                                             /* caller saved 3               */
#define t4              $12                                             /* caller saved 4               */
#define t5              $13                                             /* caller saved 5               */
#define t6              $14                                             /* caller saved 6               */
#define t7              $15                                             /* caller saved 7               */
#define s0              $16                                             /* callee saved 0               */
#define s1              $17                                             /* callee saved 1               */
#define s2              $18                                             /* callee saved 2               */
#define s3              $19                                             /* callee saved 3               */
#define s4              $20                                             /* callee saved 4               */
#define s5              $21                                             /* callee saved 5               */
#define s6              $22                                             /* callee saved 6               */
#define s7              $23                                             /* callee saved 7               */
#define t8              $24                                             /* caller saved 8               */
#define t9              $25                                             /* caller saved 9               */
#define k0              $26                                             /* kernel temp 0                */
#define k1              $27                                             /* kernel temp 1                */
#define gp              $28                                             /* global pointer               */
#define sp              $29                                             /* stack pointer                */
#define s8              $30                                             /* callee saved 8               */
#define fp              s8                                              /* callee saved 8               */
#define ra              $31                                             /* return address               */

/*********************************************************************************************************
  MIPS 协处理器0寄存器定义
*********************************************************************************************************/

#define CP0_INX         $0                                              /* tlb index                    */
#define CP0_RAND        $1                                              /* tlb random                   */
#define CP0_TLBLO       $2                                              /* tlb entry low                */
#define CP0_TLBLO0      $2                                              /* tlb entry low 0              */
#define CP0_TLBLO1      $3                                              /* tlb entry low 1              */
#define CP0_CTXT        $4                                              /* tlb context                  */
#define CP0_PAGEMASK    $5                                              /* page mask                    */
#define CP0_WIRED       $6                                              /* lb wired entries             */
#define CP0_HWRENA      $7
#define CP0_BADVADDR    $8                                              /* bad virtual address          */
#define CP0_COUNT       $9                                              /* count                        */
#define CP0_TLBHI       $10                                             /* tlb entry hi                 */
#define CP0_COMPARE     $11                                             /* compare                      */
#define CP0_STATUS      $12                                             /* status register              */
#define CP0_CAUSE       $13                                             /* exception cause              */
#define CP0_EPC         $14                                             /* exception pc                 */
#define CP0_PRID        $15
#define CP0_EBASE       $15,1
#define CP0_CONFIG      $16
#define CP0_LLADDR      $17
#define CP0_WATCHLO     $18
#define CP0_WATCHHI     $19
#define CP0_ECC         $26
#define CP0_CACHEERR    $27
#define CP0_TAGLO       $28
#define CP0_TAGHI       $29
#define CP0_ERRPC       $30

/*********************************************************************************************************
  CP0 Status Register
  ************************************************************************
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
#define S_StatusKX      7                                               /* Enable access to extended
                                                                         * kernel addresses
                                                                         * (MIPS64 only) (R/W)          */
#define M_StatusKX      (0x1 << S_StatusKX)
#define S_StatusSX      6                                               /* Enable access to extended
                                                                         * supervisor addresses
                                                                         * (MIPS64 only) (R/W)          */
#define M_StatusSX      (0x1 << S_StatusSX)
#define S_StatusUX      5                                               /* Enable access to extended user
                                                                         * addresses (MIPS64 only) (R/W)*/
#define M_StatusUX      (0x1 << S_StatusUX)
#define S_StatusKSU     3                                               /* Two-bit current mode (R/W)   */
#define M_StatusKSU     (0x3 << S_StatusKSU)
#define S_StatusUM      4                                               /* User mode if supervisor mode
                                                                         * not implemented (R/W)        */
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
  CP0 Cause Register
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
  CP0 Config Register
  ************************************************************************
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |M|                             |B| A |  A  |               | K | Config
 * | | Reserved for Implementations|E| T |  R  |    Reserved   | 0 |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_ConfigMore    31                                              /* Additional config registers
                                                                         * present (R)                  */
#define M_ConfigMore    (0x1 << S_ConfigMore)
#define S_ConfigImpl    16                                              /* Implementation-specific
                                                                         * fields                       */
#define M_ConfigImpl    (0x7fff << S_ConfigImpl)
#define S_ConfigBE      15                                              /* Denotes big-endian
                                                                         * operation (R)                */
#define M_ConfigBE      (0x1 << S_ConfigBE)
#define S_ConfigAT      13                                              /* Architecture type (R)        */
#define M_ConfigAT      (0x3 << S_ConfigAT)
#define S_ConfigAR      10                                              /* Architecture revision (R)    */
#define M_ConfigAR      (0x7 << S_ConfigAR)
#define S_ConfigMT      7                                               /* MMU Type (R)                 */
#define M_ConfigMT      (0x7 << S_ConfigMT)
#define S_ConfigK0      0                                               /* Kseg0 coherency
                                                                         * algorithm (R/W)              */
#define M_ConfigK0      (0x7 << S_ConfigK0)


/*********************************************************************************************************
  Virtual Address Definitions
*********************************************************************************************************/

#define A_K0BASE        0x80000000
#define A_K1BASE        0xa0000000
#define A_K2BASE        0xc0000000
#define A_K3BASE        0xe0000000
#define M_KMAPPED       0x40000000                                      /* KnSEG address is mapped
                                                                         * if bit is one                */
/*********************************************************************************************************
  Cache Instruction OPeration Codes
*********************************************************************************************************/
#define K_CachePriI     0                                               /* Primary Icache               */
#define K_CachePriD     1                                               /* Primary Dcache               */
#define K_CachePriU     1                                               /* Unified primary              */
#define K_CacheTerU     2                                               /* Unified Tertiary             */
#define K_CacheSecU     3                                               /* Unified secondary            */

#define CONF_CM_CACHABLE_NONCOHERENT 3

#endif                                                                  /*  __ASSEMBLY__                */

#endif                                                                  /*  __ASMMIPS_ASSEMBLER_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/

