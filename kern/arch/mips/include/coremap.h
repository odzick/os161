#ifndef _COREMAP_H_ 
#define _COREMAP_H_

#include <types.h>

/*
 *  Initializes system coremap
 * 
 *  this should only be called once 
 *  before the first kmalloc.
 */
void cm_bootstrap(void);
/*
 *  Allocates 'npages' contiguous physical
 *  pages.
 *  
 *  this function will only work after
 *  cm_bootstrap has been called
 */
paddr_t getppages(unsigned long npages);
/*
 *  Free contiguous physical pages starting 
 *  at paddr.
 * 
 *  does nothing if page has not been
 *  allocated.
 */
void releaseppages(paddr_t paddr);

struct lock *cm_lock;

 #endif /* _COREMAP_H_ */
