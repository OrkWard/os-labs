#include "types.h"

#define NR_TASKS (1 + 3) // 初始化进程数量多
#define MAX_TASKS 16     // 最大进程数量

#define TASK_RUNNING 0 // 为了简化实验, 所有的线程都只有一种状态

#define PRIORITY_MIN 1
#define PRIORITY_MAX 10

#define VM_X_MASK 0x0000000000000008
#define VM_W_MASK 0x0000000000000004
#define VM_R_MASK 0x0000000000000002
#define VM_ANONYM 0x0000000000000001

#define MAX_INT 10000

typedef uint64 *pagetable_t;

/* 用于记录 `线程` 的 `内核栈与用户栈指针` */
/* (lab2 中无需考虑, 在这里引入是为了之后实验的使用) */
struct thread_info {
    uint64 kernel_sp;
    uint64 user_sp;
};

/* 线程状态段数据结构 */
struct thread_struct {
    uint64 ra;
    uint64 sp;
    uint64 s[12];

    uint64 sepc, sstatus, sscratch;
};

struct vm_area_struct {
    uint64 vm_start; /* VMA 对应的用户态虚拟地址的开始   */
    uint64 vm_end;   /* VMA 对应的用户态虚拟地址的结束   */
    uint64 vm_flags; /* VMA 对应的 flags */
    uint64 file_offset_on_disk;       /* 文件起始位置 */
    uint64 vm_content_offset_in_file; /* 如果对应了一个文件，
         那么这块 VMA 起始地址对应的文件内容相对文件起始位置的偏移量，
                           也就是 ELF 中各段的 p_offset 值 */

    uint64 vm_content_size_in_file; /* 对应的文件内容的长度。*/
    bool mapped;
};

/* 线程数据结构 */
struct task_struct {
    struct thread_info thread_info;
    uint64 state;    // 线程状态
    uint64 counter;  // 运行剩余时间
    uint64 priority; // 运行优先级 1最低 10最高
    uint64 pid;      // 线程id

    struct thread_struct thread;

    pagetable_t pgtbl;
    uint64 vma_cnt;
    struct vm_area_struct vmas[0];
};

/* 线程初始化 创建 NR_TASKS 个线程 */
void task_init();

/* 在时钟中断处理中被调用 用于判断是否需要进行调度 */
void do_timer();

/* 调度程序 选择出下一个运行的线程 */
void schedule();

/* 线程切换入口函数*/
void switch_to(struct task_struct *next);

/* dummy funciton: 一个循环程序, 循环输出自己的 pid 以及一个自增的局部变量 */
void dummy();

void do_mmap(struct task_struct *task, uint64 addr, uint64 length, uint64 flags,
             uint64 vm_content_offset_in_file, uint64 vm_content_size_in_file);

/* 创建一个 vma */
struct vm_area_struct *find_vma(struct task_struct *task, uint64 addr);