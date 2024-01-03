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
extern struct task_struct *task[];
extern uint64 __ret_from_fork;

extern uint64 swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

struct pt_regs {
    uint64 ra, gp, tp, t0, t1, t2, t3, t4, t5, t6;
    uint64 a0, a1, a2, a3, a4, a5, a6, a7;
    uint64 sepc, stval, sscratch;
    uint64 s[12], sp;
};

void syscall(struct pt_regs *regs) {
    switch (regs->a7) {
        case SYS_WRITE: {
            uint64 write_count
                = sys_write(regs->a0, (char *)regs->a1, regs->a2);
            regs->a0 = write_count;
            return;
        }
        case SYS_GETPID: {
            uint64 pid = sys_getpid();
            regs->a0 = pid;
            return;
        }
        case SYS_CLONE: {
            // 父进程 pid
            uint64_t pid = sys_getpid();
            // 子进程的内核栈
            struct task_struct *task_page = (void *)kalloc();
            // 复制
            memcpy(task_page, task[pid], PGSIZE);

            // 设置 thread
            task_page->thread.ra = (uint64)&__ret_from_fork;
            task_page->thread.sp = regs->sp + (long)task_page - (long)task[pid];
            task_page->thread.sepc = regs->sepc + 4;
            task_page->thread.sstatus = 1 << 5 | 1 << 18;
            task_page->thread.sscratch = regs->sscratch;
            for (int i = 0; i < 12; ++i) {
                task_page->thread.s[i] = regs->s[i];
            }

            // 子进程内核栈中保存的寄存器
            struct pt_regs *child_regs
                = (void *)((long)regs + (long)task_page - (long)task[pid]);
            // 子进程 fork 返回值 0
            child_regs->a0 = 0;
            child_regs->ra = regs->ra;

            // 复制内核页表
            task_page->pgtbl = (uint64 *)kalloc();
            memcpy(task_page->pgtbl, swapper_pg_dir, PGSIZE);

            // 复制已经映射的 vma
            for (int i = 0; i < current->vma_cnt; ++i) {
                if (current->vmas[i].mapped) {
                    struct vm_area_struct *vma = &task_page->vmas[i];
                    int64_t sz = vma->vm_end - vma->vm_start;
                    /* 页偏移 */
                    int64_t offset = vma->vm_start - PGROUNDDOWN(vma->vm_start);
                    int64_t addr = alloc_pages(PGROUNDUP(sz + offset) / PGSIZE);
                    if (vma->vm_flags & VM_ANONYM) {
                        memset((void *)addr + offset, '0', sz);
                    } else {
                        memcpy((void *)(addr + offset),
                               (void *)(vma->vm_content_offset_in_file
                                        + vma->file_offset_on_disk),
                               sz);
                    }

                    create_mapping(task_page->pgtbl, vma->vm_start,
                                   addr - PA2VA_OFFSET, sz,
                                   vma->vm_flags | 0b10001);
                    vma->mapped = true;

                    memcpy((void *)addr + offset, (void *)vma->vm_start, sz);
                }
            }

            for (int i = 0; i < MAX_TASKS; ++i) {
                if (task[i] == NULL) {
                    task[i] = task_page;
                    task_page->pid = i;
                    printk("Create Child pid %d, ", i);
                    printk("counter: %d\n", task_page->counter);
                    // 父进程 fork 调用返回子进程 pid
                    regs->a0 = i;
                    return;
                }
            }

            printk("Cannot create more tasks\n");
            while (1)
                ;
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
    vma->mapped = true;
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
                printk("scause: %lx, ", scause);
                printk("sepc: %lx\n", regs->sepc);
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
            case 12:
                printk("[S-mode] Instruction Page Fault\n");
                printk("  stval: %lx, ", regs->stval);
                printk("  sepc: %lx\n", regs->sepc);
                do_page_fault(regs);
                break;
            case 13:
                printk("[S-mode] Load Page Fault\n");
                printk("  stval: %lx, ", regs->stval);
                printk("  sepc: %lx\n", regs->sepc);
                do_page_fault(regs);
                break;
                break;
            case 15:
                printk("[S-mode] Store Page Fault Fault\n");
                printk("  stval: %lx, ", regs->stval);
                printk("  sepc: %lx\n", regs->sepc);
                do_page_fault(regs);
                break;
            default:
                printk("[S-mode] !Unhandled Exception\n");
                printk("  scause: %lx, ", scause);
                printk("  stval: %lx, ", regs->stval);
                printk("  sepc: %lx\n", regs->sepc);
                while (1)
                    ;
        }
    }

    // csr_write(sstatus, 0x2);
}