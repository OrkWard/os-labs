#include "proc.h"
#include "defs.h"
#include "mm.h"
#include "printk.h"
#include "rand.h"
#include "test.h"
#include "types.h"

extern void __dummy();
extern void __switch_to(struct task_struct *prev, struct task_struct *next);
// void __switch_to(struct task_struct *prev, struct task_struct *next) {
//     // save context
//     asm volatile("mv %0, sp\n"
//                  "mv %1, ra\n"
//                  "mv %2, s0\n"
//                  "mv %3, s1\n"
//                  "mv %4, s2\n"
//                  "mv %5, s3\n"
//                  "mv %6, s4\n"
//                  "mv %7, s5\n"
//                  "mv %8, s6\n"
//                  "mv %9, s7\n"
//                  "mv %10, s8\n"
//                  "mv %11, s9\n"
//                  "mv %12, s10\n"
//                  "mv %13, s11\n"
//                  : "=r"(prev->thread.sp), "=r"(prev->thread.ra),
//                    "=r"(prev->thread.s[0]), "=r"(prev->thread.s[1]),
//                    "=r"(prev->thread.s[2]), "=r"(prev->thread.s[3]),
//                    "=r"(prev->thread.s[4]), "=r"(prev->thread.s[5]),
//                    "=r"(prev->thread.s[6]), "=r"(prev->thread.s[7]),
//                    "=r"(prev->thread.s[8]), "=r"(prev->thread.s[9]),
//                    "=r"(prev->thread.s[10]), "=r"(prev->thread.s[11]));

//     // restore context
//     asm volatile("mv sp, %0\n"
//                  "mv ra, %1\n"
//                  "mv s0, %2\n"
//                  "mv s1, %3\n"
//                  "mv s2, %4\n"
//                  "mv s3, %5\n"
//                  "mv s4, %6\n"
//                  "mv s5, %7\n"
//                  "mv s6, %8\n"
//                  "mv s7, %9\n"
//                  "mv s8, %10\n"
//                  "mv s9, %11\n"
//                  "mv s10, %12\n"
//                  "mv s11, %13\n"
//                  :
//                  : "r"(next->thread.sp), "r"(next->thread.ra),
//                    "r"(next->thread.s[0]), "r"(next->thread.s[1]),
//                    "r"(next->thread.s[2]), "r"(next->thread.s[3]),
//                    "r"(next->thread.s[4]), "r"(next->thread.s[5]),
//                    "r"(next->thread.s[6]), "r"(next->thread.s[7]),
//                    "r"(next->thread.s[8]), "r"(next->thread.s[9]),
//                    "r"(next->thread.s[10]), "r"(next->thread.s[11]));
// }

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
    mm_init();

    // 初始化 idle 进程
    uint64 page_bottom = kalloc();
    idle = (void *)page_bottom;

    idle->thread.sp = page_bottom + PGSIZE;
    idle->state = TASK_RUNNING;
    idle->counter = 0;
    idle->priority = 0;
    idle->pid = 0;

    current = idle;
    task[0] = idle;

    // 初始化其他线程
    int i;
    for (i = 1; i < NR_TASKS; ++i) {
        page_bottom = kalloc();
        task[i] = (void *)page_bottom;
        task[i]->state = TASK_RUNNING;
        task[i]->counter = task_test_counter[i];
        task[i]->priority = task_test_priority[i];
        task[i]->thread.ra = (uint64)&__dummy;
        task[i]->thread.sp = page_bottom + PGSIZE;
    }

    printk("...proc_init done!\n");
}

// arch/riscv/kernel/proc.c
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
        __switch_to(current, next);
    }
}

void do_timer() {
    if (current == idle) {
        schedule();
    } else {
        if (--current->counter) {
            return;
        }
        schedule();
    }
}

void schedule() {
    int i;
    int min_task_id = 1;
    int min_counter = task[1]->counter;
    for (i = 2; i < NR_TASKS; ++i) {
        if (task[i]->counter < min_counter) {
            min_counter = task[i]->counter;
            min_task_id = i;
        }
    }

    switch_to(task[min_task_id]);
}