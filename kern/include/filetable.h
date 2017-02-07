
#ifndef _FILETABLE_H_ 
#define _FILETABLE_H_

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
};

filetable* ft_create(void);

#endif /* _FILETABLE_H_ */
