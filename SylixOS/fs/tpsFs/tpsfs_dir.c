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
** 文   件   名: tpsfs_dir.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: tpsfs 目录操作

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
/*********************************************************************************************************
** 函数名称: tpsFsCreateEntry
** 功能描述: 创建文件或目录
** 输　入  : ptrans           事物
**           pinodeDir        父目录
**           pcFileName       文件名
**           inum             文件号
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsCreateEntry (PTPS_TRANS    ptrans,
                              PTPS_INODE    pinodeDir,
                              CPCHAR        pcFileName,
                              TPS_INUM      inum)
{
    TPS_SIZE_T szDir;
    TPS_OFF_T  off          = 0;
    UINT       uiEntryLen   = 0;
    UINT       uiItemCnt    = 0;
    UINT       i            = 0;
    PUCHAR     pucItemBuf   = LW_NULL;
    PUCHAR     pucPos       = LW_NULL;

    if ((ptrans == LW_NULL) || (pinodeDir == LW_NULL) || (pcFileName == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    uiEntryLen = sizeof(UINT) + sizeof(TPS_INUM) + lib_strlen(pcFileName) + 1;
    uiItemCnt  = uiEntryLen >> TPS_ENTRY_ITEM_SHIFT;                    /* 计算entry占用的item数目      */
    if ((uiEntryLen & TPS_ENTRY_ITEM_MASK) != 0) {
        uiItemCnt++;
    }

    pucItemBuf = (PUCHAR)TPS_ALLOC(TPS_ENTRY_ITEM_SIZE);
    if (LW_NULL == pucItemBuf) {
        return  (TPS_ERR_ALLOC);
    }

    /*
     *  使用首次适应算法查找足够长的空间
     */
    szDir = tpsFsInodeGetSize(pinodeDir);
    while (off < szDir) {
        for (i = 0; i < uiItemCnt; i++) {
            if (tpsFsInodeRead(pinodeDir,
                               off + i * TPS_ENTRY_ITEM_SIZE,
                               pucItemBuf,
                               TPS_ENTRY_ITEM_SIZE) < TPS_ENTRY_ITEM_SIZE) {
                break;
            }

            if (TPS_LE32_TO_CPU_VAL(pucItemBuf) != 0) {
                break;
            }
        }

        if ((i == uiItemCnt) || ((off + (i + 1) * TPS_ENTRY_ITEM_SIZE) > szDir)) {
            break;
        }

        off += (i + 1) * TPS_ENTRY_ITEM_SIZE;
    }

    TPS_FREE(pucItemBuf);

    pucItemBuf = (PUCHAR)TPS_ALLOC(TPS_ENTRY_ITEM_SIZE * uiItemCnt);
    if (LW_NULL == pucItemBuf) {
        return  (TPS_ERR_ALLOC);
    }
    lib_bzero(pucItemBuf, TPS_ENTRY_ITEM_SIZE * uiItemCnt);

    /*
     *  写入新entry到目录inode
     */
    pucPos = pucItemBuf;
    TPS_CPU_TO_LE32(pucPos, uiEntryLen);
    TPS_CPU_TO_IBLK(pucPos, inum);
    lib_strcpy((PCHAR)pucPos, pcFileName);
    
    if (tpsFsInodeWrite(ptrans, pinodeDir,
                        off, pucItemBuf,
                        TPS_ENTRY_ITEM_SIZE * uiItemCnt,
                        LW_TRUE) < (TPS_ENTRY_ITEM_SIZE * uiItemCnt)) {
        TPS_FREE(pucItemBuf);
        return  (TPS_ERR_INODE_WRITE);
    }
    
    TPS_FREE(pucItemBuf);
    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsEntryRemove
