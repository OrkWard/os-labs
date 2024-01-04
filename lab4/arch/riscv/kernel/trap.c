#include "defs.h"
#include "printk.h"
#include "proc.h"
#include "syscall.h"
#include "types.h"

struct pt_regs {
    uint64 ra, gp, tp, t0, t1, t2, t3, t4, t5, t6;
    uint64 a0, a1, a2, a3, a4, a5, a6, a7;
    uint64 sepc, stval;
};

void syscall(struct pt_regs *regs) {
    switch (regs->a7) {
        case SYS_WRITE: {
            uint64 write_count
                = sys_write(regs->a0, (char *)regs->a1, regs->a2);
            regs->a0 = write_count;
            break;
        }
        case SYS_GETPID: {
            uint64 pid = sys_getpid();
            regs->a0 = pid;
            break;
        }
    }
}

void trap_handler(unsigned long scause, unsigned long sepc,
                  struct pt_regs *regs) {
    // 防止在调度时触发中断，暂时关闭中断
    // csr_write(sstatus, 0);

    if (scause >> (64 - 1) & 1) {
        // is interrupt
        switch (scause << 1 >> 1) {
            case 5:
                // printk("[S-mode] Supervisor Timer Interrupt\n");
                clock_set_next_event();

                // 切换进程
                do_timer();
                break;
            default:
                printk("[S-mode] !Unhandled Interrupt\n");
                printk("  scause: %d, sepc: %lx, stval: %lx\n", scause, sepc,
                       regs->stval);
                while (1)
                    ;
        }
    } else {
        switch (scause << 1 >> 1) {
            case 8:
                // printk("[S-mode] User Environment Call\n");
                syscall(regs);
                regs->sepc += 4;
                break;
            default:
                printk("[S-mode] !Unhandled Exception\n");
                printk("  scause: %d, sepc: %lx, stval: %lx\n", scause, sepc,
                       regs->stval);
                while (1)
                    ;
        }
    }

    // csr_write(sstatus, 0x2);
}