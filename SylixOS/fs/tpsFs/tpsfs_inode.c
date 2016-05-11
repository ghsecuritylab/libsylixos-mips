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
** 文   件   名: tpsfs_inode.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: inode 操作

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
#include "tpsfs_dir.h"
#include "tpsfs_dev_buf.h"
/*********************************************************************************************************
** 函数名称: __tpsFsInodeSerial
** 功能描述: 序列号inode结构
** 输　入  : pinode           inode结构指针
**           pucBuff          序列号缓冲区
**           uiSize           缓冲区大小
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  __tpsFsInodeSerial (PTPS_INODE pinode, PUCHAR pucBuff, UINT uiSize)
{
    PUCHAR  pucPos = pucBuff;

    TPS_CPU_TO_LE32(pucPos, pinode->IND_uiMagic);
    TPS_CPU_TO_LE32(pucPos, pinode->IND_uiVersion);
    TPS_CPU_TO_LE64(pucPos, pinode->IND_ui64Generation);
    TPS_CPU_TO_IBLK(pucPos, pinode->IND_inum);
    TPS_CPU_TO_LE64(pucPos, pinode->IND_szData);
    TPS_CPU_TO_LE64(pucPos, pinode->IND_szAttr);
    TPS_CPU_TO_LE64(pucPos, pinode->IND_ui64CTime);
    TPS_CPU_TO_LE64(pucPos, pinode->IND_ui64MTime);
    TPS_CPU_TO_LE64(pucPos, pinode->IND_ui64ATime);
    TPS_CPU_TO_LE32(pucPos, pinode->IND_iMode);
    TPS_CPU_TO_LE32(pucPos, pinode->IND_iType);
    TPS_CPU_TO_LE32(pucPos, pinode->IND_uiRefCnt);
    TPS_CPU_TO_LE32(pucPos, pinode->IND_uiDataStart);
    TPS_CPU_TO_LE32(pucPos, pinode->IND_iUid);
    TPS_CPU_TO_LE32(pucPos, pinode->IND_iGid);
    TPS_CPU_TO_LE64(pucPos, pinode->IND_inumDeleted);

    return  (LW_TRUE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsInodeUnSerial
** 功能描述: 逆序列号inode结构
** 输　入  : pinode           inode结构指针
**           pucBuff          序列号缓冲区
**           uiSize           缓冲区大小
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  __tpsFsInodeUnserial (PTPS_INODE pinode, PUCHAR pucBuff, UINT uiSize)
{
    PUCHAR  pucPos = pucBuff;

    TPS_LE32_TO_CPU(pucPos, pinode->IND_uiMagic);
    TPS_LE32_TO_CPU(pucPos, pinode->IND_uiVersion);
    TPS_LE64_TO_CPU(pucPos, pinode->IND_ui64Generation);
    TPS_IBLK_TO_CPU(pucPos, pinode->IND_inum);
    TPS_LE64_TO_CPU(pucPos, pinode->IND_szData);
    TPS_LE64_TO_CPU(pucPos, pinode->IND_szAttr);
    TPS_LE64_TO_CPU(pucPos, pinode->IND_ui64CTime);
    TPS_LE64_TO_CPU(pucPos, pinode->IND_ui64MTime);
    TPS_LE64_TO_CPU(pucPos, pinode->IND_ui64ATime);
    TPS_LE32_TO_CPU(pucPos, pinode->IND_iMode);
    TPS_LE32_TO_CPU(pucPos, pinode->IND_iType);
    TPS_LE32_TO_CPU(pucPos, pinode->IND_uiRefCnt);
    TPS_LE32_TO_CPU(pucPos, pinode->IND_uiDataStart);
    TPS_LE32_TO_CPU(pucPos, pinode->IND_iUid);
    TPS_LE32_TO_CPU(pucPos, pinode->IND_iGid);
    TPS_LE64_TO_CPU(pucPos, pinode->IND_inumDeleted);

    return  (LW_TRUE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsGetFromFreeList
** 功能描述: 从空闲inode列表获取块
**           ptrans             事物
**           psb                超级块指针
**           blkKey             键值
**           blkAllocStart      返回分配得到的起始块
**           blkAllocCnt        返回分配得到的块数量
** 输　出  : 成功：0  失败：ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsGetFromFreeList (PTPS_TRANS        ptrans,
                                           PTPS_SUPER_BLOCK  psb,
                                           TPS_IBLK          blkKey,
                                           TPS_IBLK          blkCnt,
                                           TPS_IBLK         *blkAllocStart,
                                           TPS_IBLK         *blkAllocCnt)
{
    if (psb->SB_pinodeDeleted == LW_NULL) {
        if (psb->SB_inumDeleted != 0) {
            psb->SB_pinodeDeleted = tpsFsOpenInode(psb, psb->SB_inumDeleted);
        }
    }

    if (psb->SB_pinodeDeleted == LW_NULL) {
        return  (TPS_ERR_INODE_OPEN);
    }

    if (tpsFsBtreeTrunc(ptrans, psb->SB_pinodeDeleted,
                        (blkCnt > psb->SB_pinodeDeleted->IND_blkCnt ? 0 :
                                (psb->SB_pinodeDeleted->IND_blkCnt - blkCnt)),
                        blkAllocStart, blkAllocCnt) != TPS_ERR_NONE) {
        return  (TPS_ERR_BTREE_ALLOC);
    }

    psb->SB_pinodeDeleted->IND_blkCnt -= (*blkAllocCnt);

    if (psb->SB_pinodeDeleted->IND_blkCnt <= 0) {
        tpsFsBtreeFreeBlk(ptrans, psb->SB_pinodeSpaceMng,
                          psb->SB_pinodeDeleted->IND_inum,
                          psb->SB_pinodeDeleted->IND_inum, 1);

        psb->SB_inumDeleted = psb->SB_pinodeDeleted->IND_inumDeleted;
        tpsFsFlushSuperBlock(ptrans, psb);

        tpsFsCloseInode(psb->SB_pinodeDeleted);
        psb->SB_pinodeDeleted = LW_NULL;
    }

    if ((*blkAllocCnt) == 0) {
        return  (TPS_ERR_BTREE_DISK_SPACE);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFsInodeAddToFreeList
** 功能描述: 将inode添加到空闲队列
** 输　入  : ptrans           事物
**           psb              超级块指针
**           pinode           inode指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static TPS_RESULT  __tpsFsInodeAddToFreeList(PTPS_TRANS         ptrans,
                                             PTPS_SUPER_BLOCK   psb,
                                             PTPS_INODE         pinode)
{
    if (psb->SB_pinodeDeleted != LW_NULL) {
        pinode->IND_inumDeleted = psb->SB_pinodeDeleted->IND_inumDeleted;
        tpsFsFlushInodeHead(ptrans, pinode);
        psb->SB_pinodeDeleted->IND_inumDeleted = pinode->IND_inum;
        tpsFsFlushInodeHead(ptrans, psb->SB_pinodeDeleted);
    } else {
        pinode->IND_inumDeleted = psb->SB_inumDeleted;
        tpsFsFlushInodeHead(ptrans, pinode);
        psb->SB_inumDeleted = pinode->IND_inum;
        tpsFsFlushSuperBlock(ptrans, psb);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsCreateInode
** 功能描述: 创建并初始化inode
** 输　入  : ptrans           事物
**           psb              超级块指针
**           inum             文件号
**           iMode            文件模式
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsCreateInode (PTPS_TRANS ptrans, PTPS_SUPER_BLOCK psb, TPS_INUM inum, INT iMode)
{
    PTPS_INODE pinode       = LW_NULL;
    PUCHAR     pucBuff      = LW_NULL;
	UINT       uiInodeSize  = 0;

    if (psb == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

	uiInodeSize = (psb->SB_uiBlkSize - TPS_INODE_DATASTART 
                   - sizeof(TPS_BTR_NODE)) / (sizeof(TPS_IBLK) * 2);
	uiInodeSize *=  sizeof(TPS_BTR_KV);
	uiInodeSize += (TPS_INODE_DATASTART + sizeof(TPS_BTR_NODE));
    pinode = (PTPS_INODE)TPS_ALLOC(uiInodeSize);
    if (LW_NULL == pinode) {
        return  (TPS_ERR_ALLOC);
    }

    pinode->IND_psb             = psb;
    pinode->IND_inum            = inum;
    pinode->IND_iMode           = iMode;
    pinode->IND_iType           = TPS_INODE_TYPE_REG;
    pinode->IND_uiRefCnt        = 1;                                    /* 创建后必然要被引用           */
    pinode->IND_ui64Generation  = psb->SB_ui64Generation;
    pinode->IND_uiDataStart     = TPS_INODE_DATASTART;
    pinode->IND_uiMagic         = TPS_MAGIC_INODE;
    pinode->IND_uiVersion       = psb->SB_uiVersion;
    pinode->IND_pnext           = LW_NULL;
    pinode->IND_szData          = 0;
    pinode->IND_szAttr          = 0;
    pinode->IND_ui64CTime       = TPS_UTC_TIME();
    pinode->IND_ui64MTime       = pinode->IND_ui64CTime;
    pinode->IND_ui64ATime       = pinode->IND_ui64CTime;
    pinode->IND_iUid            = getuid();
    pinode->IND_iGid            = getgid();
    pinode->IND_inumDeleted     = 0;

    pucBuff = (PUCHAR)TPS_ALLOC(psb->SB_uiBlkSize);                     /* 后面将用于序列号缓冲区       */
    if (LW_NULL == pucBuff) {
        TPS_FREE(pinode);
        return  (TPS_ERR_ALLOC);
    }

    if (!__tpsFsInodeSerial(pinode, pucBuff, TPS_INODE_MAX_HEADSIZE)) { /* 序列化                       */
        TPS_FREE(pucBuff);
        TPS_FREE(pinode);
        return  (TPS_ERR_INODE_SERIAL);
    }

    if (tpsFsTransWrite(ptrans, psb,
                        pinode->IND_inum, 0,
                        pucBuff,
                        TPS_INODE_MAX_HEADSIZE) != TPS_ERR_NONE) {      /* 写磁盘                       */
        TPS_FREE(pucBuff);
        TPS_FREE(pinode);
        return  (TPS_ERR_TRANS_WRITE);
    }

    pinode->IND_pucBuff = pucBuff;

    if (tpsFsBtreeInit(ptrans, pinode) != TPS_ERR_NONE) {               /* 初始化b+tree                 */
        TPS_FREE(pucBuff);
        TPS_FREE(pinode);
        return  (TPS_ERR_BTREE_INIT);
    }

    TPS_FREE(pucBuff);
    TPS_FREE(pinode);
    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsOpenInode
** 功能描述: 打开inode
** 输　入  : psb              超级块指针
**           inum             文件号
** 输　出  : inode指针
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PTPS_INODE  tpsFsOpenInode (PTPS_SUPER_BLOCK psb, TPS_INUM inum)
{
    PTPS_INODE  pinode      = LW_NULL;
    PUCHAR      pucBuff     = LW_NULL;

    if (psb == LW_NULL) {
        return  (LW_NULL);
    }

    pinode = psb->SB_pinodeOpenList;
    while (pinode) {
        if (pinode->IND_inum == inum) {                                 /* 查找当前打开inode列表        */
            pinode->IND_uiOpenCnt++;
            return  (pinode);
        }
        pinode = pinode->IND_pnext;
    }

    pucBuff = (PUCHAR)TPS_ALLOC(psb->SB_uiBlkSize);                     /* 后面将用于序列号缓冲区       */
    if (LW_NULL == pucBuff) {
        return  (LW_NULL);
    }

    if (tpsFsDevBufRead(psb, inum, 0, pucBuff,
                        TPS_INODE_MAX_HEADSIZE) != TPS_ERR_NONE) {      /* 读取inode                    */
        TPS_FREE(pucBuff);
        return  (LW_NULL);
    }

    pinode = (PTPS_INODE)TPS_ALLOC(sizeof(TPS_INODE));
    if (LW_NULL == pinode) {
        TPS_FREE(pucBuff);
        return  (LW_NULL);
    }

    if (!__tpsFsInodeUnserial(pinode, pucBuff, TPS_INODE_MAX_HEADSIZE)) {
        TPS_FREE(pucBuff);
        TPS_FREE(pinode);
        return  (LW_NULL);
    }

    if (pinode->IND_ui64Generation != psb->SB_ui64Generation ||
        pinode->IND_uiMagic != TPS_MAGIC_INODE) {
        TPS_FREE(pucBuff);
        TPS_FREE(pinode);
        return  (LW_NULL);
    }

    pinode->IND_pucBuff     = pucBuff;
    pinode->IND_psb         = psb;
    pinode->IND_uiOpenCnt   = 1;
    pinode->IND_bDirty      = LW_FALSE;
    pinode->IND_bDeleted    = LW_FALSE;
    pinode->IND_pnext       = psb->SB_pinodeOpenList;
    psb->SB_pinodeOpenList  = pinode;
    pinode->IND_psb->SB_uiInodeOpenCnt++;
    lib_memset(pinode->IND_pBNPool, 0, sizeof(pinode->IND_pBNPool));
    lib_memset(pinode->IND_iBNRefCnt, TPS_BN_POOL_NULL, sizeof(pinode->IND_iBNRefCnt));

    pinode->IND_blkCnt      = tpsFsBtreeBlkCnt(pinode);

    return  (pinode);
}
/*********************************************************************************************************
** 函数名称: tpsFsFlushHead
** 功能描述: 如果inode为脏，则写入头部，一般用于flush文件大小
** 输　入  : ptrans           事物
**           pinode           inode指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsFlushInodeHead (PTPS_TRANS ptrans, PTPS_INODE pinode)
{
    PUCHAR              pucBuff = LW_NULL;
    PTPS_SUPER_BLOCK    psb;

    if (pinode == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    psb = pinode->IND_psb;

    if (pinode->IND_bDirty) {                                           /* inode脏则将头部写入          */
        pucBuff = pinode->IND_pucBuff;

        __tpsFsInodeSerial(pinode, pucBuff, psb->SB_uiBlkSize);         /* 序列化                       */

        if (tpsFsTransWrite(ptrans, psb,
                            pinode->IND_inum,
                            0,
                            pucBuff,
                            TPS_INODE_MAX_HEADSIZE) != TPS_ERR_NONE) {
            return  (TPS_ERR_TRANS_WRITE);
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsCloseInode
** 功能描述: 关闭inode
** 输　入  : pinode           inode指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsCloseInode (PTPS_INODE pinode)
{
    PTPS_INODE  *ppinodeIter = LW_NULL;
    INT          i;

    if (pinode == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (pinode->IND_uiOpenCnt > 0) {
        pinode->IND_uiOpenCnt--;

        if (pinode->IND_uiOpenCnt == 0) {
            if (pinode->IND_bDirty) {                                   /* inode脏则将头部写入          */
            }

            ppinodeIter = &pinode->IND_psb->SB_pinodeOpenList;
            while (*ppinodeIter) {
                if ((*ppinodeIter) == pinode) {
                    (*ppinodeIter) = (*ppinodeIter)->IND_pnext;
                    pinode->IND_psb->SB_uiInodeOpenCnt--;
                    break;
                }
                ppinodeIter = &((*ppinodeIter)->IND_pnext);
            }

            for (i = 0; i < TPS_BN_POOL_SIZE; i++) {                    /* 释放节点缓冲池               */
                if (pinode->IND_pBNPool[i] != LW_NULL) {
                    TPS_FREE(pinode->IND_pBNPool[i]);
                }
            }
            TPS_FREE(pinode->IND_pucBuff);
            TPS_FREE(pinode);
        }
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsInodeAddRef
** 功能描述: inode引用计数加1
** 输　入  : ptrans           事物
**           pinode           inode指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsInodeAddRef (PTPS_TRANS ptrans, PTPS_INODE pinode)
{
    PTPS_SUPER_BLOCK    psb     = LW_NULL;
    PUCHAR              pucBuff = LW_NULL;

    if ((ptrans == LW_NULL) || (pinode == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }
    
    if (pinode->IND_bDeleted) {
        return  (TPS_ERR_INODE_DELETED);
    }

    psb = pinode->IND_psb;

    pinode->IND_uiRefCnt++;

    pucBuff = pinode->IND_pucBuff;

    __tpsFsInodeSerial(pinode, pucBuff, psb->SB_uiBlkSize);             /* 序列化                       */

    if (tpsFsTransWrite(ptrans, psb,
                        pinode->IND_inum,
                        0,
                        pucBuff,
                        TPS_INODE_MAX_HEADSIZE) != TPS_ERR_NONE) {        /* 写磁盘                       */
        return  (TPS_ERR_TRANS_WRITE);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsInodeDelRef
** 功能描述: inode引用计数减1
** 输　入  : ptrans           事物
**           pinode           inode指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsInodeDelRef (PTPS_TRANS ptrans, PTPS_INODE pinode)
{
    PTPS_SUPER_BLOCK    psb          = LW_NULL;
    PUCHAR              pucBuff      = LW_NULL;
    PTPS_INODE         *ppinodeIter  = LW_NULL;

    if ((ptrans == LW_NULL) || (pinode == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }
    
    if (pinode->IND_bDeleted) {
        return  (TPS_ERR_INODE_DELETED);
    }

    psb = pinode->IND_psb;

    if (pinode->IND_uiRefCnt > 0) {
        pinode->IND_uiRefCnt--;
    }
    if (pinode->IND_uiRefCnt == 0) {                                    /* 引用为0时删除文件inode       */
        if (tpsFsBtreeGetLevel(pinode) > 0) {
            __tpsFsInodeAddToFreeList(ptrans, psb, pinode);
        } else {
            if (tpsFsTruncInode(ptrans, pinode, 0) != TPS_ERR_NONE) {
                return  (TPS_ERR_INODE_TRUNC);
            }
            if (tpsFsBtreeFreeBlk(ptrans, psb->SB_pinodeSpaceMng,
                                  pinode->IND_inum, pinode->IND_inum, 1) != TPS_ERR_NONE) {
                return  (TPS_ERR_BTREE_INSERT);
            }
        }

        ppinodeIter = &psb->SB_pinodeOpenList;
        while (*ppinodeIter) {                                          /* 从列表移除，防止再次被打开   */
            if ((*ppinodeIter) == pinode) {
                (*ppinodeIter) = (*ppinodeIter)->IND_pnext;
                psb->SB_uiInodeOpenCnt--;
                break;
            }
            ppinodeIter = &((*ppinodeIter)->IND_pnext);
        }

        pinode->IND_bDeleted        = LW_TRUE;
    }

    pucBuff = pinode->IND_pucBuff;

    __tpsFsInodeSerial(pinode, pucBuff, psb->SB_uiBlkSize);             /* 序列化                       */

    if (tpsFsTransWrite(ptrans, psb,
                        pinode->IND_inum,
                        0,
                        pucBuff,
                        TPS_INODE_MAX_HEADSIZE) != TPS_ERR_NONE) {        /* 写磁盘                       */
        return  (TPS_ERR_TRANS_WRITE);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsInodeDelRef
** 功能描述: 截断文件
** 输　入  : ptrans           事物
**           pinode           inode指针
**           ui64Off          截断后的长度
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsTruncInode (PTPS_TRANS ptrans, PTPS_INODE pinode, TPS_SIZE_T size)
{
    TPS_IBLK            blkStart;
    TPS_IBLK            blkPscStart;
    TPS_IBLK            blkPscCnt;
    PTPS_SUPER_BLOCK    psb;
    PUCHAR              pucBuff = LW_NULL;

    if ((ptrans == LW_NULL) || (pinode == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (pinode->IND_bDeleted) {
        return  (TPS_ERR_INODE_DELETED);
    }

    if (size > pinode->IND_szData) {
        return  (TPS_ERR_INODE_SIZE);
    }

    psb         = pinode->IND_psb;
    blkStart    = size >> psb->SB_uiBlkShift;
    if (size & psb->SB_uiBlkMask) {
        blkStart++;
    }

    while (tpsFsBtreeTrunc(ptrans, pinode, blkStart,
                           &blkPscStart, &blkPscCnt) == TPS_ERR_NONE) {
        if (blkPscCnt <= 0) {
            break;
		}

        pinode->IND_blkCnt -= blkPscCnt;

        if (tpsFsBtreeFreeBlk(ptrans, psb->SB_pinodeSpaceMng,
                             blkPscStart, blkPscStart, blkPscCnt) != TPS_ERR_NONE) {
            return  (TPS_ERR_BTREE_INSERT);
        }

        if (tpsFsBtreeAdjustBP(ptrans, psb) != TPS_ERR_NONE) {          /* 检查是否需要调空闲块队列     */
            return  (TPS_ERR_BP_ADJUST);
        }
    }

    if (blkStart < tpsFsBtreeBlkCnt(pinode)) {
        return  (TPS_ERR_BTREE_TRUNC);
    }

    pinode->IND_szData = size;
    pinode->IND_bDirty = LW_TRUE;

    pucBuff = pinode->IND_pucBuff;

    __tpsFsInodeSerial(pinode, pucBuff, psb->SB_uiBlkSize);             /* 序列化                       */

    if (tpsFsTransWrite(ptrans, psb,
                        pinode->IND_inum,
                        0,
                        pucBuff,
                        TPS_INODE_MAX_HEADSIZE) != TPS_ERR_NONE) {        /* 写磁盘                       */
        return  (TPS_ERR_TRANS_WRITE);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsInodeRead
** 功能描述: 读取文件
** 输　入  : pinode           inode指针
**           off              起始位置
**           pucBuff          缓冲区
**           szLen            读取长度
** 输　出  : 成功返回实际读取长度，失败返回-ERRNO
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_SIZE_T  tpsFsInodeRead (PTPS_INODE pinode, TPS_OFF_T off, PUCHAR pucBuff, TPS_SIZE_T szLen)
{
    TPS_IBLK            blkLogic;
    TPS_IBLK            blkPscStart;
    TPS_IBLK            blkPscCnt;
    PTPS_SUPER_BLOCK    psb         = LW_NULL;
    TPS_SIZE_T          szCompleted = 0;
    TPS_SIZE_T          szReadLen;
    TPS_OFF_T           offBlk;

    if ((pinode == LW_NULL) || (pucBuff == LW_NULL)) {
        return  (-EINVAL);
    }

    if (off >= pinode->IND_szData) {
        return  (0);
    }

    /*
     *  TODO: 文件大小可保存在inode头中时直接保存在inode头中，暂不使能
     */
#if 0
    if (pinode->IND_szData < (pinode->IND_psb->SB_uiBlkSize - TPS_INODE_DATASTART)) {
        lib_memcpy(pucItemBuf,
                   pinode->IND_data.IND_cData + off,
                   min(szLen, pinode->IND_szData - off));
        return  (min(szLen, pinode->IND_szData - off));
    }
#endif

    psb = pinode->IND_psb;

    while (szLen > 0) {
        blkLogic = off >> psb->SB_uiBlkShift;
        if (tpsFsBtreeGetBlk(pinode, blkLogic, &blkPscStart, &blkPscCnt) != TPS_ERR_NONE) {
            break;
        }

        offBlk    = off & psb->SB_uiBlkMask;
        szReadLen = psb->SB_uiBlkSize * blkPscCnt - offBlk;
        szReadLen = min(szLen, szReadLen);
        szReadLen = min(szReadLen, (pinode->IND_szData - off));

        if (tpsFsDevBufRead(psb, blkPscStart,
                            (UINT)offBlk, pucBuff + szCompleted,
                            (size_t)szReadLen) != TPS_ERR_NONE) {       /* 读取inode                    */
            szCompleted = (-EIO);
            break;
        }

        off         += szReadLen;
        szLen       -= szReadLen;
        szCompleted += szReadLen;

        if (off >= pinode->IND_szData) {
            break;
        }
    }

    return  (szCompleted);
}
/*********************************************************************************************************
** 函数名称: tpsFsInodeTransWrite
** 功能描述: 写文件
** 输　入  : ptrans           事物
**           pinode           inode指针
**           off              起始位置
**           pucBuff          缓冲区
**           szLen            读取长度
**           bTransData       文件数据是否要记录事物
** 输　出  : 成功返回实际写入长度，，失败返回-ERRNO
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_SIZE_T  tpsFsInodeWrite (PTPS_TRANS ptrans, PTPS_INODE pinode, TPS_OFF_T off,
                             PUCHAR pucBuff, TPS_SIZE_T szLen, BOOL bTransData)
{
    TPS_IBLK            blkEnd;
    TPS_IBLK            blkAlloc = 0;
    TPS_IBLK            blkPscStart;
    TPS_IBLK            blkPscCnt;

    TPS_IBLK            blkLogic;
    PTPS_SUPER_BLOCK    psb         = LW_NULL;
    TPS_SIZE_T          szCompleted = 0;
    TPS_SIZE_T          szWriteLen;
    TPS_OFF_T           offBlk;

    if ((pinode == LW_NULL) || (pucBuff == LW_NULL)) {
        return  (-EINVAL);
    }

    psb = pinode->IND_psb;
    blkEnd = (off + szLen) >> psb->SB_uiBlkShift;
    if ((off + szLen) & psb->SB_uiBlkMask) {
        blkEnd++;
    }

    blkAlloc = 0;
    if (blkEnd > pinode->IND_blkCnt) {
        blkAlloc = blkEnd - pinode->IND_blkCnt;
    }
    while (blkAlloc > 0) {                                              /* 需要新分配块到文件           */
        if (__tpsFsGetFromFreeList(ptrans, psb, 0, blkAlloc,
                                   &blkPscStart, &blkPscCnt) == TPS_ERR_NONE) {
            if (tpsFsBtreeAppendBlk(ptrans, pinode, blkPscStart, blkPscCnt) != TPS_ERR_NONE) {
                return  (-EIO);
            }
            blkAlloc -= blkPscCnt;
            pinode->IND_blkCnt += blkPscCnt;
        } else if (tpsFsBtreeAllocBlk(ptrans, psb->SB_pinodeSpaceMng,
                               0, blkAlloc,
                               &blkPscStart, &blkPscCnt) == TPS_ERR_NONE) {
            if (tpsFsBtreeAppendBlk(ptrans, pinode, blkPscStart, blkPscCnt) != TPS_ERR_NONE) {
                return  (-EIO);
            }
            blkAlloc -= blkPscCnt;
            pinode->IND_blkCnt += blkPscCnt;
        
        } else {
            return  (-ENOSPC);
        }

        if (tpsFsBtreeAdjustBP(ptrans, psb) != TPS_ERR_NONE) {          /* 检查是否需要调空闲块队列     */
            return  (-EIO);
        }
    }

    while (szLen > 0) {
        blkLogic = off >> psb->SB_uiBlkShift;
        if (tpsFsBtreeGetBlk(pinode, blkLogic,
                             &blkPscStart, &blkPscCnt) != TPS_ERR_NONE) {
            break;
        }

        offBlk = off & psb->SB_uiBlkMask;
        szWriteLen = psb->SB_uiBlkSize * blkPscCnt - offBlk;
        szWriteLen = min(szLen, szWriteLen);

        if (bTransData) {
            if (tpsFsTransWrite(ptrans, psb, blkPscStart,
                                (UINT)offBlk, pucBuff + szCompleted,
                                (size_t)szWriteLen) != TPS_ERR_NONE) {  /* 事务写                       */
                szCompleted = (-EIO);
                break;
            }
        
        } else {
            if (tpsFsDevBufWrite(psb,
                                 blkPscStart,
                                 (UINT)offBlk,
                                 pucBuff + szCompleted,
                                 (size_t)szWriteLen,
                                 LW_FALSE) != TPS_ERR_NONE) {           /* 非事务写                     */
                szCompleted = (-EIO);
                break;
            }

            if (pinode->IND_iFlag & (O_SYNC | O_DSYNC)) {               /* 需要立即同步                 */
                if (tpsFsDevBufSync(psb,
                                    blkPscStart,
                                    (UINT)offBlk,
                                    (size_t)szWriteLen) != TPS_ERR_NONE) {
                    szCompleted = (-EIO);
                    break;
                }
            }
        }

        off         += szWriteLen;
        szLen       -= szWriteLen;
        szCompleted += szWriteLen;
    }

    pinode->IND_ui64MTime = TPS_UTC_TIME();
    pinode->IND_ui64ATime = pinode->IND_ui64MTime;

    if (off > pinode->IND_szData) {
        pinode->IND_szData = off;
        pinode->IND_bDirty = LW_TRUE;
        if (bTransData) {
            if (tpsFsFlushInodeHead(ptrans, pinode) != TPS_ERR_NONE) {
                return  (-EIO);
            }
        }
    }

    return  (szCompleted);
}
/*********************************************************************************************************
** 函数名称: tpsFsInodeGetSize
** 功能描述: 写文件
** 输　入  : pinode           inode指针
** 输　出  : 获取inode文件数据长度
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_SIZE_T  tpsFsInodeGetSize (PTPS_INODE pinode)
{
    return  (pinode->IND_szData);
}
/*********************************************************************************************************
** 函数名称: tpsFsInodeSync
** 功能描述: 同步文件到
** 输　入  : pinode           inode指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsInodeSync (PTPS_INODE pinode)
{
    TPS_IBLK            blkPscStart;
    TPS_IBLK            blkPscCnt;
    TPS_IBLK            blkLogic        = 0;
    PTPS_SUPER_BLOCK    psb             = LW_NULL;

    if (pinode == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    psb = pinode->IND_psb;

    while (blkLogic * psb->SB_uiBlkSize < pinode->IND_szData) {
        if (tpsFsBtreeGetBlk(pinode, blkLogic,
                             &blkPscStart, &blkPscCnt) != TPS_ERR_NONE) {
            break;
        }

        if (tpsFsDevBufSync(psb,
                            blkPscStart,
                            0,
                            (size_t)(blkPscCnt << psb->SB_uiBlkShift)) != TPS_ERR_NONE) {
            break;                                                      /* 同步块                       */
        }

        blkLogic += blkPscCnt;
    }

    if (blkLogic * psb->SB_uiBlkSize < pinode->IND_szData) {
        return  (TPS_ERR_INODE_SYNC);
    }

    return  (TPS_ERR_NONE);
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
