#include "syscall.h"
#include "printk.h"
#include "proc.h"

extern struct task_struct *current;

uint64_t sys_write(unsigned int fd, const char *buf, size_t count) {
    if (fd == 1) {
        printk("%s", buf);
        return count;
    }
    return 0;
}

uint64_t sys_getpid() {
    return current->pid;
}