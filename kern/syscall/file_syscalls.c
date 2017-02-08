#include <types.h>
#include <copyinout.h>
#include <syscall.h>
#include <filetable.h>


int 
open(const char *filename, int flags, mode_t mode)
{
    vnode **new_vnode; 
    int result;

    result = vfs_open(filename, flags, mode,  new_vnode);

    if(result)
        return result;

    struct file* new_file = file_create(filename, *new_vnode, mode);

    return 0;
}


//ssize_t
//read(int fd, void *buf, size_t buflen)
//{
//    return 0;
//}
//
//
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
//int
//close(int fd)
//{
//    return 0;
//}
//
//
//int
//dup2(int oldfd, int newfd)
//{
//    return 0;
//}
//
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