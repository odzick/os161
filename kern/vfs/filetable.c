#include <types.h>
#include <filetable.h>
#include <kern/errno.h>
#include <lib.h>
#include <vfs.h>
#include <kern/fcntl.h>
#include <file_syscalls.h>

struct filetable* 
ft_create(void)
{
    int i;

    struct filetable* ft = kmalloc(sizeof(struct filetable));

    for(i = 0; i < OPEN_MAX; i++)
        ft->files[i] = NULL;

    ft->ft_lock = lock_create("file_table_lock");

    return ft; 
}

/* runprogram and in kernel main */ 
int
ft_init(struct filetable *ft)
{
   char stdin_path[5] = "con:";
   char stdout_path[5] = "con:";
   char stderr_path[5] = "con:";
   int fd; 
   int result;

   result = open_ft(stdin_path, O_RDONLY, 0664, &fd, ft);
   if(result)
       return result;
   KASSERT(fd == 0);

   result = open_ft(stdout_path, O_WRONLY, 0664, &fd, ft);
   if(result)
       return result;
   KASSERT(fd == 1);

   result = open_ft(stderr_path, O_WRONLY, 0664, &fd, ft);
   if(result)
       return result;
   KASSERT(fd == 2);

   return 0;
}

void
ft_destroy(struct filetable* ft)
{
    lock_destroy(ft->ft_lock);
    kfree(ft);
}

int
ft_add(struct filetable* ft, struct file* file, int* fd) 
{
   int i;

    for(i = 0; i < OPEN_MAX; i++){
        if(ft->files[i] == NULL){
            ft->files[i] = file;
            *fd = i;
            return 0;
        }    
    }

    return EMFILE;
}

int
ft_remove(struct filetable* ft, int fd) 
{
    if(fd < 3 || fd > (OPEN_MAX-1))
        return EBADF; 

    if(ft->files[fd] == NULL)
        return EBADF;

    if(ft->files[fd]->file_refcount == 1)
        vfs_close(ft->files[fd]->file_vnode);

    ft->files[fd]->file_refcount--;
    ft->files[fd] = NULL;

    return 0;
}

int 
file_create(const char* filename, struct vnode* file_vnode, int flags, mode_t file_mode, struct file** ret_file)
{
    struct file* fl = kmalloc(sizeof(struct file));
    fl->filename = filename;
    fl->file_vnode = file_vnode;
    fl->file_refcount = 1;
    fl->file_mode = file_mode;
    fl->file_flags = flags;
    fl->file_offset = 0;
    fl->file_lock = lock_create("file lock");

   *ret_file = fl;
    return 0;
}

void
file_destroy(struct file* fl)
{
    lock_destroy(fl->file_lock);
    kfree(fl);
}
