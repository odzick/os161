#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <lib.h>
#include <vm.h>
#include <addrspace.h>

struct cm_entry {
   paddr_t paddr;
   int free;
   int block_len;
}

struct cm_entry *coremap;

unsigned int cm_entries;
unsigned int cm_freepages;
unsigned int cm_basepage;

void
cm_bootstrap(void)
{
    unsigned int npages;
    unsigned int cm_size;
    paddr_t cm_addr;
    paddr_t first = ram_getfirstfree();
    paddr_t last = ram_getsize();

    KASSERT((first & PAGE_FRAME) == first);
    KASSERT((last & PAGE_FRAME) == last);

    npages = (last - first) / PAGE_SIZE;

    cm_size = npages * sizeof(struct cm_entry);
    ROUNDUP(cm_size, PAGE_SIZE);

    /*steal mem for coremap*/
    cm_addr = ram_stealmem(cm_size/PAGE_SIZE);
    KASSERT(cm_addr != 0);
    coremap = (struct cm_entry *) PADDR_TO_KVADDR(cm_addr);
    firt = ram_getfirstfree();

    cm_basepage = first / PAGE_SIZE;
    cm_entries = (last / PAGE_SIZE) - cm_basepage;
    cm_freepages = cm_entries;

    /*initialize entries*/
    for(i = 0; i < cm_entries; i++){
        coremap[i].paddr = first + (i * PAGE_SIZE);
        coremap[i].free = 1;
        coremap[i].block_len = -1;
    }
}
