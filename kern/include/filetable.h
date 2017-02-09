#ifndef _FILETABLE_H_ 
#define _FILETABLE_H_

#include <vnode.h>
#include <limits.h>
#include <synch.h>

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
        struct file *files[OPEN_MAX];
        int last;
};

struct filetable* ft_create(void);
void ft_destroy(struct filetable*);
int ft_add(struct filetable* ft, struct file* file, int* fd); 
int file_create(const char* filename, struct vnode* file_vnode, mode_t file_mode, struct file** ret_file);
void file_destroy(struct file* fl);

#endif /* _FILETABLE_H_ */
