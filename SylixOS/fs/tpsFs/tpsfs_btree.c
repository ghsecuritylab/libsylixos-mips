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
** 文   件   名: tpsfs_btree.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: btree 操作

** BUG:
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0
#include "tpsfs_type.h"
#include "tpsfs_error.h"
#include "tpsfs_port.h"
#include "tpsfs_super.h"
#include "tpsfs_trans.h"
#include "tpsfs_btree.h"
#include "tpsfs_inode.h"
#include "tpsfs_dev_buf.h"
/*********************************************************************************************************
** 函数名称: __tpsFsAllocBtrNode
** 功能描述: 分配节点内存
**           uiSize             未加载到节点的数据长度，通过计算得到该节点加载后的长度
**           iType              节点类型
** 输　出  : 成功返回新节点指针，失败返回NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PTPS_BTR_NODE  __tpsFsAllocBtrNode (PTPS_INODE pinode, UINT uiSize, INT iType)
{
    PTPS_BTR_NODE   pbtrnode  = LW_NULL;
    UINT            uiAlcSize = sizeof(TPS_BTR_NODE);
    UINT            uiMaxNodeCnt;
    INT             i;

    /*
     *  计算节点子项数目，叶子节点和非叶子节点不一样
     */
    if (iType == TPS_BTR_NODE_LEAF) {
        uiMaxNodeCnt = (uiSize - sizeof(TPS_BTR_NODE)) / sizeof(TPS_BTR_KV);

    } else {
        uiMaxNodeCnt = (uiSize - sizeof(TPS_BTR_NODE)) / (sizeof(TPS_IBLK) * 2);
    }
    
    uiAlcSize += ((uiSize - sizeof(TPS_BTR_NODE)) / (sizeof(TPS_IBLK) * 2)) * sizeof(TPS_BTR_KV);

    for (i = 0; i < TPS_BN_POOL_SIZE; i++) {                            /* 先在缓冲池中查找             */
        if (pinode->IND_pBNPool[i] != LW_NULL &&
            pinode->IND_pBNStatus[i] == TPS_BN_POOL_FREE) {
            pbtrnode                 = pinode->IND_pBNPool[i];
            pinode->IND_pBNStatus[i] = TPS_BN_POOL_BUSY;
            break;
        }
    }

    if (pbtrnode == LW_NULL) {                                          /* 未找到则分配                 */
        pbtrnode = (PTPS_BTR_NODE)TPS_ALLOC(uiAlcSize);
        if (pbtrnode == LW_NULL) {
            return  (pbtrnode);
        }

        for (i = 0; i < TPS_BN_POOL_SIZE; i++) {                        /* 试图加入缓冲池               */
            if (pinode->IND_pBNPool[i] == LW_NULL) {
                pinode->IND_pBNPool[i]   = pbtrnode;
                pinode->IND_pBNStatus[i] = TPS_BN_POOL_BUSY;
                break;
            }
        }
    }

    if (pbtrnode) {
        pbtrnode->ND_uiMaxCnt       = uiMaxNodeCnt;
        pbtrnode->ND_iType          = iType;
        pbtrnode->ND_uiMagic        = TPS_MAGIC_BTRNODE;
        pbtrnode->ND_blkNext        = 0;
        pbtrnode->ND_blkThis        = 0;
        pbtrnode->ND_blkPrev        = 0;
        pbtrnode->ND_blkParent      = 0;
        pbtrnode->ND_inumInode      = 0;
        pbtrnode->ND_uiEntrys       = 0;
        pbtrnode->ND_uiLevel        = 0;
        pbtrnode->ND_ui64Generation = 0;
    }

    return  (pbtrnode);
}
/*********************************************************************************************************
** 函数名称: __tpsFsFreeBtrNode
** 功能描述: 释放节点内存
**           pbtrnode           节点指针
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __tpsFsFreeBtrNode (PTPS_INODE pinode, PTPS_BTR_NODE pbtrnode)
{
    INT i;

    for (i = 0; i < TPS_BN_POOL_SIZE; i++) {                            /* 如在缓冲池中则修改缓冲区状态 */
        if (pinode->IND_pBNPool[i] == pbtrnode) {
            pinode->IND_pBNStatus[i] = TPS_BN_POOL_FREE;
            return;
        }
    }

    TPS_FREE(pbtrnode);
}
/*********************************************************************************************************
** 函数名称: tpsSerialBtrNode
** 功能描述: 序列化节点
**           pbtrnode           节点指针
**           pucBuff            序列化缓冲区
**           uiBlkSize          缓冲区大小
**           uiItemStart        将会被写入磁盘的起始子节点
**           uiItemCnt          将会被写入磁盘的子节点数量
**           puiOffStart        写入磁盘数据起始位置
**           puiOffEnd          写入磁盘数据结束位置
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  tpsSerialBtrNode (PTPS_BTR_NODE   pbtrnode,
                        PUCHAR          pucBuff,
                        UINT            uiBlkSize,
                        UINT            uiItemStart,
                        UINT            uiItemCnt,
                        UINT           *puiOffStart,
                        UINT           *puiOffEnd)
{
    PUCHAR pucPos = pucBuff;
    INT    i;

    TPS_CPU_TO_LE32(pucPos, pbtrnode->ND_uiMagic);
    TPS_CPU_TO_LE32(pucPos, pbtrnode->ND_iType);
    TPS_CPU_TO_LE32(pucPos, pbtrnode->ND_uiEntrys);
    TPS_CPU_TO_LE32(pucPos, pbtrnode->ND_uiMaxCnt);
    TPS_CPU_TO_LE32(pucPos, pbtrnode->ND_uiLevel);
    TPS_CPU_TO_LE32(pucPos, pbtrnode->ND_uiReserved1);
    TPS_CPU_TO_LE64(pucPos, pbtrnode->ND_ui64Generation);
    TPS_CPU_TO_LE64(pucPos, pbtrnode->ND_ui64Reserved2[0]);
    TPS_CPU_TO_LE64(pucPos, pbtrnode->ND_ui64Reserved2[1]);
    
    TPS_CPU_TO_IBLK(pucPos, pbtrnode->ND_inumInode);
    TPS_CPU_TO_IBLK(pucPos, pbtrnode->ND_blkThis);
    TPS_CPU_TO_IBLK(pucPos, pbtrnode->ND_blkParent);
    TPS_CPU_TO_IBLK(pucPos, pbtrnode->ND_blkPrev);
    TPS_CPU_TO_IBLK(pucPos, pbtrnode->ND_blkNext);

    for (i = 0; i < pbtrnode->ND_uiEntrys; i++) {
        if (LW_NULL != puiOffStart && (uiItemStart == i)) {
            (*puiOffStart) = pucPos - pucBuff;
		}

		if (LW_NULL != puiOffEnd && (uiItemStart + uiItemCnt == i)) {
			(*puiOffEnd) = pucPos - pucBuff;
		}

        TPS_CPU_TO_IBLK(pucPos, pbtrnode->ND_kvArr[i].KV_blkKey);
        if (pbtrnode->ND_iType == TPS_BTR_NODE_NON_LEAF) {
            TPS_CPU_TO_IBLK(pucPos, pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr);
        
        } else {
            TPS_CPU_TO_IBLK(pucPos, pbtrnode->ND_kvArr[i].KV_data.KV_value.KV_blkStart);
            TPS_CPU_TO_IBLK(pucPos, pbtrnode->ND_kvArr[i].KV_data.KV_value.KV_blkCnt);
        }
	}
	if (LW_NULL != puiOffStart && (uiItemStart == i)) {
		(*puiOffStart) = pucPos - pucBuff;
	}

	if (LW_NULL != puiOffEnd && (uiItemStart + uiItemCnt == i)) {
		(*puiOffEnd) = pucPos - pucBuff;
	}
}
/*********************************************************************************************************
** 函数名称: tpsUnserialBtrNode
** 功能描述: 逆序列化节点
**           pbtrnode           节点指针
**           pucBuff            序列化缓冲区
**           uiBlkSize          缓冲区大小
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  tpsUnserialBtrNode (PTPS_BTR_NODE pbtrnode, PUCHAR pucBuff, UINT uiBlkSize)
{
    PUCHAR pucPos = pucBuff;
    INT    i;

    TPS_LE32_TO_CPU(pucPos, pbtrnode->ND_uiMagic);
    TPS_LE32_TO_CPU(pucPos, pbtrnode->ND_iType);
    TPS_LE32_TO_CPU(pucPos, pbtrnode->ND_uiEntrys);
    TPS_LE32_TO_CPU(pucPos, pbtrnode->ND_uiMaxCnt);
    TPS_LE32_TO_CPU(pucPos, pbtrnode->ND_uiLevel);
    TPS_LE32_TO_CPU(pucPos, pbtrnode->ND_uiReserved1);
    TPS_LE64_TO_CPU(pucPos, pbtrnode->ND_ui64Generation);
    TPS_LE64_TO_CPU(pucPos, pbtrnode->ND_ui64Reserved2[0]);
    TPS_LE64_TO_CPU(pucPos, pbtrnode->ND_ui64Reserved2[1]);
    
    TPS_IBLK_TO_CPU(pucPos, pbtrnode->ND_inumInode);
    TPS_IBLK_TO_CPU(pucPos, pbtrnode->ND_blkThis);
    TPS_IBLK_TO_CPU(pucPos, pbtrnode->ND_blkParent);
    TPS_IBLK_TO_CPU(pucPos, pbtrnode->ND_blkPrev);
    TPS_IBLK_TO_CPU(pucPos, pbtrnode->ND_blkNext);
    
    for (i = 0; i < pbtrnode->ND_uiEntrys; i++) {
        TPS_IBLK_TO_CPU(pucPos, pbtrnode->ND_kvArr[i].KV_blkKey);
        if (pbtrnode->ND_iType == TPS_BTR_NODE_NON_LEAF) {
            TPS_IBLK_TO_CPU(pucPos, pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr);
        
        } else {
            TPS_IBLK_TO_CPU(pucPos, pbtrnode->ND_kvArr[i].KV_data.KV_value.KV_blkStart);
            TPS_IBLK_TO_CPU(pucPos, pbtrnode->ND_kvArr[i].KV_data.KV_value.KV_blkCnt);
        }
    }
}
/*********************************************************************************************************
** 函数名称: __tpsFsBtrAllocNodeBlk
** 功能描述: 分配块磁盘块
**           ptrans             事物
**           psb                超级块指针
**           pinode             文件inode指针
** 输　出  : 成功：块号  失败：0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_IBLK  __tpsFsBtrAllocNodeBlk (PTPS_TRANS         ptrans,
                                         PTPS_SUPER_BLOCK   psb,
                                         PTPS_INODE         pinode)
{
    INT           i;
    PTPS_BLK_POOL pbp       = psb->SB_pbp;
    TPS_IBLK      blk       = 0;
    TPS_IBLK      blkStart  = 0;
    TPS_IBLK      blkCnt    = 0;

    if (pinode != psb->SB_pinodeSpaceMng) {                             /* 非空间b+tree用空间b+tree分配 */
        if (tpsFsBtreeAllocBlk(ptrans, psb->SB_pinodeSpaceMng,
                               0, 1, &blkStart, &blkCnt) != TPS_ERR_NONE) {
            return  (0);
        }

        return  (blkStart);
    }

    if (pbp->BP_uiBlkCnt < 5) {
        return  (0);
    }

    /*
     * 空间b+tree使用缓冲池分配
     */
    if (tpsFsTransWrite(ptrans, psb, pbp->BP_blkStart,
        pbp->BP_uiStartOff, (PUCHAR)&blk, sizeof(blk)) != TPS_ERR_NONE) {
        return  (0);
    }

    blkStart = pbp->BP_blkArr[0];
    pbp->BP_uiStartOff += sizeof(TPS_IBLK);
    if (pbp->BP_uiStartOff >= psb->SB_uiBlkSize) {
        pbp->BP_blkStart++;
        if (pbp->BP_blkStart == psb->SB_ui64BPStartBlk + psb->SB_ui64BPBlkCnt) {
            pbp->BP_blkStart =  psb->SB_ui64BPStartBlk;
        }

        pbp->BP_uiStartOff = 0;
    }

    pbp->BP_uiBlkCnt--;

    for (i = 0; i < pbp->BP_uiBlkCnt; i++) {
        pbp->BP_blkArr[i] = pbp->BP_blkArr[i + 1];
    }

    return  (blkStart);
}
/*********************************************************************************************************
** 函数名称: __tpsFsBtrFreeNodeBlk
** 功能描述: 释放块磁盘块
**           ptrans             事物
**           psb                超级块指针
**           pinode             文件inode指针
**           blkFree            被释放物理块号
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsBtrFreeNodeBlk (PTPS_TRANS       ptrans,
                                          PTPS_SUPER_BLOCK psb,
                                          PTPS_INODE       pinode,
                                          TPS_IBLK         blkFree)
{
    PTPS_BLK_POOL pbp       = psb->SB_pbp;
    TPS_IBLK      blk       = pbp->BP_blkStart;
    UINT          uiOff;
    UCHAR         ucBuff[TPS_IBLK_SZ];
    PUCHAR        pucPos = ucBuff;

    if (pinode != psb->SB_pinodeSpaceMng) {                             /* 非空间b+tree用空间b+tree回收 */
        if (tpsFsBtreeFreeBlk(ptrans, psb->SB_pinodeSpaceMng,
                              blkFree, blkFree, 1) != TPS_ERR_NONE) {
            return  (TPS_ERR_BTREE_FREE);
        }

        return  (TPS_ERR_NONE);
    }

    if (pbp->BP_uiBlkCnt >= 256) {
        return  (TPS_ERR_BTREE_FREE_ND);
    }

    /*
     * 空间b+tree使用缓冲池回收
     */
    if (pbp->BP_uiStartOff + pbp->BP_uiBlkCnt * sizeof(TPS_IBLK) >= psb->SB_uiBlkSize) {
        blk++;
        if (blk == psb->SB_ui64BPStartBlk + psb->SB_ui64BPBlkCnt) {
            blk =  psb->SB_ui64BPStartBlk;
        }
    }

    uiOff = (pbp->BP_uiStartOff + pbp->BP_uiBlkCnt * sizeof(TPS_IBLK)) & psb->SB_uiBlkMask;

    TPS_CPU_TO_IBLK(pucPos, blkFree);
    if (tpsFsTransWrite(ptrans, psb, blk, uiOff,
                        ucBuff, sizeof(ucBuff)) != TPS_ERR_NONE) {
        return  (TPS_ERR_BP_WRITE);
    }

    pbp->BP_blkArr[pbp->BP_uiBlkCnt] = blkFree;
    pbp->BP_uiBlkCnt++;

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsBtrSearch
** 功能描述: 查找子节点
**           pbtrnode           节点指针
**           blkKey             键值
**           bInsert            是否为插入查找
**           bEqual             返回兼职是否相等
** 输　出  : 子节点下标
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __tpsFsBtrSearch (PTPS_BTR_NODE  pbtrnode,
                              TPS_IBLK       blkKey,
                              BOOL           bInsert,
                              BOOL          *bEqual)
{
    INT iLow = 0;
    INT iHigh;
    INT iMid = 0;

    if (bEqual) {
        (*bEqual) = LW_FALSE;
    }

    if (pbtrnode->ND_uiEntrys <= 0) {
        return  (0);
    }

    if (blkKey <= pbtrnode->ND_kvArr[0].KV_blkKey) {
        if ((blkKey == pbtrnode->ND_kvArr[0].KV_blkKey) && bEqual) {
            (*bEqual) = LW_TRUE;
        }
        return  (0);
    }

    iHigh = pbtrnode->ND_uiEntrys;
    while (iLow + 1 < iHigh) {                                          /* 二分法查找节点               */
        iMid = (iLow + iHigh) / 2;
        if (blkKey == pbtrnode->ND_kvArr[iMid].KV_blkKey) {
            if (bEqual) {
                (*bEqual) = LW_TRUE;
            }
            return  (iMid);

        } else if (blkKey > pbtrnode->ND_kvArr[iMid].KV_blkKey) {
            iLow = iMid;

        } else {
            iHigh = iMid;
        }
    }

    /*
     *  插入元素时返回大于blkKey的最小元素,否则返回小于blkKey的最大元素
     */
    iMid = bInsert ? iHigh : iLow;

    return  (iMid);
}
/*********************************************************************************************************
** 函数名称: __tpsFsBtrGetNode
** 功能描述: 从磁盘读取节点
**           pinode             文件inode指针
**           pbtrnode           节点指针
**           blkPtr             物理块号，从该物理块读取节点
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsBtrGetNode (PTPS_INODE pinode, PTPS_BTR_NODE pbtrnode, TPS_IBLK blkPtr)
{
    PUCHAR              pucBuff;
    UINT                uiOff;
    UINT                uiLen;
    PTPS_SUPER_BLOCK    psb = pinode->IND_psb;

    if (blkPtr == pinode->IND_inum) {
        uiOff = TPS_INODE_DATASTART;
        uiLen = psb->SB_uiBlkSize - TPS_INODE_DATASTART;
    
    } else {
        uiOff = 0;
        uiLen = psb->SB_uiBlkSize;
    }

    pucBuff = pinode->IND_pucBuff;

    if (tpsFsDevBufRead(psb, blkPtr, uiOff, pucBuff, uiLen) != TPS_ERR_NONE) {
        return  (TPS_ERR_BUF_READ);
    }

    tpsUnserialBtrNode(pbtrnode, pucBuff, uiLen);

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsBtrPutNode
** 功能描述: 保存节点到磁盘
**           ptrans             事物
**           pinode             文件inode指针
**           pbtrnode           节点指针
**           blkPtr             物理块号，保存节点到该块
**           bWriteHead         是否需要写入节点头
**           uiItemStart        将会被写入磁盘的起始子节点
**           uiItemCnt          将会被写入磁盘的子节点数量
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsBtrPutNode (PTPS_TRANS       ptrans,
                                      PTPS_INODE       pinode,
                                      PTPS_BTR_NODE    pbtrnode,
                                      TPS_IBLK         blkPtr,
                                      BOOL             bWriteHead,
                                      UINT             uiItemStart,
                                      UINT             uiItemCnt)
{
    PUCHAR              pucBuff;
    UINT                uiOff;
    UINT                uiLen;
    UINT                uiOffStart;
    UINT                uiOffEnd;
    PTPS_SUPER_BLOCK    psb = pinode->IND_psb;

    if (blkPtr == pinode->IND_inum) {
        uiOff = TPS_INODE_DATASTART;
        uiLen = psb->SB_uiBlkSize - TPS_INODE_DATASTART;
    
    } else {
        uiOff = 0;
        uiLen = psb->SB_uiBlkSize;
    }

    pucBuff = pinode->IND_pucBuff;

    tpsSerialBtrNode(pbtrnode, pucBuff, uiLen,
                     uiItemStart, uiItemCnt,
                     &uiOffStart, &uiOffEnd);                           /* 序列化                       */

    /*
     *  计算实际需要写入的数据起始和长度
     */
    if ((uiOffStart & psb->SB_uiSectorMask) != 0) {
        uiOffStart = (uiOffStart >> psb->SB_uiSectorShift) << psb->SB_uiSectorShift;
    }

    if ((uiOffEnd & psb->SB_uiSectorMask) != 0) {
        uiOffEnd = ((uiOffEnd >> psb->SB_uiSectorShift) << psb->SB_uiSectorShift) + psb->SB_uiSectorSize;
    }

    if (bWriteHead) {
        if (uiOffStart <= psb->SB_uiSectorSize) {                   /* 与节点头相邻则合并成一次写   */
            uiOffStart = 0;
        } else {
            if (tpsFsTransWrite(ptrans, psb, blkPtr, uiOff,
                                pucBuff, psb->SB_uiSectorSize) != TPS_ERR_NONE) {
                return  (TPS_ERR_BUF_READ);
            }
        }
    }

    uiOff += uiOffStart;
    uiLen  = uiOffEnd - uiOffStart;

    if (uiLen > 0) {
        if (tpsFsTransWrite(ptrans, psb, blkPtr, uiOff, pucBuff + uiOffStart, uiLen) != TPS_ERR_NONE) {
            return  (TPS_ERR_BUF_READ);
        }
    }

    if (blkPtr == pinode->IND_inum) {                                   /* root节点需根系inode缓存      */
        lib_memcpy(&pinode->IND_data.IND_btrNode,
                   pbtrnode,
                   sizeof(TPS_BTR_NODE) + (sizeof(TPS_BTR_KV) * pbtrnode->ND_uiEntrys));
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsUpdateKey
** 功能描述: 更新节点键值
**           ptrans             事物
**           pinode             文件inode指针
**           pbtrnode           节点指针
**           blkKeyOld          老键值
**           blkKeyNew          新键值
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsUpdateKey (PTPS_TRANS     ptrans,
                                     PTPS_INODE     pinode,
                                     PTPS_BTR_NODE  pbtrnode,
                                     TPS_IBLK       blkKeyOld,
                                     TPS_IBLK       blkKeyNew)
{
    PTPS_BTR_NODE       pndParent = LW_NULL;
    PTPS_SUPER_BLOCK    psb       = pinode->IND_psb;
    INT                 iParent   = 0;
    BOOL                bEqual;

    pndParent = __tpsFsAllocBtrNode(pinode, psb->SB_uiBlkSize, TPS_BTR_NODE_UNKOWN);
    if (LW_NULL == pndParent) {
        return  (TPS_ERR_ALLOC);
    }

    while (pbtrnode->ND_blkParent != 0) {                               /* 递归更新父节点键值           */
        if (__tpsFsBtrGetNode(pinode, pndParent,
                              pbtrnode->ND_blkParent) != TPS_ERR_NONE) {
            __tpsFsFreeBtrNode(pinode, pndParent);
            return  (TPS_ERR_BTREE_GET_NODE);
        }

        iParent =  __tpsFsBtrSearch(pndParent, blkKeyOld, LW_FALSE, &bEqual);
        if (!bEqual) {
            __tpsFsFreeBtrNode(pinode, pndParent);
            return  (TPS_ERR_BTREE_KEY_AREADY_EXIT);
        }

        pndParent->ND_kvArr[iParent].KV_blkKey = blkKeyNew;

        if (__tpsFsBtrPutNode(ptrans, pinode,
                              pndParent,
                              pndParent->ND_blkThis,
                              LW_FALSE, iParent, 1) != TPS_ERR_NONE) {
            __tpsFsFreeBtrNode(pinode, pndParent);
            return  (TPS_ERR_BTREE_PUT_NODE);
        }

        /*
         *  如果被更新的键值为首个子节点对应的键，则需更新父节点
         */
        if (iParent > 0) {
            break;
        
        } else {
            pbtrnode = pndParent;
        }
    }

    __tpsFsFreeBtrNode(pinode, pndParent);

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsBtreeInsertNode
** 功能描述: 插入键到节点
**           ptrans             事物
**           pinode             文件inode指针
**           pbtrnode           节点指针
**           btrkv              待插入键-值对
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsBtreeInsertNode (PTPS_TRANS       ptrans,
                                           PTPS_INODE       pinode,
                                           PTPS_BTR_NODE    pbtrnode,
                                           TPS_BTR_KV       btrkv)
{
    UINT                iSplit = 0;
    INT                 iInsert;
    INT                 i;
    INT                 j;
    BOOL                bEqual;
    PTPS_BTR_NODE       pndSplit    = LW_NULL;
    PTPS_BTR_NODE       pndSubNode  = LW_NULL;
    PTPS_SUPER_BLOCK    psb         = pinode->IND_psb;
    TPS_IBLK            blkAlloc;

    iInsert = __tpsFsBtrSearch(pbtrnode, btrkv.KV_blkKey, LW_TRUE, &bEqual);
    if (bEqual) {
        return  (TPS_ERR_BTREE_KEY_AREADY_EXIT);
    }

    if (pbtrnode->ND_uiEntrys == pbtrnode->ND_uiMaxCnt) {               /* 节点已满，分裂节点           */
        iSplit = pbtrnode->ND_uiEntrys / 2;
        pndSplit = __tpsFsAllocBtrNode(pinode, psb->SB_uiBlkSize, pbtrnode->ND_iType);
        if (LW_NULL == pndSplit) {
            return  (TPS_ERR_ALLOC);
        }

        pndSplit->ND_iType      = pbtrnode->ND_iType;
        pndSplit->ND_uiLevel    = pbtrnode->ND_uiLevel;
        pbtrnode->ND_uiEntrys   = iSplit;

        if (iInsert < iSplit) {                                         /* 插入前节点                   */
            for (i = iSplit, j = 0; i < pbtrnode->ND_uiMaxCnt; i++, j++) {
                pndSplit->ND_kvArr[j] = pbtrnode->ND_kvArr[i];
            }

            pndSplit->ND_uiEntrys = j;
            for (i = pbtrnode->ND_uiEntrys; i > iInsert; i--) {
                pbtrnode->ND_kvArr[i] = pbtrnode->ND_kvArr[i - 1];
            }
            pbtrnode->ND_kvArr[i] = btrkv;
            pbtrnode->ND_uiEntrys++;
        
        } else {                                                        /* 插入后节点                   */
            i = iSplit, j = 0;
            while (i < pbtrnode->ND_uiMaxCnt) {
                if (j != iInsert - iSplit) {
                    pndSplit->ND_kvArr[j] = pbtrnode->ND_kvArr[i];
                    i++;
                }
                j++;
            }
            if (j > iInsert - iSplit) {
                pndSplit->ND_uiEntrys = j;

            } else {
                pndSplit->ND_uiEntrys = pbtrnode->ND_uiMaxCnt - iSplit + 1;
            }
            j = iInsert - iSplit;
            pndSplit->ND_kvArr[j] = btrkv;
        }
    } else {                                                            /* 节点未满，不需要分裂         */
        for (i = pbtrnode->ND_uiEntrys; i > iInsert; i--) {
            pbtrnode->ND_kvArr[i] = pbtrnode->ND_kvArr[i - 1];
        }
        pbtrnode->ND_kvArr[i] = btrkv;
        pbtrnode->ND_uiEntrys++;
    }

    if ((iInsert == 0) && (pbtrnode->ND_blkParent != 0)) {              /* 如果为首元素则更改父节点key  */
        if (__tpsFsUpdateKey(ptrans, pinode, pbtrnode, 
                             pbtrnode->ND_kvArr[1].KV_blkKey, 
                             pbtrnode->ND_kvArr[0].KV_blkKey) != TPS_ERR_NONE) {
            if (pndSplit) {
                __tpsFsFreeBtrNode(pinode, pndSplit);
            }
            return  (TPS_ERR_BTREE_UPDATE_KEY);
        }
    }

    if (iSplit) {                                                       /* 分裂节点                     */
        blkAlloc = __tpsFsBtrAllocNodeBlk(ptrans, psb, pinode);         /* 分配块保存新节点             */
        if (blkAlloc <= 0) {
            __tpsFsFreeBtrNode(pinode, pndSplit);
            return  (TPS_ERR_BP_ALLOC);
        }

        pndSubNode = __tpsFsAllocBtrNode(pinode, psb->SB_uiBlkSize, TPS_BTR_NODE_UNKOWN);
        if (LW_NULL == pndSubNode) {
            __tpsFsFreeBtrNode(pinode, pndSplit);
            return  (TPS_ERR_ALLOC);
        }

        pndSplit->ND_blkThis    = blkAlloc;
        pndSplit->ND_blkNext    = pbtrnode->ND_blkNext;
        pbtrnode->ND_blkNext    = pndSplit->ND_blkThis;

        for (i = 0;
             (i < (pndSplit->ND_uiEntrys - 1)) && (pndSplit->ND_iType != TPS_BTR_NODE_LEAF);
             i++) {                                                     /* 修改子节点父亲属性           */
            if (__tpsFsBtrGetNode(pinode, pndSubNode,
                                  pndSplit->ND_kvArr[i].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, pndSubNode);
                __tpsFsFreeBtrNode(pinode, pndSplit);
                return  (TPS_ERR_BTREE_GET_NODE);
            }

            pndSubNode->ND_blkParent = pndSplit->ND_blkThis;

            if (__tpsFsBtrPutNode(ptrans, pinode,
                                  pndSubNode,
                                  pndSubNode->ND_blkThis,
                                  LW_TRUE, 0, 0) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, pndSubNode);
                __tpsFsFreeBtrNode(pinode, pndSplit);
                return  (TPS_ERR_BTREE_PUT_NODE);
            }
        }

        /*
         *  由于节点分裂，需在父节点中插入新子节点
         */
        if (pbtrnode->ND_blkParent == 0) {                              /* 节点是根节点，另外申请块保存 */
            blkAlloc = __tpsFsBtrAllocNodeBlk(ptrans, psb, pinode);     /* 分配块保存新节点             */
            if (blkAlloc <= 0) {
                __tpsFsFreeBtrNode(pinode, pndSubNode);
                __tpsFsFreeBtrNode(pinode, pndSplit);
                return  (TPS_ERR_BP_ALLOC);
            }

            /*
             *  设置当前节点变为非根节点，重新设置节点属性
             */
            pbtrnode->ND_blkParent = pbtrnode->ND_blkThis;
            pbtrnode->ND_blkThis = blkAlloc;
            if (pbtrnode->ND_iType == TPS_BTR_NODE_LEAF) {
                pbtrnode->ND_uiMaxCnt = (psb->SB_uiBlkSize - sizeof(TPS_BTR_NODE)) / sizeof(TPS_BTR_KV);

            } else {
                pbtrnode->ND_uiMaxCnt = (psb->SB_uiBlkSize - sizeof(TPS_BTR_NODE)) / (sizeof(TPS_IBLK) * 2);
            }

            for (i = 0;
                 (i < (pbtrnode->ND_uiEntrys - 1)) && (pbtrnode->ND_iType != TPS_BTR_NODE_LEAF);
                 i++) {                                                 /* 修改子节点父亲属性           */
                if (__tpsFsBtrGetNode(pinode, pndSubNode,
                                      pndSplit->ND_kvArr[i].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                    __tpsFsFreeBtrNode(pinode, pndSubNode);
                    __tpsFsFreeBtrNode(pinode, pndSplit);
                    return  (TPS_ERR_BTREE_GET_NODE);
                }

                pndSubNode->ND_blkParent = pbtrnode->ND_blkThis;

                if (__tpsFsBtrPutNode(ptrans, pinode,
                                      pndSubNode,
                                      pndSubNode->ND_blkThis,
                                      LW_TRUE, 0, 0) != TPS_ERR_NONE) {
                    __tpsFsFreeBtrNode(pinode, pndSubNode);
                    __tpsFsFreeBtrNode(pinode, pndSplit);
                    return  (TPS_ERR_BTREE_PUT_NODE);
                }
            }

            if (__tpsFsBtrPutNode(ptrans, pinode,
                                  pbtrnode, pbtrnode->ND_blkThis,
                                  LW_TRUE, 0, pbtrnode->ND_uiEntrys) != TPS_ERR_NONE) {
                                                                        /* 保存当前节点到新块           */
                __tpsFsFreeBtrNode(pinode, pndSubNode);
                __tpsFsFreeBtrNode(pinode, pndSplit);
                return  (TPS_ERR_BTREE_PUT_NODE);
            }

            pndSplit->ND_blkParent = pbtrnode->ND_blkParent;            /* 设置分裂出的节点父亲属性     */
            if (__tpsFsBtrPutNode(ptrans, pinode,
                                  pndSplit, pndSplit->ND_blkThis,
                                  LW_TRUE, 0, pndSplit->ND_uiEntrys) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, pndSubNode);
                __tpsFsFreeBtrNode(pinode, pndSplit);
                return  (TPS_ERR_BTREE_PUT_NODE);
            }

            /*
             *  将原节点设置为新的root节点
             */
            pinode->IND_data.IND_btrNode.ND_iType       = TPS_BTR_NODE_NON_LEAF;
            pinode->IND_data.IND_btrNode.ND_uiLevel++;
            pinode->IND_data.IND_btrNode.ND_uiEntrys    = 2;
            pinode->IND_data.IND_btrNode.ND_uiMaxCnt    = (psb->SB_uiBlkSize - TPS_INODE_DATASTART -
                                                          sizeof(TPS_BTR_NODE)) / (sizeof(TPS_IBLK) * 2);

            pinode->IND_data.IND_btrNode.ND_kvArr[0].KV_blkKey          = pbtrnode->ND_kvArr[0].KV_blkKey;
            pinode->IND_data.IND_btrNode.ND_kvArr[0].KV_data.KP_blkPtr  = pbtrnode->ND_blkThis;
            pinode->IND_data.IND_btrNode.ND_kvArr[1].KV_blkKey          = pndSplit->ND_kvArr[0].KV_blkKey;
            pinode->IND_data.IND_btrNode.ND_kvArr[1].KV_data.KP_blkPtr  = pndSplit->ND_blkThis;
            pinode->IND_data.IND_btrNode.ND_blkThis                     = pinode->IND_inum;

            if (__tpsFsBtrPutNode(ptrans, pinode,
                                  &pinode->IND_data.IND_btrNode,
                                  pinode->IND_inum,
                                  LW_TRUE, 0,
                                  pinode->IND_data.IND_btrNode.ND_uiEntrys) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, pndSubNode);
                __tpsFsFreeBtrNode(pinode, pndSplit);
                return  (TPS_ERR_BTREE_PUT_NODE);
            }

            __tpsFsFreeBtrNode(pinode, pndSubNode);
            __tpsFsFreeBtrNode(pinode, pndSplit);

        } else {                                                        /* 非root节点，递归插入新节点   */
            /*
             *  递归插入前先保存当前节点和新分裂的节点到磁盘块
             */
            if (__tpsFsBtrPutNode(ptrans, pinode,
                                  pbtrnode, pbtrnode->ND_blkThis,
                                  LW_TRUE, 0, pbtrnode->ND_uiEntrys) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, pndSubNode);
                __tpsFsFreeBtrNode(pinode, pndSplit);
                return  (TPS_ERR_BTREE_PUT_NODE);
            }

            pndSplit->ND_blkParent = pbtrnode->ND_blkParent;
            if (__tpsFsBtrPutNode(ptrans, pinode,
                                  pndSplit, pndSplit->ND_blkThis,
                                  LW_TRUE, 0, pndSplit->ND_uiEntrys) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, pndSubNode);
                __tpsFsFreeBtrNode(pinode, pndSplit);
                return  (TPS_ERR_BTREE_PUT_NODE);
            }

            if (__tpsFsBtrGetNode(pinode, pbtrnode,
                                  pndSplit->ND_blkParent) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, pndSubNode);
                __tpsFsFreeBtrNode(pinode, pndSplit);
                return  (TPS_ERR_BTREE_GET_NODE);
            }

            btrkv.KV_blkKey         = pndSplit->ND_kvArr[0].KV_blkKey;
            btrkv.KV_data.KP_blkPtr = pndSplit->ND_blkThis;

            __tpsFsFreeBtrNode(pinode, pndSubNode);
            __tpsFsFreeBtrNode(pinode, pndSplit);

            /*
             *  递归插入
             */
            return  (__tpsFsBtreeInsertNode(ptrans, pinode, pbtrnode, btrkv));
        }
    } else {                                                            /* 节点未满直接插入             */
        if (__tpsFsBtrPutNode(ptrans, pinode,
                              pbtrnode, pbtrnode->ND_blkThis,
                              LW_TRUE, iInsert,
                              (pbtrnode->ND_uiEntrys - iInsert)) != TPS_ERR_NONE) {
            return  (TPS_ERR_BTREE_PUT_NODE);
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsBtreeInsert
** 功能描述: 插入键到b+tree，用于磁盘块释放
**           ptrans             事物
**           pinode             文件inode指针
**           kvInsert           待插入键-值对
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsBtreeInsert (PTPS_TRANS ptrans, PTPS_INODE pinode, TPS_BTR_KV kvInsert)
{
    PTPS_BTR_NODE       pbtrnode;
    PTPS_SUPER_BLOCK    psb;
    INT                 i;
    BOOL                bBreak = LW_FALSE;

    psb = pinode->IND_psb;

    pbtrnode = __tpsFsAllocBtrNode(pinode, psb->SB_uiBlkSize, TPS_BTR_NODE_UNKOWN);
    if (LW_NULL == pbtrnode) {
        return  (TPS_ERR_ALLOC);
    }
    lib_memcpy(pbtrnode,
               &pinode->IND_data.IND_btrNode,
               sizeof(TPS_BTR_NODE) + (sizeof(TPS_BTR_KV) * pinode->IND_data.IND_btrNode.ND_uiEntrys));
																		/* 从inode缓存拷贝根节点        */
    while (!bBreak) {
        switch (pbtrnode->ND_iType) {
        
        case TPS_BTR_NODE_NON_LEAF:                                     /* 对于非叶子节点递归查找       */
            i = __tpsFsBtrSearch(pbtrnode, kvInsert.KV_blkKey, LW_FALSE, LW_NULL);
            if (__tpsFsBtrGetNode(pinode, pbtrnode,
                                  pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, pbtrnode);
                return  (TPS_ERR_BTREE_GET_NODE);
            }
            break;

        case TPS_BTR_NODE_LEAF:
            if ( __tpsFsBtreeInsertNode(ptrans, pinode, pbtrnode, kvInsert) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, pbtrnode);
                return  (TPS_ERR_BTREE_INSERT_NODE);
            }
            bBreak = LW_TRUE;
            break;

        default:
            __tpsFsFreeBtrNode(pinode, pbtrnode);
            return  (TPS_ERR_BTREE_NODE_TYPE);
        }
    }

    __tpsFsFreeBtrNode(pinode, pbtrnode);

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsBtreeRemoveNode
** 功能描述: 从节点删除键
**           ptrans             事物
**           pinode             文件inode指针
**           pbtrnode           节点指针
**           iRemove            待删除键下标
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsBtreeRemoveNode (PTPS_TRANS      ptrans,
                                           PTPS_INODE      pinode,
                                           PTPS_BTR_NODE   pbtrnode,
                                           INT             iRemove)
{
    TPS_RESULT          tpsresRet   = TPS_ERR_NONE;
    INT                 i;
    INT                 j;
    INT                 k;
    PTPS_BTR_NODE       pndSibling  = LW_NULL;
    PTPS_BTR_NODE       pndSubNode  = LW_NULL;
    PTPS_BTR_NODE       pndParent   = LW_NULL;
    PTPS_SUPER_BLOCK    psb         = pinode->IND_psb;
    BOOL                bEqual;
    UINT                uiMaxCnt    = pbtrnode->ND_uiMaxCnt;
    TPS_IBLK            blkKey      = 0;

    if (pbtrnode->ND_blkParent == pinode->IND_inum &&
        pinode->IND_data.IND_btrNode.ND_uiEntrys == 2) {                /* root的大小需要特殊处理       */
        uiMaxCnt = pbtrnode->ND_iType == TPS_BTR_NODE_NON_LEAF ?
                   (psb->SB_uiBlkSize - TPS_INODE_DATASTART - sizeof(TPS_BTR_NODE)) / (sizeof(TPS_IBLK) * 2) : 
                   (psb->SB_uiBlkSize - TPS_INODE_DATASTART - sizeof(TPS_BTR_NODE)) / (sizeof(TPS_BTR_KV));
    }

    if (pbtrnode->ND_uiEntrys <= (uiMaxCnt / 2)) {                      /* 需要合并节点                 */
        if (pbtrnode->ND_blkParent != 0) {
            pndParent = __tpsFsAllocBtrNode(pinode, psb->SB_uiBlkSize, TPS_BTR_NODE_UNKOWN);
            if (LW_NULL == pndParent) {
                return  (TPS_ERR_ALLOC);
            }

            pndSibling = __tpsFsAllocBtrNode(pinode, psb->SB_uiBlkSize, pbtrnode->ND_iType);
            if (LW_NULL == pndParent) {
                __tpsFsFreeBtrNode(pinode, pndParent);
                return  (TPS_ERR_ALLOC);
            }

            pndSubNode = __tpsFsAllocBtrNode(pinode, psb->SB_uiBlkSize, TPS_BTR_NODE_UNKOWN);
            if (LW_NULL == pndSubNode) {
                __tpsFsFreeBtrNode(pinode, pndSibling);
                __tpsFsFreeBtrNode(pinode, pndParent);
                return  (TPS_ERR_ALLOC);
            }

            if (__tpsFsBtrGetNode(pinode, pndParent, pbtrnode->ND_blkParent) != TPS_ERR_NONE) {
                tpsresRet = TPS_ERR_BTREE_GET_NODE;
                goto    error_out;
            }

            i = __tpsFsBtrSearch(pndParent, pbtrnode->ND_kvArr[0].KV_blkKey, LW_FALSE, &bEqual);
            if (i == pndParent->ND_uiEntrys - 1) {                      /* 向左合并                     */
                if (__tpsFsBtrGetNode(pinode, pndSibling,
                                      pndParent->ND_kvArr[i - 1].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                    tpsresRet = TPS_ERR_BTREE_GET_NODE;
                    goto    error_out;
                }

                if (pndSibling->ND_uiEntrys > (uiMaxCnt / 2)) {         /* 从左边借子节点               */
                    for (j = iRemove; j > 0; j--) {
                        pbtrnode->ND_kvArr[j] = pbtrnode->ND_kvArr[j - 1];
                    }
                    pbtrnode->ND_kvArr[0] = pndSibling->ND_kvArr[pndSibling->ND_uiEntrys - 1];
                    pndParent->ND_kvArr[i].KV_blkKey = pbtrnode->ND_kvArr[0].KV_blkKey;
                    pndSibling->ND_uiEntrys--;

                    if (pbtrnode->ND_iType != TPS_BTR_NODE_LEAF) {      /* 更改被借子节点父亲属性       */
                        if (__tpsFsBtrGetNode(pinode, pndSubNode,
                                              pbtrnode->ND_kvArr[0].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                            tpsresRet = TPS_ERR_BTREE_GET_NODE;
                            goto    error_out;
                        }

                        pndSubNode->ND_blkParent = pbtrnode->ND_blkThis;

                        if (__tpsFsBtrPutNode(ptrans, pinode,
                                              pndSubNode, pndSubNode->ND_blkThis,
                                              LW_TRUE, 0, 0) != TPS_ERR_NONE) {
                            tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                            goto    error_out;
                        }
                    }

                    if (__tpsFsBtrPutNode(ptrans, pinode,
                                          pbtrnode, pbtrnode->ND_blkThis,
                                          LW_FALSE, 0, iRemove + 1) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                        goto    error_out;
                    }

                    if (__tpsFsBtrPutNode(ptrans, pinode,
                                          pndSibling, pndSibling->ND_blkThis,
                                          LW_TRUE, 0, 0) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                        goto    error_out;
                    }

                    if (__tpsFsBtrPutNode(ptrans, pinode,
                                          pndParent, pndParent->ND_blkThis,
                                          LW_FALSE, i, 1) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                        goto    error_out;
                    }

                } else {                                                /* 与左边节点合并               */
                    for (j = pndSibling->ND_uiEntrys, k = 0; k < pbtrnode->ND_uiEntrys; k++) {
                        if (k != iRemove) {
                            pndSibling->ND_kvArr[j] = pbtrnode->ND_kvArr[k];

                            if (pbtrnode->ND_iType != TPS_BTR_NODE_LEAF) {
                                                                        /* 更改被被合并子节点父亲属性   */
                                if (__tpsFsBtrGetNode(pinode, pndSubNode,
                                        pndSibling->ND_kvArr[j].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                                    tpsresRet = TPS_ERR_BTREE_GET_NODE;
                                    goto    error_out;
                                }

                                pndSubNode->ND_blkParent = pndSibling->ND_blkThis;

                                if (__tpsFsBtrPutNode(ptrans, pinode,
                                                      pndSubNode,
                                                      pndSubNode->ND_blkThis,
                                                      LW_TRUE, 0, 0) != TPS_ERR_NONE) {
                                    tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                                    goto    error_out;
                                }
                            }

                            j++;
                        }
                    }

                    /*
                     * 释放被合并节点
                     */
                    pndSibling->ND_uiEntrys = j;
                    pndSibling->ND_blkNext = pbtrnode->ND_blkNext;
                    if (__tpsFsBtrFreeNodeBlk(ptrans,
                                              psb,
                                              pinode,
                                              pbtrnode->ND_blkThis) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BP_FREE;
                        goto    error_out;
                    }

                    if (__tpsFsBtrPutNode(ptrans, pinode,
                                          pndSibling, pndSibling->ND_blkThis,
                                          LW_TRUE, 0, pndSibling->ND_uiEntrys) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                        goto    error_out;
                    }

                    tpsresRet = __tpsFsBtreeRemoveNode(ptrans, pinode, pndParent, i);
                    if (tpsresRet != TPS_ERR_NONE) {
                        goto    error_out;
                    }
                }
            
            } else {                                                    /* 向右合并                     */
                if (__tpsFsBtrGetNode(pinode, pndSibling,
                                      pndParent->ND_kvArr[i + 1].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                    __tpsFsFreeBtrNode(pinode, pndSibling);
                    __tpsFsFreeBtrNode(pinode, pndParent);
                    return  (TPS_ERR_BTREE_GET_NODE);
                }

                for (; iRemove < pbtrnode->ND_uiEntrys - 1; iRemove++) {/* 当前节点去空位               */
                    pbtrnode->ND_kvArr[iRemove] = pbtrnode->ND_kvArr[iRemove + 1];
                }
                pbtrnode->ND_uiEntrys--;

                if (pndSibling->ND_uiEntrys > (uiMaxCnt / 2)) {         /* 从右边借节点                 */
                    pbtrnode->ND_kvArr[pbtrnode->ND_uiEntrys - 1] = pndSibling->ND_kvArr[0];
                    pbtrnode->ND_uiEntrys++;

                    if (pbtrnode->ND_iType != TPS_BTR_NODE_LEAF) {      /* 更改被借子节点父亲属性       */
                        if (__tpsFsBtrGetNode(pinode, pndSubNode,
                                pbtrnode->ND_kvArr[pbtrnode->ND_uiEntrys - 1].
                                KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                            tpsresRet = TPS_ERR_BTREE_GET_NODE;
                            goto    error_out;
                        }

                        pndSubNode->ND_blkParent = pbtrnode->ND_blkThis;

                        if (__tpsFsBtrPutNode(ptrans, pinode,
                                              pndSubNode,
                                              pndSubNode->ND_blkThis,
                                              LW_TRUE, 0, 0) != TPS_ERR_NONE) {
                            tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                            goto    error_out;
                        }
                    }

                    for (j = 0; j < pndSibling->ND_uiEntrys - 1; j++) { /* 整理右边节点，去空位         */
                        pndSibling->ND_kvArr[j] = pndSibling->ND_kvArr[j + 1];
                    }
                    pbtrnode->ND_uiEntrys--;
                    pndParent->ND_kvArr[i + 1].KV_blkKey = pndSibling->ND_kvArr[0].KV_blkKey;

                    if (__tpsFsBtrPutNode(ptrans, pinode,
                                          pbtrnode, pbtrnode->ND_blkThis,
                                          LW_FALSE, iRemove,
                                          (pbtrnode->ND_uiEntrys - iRemove)) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                        goto    error_out;
                    }

                    if (__tpsFsBtrPutNode(ptrans, pinode,
                                          pndSibling, pndSibling->ND_blkThis,
                                          LW_TRUE, 0, pndSibling->ND_uiEntrys) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                        goto    error_out;
                    }

                    if (__tpsFsBtrPutNode(ptrans, pinode,
                                          pndParent, pndParent->ND_blkThis,
                                          LW_FALSE, i + 1, 1) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                        goto    error_out;
                    }

                } else {                                                /* 与右边节点合并               */
                    for (j = pbtrnode->ND_uiEntrys - 1, k = 0;
                         k < pndSibling->ND_uiEntrys;
                         j++, k++) {
                        pbtrnode->ND_kvArr[j] = pndSibling->ND_kvArr[k];

                        if (pbtrnode->ND_iType != TPS_BTR_NODE_LEAF) {  /* 修改被合并子节点父亲属性     */
                            if (__tpsFsBtrGetNode(pinode, pndSubNode,
                                    pbtrnode->ND_kvArr[j].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                                tpsresRet = TPS_ERR_BTREE_GET_NODE;
                                goto    error_out;
                            }

                            pndSubNode->ND_blkParent = pbtrnode->ND_blkThis;

                            if (__tpsFsBtrPutNode(ptrans, pinode,
                                                  pndSubNode,
                                                  pndSubNode->ND_blkThis,
                                                  LW_TRUE, 0, 0) != TPS_ERR_NONE) {
                                tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                                goto    error_out;
                            }
                        }
                    }

                    /*
                     * 删除被合并节点
                     */
                    pbtrnode->ND_uiEntrys = j;
                    pbtrnode->ND_blkNext = pndSibling->ND_blkNext;
                    if (__tpsFsBtrFreeNodeBlk(ptrans,
                                              psb,
                                              pinode,
                                              pndSibling->ND_blkThis) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BP_FREE;
                        goto    error_out;
                    }

                    if (__tpsFsBtrPutNode(ptrans, pinode,
                                          pbtrnode, pbtrnode->ND_blkThis,
                                          LW_TRUE, 0, pbtrnode->ND_uiEntrys) != TPS_ERR_NONE) {
                        tpsresRet = TPS_ERR_BTREE_PUT_NODE;
                        goto    error_out;
                    }

                    tpsresRet = __tpsFsBtreeRemoveNode(ptrans, pinode, pndParent, i + 1);
                    if (tpsresRet != TPS_ERR_NONE) {
                        goto    error_out;
                    }
                }
            }

            __tpsFsFreeBtrNode(pinode, pndSubNode);
            __tpsFsFreeBtrNode(pinode, pndSibling);
            __tpsFsFreeBtrNode(pinode, pndParent);

            return  (TPS_ERR_NONE);

        } else {
            /*
             * root节点放在后面处理
             */
        }
    }

    /*
     *  不需要合并节点
     */
    blkKey = pbtrnode->ND_kvArr[0].KV_blkKey;
    for (i = iRemove; i < pbtrnode->ND_uiEntrys - 1; i++) {
        pbtrnode->ND_kvArr[i] = pbtrnode->ND_kvArr[i + 1];
    }
    pbtrnode->ND_uiEntrys--;

    if ((iRemove == 0) && (pbtrnode->ND_blkParent != 0)) {              /* 如果为首元素则更改父节点key  */
        if (__tpsFsUpdateKey(ptrans, pinode, pbtrnode, 
                             blkKey, 
                             pbtrnode->ND_kvArr[0].KV_blkKey) != TPS_ERR_NONE) {
            return  (TPS_ERR_BTREE_UPDATE_KEY);
        }
    }

    /*
     *  root节点,当子节点数为1时，需要将子节点设置为root节点，释放当前root节点
     */
    if ((pbtrnode->ND_uiEntrys <= 1) && (pbtrnode->ND_uiLevel > 0)) {
        pndSubNode = __tpsFsAllocBtrNode(pinode, psb->SB_uiBlkSize, TPS_BTR_NODE_UNKOWN);
        if (LW_NULL == pndSubNode) {
            return  (TPS_ERR_ALLOC);
        }

        if (__tpsFsBtrGetNode(pinode, pndSubNode,
                              pbtrnode->ND_kvArr[0].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
            __tpsFsFreeBtrNode(pinode, pndSubNode);
            return  (TPS_ERR_BTREE_GET_NODE);
        }

        if (__tpsFsBtrFreeNodeBlk(ptrans,
                                  psb,
                                  pinode,
                                  pndSubNode->ND_blkThis) != TPS_ERR_NONE) {
                                                                        /* 释放当前root节点             */
            __tpsFsFreeBtrNode(pinode, pndSubNode);
            return  (TPS_ERR_BP_FREE);
        }

        /*
         * 重新设置子节点属性
         */
        pndSubNode->ND_blkParent = 0;
        pndSubNode->ND_blkThis   = pinode->IND_inum;
        pndSubNode->ND_uiMaxCnt  = pndSubNode->ND_iType == TPS_BTR_NODE_NON_LEAF ?
                                   (psb->SB_uiBlkSize - TPS_INODE_DATASTART - sizeof(TPS_BTR_NODE)) / (sizeof(TPS_IBLK) * 2) : 
                                   (psb->SB_uiBlkSize - TPS_INODE_DATASTART - sizeof(TPS_BTR_NODE)) / (sizeof(TPS_BTR_KV));

        lib_memcpy(pbtrnode, pndSubNode,
                   sizeof(TPS_BTR_NODE) + (sizeof(TPS_BTR_KV) * pndSubNode->ND_uiEntrys));            
		                                                                /* 拷贝子节点内容到父节点       */

        for (i = 0;
             (i < (pbtrnode->ND_uiEntrys - 1)) && (pbtrnode->ND_iType != TPS_BTR_NODE_LEAF);
             i++) {                                                     /* 修改子节点父亲属性           */
            if (__tpsFsBtrGetNode(pinode, pndSubNode,
                pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, pndSubNode);
                return  (TPS_ERR_BTREE_GET_NODE);
            }

            pndSubNode->ND_blkParent = pbtrnode->ND_blkThis;

            if (__tpsFsBtrPutNode(ptrans, pinode,
                                  pndSubNode,
                                  pndSubNode->ND_blkThis,
                                  LW_TRUE, 0, 0) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, pndSubNode);
                return  (TPS_ERR_BTREE_PUT_NODE);
            }
        }

        __tpsFsFreeBtrNode(pinode, pndSubNode);

        if (__tpsFsBtrPutNode(ptrans, pinode,
                              pbtrnode, pbtrnode->ND_blkThis,
                              LW_TRUE, 0, pbtrnode->ND_uiEntrys) != TPS_ERR_NONE) {
            return  (TPS_ERR_BTREE_PUT_NODE);
        }

    } else {
        if (__tpsFsBtrPutNode(ptrans, pinode,
                              pbtrnode, pbtrnode->ND_blkThis,
                              LW_TRUE, iRemove,
                              (pbtrnode->ND_uiEntrys - iRemove)) != TPS_ERR_NONE) {
            return  (TPS_ERR_BTREE_PUT_NODE);
        }
    }

    return  (TPS_ERR_NONE);

error_out:
    if (pndSubNode) {
        __tpsFsFreeBtrNode(pinode, pndSubNode);
    }
    if (pndSibling) {
        __tpsFsFreeBtrNode(pinode, pndSibling);
    }
    if (pndParent) {
        __tpsFsFreeBtrNode(pinode, pndParent);
    }

    return  (tpsresRet);
}
/*********************************************************************************************************
** 函数名称: __tpsFsBtreeRemove
** 功能描述: 从b+tree删除键，用于分配磁盘块
**           ptrans             事物
**           pinode             文件inode指针
**           kvRemove           待删除键-值对
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsBtreeRemove (PTPS_TRANS ptrans, PTPS_INODE pinode, TPS_BTR_KV kvRemove)
{
    PTPS_BTR_NODE       pbtrnode;
    PTPS_SUPER_BLOCK    psb;
    INT                 i;
    BOOL                bEqual;
    BOOL                bBreak  = LW_FALSE;

    psb = pinode->IND_psb;

    pbtrnode = __tpsFsAllocBtrNode(pinode, psb->SB_uiBlkSize, TPS_BTR_NODE_UNKOWN);
    if (LW_NULL == pbtrnode) {
        return  (TPS_ERR_ALLOC);
    }
    lib_memcpy(pbtrnode,
               &pinode->IND_data.IND_btrNode,
               sizeof(TPS_BTR_NODE) + (sizeof(TPS_BTR_KV) * pinode->IND_data.IND_btrNode.ND_uiEntrys));                
	                                                                   /* 从inode缓存拷贝根节点        */

    while (!bBreak) {
        i = __tpsFsBtrSearch(pbtrnode, kvRemove.KV_blkKey, LW_FALSE, &bEqual);
        switch (pbtrnode->ND_iType) {
        
        case TPS_BTR_NODE_NON_LEAF:                                     /* 对于非叶子节点递归查找       */
            if (__tpsFsBtrGetNode(pinode, pbtrnode,
                                  pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, pbtrnode);
                return  (TPS_ERR_BTREE_GET_NODE);
            }
            break;

        case TPS_BTR_NODE_LEAF:
            if (!bEqual) {
                __tpsFsFreeBtrNode(pinode, pbtrnode);
                return  (TPS_ERR_BTREE_NODE_NOT_EXIST);
            }

            if ( __tpsFsBtreeRemoveNode(ptrans, pinode, pbtrnode, i) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, pbtrnode);
                return  (TPS_ERR_BTREE_INSERT_NODE);
            }

            bBreak = LW_TRUE;
            break;

        default:
            __tpsFsFreeBtrNode(pinode, pbtrnode);
            return  (TPS_ERR_BTREE_NODE_TYPE);
        }
    }

    __tpsFsFreeBtrNode(pinode, pbtrnode);

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsBtreeGet
** 功能描述: 根据键值查找附近的节点
**           pinode             文件inode指针
**           blkKey             键值
**           pkv                返回查找结果
**           bAfter             是否要查找键值后面的键
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsBtreeGet (PTPS_INODE pinode, TPS_IBLK blkKey, PTPS_BTR_KV pkv, BOOL bAfter)
{
    PTPS_BTR_NODE       pbtrnode;
    PTPS_SUPER_BLOCK    psb;
    INT                 i;
    BOOL                bEqual;
    BOOL                bBreak  = LW_FALSE;

    psb = pinode->IND_psb;

    pbtrnode = __tpsFsAllocBtrNode(pinode, psb->SB_uiBlkSize, TPS_BTR_NODE_UNKOWN);
    if (LW_NULL == pbtrnode) {
        return  (TPS_ERR_ALLOC);
    }
    lib_memcpy(pbtrnode,
               &pinode->IND_data.IND_btrNode,
               sizeof(TPS_BTR_NODE) + (sizeof(TPS_BTR_KV) * pinode->IND_data.IND_btrNode.ND_uiEntrys));                
	                                                                    /* 从inode缓存拷贝根节点        */

    while (!bBreak) {
        i = __tpsFsBtrSearch(pbtrnode, blkKey, LW_FALSE, &bEqual);
        switch (pbtrnode->ND_iType) {
        
        case TPS_BTR_NODE_NON_LEAF:
            if (__tpsFsBtrGetNode(pinode, pbtrnode,
                                  pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, pbtrnode);
                return  (TPS_ERR_BTREE_GET_NODE);
            }
            break;

        case TPS_BTR_NODE_LEAF:
            if (bAfter &&
                pbtrnode->ND_kvArr[i].KV_blkKey < blkKey &&
                pbtrnode->ND_uiEntrys > 0 &&
                i != (pbtrnode->ND_uiEntrys - 1)) {                     /* 如果需要尝试查找紧随其后的键 */
                i++;
            }

            if (pbtrnode->ND_uiEntrys > 0) {                            /* 防止越界                     */
                if (i >= pbtrnode->ND_uiEntrys) {
                    i--;
                }
                *pkv = pbtrnode->ND_kvArr[i];

            } else {                                                    /* 节点为空查                   */
                pkv->KV_blkKey                      = 0;
                pkv->KV_data.KP_blkPtr              = 0;
                pkv->KV_data.KV_value.KV_blkStart   = 0;
                pkv->KV_data.KV_value.KV_blkCnt     = 0;
            }

            __tpsFsFreeBtrNode(pinode, pbtrnode);
            bBreak = LW_TRUE;
            return  (TPS_ERR_NONE);

        default:
            __tpsFsFreeBtrNode(pinode, pbtrnode);
            return  (TPS_ERR_BTREE_NODE_TYPE);
        }
    }
    
    __tpsFsFreeBtrNode(pinode, pbtrnode);

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsBtreeSet
** 功能描述: 设置指定键的值
**           ptrans             事物
**           pinode             文件inode指针
**           blkKey             要设置的键
**           pkv                新的值
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsBtreeSet (PTPS_TRANS ptrans, PTPS_INODE pinode, TPS_IBLK blkKey, TPS_BTR_KV kv)
{
    PTPS_BTR_NODE       pbtrnode;
    PTPS_SUPER_BLOCK    psb;
    INT                 i;
    BOOL                bEqual;
    BOOL                bBreak  = LW_FALSE;

    psb = pinode->IND_psb;

    pbtrnode = __tpsFsAllocBtrNode(pinode, psb->SB_uiBlkSize, TPS_BTR_NODE_UNKOWN);
    if (LW_NULL == pbtrnode) {
        return  (TPS_ERR_ALLOC);
    }
    lib_memcpy(pbtrnode,
               &pinode->IND_data.IND_btrNode,
               sizeof(TPS_BTR_NODE) + (sizeof(TPS_BTR_KV) * pinode->IND_data.IND_btrNode.ND_uiEntrys));                
	                                                                       /* 从inode缓存拷贝根节点        */

    while (!bBreak) {
        i = __tpsFsBtrSearch(pbtrnode, blkKey, LW_FALSE, &bEqual);
        switch (pbtrnode->ND_iType) {
        
        case TPS_BTR_NODE_NON_LEAF:
            if (__tpsFsBtrGetNode(pinode, pbtrnode,
                                  pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, pbtrnode);
                return  (TPS_ERR_BTREE_GET_NODE);
            }
            break;

        case TPS_BTR_NODE_LEAF:
            if (!bEqual) {
                __tpsFsFreeBtrNode(pinode, pbtrnode);
                return  (TPS_ERR_BTREE_GET_NODE);
            }

            pbtrnode->ND_kvArr[i] = kv;
            if ((i == 0) && (pbtrnode->ND_blkParent != 0)) {            /* 如为首节点则需递归更新父节点 */
                if (__tpsFsUpdateKey(ptrans, pinode, pbtrnode, 
                                     blkKey,
                                     kv.KV_blkKey) != TPS_ERR_NONE) {
                    __tpsFsFreeBtrNode(pinode, pbtrnode);
                    return  (TPS_ERR_BTREE_UPDATE_KEY);
                }
            }

            bBreak = LW_TRUE;
            break;

        default:
            __tpsFsFreeBtrNode(pinode, pbtrnode);
            return  (TPS_ERR_BTREE_NODE_TYPE);
        }
    }

    if (__tpsFsBtrPutNode(ptrans, pinode,
                          pbtrnode, pbtrnode->ND_blkThis,
                          LW_FALSE, i, 1) != TPS_ERR_NONE) {
        __tpsFsFreeBtrNode(pinode, pbtrnode);
        return  (TPS_ERR_BTREE_PUT_NODE);
    }

    __tpsFsFreeBtrNode(pinode, pbtrnode);

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsBtrNodeCanCombin
** 功能描述: 比较两个块区间释放相连
**           kv1                第一个块区间
**           kv2                第二个块区间
**           bOverlap           返回释放覆盖
** 输　出  : 相邻：TRUE  否则：FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL __tpsFsBtrNodeCanCombin (TPS_BTR_KV kv1, TPS_BTR_KV kv2, BOOL *bOverlap)
{
    /*
     *  是否相邻
     */
    if (kv1.KV_data.KV_value.KV_blkStart +
        kv1.KV_data.KV_value.KV_blkCnt == kv2.KV_data.KV_value.KV_blkStart ||
        kv2.KV_data.KV_value.KV_blkStart +
        kv2.KV_data.KV_value.KV_blkCnt == kv1.KV_data.KV_value.KV_blkStart) {
        *bOverlap = LW_FALSE;
        return  (LW_TRUE);
    }

    /*
     *  释放覆盖
     */
    if (max(kv1.KV_data.KV_value.KV_blkStart, kv2.KV_data.KV_value.KV_blkStart) <
        min((kv1.KV_data.KV_value.KV_blkStart + kv1.KV_data.KV_value.KV_blkCnt),
                (kv2.KV_data.KV_value.KV_blkStart + kv2.KV_data.KV_value.KV_blkCnt))) {
        *bOverlap = LW_TRUE;
        return  (LW_FALSE);
    }

    *bOverlap = LW_FALSE;

    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: tpsFsBtreeFreeBlk
** 功能描述: 插入块区间到b+tree，用于释放磁盘块
**           ptrans             事物
**           pinode             文件inode指针
**           blkKey             键值
**           blkStart           起始块号
**           blkCnt             块数量
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeFreeBlk (PTPS_TRANS   ptrans,
                               PTPS_INODE   pinode,
                               TPS_IBLK     blkKey,
                               TPS_IBLK     blkStart,
                               TPS_IBLK     blkCnt)
{
    TPS_BTR_KV  kv;
    TPS_BTR_KV  kvInsert;
    TPS_IBLK    blkSet = 0;
    TPS_RESULT  tpsres;

    if (pinode == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (blkStart < pinode->IND_psb->SB_ui64DataStartBlk ||
        blkStart + blkCnt > pinode->IND_psb->SB_ui64LogStartBlk) {
        return  (TPS_ERR_BTREE_BLOCK_COUNT);
    }

    kvInsert.KV_blkKey                      = blkKey;
    kvInsert.KV_data.KV_value.KV_blkStart   = blkStart;
    kvInsert.KV_data.KV_value.KV_blkCnt     = blkCnt;

    if (__tpsFsBtreeGet(pinode, blkKey, &kv, LW_FALSE) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (kv.KV_data.KV_value.KV_blkStart + kv.KV_data.KV_value.KV_blkCnt ==
        kvInsert.KV_data.KV_value.KV_blkStart) {                        /* 前方有相邻节点               */
        kvInsert.KV_blkKey = min(kv.KV_blkKey, kvInsert.KV_blkKey);
        kvInsert.KV_data.KV_value.KV_blkStart = min(kv.KV_data.KV_value.KV_blkStart,
                                                    kvInsert.KV_data.KV_value.KV_blkStart);
        kvInsert.KV_data.KV_value.KV_blkCnt = kv.KV_data.KV_value.KV_blkCnt +
                                              kvInsert.KV_data.KV_value.KV_blkCnt;
        blkSet = kv.KV_blkKey;
    }

    if (__tpsFsBtreeGet(pinode, blkKey + blkCnt, &kv, LW_FALSE) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (kvInsert.KV_data.KV_value.KV_blkStart + kvInsert.KV_data.KV_value.KV_blkCnt ==
        kv.KV_data.KV_value.KV_blkStart) {                              /* 后方有相邻节点               */
        kvInsert.KV_blkKey = min(kv.KV_blkKey, kvInsert.KV_blkKey);
        kvInsert.KV_data.KV_value.KV_blkStart = min(kv.KV_data.KV_value.KV_blkStart,
                                                    kvInsert.KV_data.KV_value.KV_blkStart);
        kvInsert.KV_data.KV_value.KV_blkCnt = kv.KV_data.KV_value.KV_blkCnt +
                                              kvInsert.KV_data.KV_value.KV_blkCnt;
        if (!blkSet) {
            blkSet = kv.KV_blkKey;
        
        } else {
            /*
             *  如果前方也有相邻节点，删除后方节点
             */
            if (__tpsFsBtreeRemove(ptrans, pinode, kv) != TPS_ERR_NONE) {
                return  (TPS_ERR_BTREE_NODE_REMOVE);
            }
        }
    }



    if (blkSet) {
        tpsres = __tpsFsBtreeSet(ptrans, pinode, blkSet, kvInsert);     /* 存在可合并的节点             */
    
    } else {
        tpsres = __tpsFsBtreeInsert(ptrans, pinode, kvInsert);          /* 不存在可合并的节点           */
    }

    tpsFsDevBufTrim(pinode->IND_psb, blkStart, blkCnt);                 /* 调用FIOTRIM命令回收扇区      */

    return  (tpsres);
}
/*********************************************************************************************************
** 函数名称: tpsFsBtreeAllocBlk
** 功能描述: 从b+tree删除块区间，用于分配磁盘块
**           ptrans             事物
**           pinode             文件inode指针
**           blkKey             键值
**           blkAllocStart      返回分配得到的起始块
**           blkAllocCnt        返回分配得到的块数量
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeAllocBlk (PTPS_TRANS  ptrans,
                                PTPS_INODE  pinode,
                                TPS_IBLK    blkKey,
                                TPS_IBLK    blkCnt,
                                TPS_IBLK   *blkAllocStart,
                                TPS_IBLK   *blkAllocCnt)
{
    TPS_BTR_KV  kv;
    TPS_IBLK    blkKeyOld;

    if (pinode == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (blkCnt <= 0) {
        return  (TPS_ERR_PARAM);
    }

    if (__tpsFsBtreeGet(pinode, blkKey, &kv, LW_TRUE) != TPS_ERR_NONE) {/* 查找相邻区间                 */
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (kv.KV_data.KV_value.KV_blkCnt == 0) {                           /* 磁盘空间不足                 */
        return  (TPS_ERR_BTREE_DISK_SPACE);
    }

    if (kv.KV_data.KV_value.KV_blkCnt > blkCnt) {                       /* 比区间小则缩小被分配区间     */
        *blkAllocStart = kv.KV_data.KV_value.KV_blkStart;
        *blkAllocCnt   = blkCnt;
        blkKeyOld     = kv.KV_blkKey;
        kv.KV_blkKey += blkCnt;
        kv.KV_data.KV_value.KV_blkStart += blkCnt;
        kv.KV_data.KV_value.KV_blkCnt -= blkCnt;

        if (__tpsFsBtreeSet(ptrans, pinode, blkKeyOld, kv) != TPS_ERR_NONE) {
            return  (TPS_ERR_BTREE_GET_NODE);
        }

    } else {                                                            /* 比区间大则分配整个区间       */
        *blkAllocStart = kv.KV_data.KV_value.KV_blkStart;
        *blkAllocCnt   = kv.KV_data.KV_value.KV_blkCnt;

        if (__tpsFsBtreeRemove(ptrans, pinode, kv) != TPS_ERR_NONE) {
            return  (TPS_ERR_BTREE_NODE_REMOVE);
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsBtreeGetBlk
** 功能描述: 获取键值后面的连续块
**           pinode             文件inode指针
**           blkKey             键值
**           blkStart           返回得到的起始块
**           blkCnt             返回得到的块数量
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeGetBlk (PTPS_INODE pinode, TPS_IBLK blkKey, TPS_IBLK *blkStart, TPS_IBLK *blkCnt)
{
    TPS_BTR_KV  kv;

    if (pinode == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (__tpsFsBtreeGet(pinode, blkKey, &kv, LW_FALSE) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (blkKey < kv.KV_blkKey ||
        blkKey >= kv.KV_blkKey + kv.KV_data.KV_value.KV_blkCnt) {
        return  (TPS_ERR_BTREE_KEY_NOTFOUND);
    }

    /*
     *  这里考虑了键值落在块区间中间的情况
     */
    *blkStart = kv.KV_data.KV_value.KV_blkStart + (blkKey - kv.KV_blkKey);
    *blkCnt = kv.KV_data.KV_value.KV_blkCnt - ((blkKey - kv.KV_blkKey));

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsBtreeAppendBlk
** 功能描述: 从尾部追加块到b+tree
**           ptrans             事物
**           pinode             文件inode指针
**           blkStart           起始块
**           blkCnt             块数量
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeAppendBlk (PTPS_TRANS ptrans,
                                 PTPS_INODE pinode,
                                 TPS_IBLK   blkStart,
                                 TPS_IBLK   blkCnt)
{
    TPS_BTR_KV  kv;

    if ((pinode == LW_NULL) || (pinode == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (__tpsFsBtreeGet(pinode, MAX_BLK_NUM, &kv, LW_FALSE) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (kv.KV_data.KV_value.KV_blkStart + kv.KV_data.KV_value.KV_blkCnt
        == blkStart) {                                                  /* 与最后一个块区间相邻         */
        kv.KV_data.KV_value.KV_blkCnt += blkCnt;

        return  (__tpsFsBtreeSet(ptrans, pinode, kv.KV_blkKey, kv));

    } else {
        kv.KV_blkKey += kv.KV_data.KV_value.KV_blkCnt;
        kv.KV_data.KV_value.KV_blkStart = blkStart;
        kv.KV_data.KV_value.KV_blkCnt   = blkCnt;

        return  (__tpsFsBtreeInsert(ptrans, pinode, kv));
    }
}
/*********************************************************************************************************
** 函数名称: tpsFsBtreeTrunc
** 功能描述: 分多次删除b+tree中指定键值之后的块区间
**           ptrans             事物
**           pinode             文件inode指针
**           blkStart           返回本次删除的区间起始块
**           blkCnt             返回本次删除的区间结束块,如果没有可删除的块,则返回0
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeTrunc (PTPS_TRANS     ptrans,
                             PTPS_INODE     pinode,
                             TPS_IBLK       blkKey,
                             TPS_IBLK      *blkPscStart,
                             TPS_IBLK      *blkPscCnt)
{
    TPS_BTR_KV  kv;

    if (pinode == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }
    if (__tpsFsBtreeGet(pinode, MAX_BLK_NUM, &kv, LW_FALSE) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (kv.KV_data.KV_value.KV_blkCnt == 0) {
        return  (TPS_ERR_BTREE_GET_NODE);
    }

    if (blkKey <= kv.KV_blkKey) {                                       /* 键值比区间键值小则删除区间   */
        *blkPscStart = kv.KV_data.KV_value.KV_blkStart;
        *blkPscCnt   = kv.KV_data.KV_value.KV_blkCnt;

        return  (__tpsFsBtreeRemove(ptrans, pinode, kv));

    } else if (blkKey < kv.KV_blkKey + kv.KV_data.KV_value.KV_blkCnt) { /* 键值比区间键值大则缩小区间   */
        *blkPscStart = kv.KV_data.KV_value.KV_blkStart + (blkKey - kv.KV_blkKey);
        *blkPscCnt = kv.KV_data.KV_value.KV_blkCnt - (blkKey - kv.KV_blkKey);
        kv.KV_data.KV_value.KV_blkCnt = blkKey - kv.KV_blkKey;

        return  (__tpsFsBtreeSet(ptrans, pinode, kv.KV_blkKey, kv));
    
    } else {
        *blkPscStart = 0;
        *blkPscCnt   = 0;

        return  (TPS_ERR_NONE);
    }

}
/*********************************************************************************************************
** 函数名称: tpsFsBtreeBlkCnt
** 功能描述: 获取b+tree中的块数量
**           pinode             文件inode指针
** 输　出  : 块数目
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_IBLK  tpsFsBtreeBlkCnt (PTPS_INODE pinode)
{
    TPS_BTR_KV  kv;

    if (__tpsFsBtreeGet(pinode, MAX_BLK_NUM,
                        &kv, LW_FALSE) != TPS_ERR_NONE) {               /* 获取最后一个块区间           */
        return  0;
    }

    return  (kv.KV_blkKey + kv.KV_data.KV_value.KV_blkCnt);
}
/*********************************************************************************************************
** 函数名称: tpsFsBtreeInitBP
** 功能描述: 初始化块缓冲区
**           psb             超级块指针
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeInitBP (PTPS_SUPER_BLOCK psb)
{
    PUCHAR      pucBuff;
    PUCHAR      pucPos;
    TPS_IBLK    blkStart;
    TPS_IBLK    blkCnt;
    TPS_IBLK    blkAlloc;
    INT         i;

    if (LW_NULL == psb) {
        return  (TPS_ERR_PARAM);
    }

    pucBuff = (PUCHAR)TPS_ALLOC(psb->SB_uiBlkSize);
    if (LW_NULL == pucBuff) {
        return  (TPS_ERR_ALLOC);
    }

    lib_bzero(pucBuff, psb->SB_uiBlkSize);

    for (i = 0; i < psb->SB_ui64BPBlkCnt; i++) {                        /* 清空缓冲区中的块             */
        if (tpsFsDevBufWrite(psb, psb->SB_ui64BPStartBlk + i, 0,
                             pucBuff, psb->SB_uiBlkSize, LW_TRUE) != TPS_ERR_NONE) {
            TPS_FREE(pucBuff);
            return  (TPS_ERR_BP_INIT);
        }
    }

    /*
     *  分配空闲块
     */
    blkAlloc = TPS_ADJUST_BP_BLK;
    pucPos   = pucBuff;
    while (blkAlloc > 0) {                                              /* 分配缓冲区块                 */
        if (tpsFsBtreeAllocBlk(LW_NULL, psb->SB_pinodeSpaceMng, 0,
                               TPS_ADJUST_BP_BLK, &blkStart, &blkCnt) != TPS_ERR_NONE) {
            TPS_FREE(pucBuff);
            return  (TPS_ERR_BTREE_ALLOC);
        }

        for (i = 0; i < blkCnt; i++) {
            TPS_CPU_TO_IBLK(pucPos, (blkStart + i));
        }

        blkAlloc -= blkCnt;
    }

    if (tpsFsDevBufWrite(psb, psb->SB_ui64BPStartBlk, 0,
                         pucBuff, psb->SB_uiBlkSize,
                         LW_TRUE) != TPS_ERR_NONE) {                    /* 写入缓冲区列表               */
        TPS_FREE(pucBuff);
        return  (TPS_ERR_BP_INIT);
    }

    TPS_FREE(pucBuff);

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsBtreeAdjustBP
** 功能描述: 调整块缓冲区
**           ptrans          事物
**           psb             超级块指针
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeAdjustBP (PTPS_TRANS ptrans, PTPS_SUPER_BLOCK psb)
{
    PTPS_BLK_POOL pbp;
    TPS_IBLK      blk       = 0;
    TPS_IBLK      blkStart  = 0;
    TPS_IBLK      blkCnt    = 0;

    if (LW_NULL == psb) {
        return  (TPS_ERR_PARAM_NULL);
    }

    pbp = psb->SB_pbp;

    if (pbp->BP_uiBlkCnt >= TPS_MAX_BP_BLK) {                           /* 空闲块过多                   */
        while(pbp->BP_uiBlkCnt > TPS_ADJUST_BP_BLK) {
            blk = __tpsFsBtrAllocNodeBlk(ptrans, psb,
                                         psb->SB_pinodeSpaceMng);       /* 释放缓冲区块                 */
            if (blk <= 0) {
                return  (TPS_ERR_BP_ALLOC);
            }

            if (tpsFsBtreeFreeBlk(ptrans, psb->SB_pinodeSpaceMng,
                                  blk, blk, 1) != TPS_ERR_NONE) {       /* 回收到空间管理b+tree         */
                return  (TPS_ERR_BTREE_FREE);
            }
        }
    }

    if (pbp->BP_uiBlkCnt <= TPS_MIN_BP_BLK) {                           /* 空闲块太少                   */
        while (pbp->BP_uiBlkCnt < TPS_ADJUST_BP_BLK) {
            if (tpsFsBtreeAllocBlk(ptrans, psb->SB_pinodeSpaceMng,
                                   0, 1, &blkStart,
                                   &blkCnt) != TPS_ERR_NONE) {          /* 从空间管理b+tree分配块       */
                return  (TPS_ERR_BTREE_ALLOC);
            }

            if (__tpsFsBtrFreeNodeBlk(ptrans, psb,
                                      psb->SB_pinodeSpaceMng,
                                      blkStart) != TPS_ERR_NONE) {      /* 添加到块缓冲区               */
                return  (TPS_ERR_BP_FREE);
            }
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsBtreeReadBP
** 功能描述: 从磁盘读取块缓冲区
**           psb             超级块指针
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeReadBP (PTPS_SUPER_BLOCK psb)
{
    PUCHAR      pucBuff;
    INT         i;
    INT         j;
    TPS_IBLK    blk = 0;

    if (LW_NULL == psb) {
        return  (TPS_ERR_PARAM);
    }

    pucBuff = (PUCHAR)TPS_ALLOC(psb->SB_uiBlkSize);
    if (LW_NULL == pucBuff) {
        return  (TPS_ERR_ALLOC);
    }

    lib_bzero(psb->SB_pbp, sizeof(TPS_BLK_POOL));

    /*
     *  查找第一个全0的块
     */
    blk = psb->SB_ui64BPStartBlk;
    for (i = 0; i < psb->SB_ui64BPBlkCnt; i++, blk++) {
        if (tpsFsDevBufRead(psb, blk, 0,
                            pucBuff, psb->SB_uiBlkSize) != TPS_ERR_NONE) {
            TPS_FREE(pucBuff);
            return  (TPS_ERR_BP_INIT);
        }

        for (j = 0; j < psb->SB_uiBlkSize; j += sizeof(TPS_IBLK)) {
            if (TPS_IBLK_TO_CPU_VAL(pucBuff + j) != 0) {
                break;
            }
        }

        if (j == psb->SB_uiBlkSize) {                                   /* 从全0的块的下一块开始查找    */
            break;
        }
    }

    /*
     *  缓冲区从第一个全0 块后第一个非0块开始的块开始
     */
    for (i = 0; i < psb->SB_ui64BPBlkCnt; i++) {
        if (tpsFsDevBufRead(psb, blk, 0,
                            pucBuff, psb->SB_uiBlkSize) != TPS_ERR_NONE) {
            TPS_FREE(pucBuff);
            return  (TPS_ERR_BP_INIT);
        }

        j = 0;
        for (; j < psb->SB_uiBlkSize &&
               psb->SB_pbp->BP_uiBlkCnt <= 0; j += sizeof(TPS_IBLK)) {  /* 找到第一个非0块，记录起始    */
            if (TPS_IBLK_TO_CPU_VAL(pucBuff + j) != 0) {
                psb->SB_pbp->BP_blkStart = blk;
                psb->SB_pbp->BP_uiStartOff = j;
                break;
            }
        }

        for (; j < psb->SB_uiBlkSize &&
             psb->SB_pbp->BP_uiBlkCnt < TPS_MAX_BP_BLK;
             j += sizeof(TPS_IBLK)) {                                   /* 查找结束位置                 */
            if (TPS_IBLK_TO_CPU_VAL(pucBuff + j) != 0) {
                psb->SB_pbp->BP_blkArr[psb->SB_pbp->BP_uiBlkCnt] = TPS_IBLK_TO_CPU_VAL(pucBuff + j);
                psb->SB_pbp->BP_uiBlkCnt++;
            } else {
                break;
            }
        }

        if (j < psb->SB_uiBlkSize ||
            psb->SB_pbp->BP_uiBlkCnt >= TPS_MAX_BP_BLK) {
            break;
        }

        blk++;
        if (blk == psb->SB_ui64BPStartBlk + psb->SB_ui64BPBlkCnt) {     /* 循环查找                     */
            blk = psb->SB_ui64BPStartBlk;
        }
    }

    TPS_FREE(pucBuff);

    if (psb->SB_pbp->BP_uiBlkCnt >= TPS_MAX_BP_BLK ||
        psb->SB_pbp->BP_uiBlkCnt <= TPS_MIN_BP_BLK) {
        return  (tpsFsBtreeAdjustBP(LW_NULL, psb));
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsBtreeGetBlkCnt
** 功能描述: 获取b+tree中的块数量
**           pinode           inode指针
**           pbtrnode         节点指针
**           pszBlkCnt        返回块数量
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_RESULT  __tpsBtreeGetBlkCnt (PTPS_INODE pinode, PTPS_BTR_NODE pbtrnode, TPS_SIZE_T *pszBlkCnt)
{
    PTPS_SUPER_BLOCK    psb = pinode->IND_psb;
    PTPS_BTR_NODE       pbtrSubnode;
    INT                 i;

    for (i = 0; i < pbtrnode->ND_uiEntrys; i++) {
        if (pbtrnode->ND_iType != TPS_BTR_NODE_LEAF) {
            pbtrSubnode = __tpsFsAllocBtrNode(pinode, psb->SB_uiBlkSize, TPS_BTR_NODE_UNKOWN);
            if (LW_NULL == pbtrSubnode) {
                return  (TPS_ERR_ALLOC);
            }

            if (__tpsFsBtrGetNode(pinode, pbtrSubnode,
                pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                    __tpsFsFreeBtrNode(pinode, pbtrnode);
                    return  (TPS_ERR_BTREE_GET_NODE);
            }

            __tpsBtreeGetBlkCnt(pinode, pbtrSubnode, pszBlkCnt);        /* 非叶子节点递归查找           */

            __tpsFsFreeBtrNode(pinode, pbtrSubnode);
        
        } else {
            (*pszBlkCnt) += pbtrnode->ND_kvArr[i].KV_data.KV_value.KV_blkCnt;
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsBtreeGetBlkCnt
** 功能描述: 获取b+tree中的块数量
**           pinode           inode指针
** 输　出  : 块数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_SIZE_T  tpsFsBtreeGetBlkCnt (struct tps_inode *pinode)
{
    TPS_SIZE_T  szBlkCnt = 0;

    __tpsBtreeGetBlkCnt(pinode, &pinode->IND_data.IND_btrNode, &szBlkCnt);

    return  (szBlkCnt);
}
/*********************************************************************************************************
** 函数名称: tpsFsBtreeDump
** 功能描述: 打印b+tree信息
**           pinode             inode指针
**           pbtrnode           b+tree节点
** 输　出  : 块数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsBtreeDump (PTPS_INODE pinode, PTPS_BTR_NODE pbtrnode)
{
    PTPS_SUPER_BLOCK    psb = pinode->IND_psb;
    PTPS_BTR_NODE       pbtrSubnode;
    INT                 i;

#ifdef WIN32                                                            /* windows 打印格式             */
    /*
     *  打印本节点信息
     */
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_iType          = %x",       pbtrnode->ND_iType);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_uiEntrys       = %x",       pbtrnode->ND_uiEntrys);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_uiMaxCnt       = %x",       pbtrnode->ND_uiMaxCnt);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_uiLevel        = %x",       pbtrnode->ND_uiLevel);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_ui64Generation = 0x%016I64x", pbtrnode->ND_ui64Generation);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_inumInode      = 0x%016I64x", pbtrnode->ND_inumInode);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_blkThis        = 0x%016I64x", pbtrnode->ND_blkThis);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_blkParent      = 0x%016I64x", pbtrnode->ND_blkParent);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_blkPrev        = 0x%016I64x", pbtrnode->ND_blkPrev);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_blkNext        = 0x%016I64x", pbtrnode->ND_blkNext);
    _DebugHandle(__LOGMESSAGE_LEVEL, "Sub nodes:");
    
    for (i = 0; i < pbtrnode->ND_uiEntrys; i++) {
        if (pbtrnode->ND_iType != TPS_BTR_NODE_LEAF) {
            _DebugFormat(__LOGMESSAGE_LEVEL, "Sub%X: KV_blkKey = 0x%016I64x     KP_blkPtr = 0x%016I64x ",
                         i, pbtrnode->ND_kvArr[i].KV_blkKey, pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr);
        } else {
            _DebugFormat(__LOGMESSAGE_LEVEL,
                         "Sub%X: KV_blkKey = 0x%016I64x     "
                         "KV_blkStart = 0x%016I64x  KV_blkCnt = 0x%016I64x",
                         i, pbtrnode->ND_kvArr[i].KV_blkKey,
                         pbtrnode->ND_kvArr[i].KV_data.KV_value.KV_blkStart,
                         pbtrnode->ND_kvArr[i].KV_data.KV_value.KV_blkCnt);
        }
    }
    _DebugHandle(__LOGMESSAGE_LEVEL, "\n");

#else
    /*
     *  打印本节点信息
     */
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_iType          = %x",       pbtrnode->ND_iType);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_uiEntrys       = %x",       pbtrnode->ND_uiEntrys);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_uiMaxCnt       = %x",       pbtrnode->ND_uiMaxCnt);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_uiLevel        = %x",       pbtrnode->ND_uiLevel);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_ui64Generation = 0x%016qx", pbtrnode->ND_ui64Generation);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_inumInode      = 0x%016qx", pbtrnode->ND_inumInode);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_blkThis        = 0x%016qx", pbtrnode->ND_blkThis);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_blkParent      = 0x%016qx", pbtrnode->ND_blkParent);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_blkPrev        = 0x%016qx", pbtrnode->ND_blkPrev);
    _DebugFormat(__LOGMESSAGE_LEVEL, "ND_blkNext        = 0x%016qx", pbtrnode->ND_blkNext);
    _DebugHandle(__LOGMESSAGE_LEVEL, "Sub nodes:");
    
    for (i = 0; i < pbtrnode->ND_uiEntrys; i++) {
        if (pbtrnode->ND_iType != TPS_BTR_NODE_LEAF) {
            _DebugFormat(__LOGMESSAGE_LEVEL, "Sub%X: KV_blkKey = 0x%016qx     KP_blkPtr = 0x%016qx ",
                         i, pbtrnode->ND_kvArr[i].KV_blkKey, pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr);
        } else {
            _DebugFormat(__LOGMESSAGE_LEVEL,
                         "Sub%X: KV_blkKey = 0x%016qx     "
                         "KV_blkStart = 0x%016qx  KV_blkCnt = 0x%016qx",
                         i, pbtrnode->ND_kvArr[i].KV_blkKey,
                         pbtrnode->ND_kvArr[i].KV_data.KV_value.KV_blkStart,
                         pbtrnode->ND_kvArr[i].KV_data.KV_value.KV_blkCnt);
        }
    }
    _DebugHandle(__LOGMESSAGE_LEVEL, "\n");
#endif                                                                  /*  WIN32                       */

    if (pbtrnode->ND_iType == TPS_BTR_NODE_LEAF) {
        return  (TPS_ERR_NONE);
    }

    /*
     *  递归打印子节点信息
     */
    for (i = 0; i < pbtrnode->ND_uiEntrys; i++) {
        pbtrSubnode = __tpsFsAllocBtrNode(pinode, psb->SB_uiBlkSize, TPS_BTR_NODE_UNKOWN);
        if (LW_NULL == pbtrSubnode) {
            return  (TPS_ERR_ALLOC);
        }

        if (__tpsFsBtrGetNode(pinode, pbtrSubnode,
            pbtrnode->ND_kvArr[i].KV_data.KP_blkPtr) != TPS_ERR_NONE) {
                __tpsFsFreeBtrNode(pinode, pbtrnode);
                return  (TPS_ERR_BTREE_GET_NODE);
        }

        tpsFsBtreeDump(pinode, pbtrSubnode);

        __tpsFsFreeBtrNode(pinode, pbtrSubnode);
    }

    return  (TPS_ERR_NONE);
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
