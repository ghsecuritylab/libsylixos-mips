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
** ��   ��   ��: procBsp.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 03 ��
**
** ��        ��: proc �ļ�ϵͳ bsp ��Ϣ�ļ�.

** BUG:
2009.12.11  �����ļ����ݺ���Ҫ���� API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize); ��д�ļ�ʵ�ʴ�С.
2010.01.07  �޸�һЩ errno.
2010.08.11  �� read ����.
2011.03.04  proc �ļ� mode Ϊ S_IFREG.
2013.05.04  ���� self/auxv �ļ�.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../procFs.h"
#include "elf.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_PROCFS_EN > 0) && (LW_CFG_PROCFS_BSP_INFO > 0)
/*********************************************************************************************************
  bsp proc �ļ���������
*********************************************************************************************************/
static ssize_t  __procFsBspMemRead(PLW_PROCFS_NODE  p_pfsn, 
                                   PCHAR            pcBuffer, 
                                   size_t           stMaxBytes,
                                   off_t            oft);
static ssize_t  __procFsBspCpuRead(PLW_PROCFS_NODE  p_pfsn, 
                                   PCHAR            pcBuffer, 
                                   size_t           stMaxBytes,
                                   off_t            oft);
static ssize_t  __procFsBspAuxvRead(PLW_PROCFS_NODE  p_pfsn, 
                                    PCHAR            pcBuffer, 
                                    size_t           stMaxBytes,
                                    off_t            oft);
