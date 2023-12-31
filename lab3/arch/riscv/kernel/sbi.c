#include "sbi.h"
#include "types.h"

struct sbiret sbi_ecall(int ext, int fid, uint64 arg0, uint64 arg1, uint64 arg2,
                        uint64 arg3, uint64 arg4, uint64 arg5) {
    struct sbiret ret;
    __asm__ volatile("mv a0, %[arg0]\n"
                     "mv a1, %[arg1]\n"
                     "mv a2, %[arg2]\n"
                     "mv a3, %[arg3]\n"
                     "mv a4, %[arg4]\n"
                     "mv a5, %[arg5]\n"
                     "mv a6, %[arg6]\n"
                     "mv a7, %[arg7]\n"
                     "ecall\n"
                     "mv %[error], a0\n"
                     "mv %[value], a1\n"
                     : [error] "=r"(ret.error), [value] "=r"(ret.value)
                     : [arg0] "r"(arg0), [arg1] "r"(arg1), [arg2] "r"(arg2),
                       [arg3] "r"(arg3), [arg4] "r"(arg4), [arg5] "r"(arg5),
                       [arg6] "r"(fid), [arg7] "r"(ext)
                     : "memory", "a0", "a1", "a2", "a3", "a3", "a4", "a5", "a6",
                       "a7");

    return ret;
}
