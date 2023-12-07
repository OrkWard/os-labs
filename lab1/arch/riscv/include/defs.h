#ifndef _DEFS_H
#define _DEFS_H

#include "types.h"

#define csr_read(csr)                                                          \
  ({                                                                           \
    register uint64 __v;                                                       \
    asm volatile("csrr t0, " #csr "\nmv %[value], t0"                          \
                 : [value] "=r"(__v)                                           \
                 :                                                             \
                 : "memory", "t0") __v;                                        \
  })

#define csr_write(csr, val)                                                    \
  ({                                                                           \
    uint64 __v = (uint64)(val);                                                \
    asm volatile("csrw " #csr ", %0" : : "r"(__v) : "memory");                 \
  })

#endif
