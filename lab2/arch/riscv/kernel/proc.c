#include "proc.h"
#include "defs.h"
#include "mm.h"
#include "printk.h"
#include "rand.h"
#include "test.h"
#include "types.h"

extern void __dummy();
extern void __switch_to(struct task_struct *prev, struct task_struct *next);

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

void task_init() {
    test_init(NR_TASKS);

    // 初始化物理内存
    mm_init();

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
            printk("[PID = %d] is running. auto_inc_local_var = %d\n",
                   current->pid, auto_inc_local_var);
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