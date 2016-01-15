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
** 文   件   名: pciAutoCfg.h
**
** 创   建   人: Gong.YuJian (弓羽箭)
**
** 文件创建日期: 2015 年 12 月 15 日
**
** 描        述: PCI 总线自动配置.
**
*********************************************************************************************************/

#ifndef __PCIAUTOCFG_H
#define __PCIAUTOCFG_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_PCI_EN > 0)
/*********************************************************************************************************
  调试模式
*********************************************************************************************************/
#undef PCI_AUTO_CFG_DEBUG
/*********************************************************************************************************
  配置类型选择 (_STATIC 与 _RECLAIM 二选一)
*********************************************************************************************************/
#define PCI_AUTO_CFG_LIST_STATIC                                        /* 静态配置                     */
#undef  PCI_AUTO_CFG_LIST_RECLAIM                                       /* 动态可回收                   */
#ifndef PCI_AUTO_CFG_LIST_MAX
#define PCI_AUTO_CFG_LIST_MAX               32                          /* 列表大小                     */
#endif

#if defined(PCI_AUTO_CFG_LIST_RECLAIM)
#if defined(PCI_AUTO_CFG_LIST_STATIC)
#error "PCI_AUTO_CFG_LIST_RECLAIM and PCI_AUTO_CFG_LIST_STATIC A choice In pciAutoCfg.h"
#endif                                                                  /* PCI_AUTO_CFG_LIST_STATIC     */
#endif                                                                  /* PCI_AUTO_CFG_LIST_RECLAIM    */
/*********************************************************************************************************
  PCI_LOC 属性
*********************************************************************************************************/
#define PCI_AUTO_CFG_ATTR_DEV_EXCLUDE			        (UINT8)(0x01) 	/* 设备移除          			*/
#define PCI_AUTO_CFG_ATTR_DEV_DISPLAY			        (UINT8)(0x02)   /* 显示设备           			*/
#define PCI_AUTO_CFG_ATTR_DEV_PREFETCH			        (UINT8)(0x04)   /* 设备支持预取     			*/

#define PCI_AUTO_CFG_ATTR_BUS_PREFETCH			        (UINT8)(0x08) 	/* 预取              			*/
#define PCI_AUTO_CFG_ATTR_BUS_PCI				        (UINT8)(0x10)   /* PCI 桥            			*/
#define PCI_AUTO_CFG_ATTR_BUS_HOST				        (UINT8)(0x20)   /* PCI 主桥          			*/
#define PCI_AUTO_CFG_ATTR_BUS_ISA				        (UINT8)(0x40) 	/* PCI ISA 桥          			*/
#define PCI_AUTO_CFG_ATTR_BUS_4GB_IO			        (UINT8)(0x80)   /* 4G/64K IO 地址   			*/

/*********************************************************************************************************
  选项命令
*********************************************************************************************************/
#define PCI_AUTO_CFG_CTL_STRUCT_COPY                    0x0000			/* 拷贝指定结构				    */

#define PCI_AUTO_CFG_CTL_FBB_ENABLE						0x0001          /* FBB 使能                     */
#define PCI_AUTO_CFG_CTL_FBB_DISABLE					0x0002          /* FBB 禁能                     */
#define PCI_AUTO_CFG_CTL_FBB_UPDATE						0x0003          /* FBB 更新                     */
#define PCI_AUTO_CFG_CTL_FBB_STATUS_GET					0x0004          /* FBB状态获取                  */

#define PCI_AUTO_CFG_CTL_MAX_LAT_FUNC_SET			    0x0008          /* 最大等待时间函数设置         */
#define PCI_AUTO_CFG_CTL_MAX_LAT_ARG_SET			    0x0009          /* 最大等待时间函数参数设置     */
#define PCI_AUTO_CFG_CTL_MAX_LAT_ALL_SET				0x000a          /* 最大等待时间设置             */
#define PCI_AUTO_CFG_CTL_MAX_LAT_ALL_GET				0x000b          /* 最大等待时间获取             */

