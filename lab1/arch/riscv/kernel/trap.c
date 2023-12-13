#include "defs.h"
#include "printk.h"

void trap_handler(unsigned long scause, unsigned long sepc) {
  asm volatile("csrr t0, sstatus\n"
               "andi t0, t0, 0x01\n"
               "csrw sstatus, t0\n" ::
                   : "memory", "t0");

  if (scause >> (64 - 1) & 1) {
    // is interrupt
    printk("%d\n", scause);
    if (scause << 1 >> 1 == 5) {
      printk("Supervisor Timer Interrupt");
      clock_set_next_event();
    }
  }

  asm volatile("csrr t0, sstatus\n"
               "ori t0, t0, 0x10\n"
               "csrw sstatus, t0\n" ::
                   : "memory", "t0");
}