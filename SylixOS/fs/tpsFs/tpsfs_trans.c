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
** 文   件   名: tpsfs_trans.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: 事物操作

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
** 函数名称: tspFsCheckTrans
** 功能描述: 检查事物完整性
** 输　入  : ptrans           事物
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tspFsCheckTrans (PTPS_SUPER_BLOCK psb)
{
    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tspFsCompleteTrans
** 功能描述: 标记事物为一致状态
** 输　入  : ptrans           事物
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tspFsCompleteTrans (PTPS_SUPER_BLOCK psb)
{
    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsTransAllocAndInit
** 功能描述: 分配并初始化事物
** 输　入  : ptrans           事物
** 输　出  : 事物对象指针
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PTPS_TRANS  tpsFsTransAllocAndInit (PTPS_SUPER_BLOCK psb)
{
    PTPS_TRANS  ptrans = LW_NULL;

    ptrans = (PTPS_TRANS)TPS_ALLOC(sizeof(TPS_TRANS));
    if (LW_NULL == ptrans) {
        return  (LW_NULL);
    }

    lib_bzero(ptrans, sizeof(TPS_TRANS));

    ptrans->TRANS_psb = psb;

    return  (ptrans);
}
/*********************************************************************************************************
** 函数名称: tpsFsTransRollBackAndFree
** 功能描述: 回滚并释放事物
** 输　入  : ptrans           事物
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsTransRollBackAndFree (PTPS_TRANS ptrans)
{
    TPS_FREE(ptrans);

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsTransCommitAndFree
** 功能描述: 提交并释放事物
** 输　入  : ptrans           事物
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsTransCommitAndFree (PTPS_TRANS ptrans)
{
    if (tpsFsBtreeAdjustBP(ptrans, ptrans->TRANS_psb) != TPS_ERR_NONE) {
        return  (TPS_ERR_BP_ADJUST);
    }

    TPS_FREE(ptrans);

    return  (TPS_ERR_NONE);
}
/*********************************************************************************************************
** 函数名称: tpsFsTransWrite
** 功能描述: 写入数据到事物
** 输　入  : ptrans           事物
**           psb              超级快指针
**           blk              块号
**           uiOff            块内偏移
**           pucBuff          数据缓冲区
**           szLen            写入长度
** 输　出  : ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
TPS_RESULT  tpsFsTransWrite (PTPS_TRANS        ptrans,
                             PTPS_SUPER_BLOCK  psb,
                             TPS_IBLK          blk,
                             UINT              uiOff,
                             PUCHAR            pucBuff,
                             size_t            szLen)
{
    return  (tpsFsDevBufWrite(psb, blk, uiOff,
                              pucBuff, szLen, LW_TRUE));
}

#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
