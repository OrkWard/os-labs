.extern start_kernel

    .section .text.init
    .globl _start
_start:
    la t0, _traps
    csrs stvec, t0 # 设置 trap 函数地址

    csrr t0, sie
    ori t0, t0, 1 << 5
    csrw sie, t0 # 启用时间中断

    csrr t0, sstatus
    ori t0, t0, 1 << 1
    csrw sstatus, t0 # 启用中断

    la sp, boot_stack_top # 设置栈底指针
    jal start_kernel # 跳到 start_kernel

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096

    .globl boot_stack_top
boot_stack_top: