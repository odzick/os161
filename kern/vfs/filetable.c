#include <types.h>
#include <filetable.h>
#include <lib.h>

struct filetable* 
ft_create(void)
{
    int i;

    struct filetable* ft = kmalloc(sizeof(struct filetable));

    for(i = 0; i < MAX_FILES; i++)
        ft->files[i] = NULL;

    ft->last = 0;
    ft->ft_lock = lock_create("file_table lock");

    return ft; 
}

void
ft_destroy(struct filetable* ft)
{
    lock_destroy(ft->ft_lock);
    kfree(ft);
}

//TODO: check every index for last
int
ft_add(struct filetable* ft, struct file* file) 
{
   int fd; 

   if(ft->files[ft->last] != NULL) 
       return -1;

    ft->files[ft->last] = file; 
    fd = ft->last++;
    ft->last++;

    return fd;
}
