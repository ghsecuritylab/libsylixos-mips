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
** 文   件   名: tpsfs_btree.h
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: tpsfs btree操作

** BUG:
*********************************************************************************************************/

#ifndef __TPSFS_BTREE_H
#define __TPSFS_BTREE_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0

#include "sys/cdefs.h"

/*********************************************************************************************************
  Btree magic
*********************************************************************************************************/

#define TPS_MAGIC_BTRNODE       0xad48ea87

/*********************************************************************************************************
  最大子节点数
*********************************************************************************************************/
#define TPS_BTR_MAX_NODE        0x1000

/*********************************************************************************************************
  节点类型定义
*********************************************************************************************************/

#define TPS_BTR_NODE_UNKOWN     0                                       /* 根节点                       */
#define TPS_BTR_NODE_ROOT       1                                       /* 根节点                       */
#define TPS_BTR_NODE_LEAF       2                                       /* 叶子节点                     */
#define TPS_BTR_NODE_NON_LEAF   3                                       /* 内部节点                     */

/*********************************************************************************************************
  key value 对
*********************************************************************************************************/

typedef struct tps_btr_kv {
    TPS_IBLK    KV_blkKey;                                              /* key                          */
    union {
        TPS_IBLK    KP_blkPtr;                                          /* child 指针                   */
        struct {
            TPS_IBLK     KV_blkStart;                                   /* 起始块                       */
            TPS_IBLK     KV_blkCnt;                                     /* 块数量                       */
        } KV_value;
    } KV_data;
} TPS_BTR_KV;
typedef TPS_BTR_KV      *PTPS_BTR_KV;

/*********************************************************************************************************
  节点结构定义
*********************************************************************************************************/

typedef struct tps_btr_node {
    UINT32          ND_uiMagic;                                         /* 节点magic魔数                */
     INT32          ND_iType;                                           /* 节点类型                     */
    UINT32          ND_uiEntrys;                                        /* 节点条目数                   */
    UINT32          ND_uiMaxCnt;                                        /* 容量                         */
    UINT32          ND_uiLevel;                                         /* 节点条目数                   */
    UINT32          ND_uiReserved1;                                     /* 保留1                        */
    UINT64          ND_ui64Generation;                                  /* 标识一次格式化用于系统修复   */
    UINT64          ND_ui64Reserved2[2];                                /* 保留2                        */
    
    TPS_INUM        ND_inumInode;                                       /* 节点所属inode                */
    TPS_IBLK        ND_blkThis;                                         /* 本节点物理块号               */
    TPS_IBLK        ND_blkParent;                                       /* 父节点块                     */
    TPS_IBLK        ND_blkPrev;                                         /* 上一节点块                   */
    TPS_IBLK        ND_blkNext;                                         /* 下一节点块                   */
    TPS_BTR_KV      ND_kvArr __flexarr;                                 /* 块区域列表                   */
} TPS_BTR_NODE;
typedef TPS_BTR_NODE        *PTPS_BTR_NODE;

/*********************************************************************************************************
  块缓冲区定义
*********************************************************************************************************/

#define TPS_MAX_BP_BLK      128
#define TPS_ADJUST_BP_BLK   96
#define TPS_MIN_BP_BLK      64

typedef struct tps_blk_pool {
    TPS_IBLK        BP_blkStart;                                        /* 缓冲区起始块                 */
    UINT            BP_uiStartOff;                                      /* 块缓冲区列表在起始块中的偏移 */
    UINT            BP_uiBlkCnt;                                        /* 块缓冲列表块数目             */
    TPS_IBLK        BP_blkArr[TPS_MAX_BP_BLK];                          /* 以数组记录的块缓冲列表       */
} TPS_BLK_POOL;
typedef TPS_BLK_POOL  *PTPS_BLK_POOL;

/*********************************************************************************************************
  Btree API
*********************************************************************************************************/

struct tps_trans;
struct tps_inode;

                                                                    /* 添加块到btree                    */
TPS_RESULT tpsFsBtreeFreeBlk(struct tps_trans *ptrans, struct tps_inode *pinode,
                             TPS_IBLK blkKey, TPS_IBLK blkStart, TPS_IBLK blkCnt);
                                                                    /* 从btree移除块                    */
TPS_RESULT tpsFsBtreeAllocBlk(struct tps_trans *ptrans, struct tps_inode *pinode,
                              TPS_IBLK blkKey, TPS_IBLK blkCnt,
                              TPS_IBLK *blkAllocStart, TPS_IBLK *blkAllocCnt);
                                                                    /* 获取键值后面的连续块             */
TPS_RESULT tpsFsBtreeGetBlk(struct tps_inode *pinode, TPS_IBLK blkKey,
                            TPS_IBLK *blkStart, TPS_IBLK *blkCnt);
                                                                    /* 从尾部追加块到btree              */
TPS_RESULT tpsFsBtreeAppendBlk(struct tps_trans *ptrans, struct tps_inode *pinode,
                               TPS_IBLK blkStart, TPS_IBLK blkCnt);
                                                                    /* 删除btree中指定键值之后的所有块  */
TPS_RESULT tpsFsBtreeTrunc(struct tps_trans *ptrans, struct tps_inode *pinode, TPS_IBLK blkKey,
                           TPS_IBLK *blkPscStart, TPS_IBLK *blkPscCnt);
                                                                    /* 获取文件inode btree中的块数量    */
TPS_IBLK tpsFsBtreeBlkCnt(struct tps_inode *pinode);
                                                                    /* 获取空间管理结点 btree中的块数量 */
TPS_SIZE_T tpsFsBtreeGetBlkCnt(struct tps_inode *pinode);
                                                                    /* 读取块缓冲区                     */
TPS_RESULT tpsFsBtreeReadBP(PTPS_SUPER_BLOCK psb);
                                                                    /* 初始化块缓冲区                   */
TPS_RESULT tpsFsBtreeInitBP(PTPS_SUPER_BLOCK psb);
                                                                    /* 调整块缓冲区                     */
TPS_RESULT tpsFsBtreeAdjustBP(PTPS_TRANS ptrans, PTPS_SUPER_BLOCK psb);
                                                                    /* 序列化btree节点                  */
VOID tpsSerialBtrNode(PTPS_BTR_NODE pbtrnode, PUCHAR pucBuff, UINT uiBlkSize,
                      UINT uiItemStart, UINT uiItemCnt, UINT *puiOffStart, UINT *puiOffEnd);
                                                                    /* 逆序列化btree节点                */
VOID tpsUnserialBtrNode(PTPS_BTR_NODE pbtrnode, PUCHAR pucBuff, UINT uiBlkSize);
                                                                    /* 打印整颗树                       */
TPS_RESULT tpsFsBtreeDump(struct tps_inode *pinode, PTPS_BTR_NODE pbtrnode);

#endif                                                              /* LW_CFG_TPSFS_EN > 0              */
#endif                                                              /* __TPSFS_BTREE_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
