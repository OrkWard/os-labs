#include "defs.h"
#include "printk.h"

void trap_handler(unsigned long scause, unsigned long sepc) {
  asm volatile("li t0, 0\n"
               "csrw sstatus, t0\n" ::
                   : "memory", "t0");

  if (scause >> (64 - 1) & 1) {
    // is interrupt
    if (scause << 1 >> 1 == 5) {
      printk("Supervisor Timer Interrupt\n");
      clock_set_next_event();
    }
  }

  asm volatile("li t0, 0x2\n"
               "csrw sstatus, t0\n" ::
                   : "memory", "t0");
}