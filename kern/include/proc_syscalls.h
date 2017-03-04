#include <types.h>

int fork(struct trapframe *tf, pid_t *retval);
void child_entry(void *vtf, unsigned long junk);
int getpid(pid_t *retval);
void _exit(int exitcode);
//int execv(const char *program, char **args)
//int waitpid(pid_t pid, int *status, int options, pid_t *retval);
