#include "defs.h"
#include "mm.h"
#include "printk.h"
#include "proc.h"
#include "syscall.h"
#include "types.h"
#include "vm.h"
#include <stdint.h>
#include <string.h>

extern struct task_struct *current;
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

void do_page_fault(struct pt_regs *regs) {
    struct vm_area_struct *vma = find_vma(current, regs->stval);

    if (!vma) {
        // 非法地址
        printk("[S-mode] !Invalid Address\n");
        printk("sepc: %lx\n", regs->sepc);
        printk("stval: %lx\n", regs->stval);
        while (1)
            ;
    }

    int64_t sz = vma->vm_end - vma->vm_start;
    /* 页偏移 */
    int64_t offset = vma->vm_start - PGROUNDDOWN(vma->vm_start);
    int64_t addr = alloc_pages(PGROUNDUP(sz + offset) / PGSIZE);
    if (vma->vm_flags & VM_ANONYM) {
        memset((void *)addr + offset, '0', sz);
    } else {
        memcpy(
            (void *)(addr + offset),
            (void *)(vma->vm_content_offset_in_file + vma->file_offset_on_disk),
            sz);
    }

    create_mapping(current->pgtbl, vma->vm_start, addr - PA2VA_OFFSET, sz,
                   vma->vm_flags | 0b10001);
}

void trap_handler(unsigned long scause, unsigned long sepc,
                  struct pt_regs *regs) {
    // 防止在调度时触发中断，暂时关闭中断
    // csr_write(sstatus, 0);

    if (scause >> (64 - 1) & 1) {
        // is interrupt
        switch (scause << 1 >> 1) {
            case 5:
                printk("[S-mode] Supervisor Timer Interrupt\n");
                clock_set_next_event();

                // 切换进程
                do_timer();
                break;
            default:
                printk("[S-mode] !Unhandled Interrupt\n");
                printk("scause: %lx, ", scause);
                printk("sepc: %lx\n", regs->sepc);
                while (1)
                    ;
        }
    } else {
        switch (scause << 1 >> 1) {
            case 8:
                printk("[S-mode] User Environment Call\n");
                syscall(regs);
                regs->sepc += 4;
                break;
            case 12:
                printk("[S-mode] Instruction Page Fault\n");
                printk("stval: %lx, ", regs->stval);
                printk("sepc: %lx\n", regs->sepc);
                do_page_fault(regs);
                break;
            case 13:
                printk("[S-mode] Load Page Fault\n");
                do_page_fault(regs);
                break;
            case 15:
                printk("[S-mode] Store Page Fault Fault\n");
                do_page_fault(regs);
                break;
            default:
                printk("[S-mode] !Unhandled Exception\n");
                printk("scause: %lx, ", scause);
                printk("stval: %lx, ", regs->stval);
                printk("sepc: %lx\n", regs->sepc);
                while (1)
                    ;
        }
    }

    // csr_write(sstatus, 0x2);
}