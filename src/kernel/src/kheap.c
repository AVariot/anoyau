
#include "kheap.h"
#include "pmm.h"
#include "vmm.h"
#include "printk.h"
#include "irq.h"

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

static void *kmalloc_locked(size_t size) {
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

static void kfree_locked(void *ptr) {
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

/*
 * Public entry points. The heap list is updated in several steps, so an
 * interrupt in the middle would see it half-modified: interrupts are masked
 * for the whole operation. The previous RFLAGS is restored (not a blind
 * sti), so a caller that already had interrupts off keeps them off.
 */
void *kmalloc(size_t size) {
    uint64_t flags = irq_save();
    void *p = kmalloc_locked(size);
    irq_restore(flags);
    return p;
}

void kfree(void *ptr) {
    uint64_t flags = irq_save();
    kfree_locked(ptr);
    irq_restore(flags);
}

/*
 * Invariants of the heap:
 *  - the blocks follow each other with no gap: the first one is at
 *    HEAP_START and each one starts right after the data of the previous;
 *  - the last one ends exactly at heap_end;
 *  - every header has a valid magic and a non-zero size multiple of 16.
 * `expected` is the address the next header must have; it is compared with
 * the pointer BEFORE the pointer is used, so a corrupted `next` is reported
 * instead of causing a page fault.
 */
static int kheap_check_locked(void) {
    uint64_t expected = HEAP_START;
    size_t max_blocks = (heap_end - HEAP_START) / sizeof(struct block_s);
    size_t n = 0;

    for (struct block_s *b = heap_head; b != NULL; b = b->next) {
        if (++n > max_blocks) {
            printk("heap check: too many blocks (loop in the list?)\n");
            return -1;
        }
        if ((uint64_t)(uintptr_t)b != expected) {
            printk("heap check: block %p, expected at %lx (gap or overlap)\n", (void *)b, expected);
            return -1;
        }
        if (expected + sizeof(struct block_s) > heap_end) {
            printk("heap check: header at %lx is past the heap end\n", expected);
            return -1;
        }
        if (b->magic != MAGIC_USED && b->magic != MAGIC_FREE) {
            printk("heap check: block %p has bad magic %x\n", (void *)b, b->magic);
            return -1;
        }
        if (b->size == 0 || (b->size & 15) != 0) {
            printk("heap check: block %p has bad size %lu\n", (void *)b, b->size);
            return -1;
        }
        expected += sizeof(struct block_s) + b->size;
        if (expected > heap_end) {
            printk("heap check: block %p overruns the heap end\n", (void *)b);
            return -1;
        }
    }
    if (expected != heap_end) {
        printk("heap check: blocks end at %lx but the heap ends at %lx\n", expected, heap_end);
        return -1;
    }
    return 0;
}

static void kheap_stats_locked(struct kheap_stats_s *s) {
    s->heap_size = heap_end - HEAP_START;
    s->blocks = 0;
    s->used_blocks = 0;
    s->used_bytes = 0;
    s->free_bytes = 0;

    for (struct block_s *b = heap_head; b != NULL; b = b->next) {
        s->blocks++;
        if (b->magic == MAGIC_USED) {
            s->used_blocks++;
            s->used_bytes += b->size;
        } else {
            s->free_bytes += b->size;
        }
    }
    s->overhead = s->blocks * sizeof(struct block_s);
}

int kheap_check(void) {
    uint64_t flags = irq_save();
    int r = kheap_check_locked();
    irq_restore(flags);
    return r;
}

void kheap_stats(struct kheap_stats_s *s) {
    uint64_t flags = irq_save();
    kheap_stats_locked(s);
    irq_restore(flags);
}
