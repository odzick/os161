#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <lib.h>
#include <vm.h>
#include <addrspace.h>
#include <machine/coremap.h>
#include <synch.h>

struct cm_entry {
   paddr_t paddr;
   int free;
   int block_len;
};

struct cm_entry *coremap;

unsigned int cm_entries;
unsigned int cm_basepage;


void
cm_bootstrap(void)
{
    unsigned int npages;
    unsigned int cm_size;
    unsigned int i;
    paddr_t last = ram_getsize();
    paddr_t first = ram_getfirstfree();

    KASSERT((first & PAGE_FRAME) == first);
    KASSERT((last & PAGE_FRAME) == last);

    npages = last / PAGE_SIZE;

    cm_size = ROUNDUP(npages * sizeof(struct cm_entry), PAGE_SIZE);
    KASSERT((cm_size & PAGE_FRAME) == cm_size);

    /*steal pages for coremap*/
    coremap = (struct cm_entry *) PADDR_TO_KVADDR(first);
    first += cm_size;
    KASSERT(first < last);

    cm_basepage = first / PAGE_SIZE;
    cm_entries = npages;

    /*initialize entries*/
    for(i = 0; i < cm_entries; i++){
        coremap[i].paddr = (i * PAGE_SIZE);
        coremap[i].free = 1;
        coremap[i].block_len = -1;
    }

    /*add already used mem to coremap*/
    for(i = 0; coremap[i].paddr < first; i++)
        coremap[i].free = 0;
}

paddr_t
getppages(unsigned long npages)
{
    unsigned int i, j;
    unsigned int count = 0;

    if(npages < 1)
        return 0;

    if(cm_lock != NULL)
        lock_acquire(cm_lock);

    for(i = 0; i < cm_entries; i++){
        if(coremap[i].free)
            count++;
        else
            count = 0;

        if(count == npages){
            coremap[i - npages + 1].block_len = npages;
            for (j = i - npages + 1; j <= i; j++) {
                coremap[j].free = 0;
            }

            if(cm_lock != NULL)
                lock_release(cm_lock);

            return coremap[i - npages + 1].paddr;
        }
    }

    if(cm_lock != NULL)
        lock_release(cm_lock);
    /* no contiguous npages found*/
    return 0; 
}

void
releaseppages(paddr_t paddr)
{
    int i, j;

    KASSERT(cm_lock != NULL);

    lock_acquire(cm_lock);

    for (i = 0; coremap[i].paddr != paddr; i++);

    KASSERT(coremap[i].block_len != -1);

    for (j = 0; j < coremap[i].block_len; j++)
        coremap[i + j].free = 1;

    coremap[i].block_len = -1;

    lock_release(cm_lock);
}
