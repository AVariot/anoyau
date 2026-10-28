
#ifndef KHEAP_H
#define KHEAP_H

#include <stddef.h>
#include <stdint.h>

#define MAGIC_USED 0xA110C8EDU
#define MAGIC_FREE 0xF4EEB10CU

void *kmalloc(size_t size);
void kfree(void *ptr);

#endif /* KHEAP_H */
