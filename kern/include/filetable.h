#ifndef _FILETABLE_H_ 
#define _FILETABLE_H_

#include <vnode.h>
#include <limits.h>
#include <synch.h>

/* Struct representing an open file in the filetable */
struct file{
        const char *filename;
        struct vnode *file_vnode;
        int file_refcount;
        int file_flags;
        mode_t file_mode;
        off_t file_offset;
        struct lock *file_lock;
};

/* Struct representing a process filetable */
struct filetable{
        struct lock *ft_lock;
        struct file *files[OPEN_MAX];
};

struct filetable* ft_create(void);
void ft_destroy(struct filetable*);
int ft_add(struct filetable* ft, struct file* file, int* fd); 
int file_create(const char* filename, struct vnode* file_vnode, int flags, mode_t file_mode, struct file** ret_file);
void file_destroy(struct file* fl);
int ft_init(struct filetable *ft);
int ft_remove(struct filetable* ft, int fd);
int ft_copy(struct filetable* ft, struct filetable* new_ft);
#endif /* _FILETABLE_H_ */
