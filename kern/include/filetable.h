
#ifndef _FILETABLE_H_ 
#define _FILETABLE_H_

#include<vnode.h>
#include<type.h>

#define MAX_FILES 30

struct file{
        struct *vnode ft_vnode;
        mode_t mode;
        unsigned int offset;
        struct lock *file_lock;
};

struct filetable{
        struct lock *ft_lock;
        struct file *files[MAX_FILES];
        int last;
};

filetable* ft_create(void);
void ft_destroy(filetable*);
void ft_add(filetable* ft, int fd, file* file) 

#endif /* _FILETABLE_H_ */
