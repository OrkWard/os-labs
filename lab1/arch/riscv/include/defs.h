#ifndef _DEFS_H
#define _DEFS_H

#include "types.h"

#define csr_read(csr)                                                          \
  ({                                                                           \
    uint64 __v;                                                                \
    asm volatile("csrr t0, " #csr "\n"                                         \
                 "mv %[value], t0"                                             \
                 : [value] "=r"(__v)                                           \
                 :                                                             \
                 : "memory", "t0");                                            \
    __v;                                                                       \
  })

#define csr_write(csr, val)                                                    \
  ({                                                                           \
    uint64 __v = (uint64)(val);                                                \
    asm volatile("mv t0, %0\n"                                                 \
                 "csrw " #csr ", t0"                                           \
                 :                                                             \
                 : "r"(__v)                                                    \
                 : "memory");                                                  \
  })

void clock_set_next_event();

#endif