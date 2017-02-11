#include<filetable.h>

int open(const char *filename, int flags, mode_t mode, int32_t *retval);
int open_ft(const char *filename, int flags, mode_t mode, int32_t *retval, struct filetable *ft);
int close(int fd);
int __getcwd(char *buf, size_t buflen, int32_t *retval);
int chdir(char *pathname);
