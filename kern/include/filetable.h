#ifndef _FILETABLE_H_ 
#define _FILETABLE_H_

#include<vnode.h>
#include<types.h>

#define MAX_FILES 32

/* Struct representing a file in the filetable */
struct file{
        const char *filename;
        struct vnode *file_vnode;
        int file_refcount;
        mode_t mode;
        unsigned int file_offset;
        struct lock *file_lock;
};

/* Struct representing a process filetable */
struct filetable{
        struct lock *ft_lock;
        struct file *files[MAX_FILES];
        int last;
};

filetable* ft_create(void);
void ft_destroy(filetable*);
void ft_add(filetable* ft, int fd, file* file) 

#endif /* _FILETABLE_H_ */
