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
** 文   件   名: tpsfs_trans.h
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2015 年 9 月 21 日
**
** 描        述: 事物声明

** BUG:
*********************************************************************************************************/

#ifndef __TPSFS_TRANS_H
#define __TPSFS_TRANS_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_TPSFS_EN > 0

/*********************************************************************************************************
  扇区事物数据
*********************************************************************************************************/
typedef struct tps_trans_data {
    UINT                TD_uiSecAreaCnt;
    struct {
        UINT64          TD_ui64SecStart;
        UINT64          TD_ui64SecCnt;
    } TD_secareaArr[1];
} TPS_TRANS_DATA;
typedef TPS_TRANS_DATA  *PTPS_TRANS_DATA;

/*********************************************************************************************************
  事物结构
*********************************************************************************************************/

typedef struct tps_trans {
    UINT                 TRANS_uiMagic;
    UINT                 TRANS_uiGeneration;
    UINT64               TRANS_ui64SerialNum;
    INT                  TRANS_iType;
    INT                  TRANS_iStatus;                                 /* 事物状态                     */
    UINT64               TRANS_ui64Time;                                /* 修改时间                     */
    UINT64               TRANS_uiDataSec;
    UINT                 TRANS_uiDataLen;
    UINT64               TRANS_ui64Reserved[2];                         /* 保留2                        */
    UINT                 TRANS_uiCheckSum;

    struct tps_trans    *TRANS_pnext;                                   /* 事物列表指针                 */
    PTPS_SUPER_BLOCK     TRANS_psb;                                     /* 超级块指针                   */
    PTPS_TRANS_DATA      TRANS_pdata;
    PUCHAR               TRANS_pucSecData;
} TPS_TRANS;
typedef TPS_TRANS       *PTPS_TRANS;

/*********************************************************************************************************
  事务操作
*********************************************************************************************************/
                                                                        /* 检查事物完整性               */
TPS_RESULT tspFsCheckTrans(PTPS_SUPER_BLOCK psb);
                                                                        /* 标记事物为一致状态           */
TPS_RESULT tspFsCompleteTrans(PTPS_SUPER_BLOCK psb);
                                                                        /* 分配事物                     */
PTPS_TRANS tpsFsTransAllocAndInit(PTPS_SUPER_BLOCK psb);
                                                                        /* 回滚事物                     */
TPS_RESULT tpsFsTransRollBackAndFree(PTPS_TRANS ptrans);
                                                                        /* 提交事务                     */
TPS_RESULT tpsFsTransCommitAndFree(PTPS_TRANS ptrans);
                                                                        /* 写入数据到事物               */
TPS_RESULT tpsFsTransWrite(PTPS_TRANS ptrans, PTPS_SUPER_BLOCK psb,
                           TPS_IBLK blk, UINT uiOff,
                           PUCHAR pucBuff, size_t szLen);

#endif                                                                  /* LW_CFG_TPSFS_EN > 0          */
#endif                                                                  /* __TPSFS_TRANS_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
