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
** ��   ��   ��: ramDisk.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 09 �� 26 ��
**
** ��        ��: RAM DISK ��������.

** BUG:
2009.11.03  ��ʼ��ʱ BLKD_bDiskChange Ϊ LW_FALSE.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0
/*********************************************************************************************************
  �ڲ����Ͷ���
*********************************************************************************************************/
typedef struct {
    LW_BLK_DEV      RAMD_blkdRam;                                       /*  ���豸                      */
    PVOID           RAMD_pvMem;                                         /*  �ڴ����ַ                  */
} LW_RAM_DISK;
typedef LW_RAM_DISK *PLW_RAM_DISK;
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
static INT   __ramDiskReset(PLW_RAM_DISK  pramd);
static INT   __ramDiskStatusChk(PLW_RAM_DISK  pramd);
static INT   __ramDiskIoctl(PLW_RAM_DISK  pramd, INT  iCmd, LONG  lArg);
static INT   __ramDiskWrt(PLW_RAM_DISK  pramd, 
                          VOID         *pvBuffer, 
                          ULONG         ulStartSector, 
                          ULONG         ulSectorCount);
static INT   __ramDiskRd(PLW_RAM_DISK  pramd, 
                         VOID         *pvBuffer, 
                         ULONG         ulStartSector, 
                         ULONG         ulSectorCount);
/*********************************************************************************************************
** ��������: API_RamDiskCreate
** ��������: ����һ���ڴ���.
** �䡡��  : pvDiskAddr        �����ڴ���ʼ��
**           ullDiskSize       ���̴�С
**           ppblkdRam         �ڴ����������ƿ��ַ
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API  
ULONG  API_RamDiskCreate (PVOID  pvDiskAddr, UINT64  ullDiskSize, PLW_BLK_DEV  *ppblkdRam)
{
    REGISTER PLW_RAM_DISK       pramd;
    REGISTER PLW_BLK_DEV        pblkd;
    
    if (ullDiskSize < LW_CFG_MB_SIZE) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    if (!pvDiskAddr || !ppblkdRam) {
        _ErrorHandle(EINVAL);
        return  (EINVAL);
    }
    
    pramd = (PLW_RAM_DISK)__SHEAP_ALLOC(sizeof(LW_RAM_DISK));
    if (!pramd) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (ERROR_SYSTEM_LOW_MEMORY);
    }
    lib_bzero(pramd, sizeof(LW_RAM_DISK));
    
    pblkd = &pramd->RAMD_blkdRam;
    
    pblkd->BLKD_pfuncBlkRd        = __ramDiskRd;
    pblkd->BLKD_pfuncBlkWrt       = __ramDiskWrt;
    pblkd->BLKD_pfuncBlkIoctl     = __ramDiskIoctl;
    pblkd->BLKD_pfuncBlkReset     = __ramDiskReset;
    pblkd->BLKD_pfuncBlkStatusChk = __ramDiskStatusChk;
    pblkd->BLKD_ulNSector         = (ULONG)(ullDiskSize / 512ul);
    pblkd->BLKD_ulBytesPerSector  = 512ul;
    pblkd->BLKD_ulBytesPerBlock   = 512ul;
    pblkd->BLKD_bRemovable        = LW_FALSE;
    pblkd->BLKD_bDiskChange       = LW_FALSE;
    pblkd->BLKD_iRetry            = 1;
    pblkd->BLKD_iFlag             = O_RDWR;
    
    pblkd->BLKD_iLogic            = 0;                                  /*  �����豸                    */
    pblkd->BLKD_uiLinkCounter     = 0;
    pblkd->BLKD_pvLink            = LW_NULL;
    
    pblkd->BLKD_uiPowerCounter    = 0;
    pblkd->BLKD_uiInitCounter     = 0;
    
    pramd->RAMD_pvMem = pvDiskAddr;

    *ppblkdRam = &pramd->RAMD_blkdRam;                                  /*  ������ƿ�                  */
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "ram disk size : 0x%lx base : 0x%lx has been create.\r\n",
                 (ULONG)ullDiskSize, (addr_t)pvDiskAddr);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_RamDiskDelete
