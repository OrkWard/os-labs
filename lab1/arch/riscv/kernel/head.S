.extern start_kernel
.extern _traps
.extern clock_set_next_event

    .section .text.init
    .globl _start
_start:
    la sp, boot_stack_top # 设置栈底指针

    la s0, _traps
    csrw stvec, s0 # 设置 trap 函数地址

    csrr t0, sie
    ori t0, t0, 1 << 5
    csrw sie, t0 # 启用时间中断

    csrr t0, sstatus
    ori t0, t0, 1 << 1
    csrw sstatus, t0 # 启用中断

    # 设置第一次中断
    jal clock_set_next_event

    jal start_kernel # 跳到 start_kernel

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096

    .globl boot_stack_top
boot_stack_top: