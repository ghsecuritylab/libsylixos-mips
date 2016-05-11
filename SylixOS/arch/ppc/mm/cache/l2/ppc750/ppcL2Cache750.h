/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: ppcL2Cache750.h
**
** 创   建   人: Yang.HaiFeng (杨海峰)
**
** 文件创建日期: 2016 年 03 月 08 日
**
** 描        述: MPC750 体系构架 L2 CACHE 驱动.
*********************************************************************************************************/
#ifndef __ARCH_PPCL2CACHE750_H
#define __ARCH_PPCL2CACHE750_H

/*********************************************************************************************************
  L2 CACHE 配置结构
*********************************************************************************************************/

#ifndef ASSEMBLY

typedef struct {
    BOOL        CFG_bPresent;                       /*  是否存在 L2 Cache                               */
    size_t      CFG_stSize;                         /*  L2 Cache 大小，请用 L2RAM_SIZE_XXX 宏           */
    UINT        CFG_uiL2CR;                         /*  L2CR 寄存器配置值，请用 L2CR_XXX 宏组合         */
} PPC750_L2CACHE_CONFIG;

/*********************************************************************************************************
  L2 CACHE 配置
*********************************************************************************************************/

VOID  ppcL2Cache750Config(PPC750_L2CACHE_CONFIG  *pL2Config);

#endif

/*********************************************************************************************************
  L2 Cache - Define for 755/74xx with L2 cache.
*********************************************************************************************************/

#define L2CR_REG                1017                /*  L2CR register number                            */
#define HID0_REG                1008

#define ARTHUR                  8                   /*  Upper bit 16 bit value of 740/750               */
#define VGER                    0x8000              /*  Upper bit 16 bit value of 740/750               */

#define WRITE_ADDR_U            0x0060              /*  Upper 16 bits of write address                  */
#define L2_SIZE_2M_U            0x0020              /*  Upper 16 bitts of 2 Meg                         */
#define L2_ADR_INCR             0x100               /*  Address increament value                        */
#define L2_SIZE_2M              0x2000              /*  2 MG (0x200000) / 0x100 = 0x2000                */
#define L2_SIZE_1M              0x1000              /*  1 MG (0x100000) / 0x100 = 0x1000                */
#define L2_SIZE_HM              0x800               /*  512K counts                                     */
#define L2_SIZE_QM              0x400               /*  256K(0x40000) / L2_ADR_INCR = 0x40              */

#define L1CACHE_ALIGN_SIZE      32                  /*  32 bytes                                        */
#define L2RAM_SIZE_2M           2097152             /*  2M of L2 RAM                                    */
#define L2RAM_SIZE_1M           1048576             /*  1M of L2 RAM                                    */
#define L2RAM_SIZE_512KB        524288              /*  512 KB of L2 RAM                                */
#define L2RAM_SIZE_256KB        262144              /*  256 KB 0f L2 RAM                                */

/*
 * Defining values for L2CR register:
 *  -  L2 cache enable (1) / disable (0)  (bit 0)
 *  -  cache size (bits 2-3; 3: 1 MB, 2: 512 KB, 1: 256 KB)
 *  -  1.5 clock ratio (bits 4-6)
 *  -  Pinpelined (register-register) synchronous burst RAM (bits 7-8)
 *  -  L2 Data only (bit 9)
 *  -  Test mode on (1) or off (0) (bit 13)
 */

/* L2 Data Parity generation and checking enable */
#define L2CR_PE                 0x4000

/* Values for the L2SIZ bits */
#define L2CR_SIZE_2MB           0x0000
#define L2CR_SIZE_256KB         0x1000
#define L2CR_SIZE_512KB         0x2000
#define L2CR_SIZE_1MB           0x3000

/* Values for L2CLK bits */
#define L2CR_CLK_DISABLE        0x0000
#define L2CR_CLK_1              0x0200
#define L2CR_CLK_1_5            0x0400
#define L2CR_CLK_3_5            0x0600
#define L2CR_CLK_2              0x0800
#define L2CR_CLK_2_5            0x0a00
#define L2CR_CLK_3              0x0c00
#define L2CR_CLK_4              0x0e00

/* Values for L2RAM Type */
#define L2CR_RAM_SYNBRST        0x0000              /*  Synchronous burst SRAM                          */
#define L2CR_RAM_PB3            0x0080              /*  PB3 synchronous burst SRAM (7xx)                */
#define L2CR_RAM_PB2            0x0100              /*  PB2 synchronous burst SRAM                      */
#define L2CR_RAM_SYNLTWRT       0x0180              /*  Synchronouus Late write SRAM                    */

/* L2 data only */
#define PPC7450_L2CR_DO         0x0001

/* L2 data only */
#define L2CR_DO                 0x0040

/* L2 Global invalidate */
#define L2CR_GBL_INV_U          0x0020

/* L2 Ram Control (ZZ enable) */
#define L2CR_RAMCTL             0x0010

/* L2 Write Through */
#define L2CR_WT                 0x0008

/* L2 Test Support */
#define L2CR_TS                 0x0004

/* L2 Output Hold */
#define L2CR_OH_0_5ns           0x0000
#define L2CR_OH_1_0ns           0x0001
#define L2CR_OH_MORE            0x0002
#define L2CR_OH_EVEN_MORE       0x0003

#define L2CR_HWFLUSH            0x0800              /*  Hardware flush for 74X0 lower 16 bits           */

#define L2CR_DISABLE_MASK_U     0x7fff              /*  Disable L2 - upper 16 bits                      */
#define L2CR_EN_U               0x8000              /*  Set L2CR enable bit                             */
#define L2CR_IP                 0x0001              /*  Invalidation in progress                        */

#endif                                                                  /*  __ARCH_PPCL2CACHE750_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
