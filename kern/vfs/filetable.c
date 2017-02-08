#include <types.h>
#include <filetable.h>
#include <vnode.h>
#include <lib.h>

struct filetable* 
ft_create(void)
{
    int i;

    struct filetable* ft = (filetable*)kmalloc(sizeof(struct filetable));

    for(i = 0; i < MAX_FILES; i++)
        ft->files[i] = NULL;

    ft->last = 0;
    ft->ft_lock = lock_create("file_table_lock");

    return ft; 
}

void
ft_destroy(struct filetable* ft)
{
    lock_destroy(ft->ft_lock);
    kfree(ft);
}

int
ft_add(struct filetable* ft, struct file* file) 
{
   int i;
   int fd; 

   if(ft->files[ft->last] != NULL) 
       return -1;

    ft->files[ft->last] = file; 
    fd = ft->last;

    for(i = 0; i < MAX_FILES; i++)
    {
        if(ft->files[i] == NULL)
        {
            ft->last = i;
            break;
        }    
    }

    return fd;
}

struct file*
file_create(const char* filename, struct vnode* file_vnode, mode_t file_mode)
{
    struct file* fl = kmalloc(sizeof(struct file));
    fl->filename = filename;
    fl->file_vnode = file_vnode;
    fl->file_refcount = 1;
    fl->mode = file_mode;
    fl->file_offset = 0;
    fl->file_lock = lock_create("file lock");
    return fl;
}

void
file_destroy(struct *file fl)
{
    lock_destroy(fl->file_lock);
    kfree(fl);
}
