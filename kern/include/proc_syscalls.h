#include <types.h>

int fork(struct trapframe *tf, pid_t *retval);
void child_entry(void *vtf, unsigned long junk);
int getpid(pid_t *retval);
