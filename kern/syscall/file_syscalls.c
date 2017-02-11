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

int 
open(const char *filename, int flags, mode_t mode, int32_t *retval)
{
    struct vnode *new_vnode = NULL;
    struct file *new_file = NULL;
    int fd;
    int result;

    if(filename == NULL)
        return EFAULT;

    result = vfs_open((char *)filename, flags, mode,  &new_vnode);
    if(result)
        return result;

    result = file_create(filename, new_vnode, mode, &new_file);
    if(result)
        return result;

    result = ft_add(curproc->p_filetable, new_file, &fd); 
    if(result)
        return result;

    *retval = fd;
    return 0;
}

/*
 * open_ft should only be used to initialize stdin stdout stderr entires 
 * in a newly created process
 */
int 
open_ft(const char *filename, int flags, mode_t mode, int32_t *retval, struct filetable *ft)
{
    struct vnode *new_vnode = NULL;
    struct file *new_file = NULL;
    int fd;
    int result;

    if(filename == NULL)
        return EFAULT;

    result = vfs_open((char *)filename, flags, mode,  &new_vnode);
    if(result)
        return result;

    result = file_create(filename, new_vnode, mode, &new_file);
    if(result)
        return result;

    result = ft_add(ft, new_file, &fd); 
    if(result)
        return result;

    *retval = fd;
    return 0;
}

//ssize_t
//read(int fd, void *buf, size_t buflen)
//{
//    return 0;
//}


//ssize_t
//write(int fd, const void *buf, size_t nbytes)
//{
//    return 0; 
//}
//
//
//off_t
//lseek(int fd, off_t pos, int whence)
//{
//    return 0;
//}
//
//
int
close(int fd)
{
    int result;

    result = ft_remove(curproc->p_filetable, fd);
    if(result)
        return result;

    return 0;
}
//
//
//int
//dup2(int oldfd, int newfd)
//{
//    return 0;
//}
//

int
chdir(char *pathname)
{
    int result;

    if(pathname == NULL)
        return EFAULT;

    result = vfs_chdir(pathname);
    if(result)
        return result;

    return 0;
}

int
__getcwd(char *buf, size_t buflen, int32_t *retval)
{
    struct uio cwd_uio;
    struct iovec cwd_iov;
    int result;

    if(buf == NULL)
        return EFAULT;

    uio_init(&cwd_iov, &cwd_uio, buf, buflen, 0, UIO_READ);

    result = vfs_getcwd(&cwd_uio);
    if(result)
        return result;

    *retval = buflen - cwd_uio.uio_resid;

    return 0;
}