/*********************************************************************************************************
  bsp proc �ļ�����������
*********************************************************************************************************/
static LW_PROCFS_NODE_OP        _G_pfsnoBspMemFuncs = {
    __procFsBspMemRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoBspCpuFuncs = {
    __procFsBspCpuRead, LW_NULL
};
static LW_PROCFS_NODE_OP        _G_pfsnoBspAuxvFuncs = {
    __procFsBspAuxvRead, LW_NULL
};
/*********************************************************************************************************
  bsp proc �ļ�Ŀ¼��
*********************************************************************************************************/
#define __PROCFS_BUFFER_SIZE_BSPMEM     1024
#define __PROCFS_BUFFER_SIZE_CPUINFO    1024
#define __PROCFS_BUFFER_SIZE_AUXV       1024

static LW_PROCFS_NODE           _G_pfsnBsp[] = 
{
    LW_PROCFS_INIT_NODE("self", (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH), 
                        LW_NULL, LW_NULL,  0),
    
    LW_PROCFS_INIT_NODE("bspmem",  (S_IRUSR | S_IRGRP | S_IROTH | S_IFREG), 
                        &_G_pfsnoBspMemFuncs, "M", __PROCFS_BUFFER_SIZE_BSPMEM),
                        
    LW_PROCFS_INIT_NODE("cpuinfo", (S_IRUSR | S_IRGRP | S_IROTH | S_IFREG), 
                        &_G_pfsnoBspCpuFuncs, "C", __PROCFS_BUFFER_SIZE_CPUINFO),
                        
    LW_PROCFS_INIT_NODE("auxv", (S_IRUSR | S_IRGRP | S_IROTH | S_IFREG), 
                        &_G_pfsnoBspAuxvFuncs, "A", __PROCFS_BUFFER_SIZE_AUXV),
};
/*********************************************************************************************************
** ��������: __procFsBspMemRead
** ��������: procfs ��һ�� bspmem �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsBspMemRead (PLW_PROCFS_NODE  p_pfsn, 
                                    PCHAR            pcBuffer, 
                                    size_t           stMaxBytes,
                                    off_t            oft)
{
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  ʵ�ʵ��ļ����ݴ�С          */
             size_t     stCopeBytes;

    /*
     *  �������е�����, �ļ�����һ���Ѿ�������Ԥ�õ��ڴ��С.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
     
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  ��Ҫ�����ļ�                */
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_BSPMEM, 0,
                 "ROM SIZE: 0x%08zx Bytes (0x%08lx - 0x%08lx)\n"
                 "RAM SIZE: 0x%08zx Bytes (0x%08lx - 0x%08lx)\n"
                 "use \"mems\" \"zones\" \"virtuals\"... can print memory usage factor.\n",
                 bspInfoRomSize(),
                 bspInfoRomBase(),
                 (bspInfoRomBase() + bspInfoRomSize() - 1),
                 bspInfoRamSize(),
                 bspInfoRamBase(),
                 (bspInfoRamBase() + bspInfoRamSize() - 1));            /*  ����Ϣ��ӡ������            */
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  ����ʵ�ʶ�ȡ������          */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** ��������: __procFsBspCpuRead
** ��������: procfs ��һ�� cpuinfo �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsBspCpuRead (PLW_PROCFS_NODE  p_pfsn, 
                                    PCHAR            pcBuffer, 
                                    size_t           stMaxBytes,
                                    off_t            oft)
{
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  ʵ�ʵ��ļ����ݴ�С          */
             size_t     stCopeBytes;

    /*
     *  �������е�����, �ļ�����һ���Ѿ�������Ԥ�õ��ڴ��С.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  ��Ҫ�����ļ�                */
        PCHAR   pcPowerLevel;
        UINT    uiPowerLevel;
        ULONG   ulActive;
        
#if LW_CFG_POWERM_EN > 0
        API_PowerMCpuGet(&ulActive, &uiPowerLevel);
#else
        ulActive     = 1;
        uiPowerLevel = LW_CPU_POWERLEVEL_TOP;
#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */
        
        switch (uiPowerLevel) {
        
        case LW_CPU_POWERLEVEL_TOP:
            pcPowerLevel = "Top level";
            break;
            
        case LW_CPU_POWERLEVEL_FAST:
            pcPowerLevel = "Fast level";
            break;
        
        case LW_CPU_POWERLEVEL_NORMAL:
            pcPowerLevel = "Normal level";
            break;
        
        case LW_CPU_POWERLEVEL_SLOW:
            pcPowerLevel = "Slow level";
            break;
            
        default:
            pcPowerLevel = "<unknown> level";
            break;
        }
        
        stRealSize = bnprintf(pcFileBuffer, __PROCFS_BUFFER_SIZE_CPUINFO, 0,
                 "CPU      : %s\n"
                 "WORDLEN  : %d\n"
                 "NCPU     : %d\n"
                 "ACTIVE   : %d\n"
                 "PWRLevel : %s\n"
                 "CACHE    : %s\n"
                 "PACKET   : %s\n",
                 bspInfoCpu(),
                 LW_CFG_CPU_WORD_LENGHT,
                 (INT)LW_NCPUS,
                 (INT)ulActive,
                 pcPowerLevel,
                 bspInfoCache(),
                 bspInfoPacket());                                      /*  ����Ϣ��ӡ������            */
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  ����ʵ�ʶ�ȡ������          */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** ��������: __procFsBspAuxvRead
** ��������: procfs ��һ�� self/auxv �ļ�
** �䡡��  : p_pfsn        �ڵ���ƿ�
**           pcBuffer      ������
**           stMaxBytes    ��������С
**           oft           �ļ�ָ��
** �䡡��  : ʵ�ʶ�ȡ����Ŀ
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static ssize_t  __procFsBspAuxvRead (PLW_PROCFS_NODE  p_pfsn, 
                                     PCHAR            pcBuffer, 
                                     size_t           stMaxBytes,
                                     off_t            oft)
{
    REGISTER PCHAR      pcFileBuffer;
             size_t     stRealSize;                                     /*  ʵ�ʵ��ļ����ݴ�С          */
             size_t     stCopeBytes;
             Elf_auxv_t *pelfauxv;
    
    /*
     *  �������е�����, �ļ�����һ���Ѿ�������Ԥ�õ��ڴ��С.
     */
    pcFileBuffer = (PCHAR)API_ProcFsNodeBuffer(p_pfsn);
    if (pcFileBuffer == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (0);
    }
    
    stRealSize = API_ProcFsNodeGetRealFileSize(p_pfsn);
    if (stRealSize == 0) {                                              /*  ��Ҫ�����ļ�                */
        pelfauxv = (Elf_auxv_t *)pcFileBuffer;
        pelfauxv->a_type     = AT_UID;
        pelfauxv->a_un.a_val = getuid();
        pelfauxv++;
        
        pelfauxv->a_type     = AT_EUID;
        pelfauxv->a_un.a_val = geteuid();
        pelfauxv++;
        
        pelfauxv->a_type     = AT_GID;
        pelfauxv->a_un.a_val = getgid();
        pelfauxv++;
        
        pelfauxv->a_type     = AT_EGID;
        pelfauxv->a_un.a_val = getegid();
        pelfauxv++;
        
        pelfauxv->a_type     = AT_HWCAP;
        pelfauxv->a_un.a_val = (Elf_Word)bspInfoHwcap();
        pelfauxv++;
        
        pelfauxv->a_type     = AT_CLKTCK;
        pelfauxv->a_un.a_val = LW_TICK_HZ;
        pelfauxv++;
        
        pelfauxv->a_type     = AT_PAGESZ;
        pelfauxv->a_un.a_val = LW_CFG_VMM_PAGE_SIZE;
        pelfauxv++;
        
        pelfauxv->a_type     = AT_NULL;
        pelfauxv->a_un.a_val = 0;
        pelfauxv++;
        
        stRealSize = (size_t)pelfauxv - (size_t)pcFileBuffer;
        
        API_ProcFsNodeSetRealFileSize(p_pfsn, stRealSize);
    }
    if (oft >= stRealSize) {
        _ErrorHandle(ENOSPC);
        return  (0);
    }
    
    stCopeBytes  = __MIN(stMaxBytes, (size_t)(stRealSize - oft));       /*  ����ʵ�ʶ�ȡ������          */
    lib_memcpy(pcBuffer, (CPVOID)(pcFileBuffer + oft), (UINT)stCopeBytes);
    
    return  ((ssize_t)stCopeBytes);
}
/*********************************************************************************************************
** ��������: __procFsBspInfoInit
** ��������: procfs ��ʼ�� Bsp proc �ļ�
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  __procFsBspInfoInit (VOID)
{
    API_ProcFsMakeNode(&_G_pfsnBsp[0], "/");
    API_ProcFsMakeNode(&_G_pfsnBsp[1], "/");
    API_ProcFsMakeNode(&_G_pfsnBsp[2], "/");
    API_ProcFsMakeNode(&_G_pfsnBsp[3], "/self");
}
#endif                                                                  /*  LW_CFG_PROCFS_EN > 0        */
                                                                        /*  LW_CFG_PROCFS_BSP_INFO      */
/*********************************************************************************************************
  END
*********************************************************************************************************/