/**********************************************************************************************************
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
** 文   件   名: mips32Mmu.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2015 年 10 月 12 日
**
** 描        述: MIPS32 体系构架 MMU 驱动.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0
#include "arch/mips/common/cp0/mipsCp0.h"
/*********************************************************************************************************
  ENTRYLO
*********************************************************************************************************/
#define MIPS32_ENTRYLO_G_BIT            (1 << 0)
#define MIPS32_ENTRYLO_V_BIT            (1 << 1)
#define MIPS32_ENTRYLO_D_BIT            (1 << 2)

#define MIPS32_ENTRYLO_C_SHIFT          (3)
#define MIPS32_ENTRYLO_C_MASK           (0x07 << MIPS32_ENTRYLO_C_SHIFT)

#define MIPS32_ENTRYLO_PFN_MASK         (UINT32)(~0x3F)
#define MIPS32_ENTRYLO_PFN_SHIFT        (6)

#define MIPS32_ENTRYHI_VPN_SHIFT        (12)

#define MIPS32_PTE_EXEC_SHIFT           (0)
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hPGDPartition;                           /*  系统目前仅使用一个 PGD      */
static PVOID                _G_pvPTETable;                              /*  PTE 表                      */
static UINT32               _G_uiTlbSize = 0;                           /*  TLB 数组大小                */
/*********************************************************************************************************
** 函数名称: mips32MmuEnable
** 功能描述: 使能 MMU
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  mips32MmuEnable (VOID)
{

}
/*********************************************************************************************************
** 函数名称: mips32MmuDisable
** 功能描述: 禁能 MMU
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  mips32MmuDisable (VOID)
{

}
/*********************************************************************************************************
** 函数名称: mips32MmuInvalidateTLB
** 功能描述: 无效 TLB
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  mips32MmuInvalidateTLB (VOID)
{
    UINT32  uiEntryHiBak = mipsCp0EntryHiRead();
    INT     i;

    for (i = 0; i < _G_uiTlbSize; i++) {
        mipsCp0IndexWrite(i);
        mipsCp0EntryLo0Write(0);
        mipsCp0EntryLo1Write(0);
        mipsCp0EntryHiWrite(0);
        MIPS_EXEC_INS("tlbwi");
    }

    mipsCp0EntryHiWrite(uiEntryHiBak);
}
/*********************************************************************************************************
** 函数名称: mips32MmuInvalidateTLBMVA
** 功能描述: 无效指定 MVA 的 TLB
** 输　入  : ulAddr            指定 MVA
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  mips32MmuInvalidateTLBMVA (addr_t  ulAddr)
{
    UINT32  uiEntryHiBak = mipsCp0EntryHiRead();

    mipsCp0EntryHiWrite((ulAddr >> LW_CFG_VMM_PAGE_SHIFT) << MIPS32_ENTRYHI_VPN_SHIFT);

    MIPS_EXEC_INS("tlbp");

    INT32  iIndex = mipsCp0IndexRead();
    if (iIndex >= 0) {
        mipsCp0EntryLo0Write(0);
        mipsCp0EntryLo1Write(0);
        mipsCp0EntryHiWrite(0);
        MIPS_EXEC_INS("tlbwi");
    }

    mipsCp0EntryHiWrite(uiEntryHiBak);
}
/*********************************************************************************************************
** 函数名称: mips32MmuBuildPgdesc
** 功能描述: 生成一个一级描述符 (PGD 描述符)
** 输　入  : uiBaseAddr              基地址     (二级页表基地址)
**           uiFlag
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PGD_TRANSENTRY  mips32MmuBuildPgdesc (UINT32  uiBaseAddr)
{
    return  (uiBaseAddr);                                               /*  一级描述符就是二级页表基地址*/
}
/*********************************************************************************************************
** 函数名称: mips32MmuBuildPtentry
** 功能描述: 生成一个二级描述符 (PTE 描述符)
** 输　入  : uiBaseAddr              基地址     (页地址)
**           uiFlag
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  mips32MmuBuildPtentry (UINT32  uiBaseAddr,
                                                 ULONG   ulFlag)
{
    LW_PTE_TRANSENTRY   stDescriptor;
    UINT32              uiPFN;

    uiPFN = uiBaseAddr >> LW_CFG_VMM_PAGE_SHIFT;                        /*  计算 PFN                    */

    stDescriptor.PTE_uiEntryLO = uiPFN << MIPS32_ENTRYLO_PFN_SHIFT;     /*  填充 PFN                    */

    if (ulFlag & LW_VMM_FLAG_VALID) {
        stDescriptor.PTE_uiEntryLO |= MIPS32_ENTRYLO_V_BIT;             /*  填充 V 位                   */
    }

    if (ulFlag & LW_VMM_FLAG_WRITABLE) {
        stDescriptor.PTE_uiEntryLO |= MIPS32_ENTRYLO_D_BIT;             /*  填充 D 位                   */
    }

    stDescriptor.PTE_uiEntryLO |= MIPS32_ENTRYLO_G_BIT;                 /*  填充 G 位                   */

    if (ulFlag & LW_VMM_FLAG_CACHEABLE) {                               /*  填充 C 位                   */
        stDescriptor.PTE_uiEntryLO |= MIPS_CACHABLE_NONCOHERENT << MIPS32_ENTRYLO_C_SHIFT;
    }

    if (ulFlag & LW_VMM_FLAG_EXECABLE) {
        stDescriptor.PTE_uiSoftware |= 1 << MIPS32_PTE_EXEC_SHIFT;      /*  填充软件的可执行位          */
    }

    return  (stDescriptor);
}
/*********************************************************************************************************
** 函数名称: mips32MmuMemInit
** 功能描述: 初始化 MMU 页表内存区
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  mips32MmuMemInit (PLW_MMU_CONTEXT  pmmuctx)
{
#define PGD_BLOCK_SIZE  (16 * LW_CFG_KB_SIZE)
#define PTE_BLOCK_SIZE  ( 2 * LW_CFG_KB_SIZE)

    PVOID   pvPgdTable;
    PVOID   pvPteTable;
    
    ULONG   ulPgdNum = bspMmuPgdMaxNum();
    
    pvPgdTable = __KHEAP_ALLOC_ALIGN(ulPgdNum * PGD_BLOCK_SIZE, PGD_BLOCK_SIZE);

    /*
     * PTE 表大小为 8MByte，需要 8MByte 对齐才能写入 Context 寄存器
     */
    pvPteTable = __KHEAP_ALLOC_ALIGN(8 * LW_CFG_MB_SIZE, 8 * LW_CFG_MB_SIZE);
    
    if (!pvPgdTable || !pvPteTable) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page table.\r\n");
        return  (PX_ERROR);
    }
    
    _G_pvPTETable = pvPteTable;

    _G_hPGDPartition = API_PartitionCreate("pgd_pool", pvPgdTable, ulPgdNum, PGD_BLOCK_SIZE,
                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (!_G_hPGDPartition) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can not allocate page pool.\r\n");
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: mips32MmuGlobalInit
** 功能描述: 调用 BSP 对 MMU 初始化
** 输　入  : pcMachineName     使用的机器名称
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  mips32MmuGlobalInit (CPCHAR  pcMachineName)
{
    UINT32  uiConfig = mipsCp0ConfigRead();                             /*  读 Config0                  */

    if (((uiConfig & M_ConfigMT) >> S_ConfigMT) != 1) {                 /*  Config0 MT 域 != 1，没有 MMU*/
        _DebugHandle(__ERRORMESSAGE_LEVEL, "No MMU!\r\n");
        return  (PX_ERROR);
    }

    if (uiConfig & (M_ConfigMore)) {                                    /*  有 Config1                  */
        uiConfig = mipsCp0Config1Read();                                /*  读 Config1                  */
        UINT32  uiMMUSize = (uiConfig >> 25) & 0x3F;                    /*  获得 MMUSize 域             */
        _G_uiTlbSize = uiMMUSize + 1;
    } else {
        _G_uiTlbSize = 64;                                              /*  按最大算                    */
    }

    _DebugFormat(__LOGMESSAGE_LEVEL, "MMU TLB size = %d.\r\n", _G_uiTlbSize);

    archCacheReset(pcMachineName);                                      /*  复位 Cache                  */
    
    mips32MmuInvalidateTLB();                                           /*  无效 TLB                    */
    
    mipsCp0EntryHiWrite(0);                                             /*  ASID = 0                    */
    
    mipsCp0PageMaskWrite(0);                                            /*  4K 页面大小                 */

    mipsCp0WiredWrite(0);                                               /*  全部允许随机替换            */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: mips32MmuPgdOffset
** 功能描述: 通过虚拟地址计算 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : 对应的 PGD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *mips32MmuPgdOffset (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = pmmuctx->MMUCTX_pgdEntry;
    
    /*
     * ulAddr >> LW_CFG_VMM_PGD_SHIFT 计算 PGD 号，
     *
     * 而 LW_PGD_TRANSENTRY 的大小为 4
     */
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry
               | ((ulAddr >> LW_CFG_VMM_PGD_SHIFT) * 4));               /*  获得一级页表描述符地址      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: mips32MmuPmdOffset
** 功能描述: 通过虚拟地址计算 PMD 项
** 输　入  : p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PMD 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *mips32MmuPmdOffset (LW_PGD_TRANSENTRY  *p_pgdentry, addr_t  ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);                          /*  MIPS 无 PMD 项              */
}
/*********************************************************************************************************
** 函数名称: mips32MmuPteOffset
** 功能描述: 通过虚拟地址计算 PTE 项
** 输　入  : p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 对应的 PTE 表项地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static  LW_PTE_TRANSENTRY *mips32MmuPteOffset (LW_PMD_TRANSENTRY  *p_pmdentry, addr_t  ulAddr)
{
    REGISTER LW_PTE_TRANSENTRY  *p_pteentry;
    REGISTER UINT32              uiTemp;

    uiTemp = (UINT32)(*p_pmdentry);                                     /*  获得一级页表描述符          */
    
    p_pteentry = (LW_PTE_TRANSENTRY *)(uiTemp);                         /*  获得二级页表基地址          */

    /*
     * ulAddr >> (LW_CFG_VMM_PAGE_SHIFT - 3) 相当于 (ulAddr >> LW_CFG_VMM_PAGE_SHIFT) * 8
     *
     * ulAddr >> LW_CFG_VMM_PAGE_SHIFT 取得页号（全局，非段内）
     *
     * 而 LW_PTE_TRANSENTRY 的大小为 8
     *
     * & 0x7F8 == & 0b11111111000, 段内页号占 8 位（LW_CFG_VMM_PGD_SHIFT - LW_CFG_VMM_PAGE_SHIFT
     * 一段有 256 个页面）
     */
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)p_pteentry
               | ((ulAddr >> (LW_CFG_VMM_PAGE_SHIFT - 3) & 0x7F8)));    /*  获得虚拟地址页表描述符地址  */
    
    return  (p_pteentry);
}
/*********************************************************************************************************
** 函数名称: mips32MmuPgdIsOk
** 功能描述: 判断 PGD 项的描述符是否正确
** 输　入  : pgdentry       PGD 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  mips32MmuPgdIsOk (LW_PGD_TRANSENTRY  pgdentry)
{
    return  (pgdentry ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: mips32MmuPteIsOk
** 功能描述: 判断 PTE 项的描述符是否正确
** 输　入  : pteentry       PTE 项描述符
** 输　出  : 是否正确
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  mips32MmuPteIsOk (LW_PTE_TRANSENTRY  pteentry)
{
    return  (pteentry.PTE_uiEntryLO ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: mips32MmuPgdAlloc
** 功能描述: 分配 PGD 项
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址 (参数 0 即偏移量为 0 , 需要返回页表基地址)
** 输　出  : 分配 PGD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PGD_TRANSENTRY *mips32MmuPgdAlloc (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    REGISTER LW_PGD_TRANSENTRY  *p_pgdentry = (LW_PGD_TRANSENTRY *)API_PartitionGet(_G_hPGDPartition);
    
    if (!p_pgdentry) {
        return  (LW_NULL);
    }
    
    lib_bzero(p_pgdentry, PGD_BLOCK_SIZE);                              /*  无效一级页表项              */

    /*
     * p_pgdentry >> LW_CFG_VMM_PGD_SHIFT 计算 PGD 号，
     *
     * 而 LW_PGD_TRANSENTRY 的大小为 4
     */
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry
               | ((ulAddr >> LW_CFG_VMM_PGD_SHIFT) * 4));               /*  获得一级页表描述符地址      */
               
    return  (p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: mips32MmuPgdFree
** 功能描述: 释放 PGD 项
** 输　入  : p_pgdentry     pgd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  mips32MmuPgdFree (LW_PGD_TRANSENTRY  *p_pgdentry)
{
    /*
     * 释放时对齐到 PGD_BLOCK_SIZE，因为分配时对齐到 PGD_BLOCK_SIZE
     */
    p_pgdentry = (LW_PGD_TRANSENTRY *)((addr_t)p_pgdentry & (~(PGD_BLOCK_SIZE - 1)));
    
    API_PartitionPut(_G_hPGDPartition, (PVOID)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: mips32MmuPmdAlloc
** 功能描述: 分配 PMD 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pgdentry     pgd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PMD 地址
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static LW_PMD_TRANSENTRY *mips32MmuPmdAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                             LW_PGD_TRANSENTRY  *p_pgdentry,
                                             addr_t              ulAddr)
{
    return  ((LW_PMD_TRANSENTRY *)p_pgdentry);
}
/*********************************************************************************************************
** 函数名称: mips32MmuPmdFree
** 功能描述: 释放 PMD 项
** 输　入  : p_pmdentry     pmd 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  mips32MmuPmdFree (LW_PMD_TRANSENTRY  *p_pmdentry)
{
    (VOID)p_pmdentry;
}
/*********************************************************************************************************
** 函数名称: mips32MmuPteAlloc
** 功能描述: 分配 PTE 项
** 输　入  : pmmuctx        mmu 上下文
**           p_pmdentry     pmd 入口地址
**           ulAddr         虚拟地址
** 输　出  : 分配 PTE 地址
** 全局变量: 
** 调用模块: VMM 这里没有关闭中断, 回写 CACHE 时, 需要手动关中断, SylixOS 映射完毕会自动清快表, 所以
             这里不用清除快表.
*********************************************************************************************************/
static LW_PTE_TRANSENTRY  *mips32MmuPteAlloc (PLW_MMU_CONTEXT     pmmuctx,
                                              LW_PMD_TRANSENTRY  *p_pmdentry,
                                              addr_t              ulAddr)
{
    LW_PTE_TRANSENTRY  *p_pteentry;

    /*
     * ulAddr >> LW_CFG_VMM_PGD_SHIFT 计算 PGD 号
     * 每 PGD 一个二级页表
     * 二级页表的大小为 PTE_BLOCK_SIZE
     */
    p_pteentry = (LW_PTE_TRANSENTRY *)((addr_t)_G_pvPTETable
               | ((ulAddr >> LW_CFG_VMM_PGD_SHIFT) * PTE_BLOCK_SIZE));

    lib_bzero(p_pteentry, PTE_BLOCK_SIZE);

    *p_pmdentry = (LW_PMD_TRANSENTRY)mips32MmuBuildPgdesc((UINT32)p_pteentry);  /*  设置二级页表基地址  */

    return  (mips32MmuPteOffset(p_pmdentry, ulAddr));
}
/*********************************************************************************************************
** 函数名称: mips32MmuPteFree
** 功能描述: 释放 PTE 项
** 输　入  : p_pteentry     pte 入口地址
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  mips32MmuPteFree (LW_PTE_TRANSENTRY  *p_pteentry)
{
    (VOID)p_pteentry;
}
/*********************************************************************************************************
** 函数名称: mips32MmuPtePhysGet
** 功能描述: 通过 PTE 表项, 查询物理地址
** 输　入  : pteentry           pte 表项
**           pulPhysicalAddr    获得的物理地址
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  mips32MmuPtePhysGet (LW_PTE_TRANSENTRY  pteentry, addr_t  *pulPhysicalAddr)
{
    UINT32   uiPFN   = (pteentry.PTE_uiEntryLO & MIPS32_ENTRYLO_PFN_MASK)
                       >> MIPS32_ENTRYLO_PFN_SHIFT;                     /*  获得物理页面号              */

    *pulPhysicalAddr = uiPFN << LW_CFG_VMM_PAGE_SHIFT;                  /*  计算页面物理地址            */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: mips32MmuFlagGet
** 功能描述: 获得指定虚拟地址的 SylixOS 权限标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
** 输　出  : SylixOS 权限标志
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static ULONG  mips32MmuFlagGet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = mips32MmuPgdOffset(pmmuctx, ulAddr);/*  获得一级描述符地址         */

    if (mips32MmuPgdIsOk(*p_pgdentry)) {                                /*  一级描述符有效              */

        LW_PTE_TRANSENTRY  *p_pteentry = mips32MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                            ulAddr);    /*  获得二级描述符地址          */

        LW_PTE_TRANSENTRY   stDescriptor = *p_pteentry;                 /*  获得二级描述符              */

        if (mips32MmuPteIsOk(stDescriptor)) {                           /*  二级描述符有效              */

            if (stDescriptor.PTE_uiEntryLO & MIPS32_ENTRYLO_V_BIT) {    /*  二级描述符有效              */
                ULONG    ulFlag = 0;
                UINT32   uiCache;

                ulFlag |= LW_VMM_FLAG_VALID;                            /*  映射有效                    */
                ulFlag |= LW_VMM_FLAG_ACCESS;                           /*  可以访问                    */

                if (stDescriptor.PTE_uiSoftware & (1 << MIPS32_PTE_EXEC_SHIFT)) {
                    ulFlag |= LW_VMM_FLAG_EXECABLE;                     /*  可以执行                    */
                }

                if (stDescriptor.PTE_uiEntryLO & MIPS32_ENTRYLO_D_BIT) {/*  可写                        */
                    ulFlag |= LW_VMM_FLAG_WRITABLE;
                }

                uiCache = (stDescriptor.PTE_uiEntryLO & MIPS32_ENTRYLO_C_MASK)
                           >> MIPS32_ENTRYLO_C_SHIFT;                   /*  获得 CACHE 属性             */
                switch (uiCache) {
                case MIPS_UNCACHED:                                     /*  不可以 CACHE                */
                    break;

                case MIPS_CACHABLE_NONCOHERENT:                         /*  可以 CACHE, 但不参与一致性  */
                    ulFlag |= LW_VMM_FLAG_CACHEABLE;
                    break;

                default:
                    break;
                }

                return  (ulFlag);
            }
        }
    }

    return  (LW_VMM_FLAG_UNVALID);
}
/*********************************************************************************************************
** 函数名称: mips32MmuFlagSet
** 功能描述: 设置指定虚拟地址的 flag 标志
** 输　入  : pmmuctx        mmu 上下文
**           ulAddr         虚拟地址
**           ulFlag         flag 标志
** 输　出  : ERROR or OK
** 全局变量: 
** 调用模块: 
** 注  意  : 由于改变了单条目的页表, VMM 本身在这个函数并不无效快表, 所以这里需要无效指定条目的快表.
*********************************************************************************************************/
static INT  mips32MmuFlagSet (PLW_MMU_CONTEXT  pmmuctx, addr_t  ulAddr, ULONG  ulFlag)
{
    LW_PGD_TRANSENTRY  *p_pgdentry = mips32MmuPgdOffset(pmmuctx, ulAddr);/*  获得一级描述符地址         */

    if (mips32MmuPgdIsOk(*p_pgdentry)) {                                /*  一级描述符有效              */

        LW_PTE_TRANSENTRY  *p_pteentry = mips32MmuPteOffset((LW_PMD_TRANSENTRY *)p_pgdentry,
                                                            ulAddr);    /*  获得二级描述符地址          */

        LW_PTE_TRANSENTRY   stDescriptor = *p_pteentry;                 /*  获得二级描述符              */

        if (mips32MmuPteIsOk(stDescriptor)) {                           /*  二级描述符有效              */

            UINT32   uiPFN = (stDescriptor.PTE_uiEntryLO & MIPS32_ENTRYLO_PFN_MASK)
                              >> MIPS32_ENTRYLO_PFN_SHIFT;              /*  获得物理页号                */
            addr_t   ulPhysicalAddr = uiPFN << LW_CFG_VMM_PAGE_SHIFT;   /*  计算页面物理地址            */

            /*
             * 构建二级描述符并设置二级描述符
             */
            *p_pteentry = (LW_PTE_TRANSENTRY)mips32MmuBuildPtentry((UINT32)ulPhysicalAddr,
                                                                   ulFlag);
            mips32MmuInvalidateTLBMVA(ulAddr);                          /*  无效 TLB                    */
            return  (ERROR_NONE);
        } else {
            return  (PX_ERROR);
        }
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: mips32MmuMakeTrans
** 功能描述: 设置页面映射关系
** 输　入  : pmmuctx        mmu 上下文
**           p_pteentry     对应的页表项
**           ulPhysicalAddr 物理地址
**           ulFlag         对应的类型
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
** 注  意  : 这里不需要清除快表 TLB, 因为 VMM 自身会作此操作.
*********************************************************************************************************/
static VOID  mips32MmuMakeTrans (PLW_MMU_CONTEXT     pmmuctx,
                                 LW_PTE_TRANSENTRY  *p_pteentry,
                                 addr_t              ulPhysicalAddr,
                                 addr_t              ulFlag)
{
    /*
     * 构建二级描述符并设置二级描述符
     */
    *p_pteentry = (LW_PTE_TRANSENTRY)mips32MmuBuildPtentry((UINT32)ulPhysicalAddr,
                                                           ulFlag);
}
/*********************************************************************************************************
** 函数名称: mips32MmuMakeCurCtx
** 功能描述: 设置 MMU 当前上下文
** 输　入  : pmmuctx        mmu 上下文
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  mips32MmuMakeCurCtx (PLW_MMU_CONTEXT  pmmuctx)
{
    mipsCp0ContextWrite((UINT32)_G_pvPTETable);                         /*  将 PTE 表写入 Context 寄存器*/
}
/*********************************************************************************************************
** 函数名称: mips32MmuInit
** 功能描述: MMU 系统初始化
** 输　入  : pmmuop            MMU 操作函数集
**           pcMachineName     使用的机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  mips32MmuInit (LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName)
{
    pmmuop->MMUOP_ulOption        = 0ul;
    pmmuop->MMUOP_pfuncMemInit    = mips32MmuMemInit;
    pmmuop->MMUOP_pfuncGlobalInit = mips32MmuGlobalInit;
    
    pmmuop->MMUOP_pfuncPGDAlloc = mips32MmuPgdAlloc;
    pmmuop->MMUOP_pfuncPGDFree  = mips32MmuPgdFree;
    pmmuop->MMUOP_pfuncPMDAlloc = mips32MmuPmdAlloc;
    pmmuop->MMUOP_pfuncPMDFree  = mips32MmuPmdFree;
    pmmuop->MMUOP_pfuncPTEAlloc = mips32MmuPteAlloc;
    pmmuop->MMUOP_pfuncPTEFree  = mips32MmuPteFree;
    
    pmmuop->MMUOP_pfuncPGDIsOk = mips32MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPMDIsOk = mips32MmuPgdIsOk;
    pmmuop->MMUOP_pfuncPTEIsOk = mips32MmuPteIsOk;
    
    pmmuop->MMUOP_pfuncPGDOffset = mips32MmuPgdOffset;
    pmmuop->MMUOP_pfuncPMDOffset = mips32MmuPmdOffset;
    pmmuop->MMUOP_pfuncPTEOffset = mips32MmuPteOffset;
    
    pmmuop->MMUOP_pfuncPTEPhysGet = mips32MmuPtePhysGet;
    
    pmmuop->MMUOP_pfuncFlagGet = mips32MmuFlagGet;
    pmmuop->MMUOP_pfuncFlagSet = mips32MmuFlagSet;
    
    pmmuop->MMUOP_pfuncMakeTrans     = mips32MmuMakeTrans;
    pmmuop->MMUOP_pfuncMakeCurCtx    = mips32MmuMakeCurCtx;
    pmmuop->MMUOP_pfuncInvalidateTLB = mips32MmuInvalidateTLB;
    pmmuop->MMUOP_pfuncSetEnable     = mips32MmuEnable;
    pmmuop->MMUOP_pfuncSetDisable    = mips32MmuDisable;
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
