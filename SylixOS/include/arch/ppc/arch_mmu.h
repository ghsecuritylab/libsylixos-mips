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
** 文   件   名: arch_mmu.h
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 11 月 26 日
**
** 描        述: PowerPC 内存管理相关.
*********************************************************************************************************/

#ifndef __PPC_ARCH_MMU_H
#define __PPC_ARCH_MMU_H

/*********************************************************************************************************
  虚拟内存页表相关配置
*********************************************************************************************************/

#define LW_CFG_VMM_PAGE_SHIFT                 12                        /*  2^12 = 4096                 */
#define LW_CFG_VMM_PAGE_SIZE                  (1ul << LW_CFG_VMM_PAGE_SHIFT)
#define LW_CFG_VMM_PAGE_MASK                  (~(LW_CFG_VMM_PAGE_SIZE - 1))

#define LW_CFG_VMM_PMD_SHIFT                  20                        /*  NO PMD same as PGD          */
#define LW_CFG_VMM_PMD_SIZE                   (1ul << LW_CFG_VMM_PMD_SHIFT)
#define LW_CFG_VMM_PMD_MASK                   (~(LW_CFG_VMM_PMD_SIZE - 1))

#define LW_CFG_VMM_PGD_SHIFT                  20                        /*  2^20 = 1MB                  */
#define LW_CFG_VMM_PGD_SIZE                   (1ul << LW_CFG_VMM_PGD_SHIFT)
#define LW_CFG_VMM_PGD_MASK                   (~(LW_CFG_VMM_PGD_SIZE - 1))

/*********************************************************************************************************
  转换条目相关, 每一层所包含的表项个数
*********************************************************************************************************/

#define LW_CFG_VMM_PTRS_PER_PGD               256                       /*  PGD 拥有的转换条目数        */
#define LW_CFG_VMM_PTRS_PER_PMD               1                         /*  PMD 拥有的转换条目数        */
#define LW_CFG_VMM_PTRS_PER_PTE               1024                      /*  PTE 拥有的转换条目数        */

/*********************************************************************************************************
  MMU 转换条目类型
*********************************************************************************************************/

typedef UINT32  LW_PGD_TRANSENTRY;                                      /*  页目录类型                  */
typedef UINT32  LW_PMD_TRANSENTRY;                                      /*  中间页目录类型              */

typedef union {
    struct {
        UINT        PTE_uiRPN       : 20;                               /*  物理页号                    */
        UINT        PTE_ucReserved1 :  3;                               /*  保留                        */
        UINT        PTE_bRef        :  1;                               /*  引用位                      */
        UINT        PTE_bChange     :  1;                               /*  修改位                      */
        UINT        PTE_ucWIMG      :  4;                               /*  内存和 CACHE 属性位         */
        UINT        PTE_bReserved2  :  1;                               /*  保留                        */
        UINT        PTE_ucPP        :  2;                               /*  页保护权限位                */
    };
    UINT32          PTE_uiValue;                                        /*  值                          */
} LW_PTE_TRANSENTRY;                                                    /*  页表条目类型                */

#endif                                                                  /*  __PPC_ARCH_MMU_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
