    .section .text.entry
    .align 2
    .globl _traps, context_stack, context_stack_top
_traps:
    # 1. save 32 registers and sepc to stack
    csrw sscratch, sp
    la sp, context_stack
    sd x0, 0(sp)
    sd x1, 8(sp)
    # don't save sp here
    sd x3, 24(sp)
    sd x4, 32(sp)
    sd x5, 40(sp)
    sd x6, 48(sp)
    sd x7, 56(sp)
    sd x8, 64(sp)
    sd x9, 72(sp)
    sd x10, 80(sp)
    sd x11, 88(sp)
    sd x12, 96(sp)
    sd x13, 104(sp)
    sd x14, 112(sp)
    sd x15, 120(sp)
    sd x16, 128(sp)
    sd x17, 136(sp)
    sd x18, 144(sp)
    sd x19, 152(sp)
    sd x20, 160(sp)
    sd x21, 168(sp)
    sd x22, 176(sp)
    sd x23, 184(sp)
    sd x24, 192(sp)
    sd x25, 200(sp)
    sd x26, 208(sp)
    sd x27, 216(sp)
    sd x28, 224(sp)
    sd x29, 232(sp)
    sd x30, 240(sp)
    sd x31, 248(sp)
    
    csrrw s0, sscratch, x0
    csrr s1, sepc
    sd s0, 16(sp) # save sp
    sd s1, 256(sp) # save spec

    # 2. call trap_handler
    csrr a0, scause
    jal trap_handler

    # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack
    la sp, context_stack
    ld s0, 256(sp)
    csrw sepc, s0 # restore sepc

    ld x0, 0(sp)
    ld x1, 8(sp)
    # don't restore sp here
    ld x3, 24(sp)
    ld x4, 32(sp)
    ld x5, 40(sp)
    ld x6, 48(sp)
    ld x7, 56(sp)
    ld x8, 64(sp)
    ld x9, 72(sp)
    ld x10, 80(sp)
    ld x11, 88(sp)
    ld x12, 96(sp)
    ld x13, 104(sp)
    ld x14, 112(sp)
    ld x15, 120(sp)
    ld x16, 128(sp)
    ld x17, 136(sp)
    ld x18, 144(sp)
    ld x19, 152(sp)
    ld x20, 160(sp)
    ld x21, 168(sp)
    ld x22, 176(sp)
    ld x23, 184(sp)
    ld x24, 192(sp)
    ld x25, 200(sp)
    ld x26, 208(sp)
    ld x27, 216(sp)
    ld x28, 224(sp)
    ld x29, 232(sp)
    ld x30, 240(sp)
    ld x31, 248(sp)

    ld sp, 16(sp) # restore sp

    # 4. return from trap
    sret

    # return to function dummy
    .extern dummy
    .globl __dummy
__dummy:
    la s0, dummy
    csrw sepc, s0

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

    ret

    .section .bss.context
context_stack:
    .space 8 * 36
    
context_stack_top: