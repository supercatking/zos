#ifndef ZOS_PMM_H
#define ZOS_PMM_H

#include <zos/types.h>

void pmm_init(void);
void *pmm_alloc_page(void);
void pmm_free_page(void *page);
size_t pmm_total_pages(void);
size_t pmm_free_pages(void);
void pmm_self_test(void);

#endif
