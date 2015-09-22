/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 *
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __DL_MALLOC_H
#define __DL_MALLOC_H

#include "dlmalloc.h"

void  dlmalloc_init(PLW_CLASS_HEAP  pheap, void  *mem, size_t  size);

/*
 * update memory infomation
 */
#define VP_MEM_INFO(pheap)

/*
 * build a memory management
 */
#define VP_MEM_CTOR(pheap, mem, size) dlmalloc_init(pheap, mem, size)

/*
 * destory memory management
 */
#define VP_MEM_DTOR(pheap) _HeapDtor(pheap, FALSE)

/*
 * add memory to management
 */
#define VP_MEM_ADD(pheap, mem, size) _HeapAddMemory(pheap, mem, size)

/*
 * allocate memory
 */
#define VP_MEM_ALLOC(pheap, nbytes) dlmalloc(nbytes)

/*
 * allocate memory align
 */
#define VP_MEM_ALLOC_ALIGN(pheap, nbytes, align) dlmemalign(align, nbytes)

/*
 * re-allocate memory
 */
#define VP_MEM_REALLOC(pheap, ptr, new_size, do_check) dlrealloc(ptr, new_size)

/*
 * free memory
 */
#define VP_MEM_FREE(pheap, ptr, do_check) dlfree(ptr)

#endif /* __DL_MALLOC_H */

/*
 * end
 */
