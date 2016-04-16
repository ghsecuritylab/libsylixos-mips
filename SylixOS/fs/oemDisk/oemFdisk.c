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
** 文   件   名: oemFdisk.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2015 年 09 月 25 日
**
** 描        述: OEM 磁盘分区工具.
**
** 注        意: 分区操作目标为 /dev/blk/xxx 块设备文件.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
#include "../SylixOS/fs/include/fs_internal.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_OEMDISK_EN > 0
/*********************************************************************************************************
  分区表中重要信息地点
*********************************************************************************************************/
#define __DISK_PART_TYPE                    0x4
#define __DISK_PART_STARTSECTOR             0x8
#define __DISK_PART_NSECTOR                 0xc
/*********************************************************************************************************
** 函数名称: __oemFdisk
** 功能描述: 对 OEM 磁盘设备进行分区操作
** 输　入  : iBlkFd             块设备文件描述符
**           ulSecSize          扇区大小
**           ulTotalSec         扇区总数
**           fdpPart            分区信息
**           uiNPart            分区个数
** 输　出  : 分区个数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __oemFdisk (INT                     iBlkFd,
                        ULONG                   ulSecSize,
                        ULONG                   ulTotalSec,
                        const LW_OEMFDISK_PART  fdpPart[],
                        UINT                    uiNPart)
{
#ifndef MBR_Table
#define MBR_Table           446                                         /* MBR: Partition table offset  */
#endif

    UINT              i, n;
    UINT8            *pucSecBuf;
    UINT8            *pucPartEntry;

    UINT8             ucHdStart, ucHdEnd;
    UINT              uiSzCyl, uiTotCyl;
    UINT              uiBCyl, uiECyl, uiPCyl;
    UINT32            uiPStartSec, uiPNSec;

    pucSecBuf = (UINT8 *)__SHEAP_ALLOC((size_t)ulSecSize);
    if (pucSecBuf == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    if (pread(iBlkFd, pucSecBuf, (size_t)ulSecSize, 0) != (ssize_t)ulSecSize) {
        __SHEAP_FREE(pucSecBuf);
        return  (PX_ERROR);
    }

    for (n = 16; n < 256 && ulTotalSec / n / 63 > 1024; n *= 2);
    if (n == 256) {
        n--;
    }

    ucHdEnd  = n - 1;
    uiSzCyl  = 63 * n;
    uiTotCyl = ulTotalSec / uiSzCyl;

    uiBCyl       = 0;
    pucPartEntry = &pucSecBuf[MBR_Table];

    lib_bzero(pucPartEntry, 16 * 4);

    for (i = 0; i < uiNPart; i++, pucPartEntry += 16) {
        if (fdpPart[i].FDP_bIsActive) {
            pucPartEntry[0] = 0x80;
        } else {
            pucPartEntry[0] = 0x00;
        }

        pucPartEntry[4] = fdpPart[i].FDP_ucPartType;

        if (fdpPart[i].FDP_ucSzPct == 0) {
            uiPCyl = uiTotCyl - uiBCyl;
        } else {
            uiPCyl = (UINT)uiTotCyl * fdpPart[i].FDP_ucSzPct / 100;
        }

        if (!uiPCyl) {
            continue;
        }

        uiPStartSec = uiSzCyl * uiBCyl;
        uiPNSec     = uiSzCyl * uiPCyl;

        if (i == 0) {
            ucHdStart    = 1;
            uiPStartSec += 63;
            uiPNSec     -= 63;
        } else {
            ucHdStart    = 0;
        }

        uiECyl = uiBCyl + uiPCyl - 1;
        if (uiECyl >= uiTotCyl) {
            __SHEAP_FREE(pucSecBuf);
            _ErrorHandle(ENOSPC);
            return  (PX_ERROR);
        }

        pucPartEntry[1] = ucHdStart;                                    /*  Start head                  */
        pucPartEntry[2] = (BYTE)((uiBCyl >> 2) + 1);                    /*  Start sector                */
        pucPartEntry[3] = (BYTE)uiBCyl;                                 /*  Start cylinder              */

        pucPartEntry[5] = ucHdEnd;                                      /*  End head                    */
        pucPartEntry[6] = (BYTE)((uiECyl >> 2) + 63);                   /*  End sector                  */
        pucPartEntry[7] = (BYTE)uiECyl;                                 /*  End cylinder                */

        BLK_ST_DWORD(pucPartEntry + 8, uiPStartSec);                    /*  Start sector in LBA         */
        BLK_ST_DWORD(pucPartEntry + 12, uiPNSec);                       /*  Partition size              */

        uiBCyl += uiPCyl;                                               /*  Next partition              */
        if (uiBCyl > uiTotCyl) {
            break;
        }
    }

    pucSecBuf[ulSecSize - 2] = 0x55;
    pucSecBuf[ulSecSize - 1] = 0xaa;

    if (pwrite(iBlkFd, pucSecBuf, (size_t)ulSecSize, 0) != (ssize_t)ulSecSize) {
        __SHEAP_FREE(pucSecBuf);
        return  (PX_ERROR);
    }

    __SHEAP_FREE(pucSecBuf);
    return  ((INT)i);
}
/*********************************************************************************************************
** 函数名称: API_OemFdiskGet
** 功能描述: 对 OEM 磁盘设备进行分区操作
** 输　入  : pcBlkDev           块设备文件 例如: /dev/blk/sata0
**           fdpPart            分区参数
**           uiNPart            分区参数个数
** 输　出  : 产生的分区个数, PX_ERROR 表示错误.
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_OemFdisk (CPCHAR  pcBlkDev, const LW_OEMFDISK_PART  fdpPart[], UINT  uiNPart)
{
    INT         i;
    INT         iBlkFd;
    INT         iNCnt;
    struct stat statGet;
    ULONG       ulSecSize;
    ULONG       ulTotalSec;
    UINT8       ucTotalPct = 0;

    if (!pcBlkDev || !fdpPart || !uiNPart || (uiNPart > 4)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (geteuid()) {
        _ErrorHandle(EACCES);
        return  (PX_ERROR);
    }

    iBlkFd = open(pcBlkDev, O_RDWR);
    if (iBlkFd < 0) {
        return  (PX_ERROR);
    }

    if (fstat(iBlkFd, &statGet) ||
        !S_ISBLK(statGet.st_mode)) {
        close(iBlkFd);
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    if (ioctl(iBlkFd, LW_BLKD_GET_SECNUM, &ulTotalSec)) {
        close(iBlkFd);
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    if (ioctl(iBlkFd, LW_BLKD_GET_SECSIZE, &ulSecSize)) {
        ulSecSize = 512;
    }

    for (i = 0; i < uiNPart; i++) {
        ucTotalPct += fdpPart[i].FDP_ucSzPct;
        if (!fdpPart[i].FDP_ucSzPct) {
            i++;
            break;
        }
    }

    if (ucTotalPct > 100) {
        close(iBlkFd);
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    uiNPart = i;
    iNCnt   = __oemFdisk(iBlkFd, ulSecSize, ulTotalSec, fdpPart, uiNPart);
    if (iNCnt > 0) {
        fsync(iBlkFd);
    }
    close(iBlkFd);

    return  (iNCnt);
}
/*********************************************************************************************************
** 函数名称: __oemFdiskGet
** 功能描述: 获取 OEM 磁盘设备分区情况
** 输　入  : iBlkFd             块设备文件描述符
**           ulSecStart         起始扇区
**           ulSecExtStart      扩展起始扇区
**           ulSecSize          扇区大小
**           uiNCnt             分区计数
**           fdpInfo            获取信息缓冲
**           uiNPart            缓冲区可保存分区信息个数
** 输　出  : 获取的分区个数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __oemFdiskGet (INT                  iBlkFd,
                           ULONG                ulSecStart,
                           ULONG                ulSecExtStart,
                           ULONG                ulSecSize,
                           UINT                 uiNCnt,
                           LW_OEMFDISK_PINFO    fdpInfo[],
                           UINT                 uiNPart)
{
    INT                 i;
    UINT8              *pucSecBuf;
    off_t               oft = ((off_t)ulSecSize * ulSecStart);
    PLW_OEMFDISK_PINFO  pfdpInfo;
    BYTE                ucActiveFlag;
    BYTE                ucPartType;
    INT                 iPartInfoStart;
    INT                 iRet;
    ULONG               ulSSec;
    ULONG               ulNSec;

    pucSecBuf = (UINT8 *)__SHEAP_ALLOC((size_t)ulSecSize);
    if (pucSecBuf == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    if (pread(iBlkFd, pucSecBuf, (size_t)ulSecSize, oft) != (ssize_t)ulSecSize) {
        __SHEAP_FREE(pucSecBuf);
        return  (PX_ERROR);
    }

    if ((pucSecBuf[ulSecSize - 2] != 0x55) ||
        (pucSecBuf[ulSecSize - 1] != 0xaa)) {                           /*  扇区结束标志是否正确        */
        __SHEAP_FREE(pucSecBuf);
        _ErrorHandle(EFORMAT);
        return  (PX_ERROR);
    }

    for (i = 0; i < 4; i++) {                                           /*  查看各个分区信息            */
        iPartInfoStart = MBR_Table + (i * 16);                          /*  获取分区信息起始点          */

        ucActiveFlag = pucSecBuf[iPartInfoStart];                       /*  激活标志                    */
        ucPartType   = pucSecBuf[iPartInfoStart + __DISK_PART_TYPE];    /*  分区文件系统类型            */

        if (LW_DISK_PART_IS_VALID(ucPartType)) {
            pfdpInfo = &fdpInfo[uiNCnt];                                /*  逻辑分区信息                */
            ulSSec   = BLK_LD_DWORD(&pucSecBuf[iPartInfoStart + __DISK_PART_STARTSECTOR]);
            ulNSec   = BLK_LD_DWORD(&pucSecBuf[iPartInfoStart + __DISK_PART_NSECTOR]);

            if (ucActiveFlag == 0x80) {
                pfdpInfo->FDP_bIsActive = LW_TRUE;
            } else {
                pfdpInfo->FDP_bIsActive = LW_FALSE;
            }

            pfdpInfo->FDP_ucPartType = ucPartType;
            pfdpInfo->FDP_u64Size    = ((UINT64)ulNSec * ulSecSize);
            pfdpInfo->FDP_u64Oft     = ((UINT64)ulSSec * ulSecSize);

            uiNCnt++;

        } else if (LW_DISK_PART_IS_EXTENDED(ucPartType)) {
            if (ulSecStart == 0ul) {                                    /*  是否位于主分区表            */
                ulSecExtStart = BLK_LD_DWORD(&pucSecBuf[iPartInfoStart + __DISK_PART_STARTSECTOR]);
                ulSecStart    = ulSecExtStart;
            } else {                                                    /*  位于扩展分区分区表          */
                ulSecStart  = BLK_LD_DWORD(&pucSecBuf[iPartInfoStart + __DISK_PART_STARTSECTOR]);
                ulSecStart += ulSecExtStart;
            }

            iRet = __oemFdiskGet(iBlkFd, ulSecStart, ulSecExtStart,
                                 ulSecSize, uiNCnt, fdpInfo, uiNPart);
            if (iRet < 0) {
                __SHEAP_FREE(pucSecBuf);
                return  ((INT)uiNCnt);
            } else {
                uiNCnt = (UINT)iRet;
            }
        }

        if (uiNCnt >= uiNPart) {                                        /*  不能再保存更多分区信息      */
            __SHEAP_FREE(pucSecBuf);
            return  ((INT)uiNCnt);
        }
    }

    __SHEAP_FREE(pucSecBuf);
    if (uiNCnt == 0) {                                                  /*  没有检测到任何分区          */
        return  (PX_ERROR);
    } else {
        return  ((INT)uiNCnt);                                          /*  检测到的分区数量            */
    }
}
/*********************************************************************************************************
** 函数名称: API_OemFdiskGet
** 功能描述: 获取 OEM 磁盘设备分区情况
** 输　入  : pcBlkDev           块设备文件 例如: /dev/blk/sata0
**           fdpInfo            获取信息缓冲
**           uiNPart            缓冲区可保存分区信息个数
** 输　出  : 获取的分区个数, PX_ERROR 表示错误.
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_OemFdiskGet (CPCHAR  pcBlkDev, LW_OEMFDISK_PINFO  fdpInfo[], UINT  uiNPart)
{
    INT         iBlkFd;
    INT         iNCnt;
    struct stat statGet;
    ULONG       ulSecSize;

    if (!pcBlkDev || !fdpInfo || !uiNPart) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iBlkFd = open(pcBlkDev, O_RDONLY);
    if (iBlkFd < 0) {
        return  (PX_ERROR);
    }

    if (fstat(iBlkFd, &statGet) ||
        !S_ISBLK(statGet.st_mode)) {
        close(iBlkFd);
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    if (ioctl(iBlkFd, LW_BLKD_GET_SECSIZE, &ulSecSize)) {
        ulSecSize = 512;
    }

    iNCnt = __oemFdiskGet(iBlkFd, 0, 0, ulSecSize, 0, fdpInfo, uiNPart);
    close(iBlkFd);

    return  (iNCnt);
}
/*********************************************************************************************************
** 函数名称: __oemFdiskGetType
** 功能描述: 获得磁盘分区类型字段
** 输　入  : ucType             分区类型
** 输　出  : 分区类型字串
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static CPCHAR  __oemFdiskGetType (UINT8  ucType)
{
    switch (ucType) {

    case 0x00:  return  ("Empty (NULL) Partition");
    case 0x01:  return  ("MSDOS Partition 12-bit FAT");
    case 0x02:  return  ("XENIX / (slash) Partition");
    case 0x03:  return  ("XENIX /usr Partition");
    case 0x04:  return  ("MSDOS 16-bit FAT <32M Partition");
    case 0x05:  return  ("MSDOS Extended Partition");
    case 0x06:  return  ("MSDOS 16-bit FAT >=32M Partition");
    case 0x07:  return  ("HPFS / NTFS Partition");
    case 0x08:  return  ("AIX boot or SplitDrive Partition");
    case 0x09:  return  ("AIX data or Coherent Partition");
    case 0x0a:  return  ("OS/2 Boot Manager Partition");
    case 0x0b:  return  ("Win95 FAT32 Partition");
    case 0x0c:  return  ("Win95 FAT32 (LBA) Partition");
    case 0x0e:  return  ("Win95 FAT16 (LBA) Partition");
    case 0x0f:  return  ("Win95 Extended (LBA) Partition");
    case 0x10:  return  ("OPUS Partition");
    case 0x11:  return  ("Hidden DOS FAT12 Partition");
    case 0x12:  return  ("Compaq diagnostics Partition");
    case 0x14:  return  ("Hidden DOS FAT16 Partition");
    case 0x16:  return  ("Hidden DOS FAT16 (big) Partition");
    case 0x17:  return  ("Hidden HPFS/NTFS Partition");
    case 0x18:  return  ("AST Windows swapfile Partition");
    case 0x24:  return  ("NEC DOS Partition");
    case 0x3c:  return  ("PartitionMagic recovery Partition");
    case 0x40:  return  ("Venix 80286 Partition");
    case 0x41:  return  ("Linux/MINIX (shared with DRDOS) Partition");
    case 0x42:  return  ("SFS or Linux swap part (shared with DRDOS)");
    case 0x43:  return  ("Linux native (shared with DRDOS) Partition");
    case 0x50:  return  ("DM (disk manager) Partition");
    case 0x51:  return  ("DM6 Aux1 (or Novell) Partition");
    case 0x52:  return  ("CP/M or Microport SysV/AT Partition");
    case 0x53:  return  ("DM6 Aux3 Partition");
    case 0x54:  return  ("DM6 Partition");
    case 0x55:  return  ("EZ-Drive (disk manager) Partition");
    case 0x56:  return  ("Golden Bow (disk manager) Partition");
    case 0x5c:  return  ("Priam Edisk (disk manager) Partition");
    case 0x61:  return  ("SpeedStor Partition");
    case 0x63:  return  ("GNU HURD or Mach or Sys V/386 (ISC UNIX)");
    case 0x64:  return  ("Novell Netware 286 Partition");
    case 0x65:  return  ("Novell Netware 386 Partition");
    case 0x70:  return  ("DiskSecure Multi-Boot Partition");
    case 0x75:  return  ("PC/IX Partition");
    case 0x77:  return  ("QNX4.x Partition");
    case 0x78:  return  ("QNX4.x 2nd part Partition");
    case 0x79:  return  ("QNX4.x 3rd part Partition");
    case 0x80:  return  ("MINIX until 1.4a Partition");
    case 0x81:  return  ("MINIX / old Linux Partition");
    case 0x82:  return  ("Linux swap Partition");
    case 0x83:  return  ("Linux native Partition");
    case 0x84:  return  ("OS/2 hidden C: drive Partition");
    case 0x85:  return  ("Linux extended Partition");
    case 0x86:  return  ("NTFS volume set Partition");
    case 0x87:  return  ("NTFS volume set Partition");
    case 0x93:  return  ("Amoeba Partition");
    case 0x94:  return  ("Amoeba bad block table Partition");
    case 0x96:  return  ("ISO-9660 file system");
    case 0x9c:  return  ("SylixOS True Power Safe Partition");
    case 0xa0:  return  ("IBM Thinkpad hibernation Partition");
    case 0xa5:  return  ("BSD/386 Partition");
    case 0xa7:  return  ("NeXTSTEP 486 Partition");
    case 0xb1:  return  ("QNX6.x Partition");
    case 0xb2:  return  ("QNX6.x 2nd part Partition");
    case 0xb3:  return  ("QNX6.x 3rd part Partition");
    case 0xb7:  return  ("BSDI fs Partition");
    case 0xb8:  return  ("BSDI swap Partition");
    case 0xc1:  return  ("DRDOS/sec (FAT-12) Partition");
    case 0xc4:  return  ("DRDOS/sec (FAT-16, < 32M) Partition");
    case 0xc6:  return  ("DRDOS/sec (FAT-16, >= 32M) Partition");
    case 0xc7:  return  ("Syrinx Partition");
    case 0xdb:  return  ("CP/M-Concurrent CP/M-Concurrent DOS-CTOS");
    case 0xe1:  return  ("DOS access-SpeedStor 12-bit FAT ext.");
    case 0xe3:  return  ("DOS R/O or SpeedStor Partition");
    case 0xe4:  return  ("SpeedStor 16-bit FAT Ext Part. < 1024 cyl.");
    case 0xf1:  return  ("SpeedStor Partition");
    case 0xf2:  return  ("DOS 3.3+ secondary Partition");
    case 0xf4:  return  ("SpeedStor large partition Partition");
    case 0xfe:  return  ("SpeedStor >1024 cyl. or LANstep Partition");
    case 0xff:  return  ("Xenix Bad Block Table Partition");

    default:    break;
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_OemFdiskShow
** 功能描述: 打印 OEM 磁盘设备分区情况
** 输　入  : pcBlkDev           块设备文件 例如: /dev/blk/sata0
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_OemFdiskShow (CPCHAR  pcBlkDev)
{
    static PCHAR         pcPartInfo = "\n"\
    "PART ACT  SIZE(KB)  OFFSET(KB)             TYPE\n"
    "---- --- ---------- ---------- -------------------------------------\n";

    PLW_OEMFDISK_PINFO   fdpInfo;
    INT                  iRet;
    INT                  i;
    PCHAR                pcAct;
    CPCHAR               pcType;

    fdpInfo = (PLW_OEMFDISK_PINFO)__SHEAP_ALLOC(sizeof(LW_OEMFDISK_PINFO) *
                                                LW_CFG_MAX_DISKPARTS);
    if (fdpInfo == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    iRet = API_OemFdiskGet(pcBlkDev, fdpInfo, LW_CFG_MAX_DISKPARTS);
    if (iRet < ERROR_NONE) {
        fprintf(stderr, "can not analysis partition table : %s\n", lib_strerror(errno));
        __SHEAP_FREE(fdpInfo);
        return  (iRet);

    } else if (iRet == 0) {
        __SHEAP_FREE(fdpInfo);
        fprintf(stderr, "block device : %s do not have MBR partition table.\n", pcBlkDev);
        return  (iRet);
    }

    printf("block device : %s partition >>\n%s", pcBlkDev, pcPartInfo);
    for (i = 0; i < iRet; i++) {
        pcAct  = (fdpInfo[i].FDP_bIsActive) ? "*" : "";
        pcType = __oemFdiskGetType(fdpInfo[i].FDP_ucPartType);
        if (pcType) {
            printf("%4d  %1s  %10llu %10llu %s\n",
                   i, pcAct,
                   (fdpInfo[i].FDP_u64Size >> 10),
                   (fdpInfo[i].FDP_u64Oft >> 10),
                   pcType);
        } else {
            printf("%4d  %1s  %10llu %10llu 0x%02x\n",
                   i, pcAct,
                   (fdpInfo[i].FDP_u64Size >> 10),
                   (fdpInfo[i].FDP_u64Oft >> 10),
                   fdpInfo[i].FDP_ucPartType);
        }
    }

    printf("\ntotal partition %d\n", iRet);

    __SHEAP_FREE(fdpInfo);

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_OEMDISK_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
