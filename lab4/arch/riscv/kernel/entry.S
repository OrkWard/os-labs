    .section .text.entry
    .align 2
    .globl _traps
_traps:
    # 交换 sp 与 sscratch，非 0 即为 U-mode 中断，直接跳转去切换
    csrrw sp, sscratch, sp
    bne sp, x0, SWITCH
    # 否则交换回来
    csrrw sp, sscratch, sp

SWITCH:
    # 1. save 32 registers and sepc to stack
    addi sp, sp, -200
    sd ra, 0(sp)
    sd gp, 8(sp)
    sd tp, 16(sp)
    sd t0, 24(sp)
    sd t1, 32(sp)
    sd t2, 40(sp)
    sd t3, 48(sp)
    sd t4, 56(sp)
    sd t5, 64(sp)
    sd t6, 72(sp)
    sd a0, 80(sp)
    sd a1, 88(sp)
    sd a2, 96(sp)
    sd a3, 104(sp)
    sd a4, 112(sp)
    sd a5, 120(sp)
    sd a6, 128(sp)
    sd a7, 136(sp)

    csrr t0, sepc
    sd t0, 144(sp) # save spec

    # 2. call trap_handler
    csrr a0, scause
    mv a1, t0 # spec
    mv a2, sp # regs
    jal trap_handler

    # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack
    ld t0, 144(sp)
    csrw sepc, t0 # restore sepc

    ld ra, 0(sp)
    ld gp, 8(sp)
    ld tp, 16(sp)
    ld t0, 24(sp)
    ld t1, 32(sp)
    ld t2, 40(sp)
    ld t3, 48(sp)
    ld t4, 56(sp)
    ld t5, 64(sp)
    ld t6, 72(sp)
    ld a0, 80(sp)
    ld a1, 88(sp)
    ld a2, 96(sp)
    ld a3, 104(sp)
    ld a4, 112(sp)
    ld a5, 120(sp)
    ld a6, 128(sp)
    ld a7, 136(sp)

    addi sp, sp, 200

    # 4. return from trap
    sret

    # return to function dummy
    .extern dummy
    .globl __dummy
__dummy:
    # exchange sp & sscratch
    # S-mode 栈与 U-mode 栈交换
    csrrw sp, sscratch, sp

    sret

    # process switch
    .globl __switch_to
__switch_to:
    # save state to prev process
    # a0 = prev, a1 = next
    sd ra, 48(a0)
    sd sp, 56(a0)
    sd s0, 64(a0)
    sd s1, 72(a0)
    sd s2, 80(a0)
    sd s3, 88(a0)
    sd s4, 96(a0)
    sd s5, 104(a0)
    sd s6, 112(a0)
    sd s7, 120(a0)
    sd s8, 128(a0)
    sd s9, 136(a0)
    sd s10, 144(a0)
    sd s11, 152(a0)
    csrr t0, sepc
    sd t0, 160(a0)
    csrr t0, sstatus
    sd t0, 168(a0)
    csrr t0, sscratch
    sd t0, 176(a0)

    # restore state from next process
    ld ra, 48(a1)
    ld sp, 56(a1)
    ld s0, 64(a1)
    ld s1, 72(a1)
    ld s2, 80(a1)
    ld s3, 88(a1)
    ld s4, 96(a1)
    ld s5, 104(a1)
    ld s6, 112(a1)
    ld s7, 120(a1)
    ld s8, 128(a1)
    ld s9, 136(a1)
    ld s10, 144(a1)
    ld s11, 152(a1)
    ld t0, 160(a1)
    csrw sepc, t0
    ld t0, 168(a1)
    csrw sstatus, t0
    ld t0, 176(a1)
    csrw sscratch, t0
    
    # 切换页表
    li t0, 0xffffffe000000000 - 0x80000000
    ld t1, 184(a1)
    # 物理地址
    sub t1, t1, t0
    # shift right 12
    srli t1, t1, 2
    # mode Sv39
    li t2, 8 << 60
    or t1, t1, t2
    csrw satp, t1

    sfence.vma
    fence.i

    ret
