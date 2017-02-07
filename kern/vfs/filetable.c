#include<filetable.h>
#include<vnode.h>
#include<types.h>

filetable* 
ft_create(void)
{
    int i;

    struct filetable* ft = kmalloc(sizeof(struct filetable));

    for(i = 0; i < MAX_FILES; i++)
        ft->files[i] = NULL;

    ft->last = 0;
    ft->lock = lock_create("file_table lock");

    return ft; 
}

void
ft_destroy(filetable* ft)
{
    lock_destroy(ft->lock);
    kfree(ft);
}

//TODO: check every index for last
void
ft_add(filetable* ft, int fd, file* file) 
{
   int i; 

   if(ft->files[ft->last] != NULL) 
       return;

    ft->files[ft->last] = file; 
}

void
ft_remove()
{
}
