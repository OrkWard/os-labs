#include "sbi.h"
#include "types.h"

unsigned long TIMECLOCK = 10000000;

unsigned long get_cycles() {
  uint64 time;
  asm volatile("rdtime t0\nmv %[value], t0"
               : [value] "=r"(time)
               :
               : "memory", "t0");
  return time;
}

void clock_set_next_event() {
  unsigned long next = get_cycles() + TIMECLOCK;

  sbi_ecall(SBI_SET_TIMER, 0, next, 0, 0, 0, 0, 0);
}