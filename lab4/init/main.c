#include "printk.h"
#include "proc.h"
#include "sbi.h"

extern void test();

int start_kernel() {
    printk("2023 Hello RISC-V\n");

    schedule();

    test(); // DO NOT DELETE !!!

    return 0;
}
