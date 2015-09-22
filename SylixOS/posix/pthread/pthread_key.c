/*********************************************************************************************************
**
**                                    �й�������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: pthread_key.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 12 �� 30 ��
**
** ��        ��: pthread ˽�����ݼ��ݿ�.

** BUG:
2012.12.07  ������Դ�����ڵ�.
2013.05.01  If successful, the pthread_key_*() function shall store the newly created key value at *key 
            and shall return zero. Otherwise, an error number shall be returned to indicate the error.
2013.05.02  ���� destructor ���õĲ�����ʱ��.
2014.12.09  ǿ�Ʊ�ɱ���Ľ��̲�ִ�� desturtors.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../include/px_pthread.h"                                      /*  �Ѱ�������ϵͳͷ�ļ�        */
#include "../include/posixLib.h"                                        /*  posix �ڲ�������            */
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0
/*********************************************************************************************************
  �������
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../SylixOS/loader/include/loader_vppatch.h"
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  key ˽����������
*********************************************************************************************************/
typedef struct {
    LW_LIST_LINE            PKEYN_lineManage;                           /*  ���� key ������             */
    long                    PKEYN_lId;                                  /*  key id                      */
    void                  (*PKEYN_pfuncDestructor)(void *);             /*  destructor                  */
    LW_LIST_LINE_HEADER     PKEYN_plineKeyHeader;                       /*  �����߳�˽������ָ��        */
    LW_OBJECT_HANDLE        PKEYN_ulMutex;                              /*  ������                      */
    LW_RESOURCE_RAW         PKEYN_resraw;                               /*  ��Դ�����ڵ�                */
} __PX_KEY_NODE;

static LW_LIST_LINE_HEADER  _G_plineKeyHeader;                          /*  ���е� key ������           */

#define __PX_KEY_LOCK(pkeyn)        API_SemaphoreMPend(pkeyn->PKEYN_ulMutex, LW_OPTION_WAIT_INFINITE)
#define __PX_KEY_UNLOCK(pkeyn)      API_SemaphoreMPost(pkeyn->PKEYN_ulMutex)
/*********************************************************************************************************
  Э��ɾ���ص���������
*********************************************************************************************************/
static BOOL  _G_bKeyDelHookAdd = LW_FALSE;
static VOID  __pthreadDataDeleteByThread(LW_OBJECT_HANDLE  ulId, PVOID  pvRetVal, PLW_CLASS_TCB  ptcbDel);
/*********************************************************************************************************
** ��������: __pthreadKeyOnce
** ��������: ���� POSIX �߳�
** �䡡��  : lId           ��id
**           pvData        ��ʼ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __pthreadKeyOnce (VOID)
{
    API_SystemHookAdd(__pthreadDataDeleteByThread, LW_OPTION_THREAD_DELETE_HOOK);
}
/*********************************************************************************************************
** ��������: __pthreadDataSet
** ��������: ����ָ�� key �ڵ�ǰ�߳��ڲ����ݽڵ�. (���򴴽�)
** �䡡��  : lId           ��id
**           pvData        ��ʼ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __pthreadDataSet (long  lId, const void  *pvData)
{
    __PX_KEY_NODE       *pkeyn = (__PX_KEY_NODE *)lId;
    __PX_KEY_DATA       *pkeyd;
    PLW_CLASS_TCB        ptcbCur;
    LW_OBJECT_HANDLE     ulMe;
    __PX_CONTEXT        *pctx;
    PLW_LIST_LINE        plineTemp;
    
    if (pkeyn == LW_NULL) {                                             /*  û�� key ��                 */
        errno = EINVAL;
        return  (EINVAL);
    }
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    ulMe = ptcbCur->TCB_ulId;
    pctx = _posixCtxGet(ptcbCur);
    
    if (pctx == LW_NULL) {                                              /*  û���߳�������              */
        errno = ENOMEM;
        return  (EINVAL);
    }
    
    /*
     *  �����Ƿ��Ѿ����������˽������
     */
    __PX_KEY_LOCK(pkeyn);                                               /*  ��ס key ��                 */
    for (plineTemp  = pkeyn->PKEYN_plineKeyHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pkeyd = (__PX_KEY_DATA *)plineTemp;                             /*  ������ KEY DATA �ĵ�һ��Ԫ��*/
        if (pkeyd->PKEYD_ulOwner == ulMe) {                             /*  �ҵ���Ӧ��ǰ�̵߳�����      */
            pkeyd->PKEYD_pvData = (void *)pvData;
            break;
        }
    }
    __PX_KEY_UNLOCK(pkeyn);                                             /*  ���� key ��                 */
    if (plineTemp) {
        return  (ERROR_NONE);                                           /*  �Ѿ��ҵ��˶�Ӧ�Ľڵ�        */
    }
    
    /*
     *  ���û���ҵ�, ����Ҫ�½�˽������
     */
    pkeyd = (__PX_KEY_DATA  *)__SHEAP_ALLOC(sizeof(__PX_KEY_DATA));     /*  û�нڵ�, ��Ҫ�½�          */
    if (pkeyd == LW_NULL) {
        errno = ENOMEM;
        return  (ENOMEM);
    }
    pkeyd->PKEYD_lId     = lId;                                         /*  ͨ�� id ������� key        */
    pkeyd->PKEYD_pvData  = (void *)pvData;
    pkeyd->PKEYD_ulOwner = ulMe;                                        /*  ��¼�߳� ID                 */
    
    __PX_KEY_LOCK(pkeyn);                                               /*  ��ס key ��                 */
    _List_Line_Add_Ahead(&pkeyd->PKEYD_lineManage, 
                         &pkeyn->PKEYN_plineKeyHeader);                 /*  �����Ӧ key ������         */
    __PX_KEY_UNLOCK(pkeyn);                                             /*  ���� key ��                 */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pthreadDataGet
