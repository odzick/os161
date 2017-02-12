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
#include <limits.h>
#include <vnode.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <uio.h>
#include <kern/iovec.h>
#include <kern/stat.h>
#include <kern/seek.h>

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

    result = file_create(filename, new_vnode, flags, mode, &new_file);
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

    result = file_create(filename, new_vnode, flags, mode, &new_file);
    if(result)
        return result;

    result = ft_add(ft, new_file, &fd); 
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
        
    if (curproc->p_filetable->files[fd]->file_flags != O_RDONLY 
        && curproc->p_filetable->files[fd]->file_flags != O_RDWR)
        return EBADF;
        
    if (buf == NULL)
        return EFAULT;
    
    // Create struct uio (which also needs a struct iovec) so that VOP_READ 
    // can be called
    struct uio read_uio;
    struct iovec read_iovec;

    uio_init(&read_iovec, &read_uio, buf, buflen, curproc->p_filetable->files[fd]->file_offset, UIO_READ);
    
    if (VOP_READ(curproc->p_filetable->files[fd]->file_vnode, &read_uio))
        return EIO; //TODO: I think this is how you would do it
    
    unsigned int prev_offset = curproc->p_filetable->files[fd]->file_offset;
    
    curproc->p_filetable->files[fd]->file_offset = read_uio.uio_offset;
    
    *retval = curproc->p_filetable->files[fd]->file_offset - prev_offset;
        
    return 0;
}


ssize_t
write(int fd, void *buf, size_t nbytes, int32_t *retval)
{
    if (fd < 0 || fd >= OPEN_MAX || curproc->p_filetable->files[fd] == NULL)
        return EBADF;
        
    if (curproc->p_filetable->files[fd]->file_flags == O_RDONLY) 
        return EBADF;
        
    if (buf == NULL)
        return EFAULT;
    
    // Create struct uio (which also needs a struct iovec) so that VOP_READ 
    // can be called
    struct uio write_uio;
    struct iovec write_iovec;
    
    uio_init(&write_iovec, &write_uio, buf, nbytes, curproc->p_filetable->files[fd]->file_offset, UIO_WRITE);

    if (VOP_WRITE(curproc->p_filetable->files[fd]->file_vnode, &write_uio))
        return EIO; //TODO: I think this is how you would do it
    
    unsigned int prev_offset = curproc->p_filetable->files[fd]->file_offset;
   
    curproc->p_filetable->files[fd]->file_offset = write_uio.uio_offset;
    
    *retval = curproc->p_filetable->files[fd]->file_offset - prev_offset;
        
    return 0;
}


int
lseek(int fd, off_t pos, int whence, off_t *retval)
{
    if (fd < 0 || fd >= OPEN_MAX || curproc->p_filetable->files[fd] == NULL)
        return EBADF;
        
    off_t new_pos;
    int result;
    struct stat *file_stat = NULL; 

    switch(whence){
        case SEEK_SET:
        new_pos =pos;
        break;

        case SEEK_CUR:
        new_pos = curproc->p_filetable->files[fd]->file_offset + pos;
        break;

        case SEEK_END:
        result = VOP_STAT(curproc->p_filetable->files[fd]->file_vnode, file_stat); 
        if(result)
            return result;
        new_pos = file_stat->st_size + pos;
        break;

        default :
        return EINVAL;
    }

    if (new_pos < 0)
        return EINVAL;
        
    if (!VOP_ISSEEKABLE(curproc->p_filetable->files[fd]->file_vnode))
        return ESPIPE;
        
    curproc->p_filetable->files[fd]->file_offset = new_pos;
    *retval =  curproc->p_filetable->files[fd]->file_offset;
        
    return 0;
}

int
close(int fd)
{
    int result;

    result = ft_remove(curproc->p_filetable, fd);
    if(result)
        return result;

    return 0;
}

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

int
dup2(int oldfd, int newfd, int32_t *retval)
{
    if (newfd < 0 || newfd >= OPEN_MAX || newfd < 0 || newfd >= OPEN_MAX)
        return EBADF; 
    
    if(curproc->p_filetable->files[oldfd] == NULL)
        return EBADF;
        
    //if (curproc->p_filetable.isfull /*TODO: Omar probably had something 
    //like this in open*/)
        //return EMFILE;
       
    if (curproc->p_filetable->files[newfd] != NULL)
        if (close(newfd))
            return -1/*TODO: Think the error is covered in close*/;
    
    curproc->p_filetable->files[newfd] = curproc->p_filetable->files[oldfd];
    curproc->p_filetable->files[newfd]->file_refcount++;
    *retval = newfd;
    return 0;
}
