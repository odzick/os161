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

    char buff[PATH_MAX];

    result = copyinstr((const_userptr_t)filename, buff, PATH_MAX, NULL);
    if(result)
        return result;

    result = vfs_open(buff, flags, mode,  &new_vnode);
    if(result)
        return result;

    result = file_create(buff, new_vnode, flags, mode, &new_file);
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
    int result; 

    if (fd < 0 || fd >= OPEN_MAX || curproc->p_filetable->files[fd] == NULL)
        return EBADF;
        
    if ((curproc->p_filetable->files[fd]->file_flags & 3) == O_WRONLY)
        return EBADF;
        
    if (buf == NULL)
        return EFAULT;
    
    // Create struct uio (which also needs a struct iovec) so that VOP_READ 
    // can be called
    struct uio read_uio;
    struct iovec read_iovec;

    uio_init(&read_iovec, &read_uio, buf, buflen, curproc->p_filetable->files[fd]->file_offset, UIO_READ);
    
    result =VOP_READ(curproc->p_filetable->files[fd]->file_vnode, &read_uio);
    if (result)
        return result;
        
    // Set new file_offset and return the difference 
    off_t prev_offset = curproc->p_filetable->files[fd]->file_offset;
    curproc->p_filetable->files[fd]->file_offset = read_uio.uio_offset;
    *retval = curproc->p_filetable->files[fd]->file_offset - prev_offset;
        
    return 0;
}


ssize_t
write(int fd, void *buf, size_t nbytes, int32_t *retval)
{
    int result;

    if (fd < 0 || fd >= OPEN_MAX || curproc->p_filetable->files[fd] == NULL)
        return EBADF;
        
    if ((curproc->p_filetable->files[fd]->file_flags & 3) == O_RDONLY) 
        return EBADF;
        
    if (buf == NULL)
        return EFAULT;
    
    // Create struct uio (which also needs a struct iovec) so that VOP_WRITE 
    // can be called
    struct uio write_uio;
    struct iovec write_iovec;
    
    uio_init(&write_iovec, &write_uio, buf, nbytes, curproc->p_filetable->files[fd]->file_offset, UIO_WRITE);

    result = VOP_WRITE(curproc->p_filetable->files[fd]->file_vnode, &write_uio);
    if (result)
        return result; 
        
    // Set new file_offset and return the difference
    off_t prev_offset = curproc->p_filetable->files[fd]->file_offset;
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
    struct stat file_stat; 

    switch(whence){
        case SEEK_SET:
        new_pos =pos;
        break;

        case SEEK_CUR:
        new_pos = curproc->p_filetable->files[fd]->file_offset + pos;
        break;

        case SEEK_END:
        result = VOP_STAT(curproc->p_filetable->files[fd]->file_vnode, &file_stat); 
        if(result)
            return result;
        new_pos = file_stat.st_size + pos;
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
chdir(const char *pathname)
{
    int result;
    char buff[PATH_MAX];

    result = copyinstr((const_userptr_t)pathname, buff, PATH_MAX, NULL);
    if(result)
        return result;

    result = vfs_chdir(buff);
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
    int result;
    if (oldfd < 0 || oldfd >= OPEN_MAX || newfd < 0 || newfd >= OPEN_MAX)
        return EBADF; 

    if(oldfd == newfd){
        *retval = newfd;        
        return 0;
    }

    if(curproc->p_filetable->files[oldfd] == NULL)
        return EBADF;
        
    if(oldfd == newfd){
        *retval = newfd;
        return 0;
    }
       
    if (curproc->p_filetable->files[newfd] != NULL){
        result = close(newfd);
        if(result)
            return result;
    }
    
    curproc->p_filetable->files[newfd] = curproc->p_filetable->files[oldfd];
    curproc->p_filetable->files[newfd]->file_refcount++;
    *retval = newfd;
    return 0;
}
