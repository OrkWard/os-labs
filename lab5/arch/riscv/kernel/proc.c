#include "proc.h"
#include "defs.h"
#include "elf.h"
#include "mm.h"
#include "printk.h"
#include "rand.h"
#include "string.h"
#include "test.h"
#include "types.h"
#include "vm.h"

extern void __dummy();
extern void __switch_to(struct task_struct *prev, struct task_struct *next);
extern uint64 swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));
extern uint64 _sramdisk[];
extern uint64 _eramdisk[];

struct task_struct *idle;           // idle process
struct task_struct *current;        // 指向当前运行线程的 `task_struct`
struct task_struct *task[NR_TASKS]; // 线程数组, 所有的线程都保存在此

/**
 * new content for unit test of 2023 OS lab2
 */
extern uint64
    task_test_priority[]; // test_init 后, 用于初始化 task[i].priority 的数组
extern uint64
    task_test_counter[]; // test_init 后, 用于初始化 task[i].counter  的数组

void load_elf(struct task_struct *user_task) {
    // elf header
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)_sramdisk;
    printk("elf ident: %lx\n", *(uint64 *)(ehdr->e_ident));

    // elf segments 起始
    uint64 _sphdr = (uint64)ehdr + ehdr->e_phoff;
    int phdr_num = ehdr->e_phnum;

    for (int i = 0; i < phdr_num; ++i) {
        Elf64_Phdr *phdr = (Elf64_Phdr *)(_sphdr + sizeof(Elf64_Phdr) * i);
        if (phdr->p_type == PT_LOAD) {
            // 分配内存
            uint64 sz = phdr->p_memsz;
            // 链接时，没有 4KB 对齐，需要处理偏移
            uint64 offset
                = (uint64)(phdr->p_vaddr) - PGROUNDDOWN(phdr->p_vaddr);
            uint64 addr = alloc_pages(PGROUNDUP(sz + offset) / PGSIZE);

            // 复制段
            memcpy((void *)(addr + offset),
                   (void *)((uint64)ehdr + phdr->p_offset), sz);

            create_mapping(user_task->pgtbl, (uint64)phdr->p_vaddr,
                           addr - PA2VA_OFFSET, sz,
                           phdr->p_flags << 1 | 0b10001);
        }
    }

    // sepc 初始化为 e_entry
    user_task->thread.sepc = ehdr->e_entry;
    // sscratch 设置为 USER_END
    user_task->thread.sscratch = USER_END;
}

void task_init() {
    test_init(NR_TASKS);

    // 初始化 idle 进程
    uint64 page_bottom = kalloc();
    idle = (void *)page_bottom;

    idle->state = TASK_RUNNING;
    idle->counter = 0;
    idle->priority = 0;
    idle->pid = 0;
    // idle 不参与调度，thread 不用初始化

    // 设置当前进程为 idle
    current = idle;
    task[0] = idle;

    // 初始化其他线程
    int i;
    for (i = 1; i < NR_TASKS; ++i) {
        page_bottom = kalloc();
        task[i] = (void *)page_bottom;
        task[i]->pid = i;
        task[i]->state = TASK_RUNNING;
        task[i]->counter = task_test_counter[i];
        task[i]->priority = task_test_priority[i];
        // ra 初始化为 __dummy 地址
        task[i]->thread.ra = (uint64)&__dummy;
        task[i]->thread.sp = page_bottom + PGSIZE;
        // bit 8: SPP, bit 5: SPIE, bit 18: SUM
        task[i]->thread.sstatus = 1 << 5 | 1 << 18;

        // 创建页表
        task[i]->pgtbl = (uint64 *)kalloc();
        // 复制内核页表
        memcpy(task[i]->pgtbl, swapper_pg_dir, PGSIZE);

        // 栈段，可读可写，用户可访问，有效
        uint64 stack = alloc_page();
        create_mapping(task[i]->pgtbl, USER_END - PGSIZE, stack - PA2VA_OFFSET,
                       PGSIZE, 0b10111);

#ifdef USER_RAW
        // sepc 初始化为 USER_START
        task[i]->thread.sepc = USER_START;
        // sscratch 设置为 USER_END
        task[i]->thread.sscratch = USER_END;

        // 分配用户程序所需的空间
        uint64 sz = ((uint64)_eramdisk - (uint64)_sramdisk);
        uint64 uapp = alloc_pages(PGROUNDUP(sz) / PGSIZE);
        // 整个加载进内存
        memcpy((uint64 *)uapp, _sramdisk, sz);
        // 映射虚拟地址
        // 程序段，可读可写可执行，用户可访问，有效
        create_mapping(task[i]->pgtbl, USER_START, uapp - PA2VA_OFFSET, sz,
                       0b11111);
#else
        load_elf(task[i]);
#endif
    }

    printk("...proc_init done!\n");
}

void dummy() {
    schedule_test();
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while (1) {
        if ((last_counter == -1 || current->counter != last_counter)
            && current->counter > 0) {
            if (current->counter == 1) {
                --(current->counter); // forced the counter to be zero if this
                                      // thread is going to be scheduled
            } // in case that the new counter is also 1, leading the information
              // not printed.
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. auto_inc_local_var = %d, priority = "
                   "%d\n",
                   current->pid, auto_inc_local_var, current->priority);
        }
    }
}

void switch_to(struct task_struct *next) {
    if (next != current) {
        --next->counter;

        struct task_struct *prev = current;
        current = next;
        __switch_to(prev, next);
    }
}

void do_timer() {
    if (current == idle) {
        schedule();
    } else {
        if (current->counter) {
            --current->counter;
            return;
        }
        schedule();
    }
}

void schedule() {
    int i;
#ifdef SJF
    // skip idle
    int min_task_id = 0;
    int min_counter = MAX_INT;
    for (i = 1; i < NR_TASKS; ++i) {
        if (task[i]->counter && task[i]->counter < min_counter) {
            min_counter = task[i]->counter;
            min_task_id = i;
        }
    }

    // min_counter 为 0 时调度失败，此时所有 counter 均为 0
    if (min_task_id) {
        switch_to(task[min_task_id]);
    } else {
        for (i = 1; i < NR_TASKS; ++i) {
            // 随机赋值
            task[i]->counter = rand();
        }
        schedule();
    }
#else
    int max_task_id = 0;
    int max_counter = -1;
    for (i = 1; i < NR_TASKS; ++i) {
        if (task[i]->counter && (int)(task[i]->counter) >= max_counter) {
            max_counter = task[i]->counter;
            max_task_id = i;
        }
    }

    // min_counter 为 0 时调度失败，此时所有 counter 均为 0
    if (max_task_id) {
        switch_to(task[max_task_id]);
    } else {
        for (i = 1; i < NR_TASKS; ++i) {
            // 优先级赋值
            // linux 0.11 的实现为同时考虑 counter 和 prioriy
            // 由于 couter 此处必为零，因此不必考虑
            task[i]->counter = task[i]->priority;
        }
        schedule();
    }
#endif
}