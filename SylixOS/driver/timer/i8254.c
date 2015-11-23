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
** 文   件   名: i8254.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2015 年 10 月 28 日
**
** 描        述: Intel 8254 定时器支持.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "../SylixOS/config/driver/drv_cfg.h"
/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if LW_CFG_DRV_TIMER_I8254 > 0
#include "i8254.h"
/*********************************************************************************************************
  Programmable Interrupt Timer Definitions
*********************************************************************************************************/
#define PIT_REG_COUNTER0        (pctl->iobase + 0)
#define PIT_REG_COUNTER1        (pctl->iobase + 1)
#define PIT_REG_COUNTER2        (pctl->iobase + 2)
#define PIT_REG_COMMAND         (pctl->iobase + 3)
/*********************************************************************************************************
  Programmable Interrupt Timer Clock
*********************************************************************************************************/
#define PIT_CLOCK               (pctl->qcofreq)
/*********************************************************************************************************
  PIT command bit defintions
*********************************************************************************************************/
#define PIT_OCW_BINCOUNT_BCD    (1 << 0)                                /*  vs binary                   */

#define PIT_OCW_MODE_SHIFT      (1)
#define PIT_OCW_MODE_MASK       (7 << PIT_OCW_MODE_SHIFT)
#  define PIT_OCW_MODE_TMCNT    (0 << PIT_OCW_MODE_SHIFT)               /*  Terminal count              */
#  define PIT_OCW_MODE_ONESHOT  (1 << PIT_OCW_MODE_SHIFT)               /*  One shot                    */
#  define PIT_OCW_MODE_RATEGEN  (2 << PIT_OCW_MODE_SHIFT)               /*  Rate gen                    */
#  define PIT_OCW_MODE_SQUARE   (3 << PIT_OCW_MODE_SHIFT)               /*  Square wave generation      */
#  define PIT_OCW_MODE_SWTRIG   (4 << PIT_OCW_MODE_SHIFT)               /*  Software trigger            */
#  define PIT_OCW_MODE_HWTRIG   (5 << PIT_OCW_MODE_SHIFT)               /*  Hardware trigger            */

#define PIT_OCW_RL_SHIFT        (4)
#define PIT_OCW_RL_MASK         (3 << PIT_OCW_RL_SHIFT)
#  define PIT_OCW_RL_LATCH      (0 << PIT_OCW_RL_SHIFT)
#  define PIT_OCW_RL_LSBONLY    (1 << PIT_OCW_RL_SHIFT)
#  define PIT_OCW_RL_MSBONLY    (2 << PIT_OCW_RL_SHIFT)
#  define PIT_OCW_RL_DATA       (3 << PIT_OCW_RL_SHIFT)

#define PIT_OCW_COUNTER_SHIFT   (6)
#define PIT_OCW_COUNTER_MASK    (3 << PIT_OCW_COUNTER_SHIFT)
#  define PIT_OCW_COUNTER_0     (0 << PIT_OCW_COUNTER_SHIFT)
#  define PIT_OCW_COUNTER_1     (1 << PIT_OCW_COUNTER_SHIFT)
#  define PIT_OCW_COUNTER_2     (2 << PIT_OCW_COUNTER_SHIFT)
/*********************************************************************************************************
  PIT command bit defintions
*********************************************************************************************************/
#define PIT_TICK_DIVISOR        ((UINT32)PIT_CLOCK / (UINT32)LW_TICK_HZ)
#define PIT_HTIMR_DIVISOR       ((UINT32)PIT_CLOCK / (UINT32)LW_HTIMER_HZ)
/*********************************************************************************************************
** 函数名称: i8254Init
** 功能描述: 初始化 8254 定时器
** 输　入  : pctl           8254 控制块
**           uiHz           初始化频率
** 输　出  : 初始计数值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT16  i8254Init (I8254_CTL *pctl, UINT32  uiHz)
{
    UINT32  uiDivisor = ((UINT32)PIT_CLOCK / uiHz);

    /*
     * Send the command byte to configure counter 0
     */
    out8(PIT_OCW_MODE_SQUARE | PIT_OCW_RL_DATA | PIT_OCW_COUNTER_0, PIT_REG_COMMAND);

    /*
     * Set the PIT input frequency divisor
     */
    out8((UINT8)(uiDivisor & 0xff),  PIT_REG_COUNTER0);
    out8((UINT8)((uiDivisor >> 8) & 0xff), PIT_REG_COUNTER0);

    return  ((UINT16)uiDivisor);
}
/*********************************************************************************************************
** 函数名称: i8254InitAsTick
** 功能描述: 初始化 8254 定时器, 设置频率为 tick.
** 输　入  : pctl           8254 控制块
** 输　出  : 初始计数值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT16  i8254InitAsTick (I8254_CTL *pctl)
{
    return  (i8254Init(pctl, LW_TICK_HZ));
}
/*********************************************************************************************************
** 函数名称: i8254InitAsHtimer
** 功能描述: 初始化 8254 定时器, 设置频率为 tick.
** 输　入  : pctl           8254 控制块
** 输　出  : 初始计数值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT16  i8254InitAsHtimer (I8254_CTL *pctl)
{
    return  (i8254Init(pctl, LW_HTIMER_HZ));
}
/*********************************************************************************************************
** 函数名称: i8254GetCnt
** 功能描述: 获得当前定时时间.
** 输　入  : pctl           8254 控制块
** 输　出  : 计数值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT16  i8254GetCnt (I8254_CTL *pctl)
{
    UINT16  usDivisor;

    /*
     * Send the command byte to configure get counter 0
     */
    out8((UINT8)((0x11 << 6) | (1 << 4) | (1 < 1)), PIT_REG_COMMAND);

    usDivisor  = (UINT16)in8(PIT_REG_COUNTER0);
    usDivisor |= ((UINT16)in8(PIT_REG_COUNTER0) << 8);

    return  (usDivisor);
}

#endif                                                                  /*  LW_CFG_DRV_INT_I8254 > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
