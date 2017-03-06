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

/* opens stdin, stdout, stderr in a new filetable  */ 
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
    int i;

    lock_acquire(ft->ft_lock);
    for(i = 0; i < OPEN_MAX; i++){
        if(ft->files[i] != NULL){
            if(ft->files[i]->file_refcount == 1){
                file_destroy(ft->files[i]);
            }else{
                ft->files[i]->file_refcount--; 
            }
        }    
    }
    lock_release(ft->ft_lock);
    lock_destroy(ft->ft_lock);
    kfree(ft);
}

int
ft_add(struct filetable* ft, struct file* file, int* fd) 
{
    int i;

    lock_acquire(ft->ft_lock);
    for(i = 0; i < OPEN_MAX; i++){
        if(ft->files[i] == NULL){
            ft->files[i] = file;
            *fd = i;
            lock_release(ft->ft_lock);
            return 0;
        }    
    }
    lock_release(ft->ft_lock);

    return EMFILE;
}

int
ft_remove(struct filetable* ft, int fd) 
{
    if(fd < 0 || fd > (OPEN_MAX-1))
        return EBADF; 

    lock_acquire(ft->ft_lock);

    if(ft->files[fd] == NULL){
        lock_release(ft->ft_lock);
        return EBADF;
    }

    /* need to leave vnode and file open if another ref exists */
    if(ft->files[fd]->file_refcount == 1){
        vfs_close(ft->files[fd]->file_vnode);
        file_destroy(ft->files[fd]);
    }

    ft->files[fd]->file_refcount--;
    ft->files[fd] = NULL;

    lock_release(ft->ft_lock);
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

int
ft_copy(struct filetable* ft, struct filetable* new_ft){
    int i;

    KASSERT(ft != NULL);
    KASSERT(new_ft != NULL);

    lock_acquire(ft->ft_lock);
    lock_acquire(new_ft->ft_lock);
    for(i = 0; i < OPEN_MAX; i++){
        new_ft->files[i] = ft->files[i];
        if(new_ft->files[i] != NULL){
            new_ft->files[i]->file_refcount++;
        }
    }
    lock_release(new_ft->ft_lock);
    lock_release(ft->ft_lock);

    return 0;
}
