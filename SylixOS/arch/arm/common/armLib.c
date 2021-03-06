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
** 文   件   名: armLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2013 年 12 月 09 日
**
** 描        述: ARM 体系构架内部库.
**
** BUG:
2015.09.07  当支持 CLZ 指令时, archFindLsb 与 archFindMsb 使用汇编实现.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
** 函数名称: archLsb
** 功能描述: find least significant bit set 寻找 32 位数中最低的 1 位
** 输　入  : ui32      32 位数
** 输　出  : 最低位为 1 的位置
**           正确返回 [1 ~ 32], 如果参数为全 0 则返回 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if __SYLIXOS_ARM_ARCH__ < 5

INT  archFindLsb (UINT32 ui32)
{
    UINT16  usMsw = (UINT16)(ui32 >> 16);
    UINT16  usLsw = (UINT16)(ui32 & 0xffff);
    UINT8   ucByte;

    if (ui32 == 0) {
        return (0);
    }

    if (usLsw) {
        ucByte = (UINT8)(usLsw & 0xff);
        if (ucByte) {                                                   /*  byte is bits [0:7]          */
            return (_K_ucLsbBitmap[ucByte] + 1);
        
        } else {                                                        /*  byte is bits [8:15]         */
            return (_K_ucLsbBitmap[(UINT8)(usLsw >> 8)] + 8 + 1);
        }
    
    } else {
        ucByte = (UINT8)(usMsw & 0xff);                                 /*  byte is bits [16:23]        */
        if (ucByte) {
            return (_K_ucLsbBitmap[ucByte] + 16 + 1);
        
        } else {                                                        /*  byte is bits [24:31]        */
            return (_K_ucLsbBitmap[(UINT8)(usMsw >> 8)] + 24 + 1);
        }
    }
}
/*********************************************************************************************************
** 函数名称: archMsb
** 功能描述: find most significant bit set 寻找 32 位数中最高的 1 位
** 输　入  : ui32      32 位数
** 输　出  : 最高位为 1 的位置
**           正确返回 [1 ~ 32], 如果参数为全 0 则返回 0
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  archFindMsb (UINT32 ui32)
{
    UINT16  usMsw = (UINT16)(ui32 >> 16);
    UINT16  usLsw = (UINT16)(ui32 & 0xffff);
    UINT8   ucByte;
    
    if (ui32 == 0) {
        return  (0);
    }
    
    if (usMsw) {
        ucByte = (UINT8)(usMsw >> 8);                                   /*  byte is bits [24:31]        */
        if (ucByte) {
            return (_K_ucMsbBitmap[ucByte] + 24 + 1);
        
        } else {
            return (_K_ucMsbBitmap[(UINT8)usMsw] + 16 + 1);
        }
    
    } else {
        ucByte = (UINT8)(usLsw >> 8);                                   /*  byte is bits [8:15]         */
        if (ucByte) {
            return (_K_ucMsbBitmap[ucByte] + 8 + 1);
        
        } else {
            return (_K_ucMsbBitmap[(UINT8)usLsw] + 1);
        }
    }
}

#endif                                                                  /*  __SYLIXOS_ARM_ARCH__ < 5    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
