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
** 文   件   名: tpsfs.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: tpsfs API 实现

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
#include "tpsfs_dev_buf.h"
#include "tpsfs_inode.h"
#include "tpsfs_dir.h"
#include "tpsfs.h"
/*********************************************************************************************************
** 函数名称: __tpsFsCheckFileName
** 功能描述: 检查文件名操作
** 输　入  : pcName           文件名
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static errno_t  __tpsFsCheckFileName (CPCHAR  pcName)
{
    register CPCHAR  pcTemp;

    /*
     *  不能建立 . 或 .. 文件
     */
    pcTemp = lib_rindex(pcName, PX_DIVIDER);
    if (pcTemp) {
        pcTemp++;
        if (*pcTemp == '\0') {                                          /*  文件名长度为 0              */
            return  (ENOENT);
        }
        if ((lib_strcmp(pcTemp, ".")  == 0) ||
            (lib_strcmp(pcTemp, "..") == 0)) {                          /*  . , .. 检查                 */
            return  (ENOENT);
        }
    } else {
        if (pcName[0] == '\0') {                                        /*  文件名长度为 0              */
            return  (ENOENT);
        }
    }

    /*
     *  不能包含非法字符
     */
    pcTemp = (PCHAR)pcName;
    for (; *pcTemp != '\0'; pcTemp++) {
        if (lib_strchr("\\*?<>:\"|\t\r\n", *pcTemp)) {                  /*  检查合法性                  */
            return  (ENOENT);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tpsFSCreate
** 功能描述: 根据名称创建文件和目录
** 输　入  : pinodeDir        父目录
**           pcFileName       文件名称
**           iMode            文件模式
** 输　出  : 错误返回 LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PTPS_ENTRY  __tpsFSCreate (PTPS_INODE pinodeDir, CPCHAR pcFileName, INT iMode, INT *piErr)
{
    PTPS_SUPER_BLOCK    psb     = pinodeDir->IND_psb;
    TPS_INUM            inum    = 0;
    TPS_IBLK            blkPscCnt;
    PTPS_TRANS          ptrans  = LW_NULL;
    PTPS_ENTRY          pentry  = LW_NULL;

    if (piErr == LW_NULL) {
        return  (LW_NULL);
    }

    ptrans = tpsFsTransAllocAndInit(psb);                               /* 分配事物                     */
    if (ptrans == LW_NULL) {
        *piErr = EINVAL;
        return  (LW_NULL);
    }

    if (tpsFsBtreeAllocBlk(ptrans, psb->SB_pinodeSpaceMng,
                           0, 1, &inum, &blkPscCnt) != TPS_ERR_NONE) {  /* 分配inode                    */
        tpsFsTransRollBackAndFree(ptrans);
        *piErr = ENOSPC;
        return  (LW_NULL);
    }

    if (tpsFsCreateInode(ptrans, psb, inum, iMode) != TPS_ERR_NONE) {   /* 初始化inode,引用计数为1      */
        tpsFsTransRollBackAndFree(ptrans);
        *piErr = EIO;
        return  (LW_NULL);
    }

    if (tpsFsCreateEntry(ptrans, pinodeDir,
                         pcFileName, inum) != ERROR_NONE) {             /* 创建目录项并关联到inode      */
        tpsFsTransRollBackAndFree(ptrans);
        *piErr = EIO;
        return  (LW_NULL);
    }

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* 提交事务                     */
        tpsFsTransRollBackAndFree(ptrans);
        *piErr = EIO;
        return  (LW_NULL);
    }

    pentry = tpsFsFindEntry(pinodeDir, pcFileName);
    if (pentry == LW_NULL) {
        *piErr = ENOENT;
    }

    return  (pentry);
}
/*********************************************************************************************************
** 函数名称: __tpsFsWalkPath
** 功能描述: 查找文件所在目录并返回
** 输　入  : psb              super block指针
**           pcPath           文件路径
**           bFindParent      是否只查找条目所在目录
**           ppcSymLink       符号链接位置
** 输　出  : 成功返回entry项，失败分两种情况，存在父目录返回一个inumber为0的entry，否则返回NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PTPS_ENTRY  __tpsFsWalkPath (PTPS_SUPER_BLOCK psb, CPCHAR pcPath, PCHAR *ppcRemain)
{
    PCHAR   pcPathDup    = LW_NULL;
    PCHAR   pcPathRemain = LW_NULL;
    PCHAR   pcFileName   = LW_NULL;

    PTPS_ENTRY pentry    = LW_NULL;
    PTPS_ENTRY pentrySub = LW_NULL;

    if (__tpsFsCheckFileName(pcPath) != ERROR_NONE) {                   /* 检查文件路径有效性           */
        return  (LW_NULL);
    }

    pcPathDup = (PCHAR)TPS_ALLOC(lib_strlen(pcPath) + 1);
    if (LW_NULL == pcPathDup) {
        return  (LW_NULL);
    }
    lib_strcpy(pcPathDup, pcPath);
    
    pcPathRemain = pcPathDup;

    /*
     *  打开根目录
     */
    pentry = (PTPS_ENTRY)TPS_ALLOC(sizeof(TPS_ENTRY) + lib_strlen(PX_STR_DIVIDER) + 1);
    if (LW_NULL == pentry) {
        TPS_FREE(pcPathDup);
        return  (LW_NULL);
    }
    lib_bzero(pentry, sizeof(TPS_ENTRY) + lib_strlen(PX_STR_DIVIDER) + 1);
    
    pentry->ENTRY_offset = 0;
    pentry->ENTRY_psb    = psb;
    pentry->ENTRY_uiLen  = sizeof(TPS_ENTRY) + lib_strlen(PX_STR_DIVIDER) + 1;
    pentry->ENTRY_inum   = psb->SB_inumRoot;
    lib_strcpy(pentry->ENTRY_pcName, (PCHAR)PX_STR_DIVIDER);

    pentry->ENTRY_pinode = tpsFsOpenInode(pentry->ENTRY_psb, pentry->ENTRY_inum);
    if (LW_NULL == pentry->ENTRY_pinode) {
        TPS_FREE(pentry);
        TPS_FREE(pcPathDup);
        return  (LW_NULL);
    }
    pentry->ENTRY_pinodeDir = LW_NULL;

    /*
     *  目录查找
     */
    while (pcPathRemain) {
        while (*pcPathRemain == PX_DIVIDER) {
            pcPathRemain++;
        }
        pcFileName = pcPathRemain;

        if ((*pcFileName == 0) || !S_ISDIR(pentry->ENTRY_pinode->IND_iMode)) {
            break;
        }

        while ((*pcPathRemain) && (*pcPathRemain != PX_DIVIDER)) {
            pcPathRemain++;
        }
        if (*pcPathRemain != 0) {
            *pcPathRemain++ = 0;
        }

        pentrySub = tpsFsFindEntry(pentry->ENTRY_pinode, pcFileName);   /* 查找目录项                   */
        if (LW_NULL == pentrySub) {
            break;
        }
        tpsFsEntryFree(pentry);
        pentry = pentrySub;
    }

    TPS_FREE(pcPathDup);

    if (ppcRemain) {
        *ppcRemain = (PCHAR)pcPath + (pcFileName - pcPathDup);
    }

    return  (pentry);
}
/*********************************************************************************************************
** 函数名称: tpsFsOpen
** 功能描述: 打开文件
** 输　入  : psb              super block指针
**           pcPath           文件路径
**           iFlags           方式
**           iMode            文件模式
**           ppinode          用于输出inode结构指针
** 输　出  : 返回ERROR，成功ppinode输出inode结构指针，失败ppinode输出LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsOpen (PTPS_SUPER_BLOCK     psb,
                    CPCHAR               pcPath,
                    INT                  iFlags,
                    INT                  iMode,
                    PCHAR               *ppcRemain,
                    PTPS_INODE          *ppinode)
{
    PTPS_ENTRY pentry       = LW_NULL;
    PTPS_ENTRY pentryNew    = LW_NULL;
    CPCHAR     pcFileName   = LW_NULL;
    PCHAR      pcRemain     = LW_NULL;
    errno_t    iErr         = ERROR_NONE;

    if (LW_NULL == ppinode) {
        return  (EINVAL);
    }
    *ppinode = LW_NULL;

    if (LW_NULL == pcPath) {
        return  (EINVAL);
    }

    if (LW_NULL == psb) {
        return  (EFORMAT);
    }

    if (iFlags & (O_CREAT | O_TRUNC)) {
        if (!(psb->SB_uiFlags & TPS_MOUNT_FLAG_WRITE)) {
            return  (EROFS);
        }
    }

    if ((lib_strcmp(pcPath, PX_STR_DIVIDER) == 0) || (pcPath[0] == '\0')) {
        *ppinode = tpsFsOpenInode(psb, psb->SB_inumRoot);

        return  (*ppinode == LW_NULL ? EIO : ERROR_NONE);
    }

    pentry = __tpsFsWalkPath(psb, pcPath, &pcRemain);
    if (LW_NULL == pentry) {
        return  (ENOENT);
    }

    if (!S_ISLNK(pentry->ENTRY_pinode->IND_iMode) && (*pcRemain == 0)) {
        if (iFlags & O_EXCL) {                                          /* 以互斥方式打开               */
            tpsFsEntryFree(pentry);
            return  (EEXIST);
        }
    }

    if (S_ISLNK(pentry->ENTRY_pinode->IND_iMode) || (*pcRemain == 0)) {
        goto  entry_ok;
    }

    if (!(iFlags & O_CREAT)) {
        tpsFsEntryFree(pentry);
        return  (ENOENT);
    }

    pcFileName = lib_rindex(pcPath, PX_DIVIDER);                        /* 获取文件名                   */
    if (pcFileName) {
        pcFileName += 1;
    } else {
        pcFileName = pcPath;
    }

    if (pcRemain != pcFileName) {
        tpsFsEntryFree(pentry);
        return  (ENOENT);
    }

    pentryNew = __tpsFSCreate(pentry->ENTRY_pinode, pcFileName, iMode, &iErr);
    tpsFsEntryFree(pentry);
    pentry = pentryNew;
    if (LW_NULL == pentry) {
        return  (iErr);
    }