** ��������: ��ȡָ�� key �ڵ�ǰ�߳��ڲ����ݽڵ�.
** �䡡��  : lId           ��id
**           ppvData       ��ʼ����(����)
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __pthreadDataGet (long  lId, void  **ppvData)
{
    __PX_KEY_NODE       *pkeyn = (__PX_KEY_NODE *)lId;
    __PX_KEY_DATA       *pkeyd;
    LW_OBJECT_HANDLE     ulMe  = API_ThreadIdSelf();
    PLW_LIST_LINE        plineTemp;
    
    if (ppvData == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    if (pkeyn == LW_NULL) {                                             /*  û�� key ��                 */
        errno = EINVAL;
        return  (EINVAL);
    }
    
    *ppvData = LW_NULL;
    
    __PX_KEY_LOCK(pkeyn);                                               /*  ��ס key ��                 */
    for (plineTemp  = pkeyn->PKEYN_plineKeyHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {
        
        pkeyd = (__PX_KEY_DATA *)plineTemp;                             /*  ������ KEY DATA �ĵ�һ��Ԫ��*/
        if (pkeyd->PKEYD_ulOwner == ulMe) {                             /*  �ҵ���Ӧ��ǰ�̵߳�����      */
            *ppvData = pkeyd->PKEYD_pvData;
            break;
        }
    }
    __PX_KEY_UNLOCK(pkeyn);                                             /*  ���� key ��                 */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pthreadDataDeleteByKey
** ��������: ɾ��ָ�� key �����������ݽڵ�.
** �䡡��  : lId           ��id
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __pthreadDataDeleteByKey (long  lId)
{
    __PX_KEY_NODE       *pkeyn = (__PX_KEY_NODE *)lId;
    __PX_KEY_DATA       *pkeyd;
    
    if (pkeyn == LW_NULL) {                                             /*  û�� key ��                 */
        errno = EINVAL;
        return  (EINVAL);
    }
    
    /*
     *  key ��ɾ��, ��Ҫ���� key ����˽�����ݱ�, ɾ������˽������
     */
    __PX_KEY_LOCK(pkeyn);                                               /*  ��ס key ��                 */
    while (pkeyn->PKEYN_plineKeyHeader) {
        pkeyd = (__PX_KEY_DATA *)pkeyn->PKEYN_plineKeyHeader;           /*  ������ KEY DATA �ĵ�һ��Ԫ��*/
        
        _List_Line_Del(&pkeyd->PKEYD_lineManage,
                       &pkeyn->PKEYN_plineKeyHeader);                   /*  ��������ɾ��                */
                       
        __SHEAP_FREE(pkeyd);                                            /*  �ͷ��߳�˽�������ڴ�        */
    }
    __PX_KEY_UNLOCK(pkeyn);                                             /*  ���� key ��                 */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pthreadDataDeleteByThread
** ��������: ɾ�������뵱ǰ�߳���ص��ڲ����ݽڵ�.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __pthreadDataDeleteByThread (LW_OBJECT_HANDLE  ulId, PVOID  pvRetVal, PLW_CLASS_TCB  ptcbDel)
{
    __PX_KEY_NODE       *pkeyn;
    __PX_KEY_DATA       *pkeyd;
    LW_OBJECT_HANDLE     ulMe = ptcbDel->TCB_ulId;
    __PX_CONTEXT        *pctx = _posixCtxTryGet(ptcbDel);               /*  �������򲻴���              */
    
    PLW_LIST_LINE        plineTempK;
    PLW_LIST_LINE        plineTempD;
    
    PVOID                pvPrevValue;
    BOOL                 bCall = LW_TRUE;
    
#if LW_CFG_MODULELOADER_EN > 0
    LW_LD_VPROC         *pvprocDel;
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */
    
    if (pctx == LW_NULL) {                                              /*  ���� posix �߳�             */
        return;
    }
    
#if LW_CFG_MODULELOADER_EN > 0
    pvprocDel = __LW_VP_GET_TCB_PROC(ptcbDel);
    if (pvprocDel && pvprocDel->VP_bForceTerm) {                        /*  ���̲���Ҫִ�� destructor   */
        bCall = LW_FALSE;
    }
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */

    /*
     *  �߳�ɾ��, ��Ҫ�������� key ����˽�����ݱ�, ɾ���뱾�߳���ص�˽������
     */
__re_check:
    __PX_LOCK();                                                        /*  ��ס posix ��               */
    for (plineTempK  = _G_plineKeyHeader;
         plineTempK != LW_NULL;
         plineTempK  = _list_line_get_next(plineTempK)) {               /*  �������� key ��             */
        
        pkeyn = (__PX_KEY_NODE *)plineTempK;
        
        plineTempD = pkeyn->PKEYN_plineKeyHeader;                       /*  ���� key ���ڵ����нڵ�     */
        while (plineTempD) {
            pkeyd = (__PX_KEY_DATA *)plineTempD;
            plineTempD  = _list_line_get_next(plineTempD);
            
            if (pkeyd->PKEYD_ulOwner == ulMe) {                         /*  �Ƿ�Ϊ��ǰ�߳����ݽڵ�      */
                if (pkeyd->PKEYD_pvData) {                              /*  ��Ҫ���� destructor         */
                    pvPrevValue = pkeyd->PKEYD_pvData;
                    pkeyd->PKEYD_pvData = LW_NULL;                      /*  �´β��ٵ��� destructor     */
                    
                    __PX_UNLOCK();                                      /*  ���� posix ��               */
                    if (pkeyn->PKEYN_pfuncDestructor && bCall) {        /*  ����ɾ������                */
                        pkeyn->PKEYN_pfuncDestructor(pvPrevValue);
                    }
                    goto    __re_check;                                 /*  ���¼��                    */
                }
                
                _List_Line_Del(&pkeyd->PKEYD_lineManage,
                               &pkeyn->PKEYN_plineKeyHeader);           /*  ��������ɾ��                */
                __SHEAP_FREE(pkeyd);                                    /*  �ͷ��߳�˽�������ڴ�        */
            }
        }
    }
    __PX_UNLOCK();                                                      /*  ���� posix ��               */
}
/*********************************************************************************************************
** ��������: pthread_key_create
** ��������: ����һ�����ݼ�.
** �䡡��  : pkey          �� (����)
**           destructor    ɾ������
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
** ע  ��  : key �ź�������Ϊ LW_OPTION_OBJECT_GLOBAL ����Ϊ key �Ѿ�ʹ����ԭʼ��Դ���л���.
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_key_create (pthread_key_t  *pkey, void (*destructor)(void *))
{
    __PX_KEY_NODE   *pkeyn;
    
    if (pkey == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    API_ThreadOnce(&_G_bKeyDelHookAdd, __pthreadKeyOnce);               /*  ��װ�߳�ɾ���ص�            */
    
    pkeyn = (__PX_KEY_NODE *)__SHEAP_ALLOC(sizeof(__PX_KEY_NODE));      /*  �����ڵ��ڴ�                */
    if (pkeyn == LW_NULL) {
        errno = ENOMEM;
        return  (ENOMEM);
    }
    pkeyn->PKEYN_lId             = (long)pkeyn;
    pkeyn->PKEYN_pfuncDestructor = destructor;
    pkeyn->PKEYN_plineKeyHeader  = LW_NULL;
    pkeyn->PKEYN_ulMutex         = API_SemaphoreMCreate("pxkey", LW_PRIO_DEF_CEILING, 
                                            LW_OPTION_INHERIT_PRIORITY |
                                            LW_OPTION_DELETE_SAFE |
                                            LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pkeyn->PKEYN_ulMutex == LW_OBJECT_HANDLE_INVALID) {
        __SHEAP_FREE(pkeyn);
        errno = ENOSPC;
        return  (ENOSPC);
    }
    
    __PX_LOCK();                                                        /*  ��ס posix ��               */
    _List_Line_Add_Ahead(&pkeyn->PKEYN_lineManage,
                         &_G_plineKeyHeader);                           /*  ���� key ������             */
    __PX_UNLOCK();                                                      /*  ���� posix ��               */
    
    __resAddRawHook(&pkeyn->PKEYN_resraw, (VOIDFUNCPTR)pthread_key_delete, 
                    pkeyn, 0, 0, 0, 0, 0);                              /*  ������Դ������              */
    
    *pkey = (pthread_key_t)pkeyn;                                       /*  ���ڴ��ַ����Ϊ id         */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_key_delete
** ��������: ɾ��һ�����ݼ�. (ע��, ɾ������������ô���ʱ��װ����������)
** �䡡��  : key          ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_key_delete (pthread_key_t  key)
{
    __PX_KEY_NODE   *pkeyn = (__PX_KEY_NODE  *)key;

    if (key == 0) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    __pthreadDataDeleteByKey(key);                                      /*  ɾ��������� key ��ص����� */
    
    __PX_LOCK();                                                        /*  ��ס posix ��               */
    _List_Line_Del(&pkeyn->PKEYN_lineManage,
                   &_G_plineKeyHeader);                                 /*  �� key ��������ɾ��         */
    __PX_UNLOCK();                                                      /*  ���� posix ��               */
    
    API_SemaphoreMDelete(&pkeyn->PKEYN_ulMutex);
    
    __resDelRawHook(&pkeyn->PKEYN_resraw);
    
    __SHEAP_FREE(pkeyn);                                                /*  �ͷ� key                    */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: pthread_setspecific
** ��������: �趨һ�����ݼ�ָ����ǰ�̵߳�˽������.
** �䡡��  : key          ��
**           pvalue       ֵ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
int  pthread_setspecific (pthread_key_t  key, const void  *pvalue)
{
    if (key == 0) {
        errno = EINVAL;
        return  (EINVAL);
    }
    
    return  (__pthreadDataSet(key, pvalue));
}
/*********************************************************************************************************
** ��������: pthread_getspecific
** ��������: ��ȡһ�����ݼ�ָ����ǰ�̵߳�˽������.
** �䡡��  : key          ��
**           pvalue       ֵ
** �䡡��  : ERROR CODE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
*********************************************************************************************************/
LW_API 
void *pthread_getspecific (pthread_key_t  key)
{
    void   *pvalue = LW_NULL;

    if (key == 0) {
        errno = EINVAL;
        return  (LW_NULL);
    }
    
    __pthreadDataGet(key, &pvalue);
    
    return  (pvalue);
}
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/