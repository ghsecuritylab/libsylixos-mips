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
** 文   件   名: dualportmem.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 04 月 10 日
**
** 描        述: 系统支持共享内存式多处理器双口内存管理头文件

** BUG
2007.09.12  加入裁剪支持。
*********************************************************************************************************/

#ifndef  __DUALPORTMEM_H
#define  __DUALPORTMEM_H

#if LW_CFG_MPI_EN > 0

/*********************************************************************************************************
  DUAL PORT MEMORY CONTRL
*********************************************************************************************************/

typedef struct {
    LW_LIST_MONO               DPMA_monoResrcList;                      /*  资源链表                    */
    
    PVOID                      DPMA_pvInternalBase;                     /*  内部基地址                  */
    PVOID                      DPMA_pvExternalBase;                     /*  外部基地址                  */
    size_t                     DPMA_stByteLength;                       /*  内存区大小                  */
    
    UINT16                     DPMA_usIndex;                            /*  控制块索引                  */
    CHAR                       DPMA_cDpmaName[LW_CFG_OBJECT_NAME_SIZE]; /*  名字                        */
} LW_CLASS_DPMA;
typedef LW_CLASS_DPMA         *PLW_CLASS_DPMA;

/*********************************************************************************************************
  VAR
*********************************************************************************************************/

#ifdef  __DPMA_MAIN_FILE
#define __DPMA_EXT
#else
#define __DPMA_EXT    extern
#endif

/*********************************************************************************************************
  DPMA 缓冲区
*********************************************************************************************************/

__DPMA_EXT  LW_CLASS_DPMA          _G_dpmaBuffer[LW_CFG_MAX_MPDPMAS];   /*  控制块缓冲区                */
__DPMA_EXT  LW_CLASS_OBJECT_RESRC  _G_resrcDpma;                        /*  对象资源结构                */

/*********************************************************************************************************
  INTERNAL FUNCTION
*********************************************************************************************************/

VOID            _DpmaInit(VOID);
PLW_CLASS_DPMA  _Allocate_Dpma_Object(VOID);
VOID            _Free_Dpma_Object(PLW_CLASS_DPMA  pdpma);

/*********************************************************************************************************
  INLINE FUNCTION
*********************************************************************************************************/

static LW_INLINE INT  _Dpma_Index_Invalid (UINT16    usIndex)
{
    return  (usIndex >= LW_CFG_MAX_MPDPMAS);
}

#endif                                                                  /*  LW_CFG_MPI_EN               */
#endif                                                                  /*  __DUALPORTMEM_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
