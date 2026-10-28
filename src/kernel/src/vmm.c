
#include "vmm.h"
#include <stddef.h>
#include "pmm.h"
#include "string.h"

/*
 * By claude (I did not really understand)
 * Page table walk, done in software.
 *
 * The CPU does exactly this on every memory access (the result is cached
 * in the TLB). Here we redo it by hand to learn where a virtual address
 * really lands in physical memory.
 *
 * A 48-bit virtual address is cut in 5 pieces:
 *
 *   47      39 38      30 29      21 20      12 11         0
 *  [ PML4 idx ][ PDPT idx ][  PD idx  ][  PT idx  ][  offset  ]
 *     9 bits      9 bits      9 bits      9 bits      12 bits
 *
 * Each "idx" selects one of the 512 entries (512 * 8 bytes = 4096 bytes,
 * one frame) of a table. The entry tells where the next table is.
 * The last piece is the position inside the final 4 KiB page.
 *
 * Returns the physical address, or 0 if the address is not mapped.
 * 0 is safe as an error value: the PMM never hands out physical page 0.
 */
uint64_t vmm_translate(uint64_t virt) {
    /*
     * CR3 holds the PHYSICAL address of the top table (PML4). It is a
     * register, so the only way to read it is an instruction: no C
     * expression can name it. The "=r" output makes the compiler pick
     * any general register and copy it into our variable.
     */
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));

    /*
     * The low 12 bits of CR3 are flags (cache options), not address: the
     * table is 4096-aligned so they would be 0 anyway. Mask them off.
     *
     * Using a physical address as a C pointer only works because the
     * boot code identity-maps the first GiB (virtual == physical there)
     * and our tables live in the kernel image, below 1 GiB.
     */
    uint64_t *pml4 = (uint64_t *)(uintptr_t)(cr3 & PTE_ADDR_MASK);

    /* level 1: PML4. Bit 0 (Present) clear => nothing mapped below */
    uint64_t e = pml4[(virt >> 39) & 0x1FF];
    if (!(e & PTE_PRESENT)) {
        return 0;
    }

    /*
     * An entry mixes an address (bits 51..12) and flags (bits 11..0, and
     * 63 = NX). PTE_ADDR_MASK keeps only the address: using the raw entry
     * as a pointer would aim a few bytes after the table (0x110023 instead
     * of 0x110000).
     */
    uint64_t *pdpt = (uint64_t *)(uintptr_t)(e & PTE_ADDR_MASK);

    /* level 2: PDPT. Each entry covers 1 GiB. */
    e = pdpt[(virt >> 30) & 0x1FF];
    if (!(e & PTE_PRESENT)) {
        return 0;
    }
    /*
     * Bit 7 (PS) at this level would mean a 1 GiB page. Our boot never
     * builds one, so it is not handled here.
     */
    uint64_t *pd = (uint64_t *)(uintptr_t)(e & PTE_ADDR_MASK);

    /* level 3: PD. Each entry covers 2 MiB. */
    e = pd[(virt >> 21) & 0x1FF];
    if (!(e & PTE_PRESENT)) {
        return 0;
    }

    /*
     * Bit 7 (PS, "page size") changes the meaning of this entry:
     *  - set:   the entry maps a 2 MiB page directly (what the boot code
     *           does). There is no 4th level, so the offset is the 21 low
     *           bits of the virtual address.
     *  - clear: the entry points to a last table (PT) of 4 KiB pages.
     *
     * Base of a 2 MiB page is 2 MiB-aligned: bits 20..0 must be dropped
     * from the entry (they are flags/reserved) and replaced by the offset.
     */
    if (e & PTE_HUGE) {
        return (e & PTE_ADDR_MASK & ~0x1FFFFFULL) | (virt & 0x1FFFFF);
    }

    uint64_t *pt = (uint64_t *)(uintptr_t)(e & PTE_ADDR_MASK);

    /* level 4: PT. Each entry maps one 4 KiB page. */
    e = pt[(virt >> 12) & 0x1FF];
    if (!(e & PTE_PRESENT)) {
        return 0;
    }

    /* frame base from the entry + position inside the page (12 bits) */
    return (e & PTE_ADDR_MASK) | (virt & 0xFFF);
}

/* Returns the table pointed to by table[idx], creating it if needed. NULL on failure. */
static uint64_t *next_table(uint64_t *table, size_t idx) {
    if (!(table[idx] & PTE_PRESENT)) {
        uint64_t frame = pmm_alloc_frame();
        if (frame == 0) return NULL;
        memset((void *)(uintptr_t)frame, 0, FRAME_SIZE);
        table[idx] = frame | PTE_PRESENT | PTE_WRITE;
    }
    if (table[idx] & PTE_HUGE) return NULL;
    return (uint64_t *)(uintptr_t)(table[idx] & PTE_ADDR_MASK);
}

int vmm_map(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    uint64_t *pml4 = (uint64_t *)(uintptr_t)(cr3 & PTE_ADDR_MASK);

    uint64_t *pdpt = next_table(pml4, (virt >> 39) & 0x1FF);
    if (pdpt == NULL) {
        return -1;
    }
    uint64_t *pd = next_table(pdpt, (virt >> 30) & 0x1FF);
    if (pd == NULL) {
        return -1;
    }
    uint64_t *pt = next_table(pd, (virt >> 21) & 0x1FF);
    if (pt == NULL) {
        return -1;
    }
    pt[(virt >> 12) & 0x1FF] = phys | flags;
    __asm__ volatile("invlpg (%0)" :: "r"(virt) : "memory");
    return 0;
}

/*
 * Same as next_table() but read-only: a missing table is an error,
 * not something to create. Also NULL on a huge page, because there is
 * no lower level to walk into.
 */
static uint64_t *find_table(uint64_t *table, size_t idx) {
    if (!(table[idx] & PTE_PRESENT) || (table[idx] & PTE_HUGE)) {
        return NULL;
    }
    return (uint64_t *)(uintptr_t)(table[idx] & PTE_ADDR_MASK);
}

/*
 * Removes the mapping of the 4 KiB page containing virt.
 * Returns 0 on success, -1 if the address was not mapped by a 4 KiB page.
 *
 * The physical frame is NOT freed: unmapping only cuts the link. The
 * caller owns the frame and may reuse it or map it somewhere else.
 */
 // TODO: free one entry but not a PT became empty
int vmm_unmap(uint64_t virt) {
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    uint64_t *pml4 = (uint64_t *)(uintptr_t)(cr3 & PTE_ADDR_MASK);

    uint64_t *pdpt = find_table(pml4, (virt >> 39) & 0x1FF);
    if (pdpt == NULL) {
        return -1;
    }
    uint64_t *pd = find_table(pdpt, (virt >> 30) & 0x1FF);
    if (pd == NULL) {
        return -1;
    }
    uint64_t *pt = find_table(pd, (virt >> 21) & 0x1FF);
    if (pt == NULL) {
        return -1;
    }

    size_t idx = (virt >> 12) & 0x1FF;
    if (!(pt[idx] & PTE_PRESENT)) {
        return -1;
    }
    pt[idx] = 0;

    /*
     * Mandatory here: the CPU caches translations in the TLB. Without
     * this, the old mapping would keep working until the entry happens
     * to be evicted, and a use-after-unmap would go unnoticed.
     */
    __asm__ volatile ("invlpg (%0)" :: "r"(virt) : "memory");
    return 0;
}
