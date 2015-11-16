/**********************************************************************************************************
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
** 文   件   名: mipsBacktrace.c
**
** 创   建   人: Ryan.Xin (信金龙)
**
** 文件创建日期: 2013 年 12 月 09 日
**
** 描        述: MIPS 体系构架堆栈回溯 (来源于 glibc).
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  Only GCC support now.
*********************************************************************************************************/
#ifdef   __GNUC__
#include "mipsBacktrace.h"
/*********************************************************************************************************
  This implementation assumes a stack layout that matches the defaults
  used by gcc's `__builtin_frame_address' and `__builtin_return_address'
  (FP is the frame pointer register):

        +-----------------+     +-----------------+
  FP -> | previous FP     |---->| previous FP     |---->...
        |                 |     |                 |
        | return address  |     | return address  |
        +-----------------+     +-----------------+
*********************************************************************************************************/
/*********************************************************************************************************
  Get some notion of the current stack.  Need not be exactly the top
  of the stack, just something somewhere in the current frame.
*********************************************************************************************************/
#ifndef CURRENT_STACK_FRAME
#define CURRENT_STACK_FRAME         ({ char __csf; &__csf; })
#endif
/*********************************************************************************************************
  By default we assume that the stack grows downward.
*********************************************************************************************************/
#ifndef INNER_THAN
#define INNER_THAN                  <
#endif
/*********************************************************************************************************
  By default assume the `next' pointer in struct layout points to the
  next struct layout.
*********************************************************************************************************/
#ifndef ADVANCE_STACK_FRAME
#define ADVANCE_STACK_FRAME(next)   BOUNDED_1((struct layout *) (next))
#endif
/*********************************************************************************************************
  By default, the frame pointer is just what we get from gcc.
*********************************************************************************************************/
#ifndef FIRST_FRAME_POINTER
#define FIRST_FRAME_POINTER         __builtin_frame_address(0)
#endif
/*********************************************************************************************************
** 函数名称: getEndStack
** 功能描述: 获得堆栈结束地址
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PVOID  getEndStack (VOID)
{
    PLW_CLASS_TCB  ptcbCur;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    return  ((PVOID)ptcbCur->TCB_pstkStackTop);
}

/*********************************************************************************************************
** 函数名称: backtrace
** 功能描述: 获得当前任务调用栈
** 输　入  : array     获取数组
**           size      数组大小
** 输　出  : 获取的数目
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
int  backtrace (void **array, int size)
{
#if 1
    struct layout *current;
    void *__unbounded top_frame;
    void *__unbounded top_stack;
    void *__unbounded end_stack;
    int cnt = 0;

    top_frame = FIRST_FRAME_POINTER;
    top_stack = CURRENT_STACK_FRAME;
    end_stack = getEndStack();

    /*
     * We skip the call to this function, it makes no sense to record it.
     */
    current = BOUNDED_1((struct layout *)top_frame);
    current = ADJUST_FRAME_POINTER(current);

    while (cnt < size) {
        if ((void *) current INNER_THAN top_stack || !((void *) current INNER_THAN end_stack)) {
            /*
             * This means the address is out of range.  Note that for the
             * toplevel we see a frame pointer with value NULL which clearly is
             * out of range.
             */
            break;
        }
        array[cnt++] = current->return_address;

        current = ADVANCE_STACK_FRAME(current->next);
        current = ADJUST_FRAME_POINTER(current);
    }

    return cnt;
#else
#define ABS(X) ((X)>=0?(X):(-(X)))
    ULONG *pulFuncAddr;
    ULONG *pulRAAddr;
    ULONG *pulSPAddr;
    UINT  uiRAOffset    = 0;
    UINT  uiStatckSize  = 0;
    UINT  uiCnt         = 0;

    if (!array || size < 0) {
        return uiCnt;
    }

    /*
     * 获取当前RA和SP Register的Value
     */
    MIPS_EXEC_INS("move %0," MIPS_RA : "=r"(pulRAAddr));
    MIPS_EXEC_INS("move %0," MIPS_SP : "=r"(pulSPAddr));

    /*
     * backtrace 是LEAF Function, 所以RA不会被占用, 因此不会将RA入栈
     * 不用去找RA在SP中的offset值, 因此RA中的值就是call backtrace的下一条指令
     */
    /*
     * 从当前函数的起始地址找堆栈大小
     */
    for (pulFuncAddr = (ULONG *)backtrace; ; ++pulFuncAddr) {
        /*
         * 0x2fbd is "addiu sp,sp", 指令是为函数开辟堆栈
         */
        if ((ULONG)(*pulFuncAddr & 0xffff0000) == 0x27bd0000) {
            /*
             * 取出堆栈大小
             * mips堆栈是负增长
             */
            uiStatckSize = ABS((INT16)(*pulFuncAddr&0xffff));
            if (uiStatckSize) {
                break;
            }
        }else if ((ULONG)*pulFuncAddr == 0x3e00008) {
            /*
             * 0x3e00008 is "jr ra"
             * 发现返回指令，说明已经找到头了，退出查找
             */
            break;
        }
    }

    /*
     * 找到了backtrace使用堆栈的大小, 就可以算出backtrace调用者的堆栈指针
     */
    pulSPAddr =(ULONG *)((ULONG)pulSPAddr + uiStatckSize);

    /*
     * 进行backtrace搜索
     */
    for (uiCnt = 0; uiCnt < size && pulRAAddr; uiCnt++) {
        array[uiCnt] = pulRAAddr;

        uiRAOffset = uiStatckSize = 0;

        for (pulFuncAddr = pulRAAddr; uiRAOffset == 0 || uiStatckSize == 0; pulFuncAddr--) {
            /*
             * Get Instruction
             */
            switch (*pulFuncAddr&0xffff0000) {
            /*
             * 找到开辟堆栈的指令, 保存堆栈的值
             */
            case 0x27bd0000:
                uiStatckSize = ABS((INT16)(*pulFuncAddr&0xffff));
                break;

            /*
             * 0xafbf 是"sw ra (XX)sp"，这里就是ra存放的偏移地址
             */
            case 0xafbf0000:
                uiRAOffset = (INT16)(*pulFuncAddr&0xffff);
                break;

            /*
             * 0x3c1c 是"lui gp 找到C/C++ 函数的最后一层, 停止backtrace
             */
            case 0x3c1c0000:
                return uiCnt + 1;
                break;

            default:
                break;

            }
        }

        /*
         * 设置上一层调用者的调用本层函数的返回地址(ra的地址在上一层函数中)和堆栈地址
         */
        pulRAAddr =(ULONG *)((ULONG)pulRAAddr + uiRAOffset);
        pulSPAddr =(ULONG *)((ULONG)pulSPAddr + uiStatckSize);
    }

    return uiCnt;
#endif
}
#endif                                                                  /*  __GNUC__                    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