entry_ok:
    *ppinode = tpsFsOpenInode(psb, pentry->ENTRY_inum);
    tpsFsEntryFree(pentry);
    if (*ppinode == LW_NULL) {
        return  (EIO);
    }

    /*
     *  截断文件,只有当不是符号链接时才有效
     */
    if (!S_ISLNK((*ppinode)->IND_iMode)) {
        if (iFlags & O_TRUNC) {
            iErr = tpsFsTrunc(*ppinode, 0);
            if (iErr != ERROR_NONE) {
                tpsFsCloseInode(*ppinode);
                *ppinode = LW_NULL;
                return  (iErr);
            }
        }
    }

    if (ppcRemain) {
        *ppcRemain = pcRemain;
    }

    (*ppinode)->IND_iFlag = iFlags;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsRemove
** 功能描述: 删除entry
** 输　入  : psb              super block指针
**           pcPath           文件路径
** 输　出  : 成功返回0，失败返回错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsRemove (PTPS_SUPER_BLOCK psb, CPCHAR pcPath)
{
    PTPS_ENTRY pentry       = LW_NULL;
    PTPS_TRANS ptrans       = LW_NULL;
    PCHAR      pcRemain     = LW_NULL;

    if ((LW_NULL == psb) || (LW_NULL == pcPath)) {
        return  (EINVAL);
    }

    pentry = __tpsFsWalkPath(psb, pcPath, &pcRemain);
    if (LW_NULL == pentry) {
        return  (ENOENT);
    }

    if (*pcRemain != 0) {
        return  (ENOENT);
    }

    ptrans = tpsFsTransAllocAndInit(psb);                               /* 分配事物                     */
    if (ptrans == LW_NULL) {
        tpsFsEntryFree(pentry);
        return  (ENOMEM);
    }

    if (tpsFsInodeDelRef(ptrans,
                         pentry->ENTRY_pinode) != TPS_ERR_NONE) {       /* inode引用减1                 */
        tpsFsEntryFree(pentry);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsEntryRemove(ptrans, pentry) != TPS_ERR_NONE) {             /* 移除entry                    */
        tpsFsEntryFree(pentry);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    tpsFsEntryFree(pentry);

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* 提交事务                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsMove
** 功能描述: 移动entry
** 输　入  : psb              super block指针
**           pcPathSrc        源文件路径
**           pcPathDst        目标文件路径
** 输　出  : 成功返回0，失败返回错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsMove (PTPS_SUPER_BLOCK psb, CPCHAR pcPathSrc, CPCHAR pcPathDst)
{
    PTPS_ENTRY pentrySrc        = LW_NULL;
    PTPS_ENTRY pentryDst        = LW_NULL;
    PTPS_TRANS ptrans           = LW_NULL;
    CPCHAR     pcFileName       = LW_NULL;
    PCHAR      pcRemain         = LW_NULL;

    if ((LW_NULL == psb) || (LW_NULL == pcPathSrc) || (LW_NULL == pcPathDst)) {
        return  (EINVAL);
    }

    pentrySrc = __tpsFsWalkPath(psb, pcPathSrc, &pcRemain);
    if (LW_NULL == pentrySrc) {
        return  (ENOENT);
    }

    pentryDst = __tpsFsWalkPath(psb, pcPathDst, &pcRemain);
    if (LW_NULL == pentryDst) {
        tpsFsEntryFree(pentrySrc);
        return  (ENOENT);
    }

    if (*pcRemain == 0) {                                               /* 文件已存在                   */
        tpsFsEntryFree(pentryDst);
        tpsFsEntryFree(pentrySrc);
        return  (EEXIST);
    }

    pcFileName = lib_rindex(pcPathDst, PX_DIVIDER);                     /* 获取文件名                   */
    if (pcFileName) {
        pcFileName += 1;
    } else {
        pcFileName = pcPathDst;
    }

    if (pcRemain != pcFileName) {
        tpsFsEntryFree(pentryDst);
        tpsFsEntryFree(pentrySrc);
        return  (ENOENT);
    }

    ptrans = tpsFsTransAllocAndInit(psb);                               /* 分配事物                     */
    if (ptrans == LW_NULL) {
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        return  (ENOMEM);
    }

    if (tpsFsCreateEntry(ptrans, pentryDst->ENTRY_pinode,
                         pcFileName, pentrySrc->ENTRY_inum) != TPS_ERR_NONE) {
                                                                        /* 创建目录项并关联到inode      */
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsEntryRemove(ptrans, pentrySrc) != TPS_ERR_NONE) {          /* 移除entry                    */
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    tpsFsEntryFree(pentrySrc);
    tpsFsEntryFree(pentryDst);

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* 提交事务                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsLink
** 功能描述: 建立硬链接
** 输　入  : psb              super block指针
**           pcPathSrc        源文件路径
**           pcPathDst        目标文件路径
** 输　出  : 成功返回0，失败返回错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsLink (PTPS_SUPER_BLOCK psb, CPCHAR pcPathSrc, CPCHAR pcPathDst)
{
    PTPS_ENTRY pentrySrc        = LW_NULL;
    PTPS_ENTRY pentryDst        = LW_NULL;
    PTPS_TRANS ptrans           = LW_NULL;
    CPCHAR     pcFileName       = LW_NULL;
    PCHAR      pcRemain         = LW_NULL;

    if ((LW_NULL == psb) || (LW_NULL == pcPathSrc) || (LW_NULL == pcPathDst)) {
        return  (EINVAL);
    }

    pentrySrc = __tpsFsWalkPath(psb, pcPathSrc, &pcRemain);
    if (LW_NULL == pentrySrc) {
        return  (ENOENT);
    }

    pentryDst = __tpsFsWalkPath(psb, pcPathDst, &pcRemain);
    if (LW_NULL == pentryDst) {
        tpsFsEntryFree(pentrySrc);
        return  (ENOENT);
    }

    if (*pcRemain == 0) {                                               /* 文件已存在                   */
        tpsFsEntryFree(pentryDst);
        tpsFsEntryFree(pentrySrc);
        return  (EEXIST);
    }

    pcFileName = lib_rindex(pcPathDst, PX_DIVIDER);                     /* 获取文件名                   */
    if (pcFileName) {
        pcFileName += 1;
    } else {
        pcFileName = pcPathDst;
    }

    if (pcRemain != pcFileName) {
        tpsFsEntryFree(pentryDst);
        tpsFsEntryFree(pentrySrc);
        return  (ENOENT);
    }

    ptrans = tpsFsTransAllocAndInit(psb);                               /* 分配事物                     */
    if (ptrans == LW_NULL) {
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        return  (ENOMEM);
    }

    if (tpsFsCreateEntry(ptrans, pentryDst->ENTRY_pinode,
                         pcFileName, pentrySrc->ENTRY_inum) != TPS_ERR_NONE) {
                                                                        /* 创建目录项并关联到inode      */
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsInodeAddRef(ptrans,
                         pentrySrc->ENTRY_pinode) != ERROR_NONE) {      /* inode引用加1                 */
        tpsFsEntryFree(pentrySrc);
        tpsFsEntryFree(pentryDst);
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    tpsFsEntryFree(pentrySrc);
    tpsFsEntryFree(pentryDst);

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* 提交事务                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsRead
** 功能描述: 读取文件
** 输　入  : pinode           inode指针
**           off              起始位置
**           pucItemBuf       缓冲区
**           szLen            读取长度
**           pszRet           用于输出实际读取长度
** 输　出  : 返回ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsRead (PTPS_INODE   pinode,
                    PUCHAR       pucBuff,
                    TPS_OFF_T    off,
                    TPS_SIZE_T   szLen,
                    TPS_SIZE_T  *pszRet)
{
    errno_t  iErr;
    
    if (LW_NULL == pszRet) {
        return  (EINVAL);
    }

    *pszRet = 0;

    if ((LW_NULL == pinode) || (LW_NULL == pucBuff)) {
        return  (EINVAL);
    }

    *pszRet = tpsFsInodeRead(pinode, off, pucBuff, szLen);
    if (*pszRet < 0) {
        iErr = (errno_t)(-(*pszRet));
        return  (iErr);
    
    } else {
        return  (TPS_ERR_NONE);
    }
}
/*********************************************************************************************************
** 函数名称: tpsFsWrite
** 功能描述: 写文件
** 输　入  : pinode           inode指针
**           off              起始位置
**           pucBuff          缓冲区
**           szLen            读取长度
**           pszRet           用于输出实际写入长度
** 输　出  : 返回ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsWrite (PTPS_INODE  pinode,
                     PUCHAR      pucBuff,
                     TPS_OFF_T   off,
                     TPS_SIZE_T  szLen,
                     TPS_SIZE_T *pszRet)
{
    PTPS_TRANS  ptrans = LW_NULL;
    errno_t     iErr;

    if (LW_NULL == pszRet) {
        return  (EINVAL);
    }

    *pszRet = 0;

    if ((LW_NULL == pinode) || (LW_NULL == pucBuff)) {
        return  (EINVAL);
    }

    ptrans = tpsFsTransAllocAndInit(pinode->IND_psb);                   /* 分配事物                     */
    if (ptrans == LW_NULL) {
        return  (ENOMEM);
    }

    *pszRet = tpsFsInodeWrite(ptrans, pinode, off, pucBuff, szLen, LW_FALSE);
    if (*pszRet < 0) {
        tpsFsTransRollBackAndFree(ptrans);
        iErr = (errno_t)(-(*pszRet));
        return  (iErr);
    }

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* 提交事务                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsClose
** 功能描述: 关闭文件
** 输　入  : pinode           文件inode指针
** 输　出  : 成功返回0，失败返回错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsClose (PTPS_INODE pinode)
{
    PTPS_TRANS  ptrans      = LW_NULL;

    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    ptrans = tpsFsTransAllocAndInit(pinode->IND_psb);                   /* 分配事物                     */
    if (ptrans == LW_NULL) {
        return  (ENOMEM);
    }

    if (tpsFsFlushInodeHead(ptrans, pinode) != TPS_ERR_NONE) {
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* 提交事务                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (tpsFsCloseInode(pinode));
}
/*********************************************************************************************************
** 函数名称: tpsFsFlushHead
** 功能描述: 提交文件头属性修改
** 输　入  : pinode           文件inode指针
** 输　出  : 成功返回0，失败返回错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsFlushHead (PTPS_INODE pinode)
{
    PTPS_TRANS  ptrans  = LW_NULL;

    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    ptrans = tpsFsTransAllocAndInit(pinode->IND_psb);                   /* 分配事物                     */
    if (ptrans == LW_NULL) {
        return  (ENOMEM);
    }

    if (tpsFsFlushInodeHead(ptrans, pinode) != TPS_ERR_NONE) {
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* 提交事务                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsTrunc
** 功能描述: 截断文件
** 输　入  : pinode           inode指针
**           ui64Off          截断后的长度
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsTrunc (PTPS_INODE pinode, TPS_SIZE_T szNewSize)
{
    PTPS_TRANS  ptrans  = LW_NULL;
    errno_t     iErr    = ERROR_NONE;

    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    ptrans = tpsFsTransAllocAndInit(pinode->IND_psb);                   /* 分配事物                     */
    if (ptrans == LW_NULL) {
        return  (ENOMEM);
    }

    if (tpsFsTruncInode(ptrans, pinode, szNewSize) != TPS_ERR_NONE) {
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    if (tpsFsTransCommitAndFree(ptrans) != TPS_ERR_NONE) {              /* 提交事务                     */
        tpsFsTransRollBackAndFree(ptrans);
        return  (EIO);
    }

    return  (iErr);
}
/*********************************************************************************************************
** 函数名称: 创建目录
** 功能描述: 打开文件
** 输　入  : psb              super block指针
**           pcPath           文件路径
**           iFlags           方式
**           iMode            文件模式
** 输　出  : 成功返回inode结构指针，失败返回NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsMkDir (PTPS_SUPER_BLOCK psb, CPCHAR pcPath, INT iFlags, INT iMode)
{
    PTPS_INODE  pinode;
    errno_t     iErr    = ERROR_NONE;

    if ((LW_NULL == psb) || (LW_NULL == pcPath)) {
        return  (EINVAL);
    }

    iMode &= ~S_IFMT;

    iErr = tpsFsOpen(psb, pcPath, iFlags | O_CREAT,
                     iMode | S_IFDIR, LW_NULL, &pinode);
    if (iErr != ERROR_NONE) {
        return  (iErr);
    }

    return  (tpsFsClose(pinode));
}
/*********************************************************************************************************
** 函数名称: tpsFsOpenDir
** 功能描述: 打开目录
** 输　入  : psb              super block指针
**           pcPath           文件路径
**           ppdir            用于输出TPS_DIR指针
** 输　出  : 返回ERROR，成功ppdir输出TPS_DIR指针，失败ppdir输出LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsOpenDir (PTPS_SUPER_BLOCK psb, CPCHAR pcPath, PTPS_DIR *ppdir)
{
    errno_t        iErr    = ERROR_NONE;

    if (LW_NULL == ppdir) {
        return  (EINVAL);
    }

    *ppdir = LW_NULL;

    if ((LW_NULL == psb) || (LW_NULL == pcPath)) {
        return  (EINVAL);
    }

    *ppdir = (PTPS_DIR)TPS_ALLOC(sizeof(TPS_DIR));
    if (LW_NULL == *ppdir) {
        return  (ENOMEM);
    }

    iErr = tpsFsOpen(psb, pcPath, O_RDWR, 0, LW_NULL, &(*ppdir)->DIR_pinode);
    if (iErr != ERROR_NONE) {
        return  (iErr);
    }

    (*ppdir)->DIR_pentry = LW_NULL;
    (*ppdir)->DIR_offPos = 0;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsCloseDir
** 功能描述: 关闭目录
** 输　入  : pdir             TPS_DIR结构指针
** 输　出  : 错误码
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsCloseDir (PTPS_DIR pdir)
{
    if (LW_NULL == pdir) {
        return  (EINVAL);
    }

    if (pdir->DIR_pentry) {
        tpsFsEntryFree(pdir->DIR_pentry);
    }

    if (pdir->DIR_pinode) {
        tpsFsCloseInode(pdir->DIR_pinode);
    }

    TPS_FREE(pdir);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsReadDir
** 功能描述: 读取目录
** 输　入  : pdir             TPS_DIR结构指针
**           ppentry          用于输出entry指针
** 输　出  : 返回ERROR，成功ppentry输出entry指针，失败ppentry输出LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsReadDir (PTPS_DIR pdir, PTPS_ENTRY *ppentry)
{
    if (LW_NULL == ppentry) {
        return  (EINVAL);
    }

    *ppentry = LW_NULL;

    if ((LW_NULL == pdir) || (LW_NULL == pdir->DIR_pinode)) {
        return  (EINVAL);
    }

    if (pdir->DIR_pentry) {
        tpsFsEntryFree(pdir->DIR_pentry);
    }

    pdir->DIR_pentry = tpsFsEntryRead(pdir->DIR_pinode, pdir->DIR_offPos);
    *ppentry = pdir->DIR_pentry;

    if (pdir->DIR_pentry) {
        pdir->DIR_offPos = pdir->DIR_pentry->ENTRY_offset + pdir->DIR_pentry->ENTRY_uiLen;
    } else {
        return  (ENOENT);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsSync
** 功能描述: 同步文件
** 输　入  : pinode           inode指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsSync (PTPS_INODE pinode)
{
    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    if (tpsFsInodeSync(pinode) != TPS_ERR_NONE) {
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsVolSync
** 功能描述: 同步整个文件系统分区
** 输　入  : psb               超级块指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsVolSync (PTPS_SUPER_BLOCK psb)
{
    if (LW_NULL == psb) {
        return  (EINVAL);
    }

    if (tpsFsDevBufSync(psb,
                        psb->SB_ui64DataStartBlk,
                        0,
                        (size_t)(psb->SB_ui64DataBlkCnt << psb->SB_uiBlkShift)) != TPS_ERR_NONE) {
                                                                        /* 同步所有数据块               */
        return  (EIO);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsStat
** 功能描述: tpsfs 获得文件 stat
** 输　入  : psb              文件系统超级块
**           pinode           inode指针
**           pstat            获得的 stat
** 输　出  : 创建结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#ifndef WIN32

VOID  tpsFsStat (PTPS_SUPER_BLOCK  psb, PTPS_INODE  pinode, struct stat *pstat)
{
    if (pinode) {
        if (LW_NULL == pinode->IND_psb) {
            lib_memset(pstat, 0, sizeof(struct stat));

        } else {
            pstat->st_dev     = (dev_t)pinode->IND_psb->SB_dev;
            pstat->st_ino     = (ino_t)pinode->IND_inum;
            pstat->st_mode    = pinode->IND_iMode;
            pstat->st_nlink   = pinode->IND_uiRefCnt;
            pstat->st_uid     = 0;
            pstat->st_gid     = 0;
            pstat->st_rdev    = 1;
            pstat->st_size    = pinode->IND_szData;
            pstat->st_atime   = pinode->IND_ui64ATime;
            pstat->st_mtime   = pinode->IND_ui64MTime;
            pstat->st_ctime   = pinode->IND_ui64CTime;
            pstat->st_blksize = pinode->IND_psb->SB_uiBlkSize;
            pstat->st_blocks  = 0;
        }

    } else if (psb) {
        pstat->st_dev     = (dev_t)psb;
        pstat->st_ino     = (ino_t)psb->SB_inumRoot;
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;                                          /*  不支持                      */
        pstat->st_gid     = 0;                                          /*  不支持                      */
        pstat->st_rdev    = 1;                                          /*  不支持                      */
        pstat->st_atime   = (time_t)psb->SB_ui64Generation;
        pstat->st_mtime   = (time_t)psb->SB_ui64Generation;
        pstat->st_ctime   = (time_t)psb->SB_ui64Generation;
        pstat->st_blksize = psb->SB_uiBlkSize;
        pstat->st_blocks  = (blkcnt_t)psb->SB_ui64DataBlkCnt;

        pstat->st_mode = S_IFDIR;
        if (psb->SB_uiFlags & TPS_MOUNT_FLAG_READ) {
            pstat->st_mode |= S_IRUSR | S_IRGRP | S_IROTH;
        }
        if (psb->SB_uiFlags & TPS_MOUNT_FLAG_WRITE) {
            pstat->st_mode |= S_IWUSR | S_IWGRP | S_IWOTH;
        }
        
    } else {
        pstat->st_dev     = (dev_t)TPS_SUPER_MAGIC;
        pstat->st_ino     = (ino_t)TPS_SUPER_MAGIC;
        pstat->st_nlink   = 1;
        pstat->st_uid     = 0;                                          /*  不支持                      */
        pstat->st_gid     = 0;                                          /*  不支持                      */
        pstat->st_rdev    = 1;                                          /*  不支持                      */
        pstat->st_atime   = (time_t)TPS_UTC_TIME();
        pstat->st_mtime   = (time_t)TPS_UTC_TIME();
        pstat->st_ctime   = (time_t)TPS_UTC_TIME();
        pstat->st_blksize = 512;
        pstat->st_blocks  = (blkcnt_t)1;
        pstat->st_mode    = S_IFDIR | DEFAULT_DIR_PERM;
    }
}

#endif                                                                  /*  WIN32                       */
/*********************************************************************************************************
** 函数名称: tpsFsStatfs
** 功能描述: tpsfs 获得文件系统 statfs
** 输　入  : psb              文件系统超级块
**           pstatfs            获得的 statfs
** 输　出  : 创建结果
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  tpsFsStatfs (PTPS_SUPER_BLOCK  psb, struct statfs *pstatfs)
{
    if ((LW_NULL == psb) || (LW_NULL == pstatfs)) {
        return;
    }

    pstatfs->f_bsize  = (long)psb->SB_uiBlkSize;
    pstatfs->f_blocks = (long)psb->SB_ui64DataBlkCnt;
    pstatfs->f_bfree  = (long)tpsFsBtreeGetBlkCnt(psb->SB_pinodeSpaceMng);
    pstatfs->f_bavail = pstatfs->f_bfree;
}
/*********************************************************************************************************
** 函数名称: tpsFsGetSize
** 功能描述: 获取文件大小
** 输　入  : pinode           inode指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_SIZE_T  tpsFsGetSize (PTPS_INODE pinode)
{
    if (LW_NULL == pinode) {
        return  (0);
    }

    return  (pinode->IND_szData);
}
/*********************************************************************************************************
** 函数名称: tpsFsGetmod
** 功能描述: 获取文件模式
** 输　入  : pinode           inode指针
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  tpsFsGetmod (PTPS_INODE pinode)
{
    if (LW_NULL == pinode) {
        return  (0);
    }

    return  (pinode->IND_iMode);
}
/*********************************************************************************************************
** 函数名称: tpsFsChmod
** 功能描述: 修改文件模式
** 输　入  : pinode           inode指针
**           iMode            模式
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsChmod (PTPS_INODE pinode, INT iMode)
{
    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    iMode |= S_IRUSR;
    iMode &= ~S_IFMT;

    pinode->IND_iMode  &= S_IFMT;
    pinode->IND_iMode  |= iMode;
    pinode->IND_bDirty  = LW_TRUE;
    
    return  (tpsFsFlushHead(pinode));
}
/*********************************************************************************************************
** 函数名称: tpsFsChown
** 功能描述: 修改文件所有者
** 输　入  : pinode           inode指针
**           uid              用户id
**           gid              用户组id
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsChown (PTPS_INODE pinode, uid_t uid, gid_t gid)
{
    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    pinode->IND_iUid    = uid;
    pinode->IND_iGid    = gid;
    pinode->IND_bDirty  = LW_TRUE;
    
    return  (tpsFsFlushHead(pinode));
}
/*********************************************************************************************************
** 函数名称: tpsFsChtime
** 功能描述: 修改文件时间
** 输　入  : pinode           inode指针
**           utim             时间
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
errno_t  tpsFsChtime (PTPS_INODE pinode, struct utimbuf  *utim)
{
    if (LW_NULL == pinode) {
        return  (EINVAL);
    }

    pinode->IND_ui64ATime = utim->actime;
    pinode->IND_ui64MTime = utim->modtime;
    pinode->IND_bDirty    = LW_TRUE;
    
    return  (tpsFsFlushHead(pinode));
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
