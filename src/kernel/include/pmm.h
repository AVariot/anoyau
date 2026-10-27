
#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>

#define FRAME_SIZE 4096

void pmm_init(uint32_t magic, uint32_t info);
uint64_t pmm_alloc_frame(void);
void pmm_free_frame(uint64_t addr);
size_t pmm_free_count(void);

#endif /* PMM_H */
