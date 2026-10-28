
#ifndef KHEAP_H
#define KHEAP_H

#include <stddef.h>
#include <stdint.h>

#define MAGIC_USED 0xA110C8EDU
#define MAGIC_FREE 0xF4EEB10CU

struct kheap_stats_s {
    size_t heap_size;     /* bytes mapped for the heap */
    size_t blocks;        /* number of blocks, used or free */
    size_t used_blocks;
    size_t used_bytes;    /* data bytes in used blocks */
    size_t free_bytes;    /* data bytes in free blocks */
    size_t overhead;      /* bytes taken by block headers */
};

void *kmalloc(size_t size);
void kfree(void *ptr);

/* Walks the whole heap and checks its invariants. 0 = ok, -1 = corrupted
 * (the reason is printed). Never dereferences a pointer outside the heap. */
int kheap_check(void);

/* Fills *s. Trusts the list: call kheap_check() first if in doubt. */
void kheap_stats(struct kheap_stats_s *s);

#endif /* KHEAP_H */
