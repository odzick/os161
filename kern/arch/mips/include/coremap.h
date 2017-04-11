#ifndef _COREMAP_H_ 
#define _COREMAP_H_

#include <types.h>

void cm_bootstrap(void);
paddr_t getppages(unsigned long npages);
void releaseppages(paddr_t paddr);

struct lock *cm_lock;

 #endif /* _COREMAP_H_ */
