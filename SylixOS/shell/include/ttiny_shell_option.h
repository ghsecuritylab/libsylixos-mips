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
** 文   件   名: ttniy_shell_option.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 07 月 27 日
**
** 描        述: 这是 ttniy_shell 启动选项定义.
*********************************************************************************************************/

#ifndef __TTINY_SHELL_OPTION_H
#define __TTINY_SHELL_OPTION_H

/*********************************************************************************************************
  ttniy_shell 启动参数
*********************************************************************************************************/
#define LW_OPTION_TSHELL_VT100          0x00000001                      /*  使用 VT100 中断控制字符     */

#define LW_OPTION_TSHELL_AUTHEN         0x80000000                      /*  使用用户认证                */
#define LW_OPTION_TSHELL_NOLOGO         0x40000000                      /*  是否不显示 logo             */
#define LW_OPTION_TSHELL_NOECHO         0x20000000                      /*  无回显                      */

#define LW_OPTION_TSHELL_PROMPT_FULL    0x10000000                      /*  全部显示命令提示符          */

#define LW_OPTION_TSHELL_CLOSE_FD       0x08000000                      /*  shell 退出时关闭 fd         */

/*********************************************************************************************************
  keyword 高级创建选项
*********************************************************************************************************/

#define LW_OPTION_KEYWORD_SYNCBG        0x00000001                      /*  此命令必须使用同步背景执行  */
                                                                        /*  在另外一个任务上下文中执行  */
                                                                        /*  并且等待执行结束            */
                                                                        
#define LW_OPTION_KEYWORD_ASYNCBG       0x00000002                      /*  此命令必须使用同步背景执行  */
                                                                        /*  在另外一个任务上下文中执行  */
                                                                        
#endif                                                                  /*  __TTINY_SHELL_OPTION_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
