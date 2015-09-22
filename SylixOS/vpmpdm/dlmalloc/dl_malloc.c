/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#define  __SYLIXOS_KERNEL /* need some kernel function */
#include <unistd.h>

/*
 * sbrk lock
 */
void __vp_patch_lock(void);
void __vp_patch_unlock(void);
void __vp_patch_sbrk(BOOL lock);

/*
 * global heap
 */
static PLW_CLASS_HEAP  pctx_heap;

/*
 * dlmalloc_init
 */
void  dlmalloc_init (PLW_CLASS_HEAP  pheap, void  *mem, size_t  size)
{
    _HeapCtor(pheap, mem, size);
    pctx_heap = pheap;
}

/*
 * dlmalloc_sbrk
 */
void *dlmalloc_sbrk (int  size)
{
    void *mem;
    int mextern = 0;
    static void *previous_top = NULL;

    if (size == 0) {
        return  (previous_top);

    } else if (size < 0) {
        return  ((void *)(-1));
    }

__re_try:
    __vp_patch_lock();
    mem = _HeapAllocate(pctx_heap, (size_t)size, __func__);
    if ((mem == NULL) && mextern) {
        __vp_patch_unlock();
        return  ((void *)(-1));
    }

    if (mem) {
        previous_top = (void *)((size_t)mem + (size_t)size);
        __vp_patch_unlock();

    } else {
        mextern = 1;
        __vp_patch_sbrk(FALSE);
        __vp_patch_unlock();
        goto  __re_try;
    }

    return  (mem);
}

/*
 * end
 */