** 功能描述: 删除文件或目录
** 输　入  : ptrans           事物
**           pentry           entry指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsEntryRemove (PTPS_TRANS ptrans, PTPS_ENTRY pentry)
{
    UINT       uiEntryLen   = 0;
    UINT       uiItemCnt    = 0;
    PUCHAR     pucItemBuf   = LW_NULL;
    PTPS_INODE pinodeDir    = LW_NULL;

    if ((ptrans == LW_NULL) || (pentry == LW_NULL)) {
        return  (TPS_ERR_PARAM_NULL);
    }

    uiEntryLen = sizeof(UINT) + sizeof(TPS_INUM) + lib_strlen(pentry->ENTRY_pcName) + 1;
    uiItemCnt  = uiEntryLen >> TPS_ENTRY_ITEM_SHIFT;                    /* 计算entry占用的item数目      */
    if ((uiEntryLen & TPS_ENTRY_ITEM_MASK) != 0) {
        uiItemCnt++;
    }

    pucItemBuf = (PUCHAR)TPS_ALLOC(TPS_ENTRY_ITEM_SIZE * uiItemCnt);
    if (LW_NULL == pucItemBuf) {
        return  (TPS_ERR_ALLOC);
    }
    lib_bzero(pucItemBuf, TPS_ENTRY_ITEM_SIZE * uiItemCnt);

    pinodeDir = tpsFsOpenInode(pentry->ENTRY_psb, pentry->ENTRY_inumDir);
    if (LW_NULL == pinodeDir) {
        TPS_FREE(pucItemBuf);
        return  (TPS_ERR_INODE_OPEN);
    }

    if (tpsFsInodeWrite(ptrans, pinodeDir,
                        pentry->ENTRY_offset, pucItemBuf,
                        TPS_ENTRY_ITEM_SIZE * uiItemCnt,
                        LW_TRUE) < (TPS_ENTRY_ITEM_SIZE * uiItemCnt)) {
        tpsFsCloseInode(pinodeDir);
        TPS_FREE(pucItemBuf);
        return  (TPS_ERR_INODE_WRITE);
    }

    tpsFsCloseInode(pinodeDir);
    TPS_FREE(pucItemBuf);
    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsFindEntry
** 功能描述: 查找entry
** 输　入  : pinodeDir           父目录
**           pcFileName          文件名
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PTPS_ENTRY  tpsFsFindEntry (PTPS_INODE pinodeDir, CPCHAR pcFileName)
{
    TPS_SIZE_T szDir;
    TPS_OFF_T  off          = 0;
    UINT       uiEntryLen   = 0;
    UINT       uiItemCnt    = 0;
    PUCHAR     pucItemBuf   = LW_NULL;
    PUCHAR     pucPos       = LW_NULL;
    PTPS_ENTRY pentry       = LW_NULL;

    if ((pinodeDir == LW_NULL) || (pcFileName == LW_NULL)) {
        return  (LW_NULL);
    }

    uiEntryLen = sizeof(UINT) + sizeof(TPS_INUM) + lib_strlen(pcFileName) + 1;
    uiItemCnt  = uiEntryLen >> TPS_ENTRY_ITEM_SHIFT;                    /* 计算entry占用的item数目      */
    if ((uiEntryLen & TPS_ENTRY_ITEM_MASK) != 0) {
        uiItemCnt++;
    }

    pucItemBuf = (PUCHAR)TPS_ALLOC(TPS_ENTRY_ITEM_SIZE * uiItemCnt);
    if (LW_NULL == pucItemBuf) {
        return  (LW_NULL);
    }

    /*
     *  查找entry
     */
    szDir = tpsFsInodeGetSize(pinodeDir);
    while (off < szDir) {
        if (tpsFsInodeRead(pinodeDir,
                           off,
                           pucItemBuf,
                           TPS_ENTRY_ITEM_SIZE) < TPS_ENTRY_ITEM_SIZE) {
            TPS_FREE(pucItemBuf);
            return  (LW_NULL);
        }

        if (TPS_LE32_TO_CPU_VAL(pucItemBuf) != uiEntryLen) {
            off += (TPS_LE32_TO_CPU_VAL(pucItemBuf) == 0 ? TPS_ENTRY_ITEM_SIZE : TPS_LE32_TO_CPU_VAL(pucItemBuf));
            if (off & TPS_ENTRY_ITEM_MASK) {
                off = TPS_ROUND_UP(off, TPS_ENTRY_ITEM_SIZE);
            }
            continue;
        }

        if (tpsFsInodeRead(pinodeDir,
                           off,
                           pucItemBuf,
                           uiEntryLen) < uiEntryLen) {
            TPS_FREE(pucItemBuf);
            return  (LW_NULL);
        }

        if (lib_strcmp((PCHAR)pucItemBuf + sizeof(UINT) + sizeof(TPS_INUM), pcFileName) != 0) {
            off += uiEntryLen;
            if (off & TPS_ENTRY_ITEM_MASK) {
                off = TPS_ROUND_UP(off, TPS_ENTRY_ITEM_SIZE);
            }
            continue;
        }
        
        break;
    }

    if (off >= szDir) {
        TPS_FREE(pucItemBuf);
        return  (LW_NULL);
    }

    /*
     *  构建entry对象
     */
    pentry = (PTPS_ENTRY)TPS_ALLOC(sizeof(TPS_ENTRY) + lib_strlen(pcFileName) + 1);
    if (LW_NULL == pentry) {
        TPS_FREE(pucItemBuf);
        return  (LW_NULL);
    }
    lib_bzero(pentry, sizeof(TPS_ENTRY) + lib_strlen(pcFileName) + 1);
    
    pentry->ENTRY_offset    = off;
    pentry->ENTRY_psb       = pinodeDir->IND_psb;
    pentry->ENTRY_inumDir   = pinodeDir->IND_inum;
    pucPos = pucItemBuf;
    TPS_LE32_TO_CPU(pucPos, pentry->ENTRY_uiLen);
    TPS_IBLK_TO_CPU(pucPos, pentry->ENTRY_inum);
    lib_strcpy(pentry->ENTRY_pcName, (PCHAR)pucPos);

    TPS_FREE(pucItemBuf);

    pentry->ENTRY_pinode = tpsFsOpenInode(pentry->ENTRY_psb, pentry->ENTRY_inum);
    if (LW_NULL == pentry->ENTRY_pinode) {
        TPS_FREE(pentry);
        return  (LW_NULL);
    }
    

    return  (pentry);
}
/*********************************************************************************************************
** 函数名称: tpsFsEntryRead
** 功能描述: 读取entry
** 输　入  : pinodeDir           父目录
**           off                 偏移位置
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PTPS_ENTRY  tpsFsEntryRead (PTPS_INODE pinodeDir, TPS_OFF_T off)
{
    TPS_SIZE_T szDir;
    PUCHAR     pucItemBuf   = LW_NULL;
    UINT       uiEntryLen   = 0;
    PUCHAR     pucPos       = LW_NULL;
    PTPS_ENTRY pentry       = LW_NULL;
    
    if (pinodeDir == LW_NULL) {
        return  (LW_NULL);
    }

    if (off & TPS_ENTRY_ITEM_MASK) {
        off = TPS_ROUND_UP(off, TPS_ENTRY_ITEM_SIZE);
    }

    pucItemBuf = (PUCHAR)TPS_ALLOC(TPS_ENTRY_ITEM_SIZE);
    if (LW_NULL == pucItemBuf) {
        return  (LW_NULL);
    }
    
    /*
     *  查找entry
     */
    szDir = tpsFsInodeGetSize(pinodeDir);
    while (off < szDir) {
        if (tpsFsInodeRead(pinodeDir,
                           off,
                           pucItemBuf,
                           TPS_ENTRY_ITEM_SIZE) < TPS_ENTRY_ITEM_SIZE) {
            TPS_FREE(pucItemBuf);
            return  (LW_NULL);
        }

        uiEntryLen = TPS_LE32_TO_CPU_VAL(pucItemBuf);
        if (uiEntryLen != 0) {
            break;
        }

        off += TPS_ENTRY_ITEM_SIZE;
    }

    if ((off >= szDir) || (uiEntryLen == 0)) {
        TPS_FREE(pucItemBuf);
        return  (LW_NULL);
    }

    TPS_FREE(pucItemBuf);

    pucItemBuf = (PUCHAR)TPS_ALLOC(uiEntryLen);
    if (LW_NULL == pucItemBuf) {
        TPS_FREE(pucItemBuf);
        return  (LW_NULL);
    }

    if (tpsFsInodeRead(pinodeDir,
                       off,
                       pucItemBuf,
                       uiEntryLen) < uiEntryLen) {
        TPS_FREE(pucItemBuf);
        return  (LW_NULL);
    }


    /*
     *  构建entry对象
     */
    pentry = (PTPS_ENTRY)TPS_ALLOC(sizeof(TPS_ENTRY) + uiEntryLen);
    if (LW_NULL == pentry) {
        TPS_FREE(pucItemBuf);
        return  (LW_NULL);
    }
    lib_bzero(pentry, sizeof(TPS_ENTRY) + uiEntryLen);
    
    pentry->ENTRY_offset    = off;
    pentry->ENTRY_psb       = pinodeDir->IND_psb;
    pentry->ENTRY_inumDir   = pinodeDir->IND_inum;
    pucPos = pucItemBuf;
    TPS_LE32_TO_CPU(pucPos, pentry->ENTRY_uiLen);
    TPS_IBLK_TO_CPU(pucPos, pentry->ENTRY_inum);
    lib_strcpy(pentry->ENTRY_pcName, (PCHAR)pucPos);

    TPS_FREE(pucItemBuf);

    pentry->ENTRY_pinode = tpsFsOpenInode(pentry->ENTRY_psb, pentry->ENTRY_inum);
    if (LW_NULL == pentry->ENTRY_pinode) {
        TPS_FREE(pentry);
        return  (LW_NULL);
    }

    return  (pentry);
}
/*********************************************************************************************************
** 函数名称: tpsFsEntryFree
** 功能描述: 释放entry内存指针
** 输　入  : pentry           entry对象指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsEntryFree (PTPS_ENTRY pentry)
{
    if (pentry == LW_NULL) {
        return  (TPS_ERR_PARAM_NULL);
    }

    if (pentry->ENTRY_pinode) {
        tpsFsCloseInode(pentry->ENTRY_pinode);
    }

    TPS_FREE(pentry);

    return  (TPS_ERR_NONE);
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
