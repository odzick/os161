#include<filetable.h>

int open(const char *filename, int flags, mode_t mode, int32_t *retval);
int open_ft(const char *filename, int flags, mode_t mode, int32_t *retval, struct filetable *ft);
int close(int fd);
int __getcwd(char *buf, size_t buflen, int32_t *retval);
int chdir(const char *pathname);
ssize_t read(int fd, void *buf, size_t buflen, int32_t *retval);
ssize_t write(int fd, void *buf, size_t nbytes, int32_t *retval);
int lseek(int fd, off_t pos, int whence, off_t *retval);
int dup2(int oldfd, int newfd, int32_t *retval);
