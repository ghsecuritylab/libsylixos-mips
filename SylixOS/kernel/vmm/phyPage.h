/*********************************************************************************************************
**
**                                    �й�������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: phyPage.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 12 �� 09 ��
**
** ��        ��: ����ҳ�����.
*********************************************************************************************************/

#ifndef __PHYPAGE_H
#define __PHYPAGE_H

/*********************************************************************************************************
  �����ռ����
*********************************************************************************************************/

ULONG           __vmmPhysicalCreate(ULONG  ulZoneIndex, addr_t  ulAddr, size_t  stSize, UINT  uiAttr);
PLW_VMM_PAGE    __vmmPhysicalPageAlloc(ULONG  ulPageNum, UINT  uiAttr, ULONG  *pulZoneIndex);
PLW_VMM_PAGE    __vmmPhysicalPageAllocZone(ULONG  ulZoneIndex, ULONG  ulPageNum, UINT  uiAttr);
PLW_VMM_PAGE    __vmmPhysicalPageAllocAlign(ULONG   ulPageNum, 
                                            size_t  stAlign, 
                                            UINT    uiAttr, 
                                            ULONG  *pulZoneIndex);
PLW_VMM_PAGE    __vmmPhysicalPageClone(PLW_VMM_PAGE  pvmpage);
PLW_VMM_PAGE    __vmmPhysicalPageRef(PLW_VMM_PAGE  pvmpage);
VOID            __vmmPhysicalPageFree(PLW_VMM_PAGE  pvmpage);
VOID            __vmmPhysicalPageFreeAll(PLW_VMM_PAGE  pvmpageVirtual);
VOID            __vmmPhysicalPageSetFlag(PLW_VMM_PAGE  pvmpage, ULONG  ulFlag);
VOID            __vmmPhysicalPageSetFlagAll(PLW_VMM_PAGE  pvmpageVirtual, ULONG  ulFlag);

#if LW_CFG_CACHE_EN > 0
VOID            __vmmPhysicalPageFlush(PLW_VMM_PAGE  pvmpage);
VOID            __vmmPhysicalPageFlushAll(PLW_VMM_PAGE  pvmpageVirtual);
VOID            __vmmPhysicalPageInvalidate(PLW_VMM_PAGE  pvmpage);
VOID            __vmmPhysicalPageInvalidateAll(PLW_VMM_PAGE  pvmpageVirtual);
VOID            __vmmPhysicalPageClear(PLW_VMM_PAGE  pvmpage);
VOID            __vmmPhysicalPageClearAll(PLW_VMM_PAGE  pvmpageVirtual);
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

ULONG           __vmmPhysicalGetZone(addr_t  ulAddr);
ULONG           __vmmPhysicalPageGetMinContinue(ULONG  *pulZoneIndex, UINT  uiAttr);

#endif                                                                  /*  __PHYPAGE_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/