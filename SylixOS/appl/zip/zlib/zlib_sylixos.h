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
** 文   件   名: zlib_sylixos.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 01 月 16 日
**
** 描        述: 操作系统 zlib shell 注册接口.
*********************************************************************************************************/

#ifndef __ZLIB_SYLIXOS_H
#define __ZLIB_SYLIXOS_H

/*********************************************************************************************************
  注意, zlib 对堆栈的消耗非常大, 所以使用时一定要加大堆栈
*********************************************************************************************************/

VOID    zlibShellInit(VOID);
#define zlibShellRegister        zlibShellInit

#endif                                                                  /*  __ZLIB_SYLIXOS_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
