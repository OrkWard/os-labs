#include "defs.h"
#include "printk.h"
#include "sbi.h"

extern void test();

int start_kernel() {
    printk("2023");
    printk(" Hello RISC-V\n");

    uint64 sstatus = csr_read(sstatus);
    printk("sstatus: %lx\n", sstatus);

    csr_write(sscratch, 0x114514);
    uint64 sscratch = csr_read(sscratch);
    printk("sscratch: %lx\n", sscratch);

    test(); // DO NOT DELETE !!!

    return 0;
}
