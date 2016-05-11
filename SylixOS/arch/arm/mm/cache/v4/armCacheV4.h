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
** 文   件   名: armCacheV4.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 09 日
**
** 描        述: ARMv4 体系构架 CACHE 驱动.
*********************************************************************************************************/

#ifndef __ARMCACHEV4_H
#define __ARMCACHEV4_H

VOID  armCacheV4Init(LW_CACHE_OP *pcacheop, 
                     CACHE_MODE   uiInstruction, 
                     CACHE_MODE   uiData, 
                     CPCHAR       pcMachineName);
                      
VOID  armCacheV4Reset(CPCHAR  pcMachineName);

#endif                                                                  /*  __ARMCACHEV4_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