#define PCI_AUTO_CFG_CTL_MSG_LOG_FUNC_SET		        0x000c          /* 日志信息函数设置             */

#define PCI_AUTO_CFG_CTL_MAX_BUS_SET					0x0010          /* 最大总线数设置               */
#define PCI_AUTO_CFG_CTL_MAX_BUS_GET					0x0011          /* 最大总线数获取               */
#define PCI_AUTO_CFG_CTL_CACHE_SIZE_SET					0x0012          /* 设置高速缓存大小             */
#define PCI_AUTO_CFG_CTL_CACHE_SIZE_GET					0x0013          /* 获取高速缓冲大小             */
#define PCI_AUTO_CFG_CTL_AUTO_INT_ROUTE_SET				0x0014          /* 中断路由使能                 */
#define PCI_AUTO_CFG_CTL_AUTO_INT_ROUTE_GET				0x0015          /* 中断路由状态                 */
#define PCI_AUTO_CFG_CTL_MEM32_LOC_SET					0x0016          /* 32 位可预取空间地址          */
#define PCI_AUTO_CFG_CTL_MEM32_SIZE_SET					0x0017          /* 32 位可预取空间大小          */
#define PCI_AUTO_CFG_CTL_MEMIO32_LOC_SET		    	0x0018          /* 32 位不可预取空间地址        */
#define PCI_AUTO_CFG_CTL_MEMIO32_SIZE_SET				0x0019          /* 32 位不可预取空间大小        */
#define PCI_AUTO_CFG_CTL_IO32_LOC_SET					0x001a          /* 32 位 IO 空间地址            */
#define PCI_AUTO_CFG_CTL_IO32_SIZE_SET					0x001b          /* 32 位 IO 空间大小            */
#define PCI_AUTO_CFG_CTL_IO16_LOC_SET					0x001c          /* 16 位 IO 空间地址            */
#define PCI_AUTO_CFG_CTL_IO16_SIZE_SET					0x001d          /* 16 位 IO 空间大小            */
#define PCI_AUTO_CFG_CTL_INCLUDE_FUNC_SET				0x001e          /* 包含设备函数设置             */
#define PCI_AUTO_CFG_CTL_INT_ASSIGN_FUNC_SET	        0x001f          /* 中断分配函数设置             */
#define PCI_AUTO_CFG_CTL_BRIDGE_PRE_CONFIG_FUNC_SET		0x0020          /* 桥设备准备配置函数设置       */
#define PCI_AUTO_CFG_CTL_BRIDGE_POST_CONFIG_FUNC_SET    0x0021          /* 桥设备结束配置函数设置       */
#define PCI_AUTO_CFG_CTL_ROLLCALL_FUNC_SET				0x0022          /* 探测函数设置                 */

#define PCI_AUTO_CFG_CTL_MEM32_SIZE_GET					0x0030          /* 获取 32 位可预取空间大小     */
#define PCI_AUTO_CFG_CTL_MEMIO32_SIZE_GET				0x0031          /* 获取 32 位不可预取空间大小   */
#define PCI_AUTO_CFG_CTL_IO32_SIZE_GET					0x0032          /* 获取 32 位 IO 空间大小       */
#define PCI_AUTO_CFG_CTL_IO16_SIZE_GET					0x0033          /* 获取 16 位 IO 空间大小       */

#define PCI_AUTO_CFG_CTL_TEMP_SPACE_SET					0x0200          /* 暂未使用                     */
#define PCI_AUTO_CFG_CTL_MINIMIZE_RESOURCES				0x0201          /* 暂未使用                     */

/*********************************************************************************************************
  内存信息控制块
*********************************************************************************************************/
typedef struct {
	PVOID		        PACM_pvMemAddr;                                 /* 内存起始地址                 */
    INT			        PACM_iMemSize;                                  /* 内存大小                     */
} PCI_AUTO_CFG_MEM;
/*********************************************************************************************************
  设备信息控制块
*********************************************************************************************************/
typedef struct {
    INT 		        PACL_iBus;                                      /* 总线号                       */
    INT 		        PACL_iDevice;                                   /* 设备号, 等同于槽号           */
    INT 		        PACL_iFunction;                                 /* 功能号                       */
    INT 		        PACL_iAttribute;                                /* 设备特性信息                 */
    INT 		        PACL_iOffset;                                   /* 中断路由                     */
} PCI_AUTO_CFG_LOC;
/*********************************************************************************************************
  设备 ID 控制块
*********************************************************************************************************/
typedef struct {
    PCI_AUTO_CFG_LOC    PACI_paclLoc;                                   /* 设备信息                     */
    UINT32 		        PACI_uiDevVend;                                 /* 厂商及设备 ID                */
} PCI_AUTO_CFG_ID;

typedef struct pci_auto_cfg_system {
    INT                 PACS_iIndex;                                    /* 配置索引                     */
    UINT32              PACS_uiBusMax;                                  /* 最大总线数                   */

    UINT32 		        PACS_uiMem32Addr;						        /* 32 位可预取空间地址		    */
    UINT32 		        PACS_uiMem32Size;					            /* 32 位可预取空间大小		    */
    UINT32 		        PACS_uiMemIo32Addr;						        /* 32 位不可预取空间地址	 	*/
    UINT32 		        PACS_uiMemIo32Size;						        /* 32 位不可预取空间大小 		*/
    
    UINT32 		        PACS_uiIo32Addr;							    /* 32 位 IO 空间地址	 		*/
    UINT32 		        PACS_uiIo32Size;							    /* 32 位 IO 空间大小			*/
    UINT32 		        PACS_uiIo16Addr;							    /* 16 位 IO 空间地址 			*/
    UINT32 		        PACS_uiIo16Size;							    /* 16 位 IO 空间大小 			*/

    INT 		        PACS_iCacheSize;					            /* 高速缓冲行大小 				*/
    UINT32 		        PACS_uiLatencyMax;					            /* 最大等待时间 				*/
    BOOL 		        PACS_bAutoIntRouting;        		            /* 中断自动路由使能 	        */
                                                                        /* 中断分配                     */
    UINT8  		        (*PACS_pfuncIntAssign)(struct pci_auto_cfg_system *ppacsSystem,
                                               PCI_AUTO_CFG_LOC           *ppaclLoc,
                                               UINT32 	                   uiDevVend);
                                                                        /* 包含设备                     */
    INT                 (*PACS_pfuncInclude)(struct pci_auto_cfg_system *ppacsSystem,
                                             PCI_AUTO_CFG_LOC           *ppaclLoc,
                                             UINT32                      uiDevVend);
                                                                        /* 桥配置准备                   */
    VOID                (*PACS_pfuncBridgePreInit)(struct pci_auto_cfg_system *ppacsSystem,
                                                   PCI_AUTO_CFG_LOC           *ppaclLoc,
                                                   UINT32                      uiDevVend);
                                                                        /* 桥配置结束                   */
    VOID                (*PACS_pfuncBridgePostInit)(struct pci_auto_cfg_system *ppacsSystem,
                                                    PCI_AUTO_CFG_LOC           *ppaclLoc,
                                                    UINT32                      uiDevVend);
                                                                        /* 探测设备                     */
    INT                 (*PACS_pfuncRollcall)();
} PCI_AUTO_CFG_SYSTEM;

typedef INT     (*PCI_AUTO_CFG_LOG_MSG_FUNC_PTR)(CPCHAR pcFmt, INT iArg0, INT iArg1, INT iArg2,
                                                 INT iArg3, INT iArg4, INT iArg5);
typedef INT     (*PCI_AUTO_CFG_INCLUDE_FUNC_PTR)(PCI_AUTO_CFG_SYSTEM *ppacsSystem,
                                                 PCI_AUTO_CFG_LOC    *ppaclLoc,
                                                 UINT32               uiDevVend);
typedef UINT8   (*PCI_AUTO_CFG_INT_ASSIGN_FUNC_PTR)(PCI_AUTO_CFG_SYSTEM *ppacsSystem,
                                                    PCI_AUTO_CFG_LOC    *ppaclLoc,
                                                    UINT32               uiDevVend);
typedef VOID    (*PCI_AUTO_CFG_BRIDGE_PRE_INIT_FUNC_PTR)(PCI_AUTO_CFG_SYSTEM *ppacsSystem,
                                                         PCI_AUTO_CFG_LOC    *ppaclLoc,
                                                         UINT32               uiDevVend);
typedef VOID    (*PCI_AUTO_CFG_BRIDGE_POST_INIT_FUNC_PTR)(PCI_AUTO_CFG_SYSTEM *ppacsSystem,
                                                          PCI_AUTO_CFG_LOC    *ppaclLoc,
                                                          UINT32               uiDevVend);

typedef INT     (*PCI_AUTO_CFG_ROLLCALL_FUNC_PTR)();
typedef UINT8   (*PCI_AUTO_CFG_LATENCY_MAX_FUNC_PTR)(INT iBus, INT iDevice, INT iFunction, PVOID pvArg);
typedef UINT32  (*PCI_AUTO_CFG_MEM_BUS_EXTRA_FUNC_PTR)(INT iBus, INT iDevice, INT iFunction, PVOID pvArg);

/*********************************************************************************************************
  函数操作集
*********************************************************************************************************/
LW_API INT      API_PciAutoCfgAddrAlign(UINT32  uiBase, UINT32  uiLimit, UINT32  uiSizeReq,
                                        UINT32 *puiAlignedBase);

LW_API INT      API_PciAutoCfgBarConfig(PCI_AUTO_CFG_SYSTEM  *ppacsSystem, PCI_AUTO_CFG_LOC *ppaclFunc,
                                        UINT32  uiBarOffset, UINT32  uiSize, UINT32  uiAddrInfo);

LW_API VOID     API_PciAutoCfgFuncDisable(PCI_AUTO_CFG_LOC *ppaclFunc);
LW_API VOID     API_PciAutoCfgFuncEnable(PCI_AUTO_CFG_SYSTEM *ppacsSystem, PCI_AUTO_CFG_LOC *ppaclFunc);

LW_API INT      API_PciAutoCfgDevReset(PCI_AUTO_CFG_LOC *ppaclLoc);

LW_API INT      API_PciAutoCfgBusNumberSet(PCI_AUTO_CFG_LOC *ppaclLoc,
                                           INT iPriBus, INT iSecBus, INT iSubordinate);

LW_API INT      API_PciAutoCfgCtl(PVOID  pvOpts, INT  iCmd, PVOID  pvArg);
LW_API PVOID    API_PciAutoCfgLibInit(PVOID pvArg);
LW_API VOID     API_PciAutoCfgInit(PCI_AUTO_CFG_SYSTEM *ppacsSystem);

#define pciAutoCfgAddrAlign         API_PciAutoCfgAddrAlign

#define pciAutoCfgBarConfig         API_PciAutoCfgBarConfig

#define pciAutoCfgFuncDisable       API_PciAutoCfgFuncDisable
#define pciAutoCfgFuncEnable        API_PciAutoCfgFuncEnable

#define pciAutoCfgDevReset          API_PciAutoCfgDevReset

#define pciAutoCfgBusNumberSet      API_PciAutoCfgBusNumberSet

#define pciAutoCfgCtl               API_PciAutoCfgCtl
#define pciAutoCfgInit              API_PciAutoCfgInit

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_PCI_EN > 0)         */
#endif                                                                  /*  __PCIAUTOCFG_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
