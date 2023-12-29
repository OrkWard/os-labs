#ifndef _DEFS_H
#define _DEFS_H

#include "types.h"

#define PGSIZE 0x1000 // 4KB
#define PGROUNDUP(addr) ((addr + PGSIZE - 1) & (~(PGSIZE - 1)))
#define PGROUNDDOWN(addr) (addr & (~(PGSIZE - 1)))

#define OPENSBI_SIZE (0x200000)

#define PHY_START 0x0000000080000000
#define PHY_SIZE 128 * 1024 * 1024 // 128MB，QEMU 默认内存大小
#define PHY_END (PHY_START + PHY_SIZE)

#define VM_START (0xffffffe000000000)
#define VM_END (0xffffffff00000000)
#define VM_SIZE (VM_END - VM_START)

#define USER_START (0x0000000000000000) // user space start virtual address
#define USER_END (0x0000004000000000)   // user space end virtual address

#define PA2VA_OFFSET (VM_START - PHY_START)

#define csr_read(csr)                                                          \
    ({                                                                         \
        uint64 __v;                                                            \
        asm volatile("csrr t0, " #csr "\nmv %[value], t0"                      \
                     : [value] "=r"(__v)                                       \
                     :                                                         \
                     : "memory", "t0");                                        \
        __v;                                                                   \
    })

#define csr_write(csr, val)                                                    \
    ({                                                                         \
        uint64 __v = (uint64)(val);                                            \
        asm volatile("csrw " #csr ", %0" : : "r"(__v) : "memory");             \
    })

void clock_set_next_event();

#endif