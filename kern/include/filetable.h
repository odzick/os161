#ifndef _FILETABLE_H_ 
#define _FILETABLE_H_

#include <vnode.h>
#include <synch.h>

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

struct filetable* ft_create(void);
void ft_destroy(struct filetable*);
int ft_add(struct filetable* ft, struct file* file); 
struct file* file_create(const char *filename, struct vnode *file_vnode, mode_t file_mode);
void file_destroy(void);

#endif /* _FILETABLE_H_ */
