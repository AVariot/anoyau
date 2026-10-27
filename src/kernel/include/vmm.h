
#ifndef VMM_H
#define VMM_H

#include <stdint.h>

/* bits 51..12 of a table entry: the physical address it points to */
#define PTE_ADDR_MASK 0x000FFFFFFFFFF000ULL

#define PTE_PRESENT   (1ULL << 0)   /* entry is valid */
#define PTE_HUGE      (1ULL << 7)   /* PS: maps a 2 MiB page (in a PD) */
#define PTE_WRITE     (1ULL << 1)   /* writes allowed */

uint64_t vmm_translate(uint64_t virt);
int vmm_map(uint64_t virt, uint64_t phys, uint64_t flags);
int vmm_unmap(uint64_t virt);

#endif /* VMM_H */
