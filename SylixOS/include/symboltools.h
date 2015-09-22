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
** 文   件   名: symboltools.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2010 年 02 月 26 日
**
** 描        述: 系统符号表生成头文件 (用户任何程序不要引用此头文件).
*********************************************************************************************************/

#ifndef __SYMBOLTOOLS_H
#define __SYMBOLTOOLS_H

/*********************************************************************************************************
  类型
*********************************************************************************************************/
#define LW_SYMBOL_DATA              0x80000000 | 0x00000001 | 0x00000002
#define LW_SYMBOL_TEXT              0x80000000 | 0x00000001 | 0x00000004

/*********************************************************************************************************
  结构 (与 LW_SYMBOL 所占的内存结构相同)
*********************************************************************************************************/
typedef struct __symbol_dump_list {
    void                *DUMPLIST_pv1;
    void                *DUMPLIST_pv2;
} __SYMBOL_DUMP_LIST;

typedef struct lw_static_symbol {
    __SYMBOL_DUMP_LIST   LWSSYMBOL_dumplist;
    char                *LWSSYMBOL_pcName;
    char                *LWSSYMBOL_pcAddr;
    int                  LWSSYMBOL_iFlag;
} LW_STATIC_SYMBOL;

#endif                                                                  /*  __SYMBOLTOOLS_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
