#ifndef __PAGER_H
#define __PAGER_H
#include <stdint.h>

typedef struct {
    int fd;
    uint32_t file_length;
    void *pages[100];
    //void *pages[TABLE_MAX_PAGES];
} Pager;


Pager* pager_open(const char *filename);
void pager_flush(Pager *pager, uint32_t page_num, uint32_t size);
#endif
