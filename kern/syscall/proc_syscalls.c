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
#include <kern/wait.h>

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

    new_proc = proc_create_runprogram(curproc->p_name);

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

    new_proc->p_parent_pid = curproc->p_pid;
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

//int
//execv(const char *program, char **args)
//{
//    int result, argc;
//    char *kernbuf;
//    struct addrspace *new_as;
//    struct vnode *v;
//	vaddr_t entrypoint, stackptr;
//    
//    kernbuf = kmalloc(sizeof(void*));
//    
//    // Check the first arg to see if it's safe, 
//    result = copyin((const_userptr_t) args, kernbuf, 4);
//    if (result){
//        kfree(kernbuf);
//        return result;
//    }
//    kfree(kernbuf);
//    
//    arc = 0;
//    while (args[argc] != NULL)
//        argc++;
//        
//    // TODO: some checks before copyinstr()
//    
//    result = copyinstr((const_userptr_t) program, kernbuf, /*idk*/, /*idk*/)
//    if (result);
//        //TODO: Error stuff
//        
//    result = vfs_open(program, O_RDONLY, 0, &v);
//    if (result);
//        //TODO: Error stuff
//        
//    /* Create a new address space. */
//	new_as = as_create();
//	if (new_as == NULL) {
//		vfs_close(v);
//		return ENOMEM;
//	}
//	
//	curproc->p_addrspace = new_as;
//	as_activate();
//	
//	/* Load the executable. */
//	result = load_elf(v, &entrypoint);
//	if (result) {
//		/* p_addrspace will go away when curproc is destroyed */
//		vfs_close(v);
//		return result;
//	}
//
//	/* Done with the file now. */
//	vfs_close(v);
//     
//     
//    /* Define the user stack in the address space */
//	result = as_define_stack(curproc->p_addrspace, &stackptr);
//	if (result) {
//		/* p_addrspace will go away when curproc is destroyed */
//		return result;
//	}
//	
//	/* Warp to user mode. */
//	// TODO: Fix these args
//	enter_new_process(argc, NULL /*userspace addr of argv*/,
//			  NULL /*userspace addr of environment*/,
//			  stackptr, entrypoint);
//}


int
waitpid(pid_t pid, int *status, pid_t *retval)
{
    struct proc* waitproc;

    if(curproc->p_pid == pid)
        return EINVAL;

    if(pid > PID_MAX || pid < 0)
        return EINVAL;

   // if (options != 0 && options != WNOHANG)
   //     return EINVAL;

    waitproc = get_proc(pid);
    if (waitproc == NULL)
        return ESRCH;

   lock_acquire(waitproc->p_waitlock);
   if(waitproc->p_parent_pid != curproc->p_pid){
        lock_release(waitproc->p_waitlock);
        return EPERM;
   }

    while(waitproc->p_exited == 0){
        cv_wait(waitproc->p_cv, waitproc->p_waitlock);  
    }

    // TODO handle case where child does not exit
    *status = _MKWAIT_EXIT(waitproc->p_exit_code); 
    *retval = pid;

    lock_release(waitproc->p_waitlock);

    return 0;
}

void
_exit(int exitcode)
{
    lock_acquire(curproc->p_waitlock);
    curproc->p_exited = 1;
    curproc->p_exit_code = exitcode;
    cv_signal(curproc->p_cv, curproc->p_waitlock);  
    lock_release(curproc->p_waitlock);
    thread_exit();
}
