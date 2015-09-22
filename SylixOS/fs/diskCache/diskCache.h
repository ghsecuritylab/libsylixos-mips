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
** 文   件   名: diskCache.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 11 月 26 日
**
** 描        述: 磁盘高速缓冲控制器.
*********************************************************************************************************/

#ifndef __DISKCACHE_H
#define __DISKCACHE_H

/*********************************************************************************************************
  注意:
  
  如果使用 CACHE, 请保证与物理设备的一一对应, (多个逻辑分区将共享一个 CACHE)
  
  DISK CACHE 的使用:

  使用 DISK CACHE 的最大目的就是提高慢速块设备 IO 的访问效率, 利用系统内存作为数据的缓冲, 
  使用 DISK CACHE 的最大问题就在于磁盘数据与缓冲数据的不同步性, 同步数据可以调用 ioctl(..., FIOFLUSH, ...)
  实现.
  
  pblkDev = xxxBlkDevCreate(...);
  diskCacheCreate(pblkDev, ..., &pCacheBlk);
  ...(如果存在多磁盘分区, 详见: diskPartition.h)
  fatFsDevCreate(pVolName, pCacheBlk);
  
  ...操作设备
  
  umount(pVolName);
  ...(如果存在多磁盘分区, 详见: diskPartition.h)
  diskCacheDelete(pCacheBlk);
  xxxBlkDevDelete(pblkDev);
  
  推荐使用 oem 磁盘操作库.
*********************************************************************************************************/


/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKCACHE_EN > 0)

/*********************************************************************************************************
  ioctl 附加命令
*********************************************************************************************************/

#define LW_BLKD_DISKCACHE_GET_OPT       LW_OSIOR('b', 150, INT)         /*  获取 CACHE 选项             */
#define LW_BLKD_DISKCACHE_SET_OPT       LW_OSIOD('b', 151, INT)         /*  设置 CACHE 选项             */
#define LW_BLKD_DISKCACHE_INVALID       LW_OSIO( 'b', 152)              /*  使 CACHE 回写并全部不命中   */
#define LW_BLKD_DISKCACHE_RAMFLUSH      LW_OSIOD('b', 153, ULONG)       /*  随机回写一些脏扇区          */

/*********************************************************************************************************
  操作参数宏
  
  硬件如有 DMA 操作不允许旁路 DISK CACHE, 因为 DISK CACHE 针对 CPU CACHE 拥有相关硬件对齐性质.
*********************************************************************************************************/

#define LW_DISKCACHE_OPT_DISABLE        0x0                             /*  设置 CACHE 旁路             */
#define LW_DISKCACHE_OPT_ENABLE         0x1                             /*  设置 CACHE 有效             */

/*********************************************************************************************************
  API
*********************************************************************************************************/
LW_API ULONG  API_DiskCacheCreate(PLW_BLK_DEV   pblkdDisk, 
                                  PVOID         pvDiskCacheMem, 
                                  size_t        stMemSize, 
                                  INT           iMaxBurstSector,
                                  PLW_BLK_DEV  *ppblkDiskCache);
                                  
LW_API INT    API_DiskCacheDelete(PLW_BLK_DEV   pblkdDiskCache);

#define diskCacheCreate     API_DiskCacheCreate
#define diskCacheDelete     API_DiskCacheDelete

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKCACHE_EN > 0)   */
#endif                                                                  /*  __DISKCACHE_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
