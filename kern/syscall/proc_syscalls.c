#include <types.h>
#include <copyinout.h>
#include <syscall.h>
#include <vfs.h>
#include <kern/errno.h>
#include <proc.h>
#include <current.h>
#include <uio.h>
#include <filetable.h>
#include <file_syscalls.h>
#include <proc_syscalls.h>
#include <limits.h>
#include <vnode.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <uio.h>
#include <kern/iovec.h>
#include <machine/trapframe.h>
#include <addrspace.h>

//TODO code to generate reasonable pids that won't run out 
int
getpid(pid_t *retval)
{
    *retval = curproc->p_pid;
    return 0;
}

int
fork(struct trapframe *tf, pid_t *retval)
{
    struct trapframe *new_tf;
    struct proc *new_proc;
    int result;

    //TODO a make the name something different
    new_proc = proc_create_runprogram("child");

    KASSERT(new_proc != NULL);

    /* copy file table */
    result = ft_copy(new_proc->p_filetable, curproc->p_filetable);
    if(result){
        proc_destroy(new_proc); 
        return result;
    }

    /* copy address space */
    result = as_copy(curproc->p_addrspace, &new_proc->p_addrspace);
    if (result) {
        proc_destroy(new_proc); 
        return result;
    }

    /* copy current trapframe */
    new_tf = kmalloc(sizeof(struct trapframe));
    if (new_tf==NULL){
        return ENOMEM;
    }
    *new_tf = *tf;

    result = thread_fork(new_proc->p_name, new_proc, child_entry, (void *)new_tf, 0);
    if (result){
        kfree(new_tf);
        return result;
    }
    *retval = new_proc->p_pid;
    return 0;
}

void
child_entry(void *vtf, unsigned long junk)
{
    struct trapframe mytf;
    struct trapframe *ntf = vtf;

    (void)junk;

    mytf = *ntf;
    kfree(ntf);

    enter_forked_process(&mytf);
}

int
execv(const char *program, char **args)
{
    int result, argc;
    char *kernbuf;
    struct addrspace *new_as;
    struct vnode *v;
	vaddr_t entrypoint, stackptr;
    
    kernbuf = kmalloc(sizeof(void*));
    
    // Check the first arg to see if it's safe, 
    result = copyin((const_userptr_t) args, kernbuf, 4);
    if (result){
        kfree(kernbuf);
        return result;
    }
    kfree(kernbuf);
    
    arc = 0;
    while (args[argc] != NULL)
        argc++;
        
    // TODO: some checks before copyinstr()
    
    result = copyinstr((const_userptr_t) program, kernbuf, /*idk*/, /*idk*/)
    if (result);
        //TODO: Error stuff
        
    result = vfs_open(program, O_RDONLY, 0, &v);
    if (result);
        //TODO: Error stuff
        
    /* Create a new address space. */
	new_as = as_create();
	if (new_as == NULL) {
		vfs_close(v);
		return ENOMEM;
	}
	
	curproc->p_addrspace = new_as;
	as_activate();
	
	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);
     
     
    /* Define the user stack in the address space */
	result = as_define_stack(curproc->p_addrspace, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}
	
	/* Warp to user mode. */
	// TODO: Fix these args
	enter_new_process(argc, NULL /*userspace addr of argv*/,
			  NULL /*userspace addr of environment*/,
			  stackptr, entrypoint);
}

/*
void
_exit(int exitcode)
{

}
*/
