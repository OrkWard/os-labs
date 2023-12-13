#include "printk.h"

void trap_handler(unsigned long scause, unsigned long sepc) {
  // 如果是timer interrupt 则打印输出相关信息, 并通过 `clock_set_next_event()`
  // 设置下一次时钟中断 `clock_set_next_event()` 见 4.3.4 节 其他interrupt /

  if (scause >> (64 - 1) & 1) {
    // is interrupt
    if ((scause & ((64 - 2) - 1)) == 5) {
      printk("Supervisor Timer Interrupt");
    }
  }
}