#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#define SYS_WRITE 1
#define SYS_EXIT  2

// C stubs (link in user programs)
int write(int fd, const char *buf, uint32_t len);
void exit(int code);

// Kernel internal entry point
void syscall_handler(uint32_t *esp);

#endif /* SYSCALL_H */