** ��������: ɾ��һ���ڴ���. 
** �䡡��  : pblkdRam          �ڴ����������ƿ�
** �䡡��  : �������.
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : ɾ���ڴ���֮ǰ����Ҫʹ�� remove ����ж�ؾ�, 
             ����:
                    BLK_DEV   *pblkdRam;
                    
                    ramDiskCreate(..., &pblkdRam);
                    fatFsDevCreate("/ram0", pblkdRam);
                    ...
                    unlink("/ram0");
                    ramDiskDelete(pblkdRam);
                    
                                           API ����
*********************************************************************************************************/
LW_API  
INT  API_RamDiskDelete (PLW_BLK_DEV  pblkdRam)
{
    if (pblkdRam) {
        __SHEAP_FREE(pblkdRam);
        return  (ERROR_NONE);
    
    } else {
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __ramDiskReset
** ��������: ��λһ���ڴ���.
** �䡡��  : pramd             �ڴ��̿��ƿ�
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ramDiskReset (PLW_RAM_DISK  pramd)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramDiskStatusChk
** ��������: ���һ���ڴ���.
** �䡡��  : pramd             �ڴ��̿��ƿ�
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ramDiskStatusChk (PLW_RAM_DISK  pramd)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramDiskIoctl
** ��������: ����һ���ڴ���.
** �䡡��  : pramd             �ڴ��̿��ƿ�
**           iCmd              ��������
**           lArg              ���Ʋ���
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ramDiskIoctl (PLW_RAM_DISK  pramd, INT  iCmd, LONG  lArg)
{
    switch (iCmd) {
    
    /*
     *  ����Ҫ֧�ֵ� 4 ������
     */
    case FIOSYNC:
    case FIOFLUSH:                                                      /*  ������д�����              */
        break;
    
    case FIOUNMOUNT:                                                    /*  ж�ؾ�                      */
        break;
        
    case FIODISKINIT:                                                   /*  ��ʼ������                  */
        break;
    
    /*
     *  �ͼ���ʽ��
     */    
    case FIODISKFORMAT:                                                 /*  ��ʽ����                    */
        return  (PX_ERROR);                                             /*  ��֧�ֵͼ���ʽ��            */
    
    /*
     *  FatFs ��չ����
     */
    case LW_BLKD_CTRL_POWER:
    case LW_BLKD_CTRL_LOCK:
    case LW_BLKD_CTRL_EJECT:
        break;
    
    default:
        _ErrorHandle(ERROR_IO_UNKNOWN_REQUEST);
        return  (PX_ERROR);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramDiskWrt
** ��������: дһ���ڴ���.
** �䡡��  : pramd             �ڴ��̿��ƿ�
**           pvBuffer          ������
**           ulStartSector     ��ʼ������
**           ulSectorCount     ��������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ramDiskWrt (PLW_RAM_DISK  pramd, 
                          VOID         *pvBuffer, 
                          ULONG         ulStartSector, 
                          ULONG         ulSectorCount)
{
    REGISTER PBYTE      pucStartMem = ((PBYTE)pramd->RAMD_pvMem
                                    + (ulStartSector * 512));
                                    
    lib_memcpy(pucStartMem, pvBuffer, (INT)(512 * ulSectorCount));
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __ramDiskRd
** ��������: ��һ���ڴ���.
** �䡡��  : pramd             �ڴ��̿��ƿ�
**           pvBuffer          ������
**           ulStartSector     ��ʼ������
**           ulSectorCount     ��������
** �䡡��  : 0
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __ramDiskRd (PLW_RAM_DISK  pramd, 
                         VOID         *pvBuffer, 
                         ULONG         ulStartSector, 
                         ULONG         ulSectorCount)
{
    REGISTER PBYTE      pucStartMem = ((PBYTE)pramd->RAMD_pvMem
                                    + (ulStartSector * 512));
                                    
    lib_memcpy(pvBuffer, pucStartMem, (INT)(512 * ulSectorCount));
    
    return  (ERROR_NONE);
}
#endif                                                                  /*  LW_CFG_MAX_VOLUMES          */
/*********************************************************************************************************
  END
*********************************************************************************************************/