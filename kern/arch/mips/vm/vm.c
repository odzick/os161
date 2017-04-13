#include <types.h>
#include <kern/errno.h>
#include <machine/coremap.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>

void
vm_bootstrap(void)
{
    cm_bootstrap();
    cm_lock = lock_create("coremap_lock");
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t
alloc_kpages(unsigned npages)
{
	paddr_t pa;
	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void
free_kpages(vaddr_t addr)
{
    releaseppages(KVADDR_TO_PADDR(addr));
}

void
vm_tlbshootdown_all(void)
{
	panic("dumbvm tried to do tlb shootdown?!\n");
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
    (void) faulttype;
    (void) faultaddress;
	return EFAULT;
}
