#include <types.h>
#include <copyinout.h>
#include <syscall.h>
#include <vfs.h>
#include <proc.h>
#include <current.h>
#include <filetable.h>
#include <file_syscalls.h>
#include <limits.h>
#include <vnode.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <uio.h>
#include <kern/iovec.h>

int 
open(char *filename, int flags, mode_t mode, int32_t *retval)
{
    struct vnode *new_vnode = NULL;
    struct file *new_file = NULL;
    int fd;
    int result;

    result = vfs_open(filename, flags, mode,  &new_vnode);
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


ssize_t
read(int fd, void *buf, size_t buflen, int32_t *retval)
{
    if (fd < 0 || fd >= OPEN_MAX || curproc->p_filetable->files[fd] == NULL)
        return EBADF;
        
    if (curproc->p_filetable->files[fd]->mode != O_RDONLY 
        || curproc->p_filetable->files[fd]->mode != O_RDWR)
        return EBADF;
        
    if (buf == NULL)
        return EFAULT;
    
    // Create struct uio (which also needs a struct iovec) so that VOP_READ 
    // can be called
    struct uio read_uio;
    struct iovec read_iovec;
    
    read_iovec.iov_ubase = buf;
    read_iovec.iov_len = buflen;
    
    read_uio.uio_iov = &read_iovec;
    read_uio.uio_iovcnt = 1;
    read_uio.uio_offset = curproc->p_filetable->files[fd]->file_offset;
    read_uio.uio_resid = buflen;
    read_uio.uio_segflg = UIO_USERSPACE;
    read_uio.uio_rw = UIO_READ;
    read_uio.uio_space = /*RIP*/;
    
    if (VOP_READ(curproc->p_filetable->files[fd]->file_vnode, &read_uio))
        return EIO; //TODO: I think this is how you would do it
    
    unsigned int prev_offset = curproc->p_filetable->files[fd]->file_offset;
    
    curproc->p_filetable->files[fd]->file_offset = read_uio.uio_offset;
    
    *retval = curproc->p_filetable->files[fd]->file_offset - prev_offset;
        
    return 0;
}


ssize_t
write(int fd, const void *buf, size_t nbytes, int32_t *retval)
{
    if (fd < 0 || fd >= OPEN_MAX || curproc->p_filetable->files[fd] == NULL)
        return EBADF;
        
    if (curproc->p_filetable->files[fd]->mode != O_WRONLY 
        || curproc->p_filetable->files[fd]->mode != O_RDWR)
        return EBADF;
        
    if (buf == NULL)
        return EFAULT;
    
    // Create struct uio (which also needs a struct iovec) so that VOP_READ 
    // can be called
    struct uio write_uio;
    struct iovec write_iovec;
    
    write_iovec.iov_ubase = buf;
    write_iovec.iov_len = nbytes;
    
    write_uio.uio_iov = &write_iovec;
    write_uio.uio_iovcnt = 1;
    write_uio.uio_offset = curproc->p_filetable->files[fd]->file_offset;
    write_uio.uio_resid = nbytes;
    write_uio.uio_segflg = UIO_USERSPACE;
    write_uio.uio_rw = UIO_WRITE;
    write_uio.uio_space = /*RIP*/;
    
    if (VOP_WRITE(curproc->p_filetable->files[fd]->file_vnode, &read_uio))
        return EIO; //TODO: I think this is how you would do it
    
    unsigned int prev_offset = curproc->p_filetable->files[fd]->file_offset;
    
    curproc->p_filetable->files[fd]->file_offset = write_uio.uio_offset;
    
    *retval = curproc->p_filetable->files[fd]->file_offset - prev_offset;
        
    return 0;
}


off_t
lseek(int fd, off_t pos, int whence, off_t *retval)
{
    if (fd < 0 || fd >= OPEN_MAX || curproc->p_filetable->files[fd] == NULL)
        return EBADF;
        
    if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)
        return EINVAL;
        
    off_t new_pos;
    if (whence == SEEK_SET)
        new_pos = pos;
    else if (whence == SEEK_CUR)
        new_pos = curproc->p_filetable->files[fd]->file_offset + pos;
    else
        new_pos = /*TODO:end OF file*/ + pos;
        
    if (new_pos < 0)
        return EINVAL;
        
    if (VOP_ISSEEKABLE(curproc->p_filetable->files[fd]->file_vnode)
        return ESPIPE;
        
    curproc->p_filetable->files[fd]->file_offset = new_pos;
    *retval =  curproc->p_filetable->files[fd]->file_offset;
        
    return 0;
}
//
//
//int
//close(int fd)
//{
//    return 0;
//}
//
//
int
dup2(int oldfd, int newfd, int32_t *retval)
{
    if (newfd < 0 || newfd >= OPEN_MAX || newfd < 0 || newfd >= OPEN_MAX)
        return EBADF; 
    
    if(curproc->p_filetable->files[oldfd] == NULL)
        return EBADF;
        
    if (curproc->p_filetable.isfull /*TODO: Omar probably had something 
    like this in open*/)
        return EMFILE;
       
    if (curproc->p_filetable->files[newfd] != NULL)
        if (close(newfd))
            return -1/*TODO: Think the error is covered in close*/;
    
    curproc->p_filetable->files[newfd] = curproc->p_filetable->files[oldfd];
    curproc->p_filetable->files[newfd]->file_refcount++;
    *retval = newfd;
    return 0;
}

//int
//chdir(const char *pathname)
//{
//    return 0;
//}
//
//
//int
//__getcwd(char *buf, size_t buflen)
//{
//    return 0;
//}
