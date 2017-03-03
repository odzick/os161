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
        proc_destroy(new_proc); 
        return ENOMEM;
    }
    *new_tf = *tf;

    result = thread_fork(new_proc->p_name, new_proc, child_entry, (void *)new_tf, 0);
    if (result){
        kfree(new_tf);
        proc_destroy(new_proc); 
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
    int result, argc, i, j, len;
    char *kernbuf;
    struct addrspace *new_as;
    struct vnode *v;
    size_t size;
	vaddr_t entrypoint, stackptr;
    
    kernbuf = (char *) kmalloc(sizeof(void*));
    
    /*
    Don't know if we need this
    
    // Check the first arg to see if it's safe, 
    result = copyin((const_userptr_t) args, kernbuf, 4);
    if (result){
        kfree(kernbuf);
        return result;
    }
    kfree(kernbuf);*/
    
    /*arc = 0;
    while (args[argc] != NULL)
        argc++; */
        
    kernbuf = (char *)kmalloc(PATH_MAX*sizeof(char));
    if (kernbuf == NULL)
        return ENOMEM;
    
    result = copyinstr((const_userptr_t) program, kernbuf, PATH_MAX, &size)
    if (result){
        kfree(kernbuf);
        return EFAULT;
    }
    
    // TODO: Maybe check if size is 1
    
    
    char **kernargs = (char **) kmalloc(sizeof(char**));
    
    result = copyin((const_userptr_t) args, kernargs, sizeof(char **));
    if (result){
        kfree(kernbuf);
        kfree(kernargs);
        return EFAULT;
    }
    
    i = 0;
    // Copy arguments to kernel
    while (args[i] != NULL) {
        kernargs[i] = (char *) kmalloc(sizeof(char) * PATH_MAX);
        result = copyinstr((const_userptr_t) args[i], PATH_MAX), &size);
        if (result) {
            kfree(kernbuf);
            kfree(kernargs);
            return EFAULT;
        }
        i++;
    }
        
    result = vfs_open(program, O_RDONLY, 0, &v);
    if (result){
        kfree(kernbuf);
        kfree(kernargs);
        return result;
    }
        
    // TODO: Delete last address space?
        
    /* Create a new address space. */
	new_as = as_create();
	if (new_as == NULL) { 
	    kfree(kernbuf);
        kfree(kernargs);
		vfs_close(v);
		return ENOMEM;
	}
	
	curproc->p_addrspace = new_as;
	as_activate();
	
	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		kfree(kernbuf);
        kfree(kernargs);
		vfs_close(v);
		return result;
	}

	/* Done with the file now. TODO: Is this the right spot?*/
	vfs_close(v);
     
     
    /* Define the user stack in the address space */
	result = as_define_stack(curproc->p_addrspace, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		kfree(kernbuf);
        kfree(kernargs);
		return result;
	}
	
	j = 0;
	
	// TODO: Check logic for loops below
	
	while(kernargs[j] != NULL){
	    char * currarg;
	    len = strlen(kernargs[j]) + 1;
	    
	    int origlen = len;
	    if (len % 4 != 0)
	        len = len + (4 - len % 4);
	        
	    currarg = kmalloc(sizeof(len));
	    currarg = kstrdup(kernargs[j]);
	    
	    for (int i = 0; i < len; i++){
	        if (i >= origlen)
	            currarg[i] = '\0';
	        else
	            currarg[i] = kernargs[j][i];
	    }
	    
	    stackptr -= len;
	    
	    result = copyout((const void *) currarg, (userptr_t) stackptr, 
	                    (size_t) len);
	    if (result){
	        kfree(currarg);
	        kfree(kernbuf);
            kfree(kernargs);
		    return result;
	    }
	    j++;
	}
	
	if (kernargs[j] == NULL)
	    stackptr -= 4 * sizeof(char);
	    
	for (int i = (j - 1); i >= 0; i--){
	    stackptr = stackptr - sizeof(char*);
	    result = copyout((const void *) (kernargs + i), (userptr_t) stackptr, 
	                    (sizeof(char*)));
	    if (result) {
		    kfree(kernbuf);
            kfree(kernargs);
		    return result;
	    }        
	}
	
	kfree(kernbuf);
    kfree(kernargs);
	
	/* Warp to user mode. */
	// TODO: Fix these args
	enter_new_process(argc, NULL /*userspace addr of argv*/,
			  NULL /*userspace addr of environment*/,
			  stackptr, entrypoint);
    return EINVAL;
}

/*
int
waitpid(pid_t pid, int *status, int options, pid_t *retval)
{
    return 0;
}
*/
void
_exit(int exitcode)
{
    curproc->p_exit_status = 1;
    curproc->p_exit_code = exitcode;
    thread_exit();
}
