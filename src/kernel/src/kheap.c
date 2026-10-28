
#include "kheap.h"
#include "pmm.h"
#include "vmm.h"
#include "printk.h"

#define HEAP_START 0x40000000UL

static uint64_t heap_end = HEAP_START;

struct block_s {
    size_t size; // Data size without header
    struct block_s *next;
    uint32_t magic;
} __attribute__((aligned(16)));

_Static_assert(sizeof(struct block_s) % 16 == 0, "block header must keep 16-byte alignment");

static struct block_s *heap_head = NULL;

#define HEAP_MAX_SIZE (64UL << 20)      /* 64 MiB cap on the heap */

/* Undo the last `pages` pages mapped by heap_grow: give their frames back. */
static void heap_shrink(size_t pages) {
    while (pages--) {
        heap_end -= FRAME_SIZE;
        uint64_t phys = vmm_translate(heap_end);   /* read BEFORE unmapping */
        vmm_unmap(heap_end);
        pmm_free_frame(phys & PTE_ADDR_MASK);
    }
}

/* All or nothing: either `pages` pages are added, or the heap is unchanged. */
static int heap_grow(size_t pages) {
    /* heap_end never exceeds the cap, so this subtraction cannot wrap */
    if (pages > (HEAP_START + HEAP_MAX_SIZE - heap_end) / FRAME_SIZE) {
        return -1;
    }
    for (size_t i = 0; i < pages; i++) {
        uint64_t frame = pmm_alloc_frame();
        if (frame == 0) {
            heap_shrink(i);             /* the i pages mapped so far */
            return -1;
        }
        if (vmm_map(heap_end, frame, PTE_PRESENT | PTE_WRITE) != 0) {
            pmm_free_frame(frame);      /* this one was never mapped */
            heap_shrink(i);
            return -1;
        }
        heap_end += FRAME_SIZE;
    }
    return 0;
}

void *kmalloc(size_t size) {
    /* checked BEFORE rounding: a huge size would wrap around to 0 */
    if (size == 0 || size > HEAP_MAX_SIZE) {
        return NULL;
    }
    size_t clean_size = (size + 15) & ~15UL;

    for (;;) {
        for (struct block_s *b = heap_head; b != NULL; b = b->next) {
            if (b->magic == MAGIC_FREE && b->size >= clean_size) {
                if (b->size >= clean_size + sizeof(struct block_s) + 16) {
                    struct block_s *rest = (struct block_s *)((char *)(b + 1) + clean_size);
                    rest->size = b->size - clean_size - sizeof(struct block_s);
                    rest->magic = MAGIC_FREE;       /* the leftover is free */
                    rest->next = b->next;
                    b->size = clean_size;
                    b->next = rest;
                }
                b->magic = MAGIC_USED;              /* the block we hand out */
                return b + 1;
            }
        }

        /* nothing fits: grow the heap and describe the new space as one free block */
        size_t needed = clean_size + sizeof(struct block_s);
        size_t pages = (needed + FRAME_SIZE - 1) / FRAME_SIZE;
        uint64_t old_end = heap_end;

        if (heap_grow(pages) != 0) {
            return NULL;
        }

        struct block_s *fresh = (struct block_s *)(uintptr_t)old_end;
        fresh->size = pages * FRAME_SIZE - sizeof(struct block_s);
        fresh->magic = MAGIC_FREE;                  /* new space is free */
        fresh->next = NULL;

        if (heap_head == NULL) {
            heap_head = fresh;
        } else {
            struct block_s *last = heap_head;
            while (last->next != NULL) {
                last = last->next;
            }
            last->next = fresh;
        }
    }
}

void kfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    if ((uint64_t)(uintptr_t)ptr < HEAP_START + sizeof(struct block_s) ||
        (uint64_t)(uintptr_t)ptr >= heap_end) {
            printk("kfree: %p unauthorized\n", ptr);
            return;
        }

    // Avoid free call with local variable
    if (((uintptr_t)ptr & 15) != 0) {
        printk("kfree: misaligned pointer %p\n", ptr);
        return;
    }

    struct block_s *b = (struct block_s *)ptr - 1;

    if (b->magic == MAGIC_FREE) {
        printk("kfree: double free\n");
        return;
    }
    if (b->magic != MAGIC_USED) {
        printk("kfree: %p unknown magic %x\n", ptr, b->magic);
        return;
    }

    b->magic = MAGIC_FREE;

    for (struct block_s *cur = heap_head; cur != NULL; cur = cur->next) {
        while (cur->magic == MAGIC_FREE && cur->next != NULL && cur->next->magic == MAGIC_FREE) {
            cur->size += sizeof(struct block_s) + cur->next->size;
            cur->next = cur->next->next;
        }
    }
}
