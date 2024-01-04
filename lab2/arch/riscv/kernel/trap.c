#include "defs.h"
#include "printk.h"
#include "proc.h"

void trap_handler(unsigned long scause, unsigned long sepc) {
    // 防止在调度时触发中断，暂时关闭中断
    // csr_write(sstatus, 0);

    if (scause >> (64 - 1) & 1) {
        // is interrupt
        if (scause << 1 >> 1 == 5) {
            // printk("Supervisor Timer Interrupt\n");
            clock_set_next_event();

            // 切换进程
            do_timer();
        }
    }

    // csr_write(sstatus, 0x2);
}