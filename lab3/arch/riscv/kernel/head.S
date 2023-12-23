.extern start_kernel
.extern _traps
.extern clock_set_next_event
.extern task_init
.extern setup_vm
.extern relocate
.extern early_pgtbl

    .section .text.init
    .globl _start
_start:
    la t0, boot_stack_top
    li t1, 0xffffffe000000000 - 0x80000000
    sub sp, t0, t1

    jal setup_vm
    jal relocate

    la sp, boot_stack_top # 设置栈底指针

    la s0, _traps
    csrw stvec, s0 # 设置 trap 函数地址

    li t0, 1 << 5
    csrw sie, t0 # 启用时间中断

    li t0, 1 << 1
    csrw sstatus, t0 # 启用中断
    
    # 初始化进程
    jal task_init

    # 设置第一次中断
    jal clock_set_next_event

    j start_kernel # 跳到 start_kernel

relocate:
    # set ra = ra + PA2VA_OFFSET
    li t0, 0xffffffe000000000 - 0x80000000
    add ra, ra, t0
    add sp, sp, t0

    # set satp with early_pgtbl
    la t1, early_pgtbl
    # substruct to get physical location
    sub t1, t1, t0
    srli t1, t1, 12

    # mode Sv39
    li t2, 8 << 60
    or t1, t1, t2
    
    # write MODE | PPN to satp
    csrw satp, t1

    # flush tlb
    sfence.vma
    
    # flush fence
    fence.i

    ret

    .section .bss.stack
    .globl boot_stack

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096

    .globl boot_stack_top
boot_stack_top: