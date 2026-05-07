// call noop_action() for polymorphic no operation. Adds minor delays with random actions. 
// srand needs to be initiated in other (main) function.

#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>

// Timing randomization modes
#define TIMING_NONE      0
#define TIMING_BUSY_LOOP 1
#define TIMING_MATH_OPS  2
#define TIMING_SYSCALL   3
#define TIMING_CACHE     4
#define TIMING_RDTSC     5
#define TIMING_JITTER    6
#define TIMING_ADAPTIVE  7

#define NUM_TIMING_MODES 8      // Total number of delay methods

// Base delay range (multiples of ~1ms)
#define MIN_DELAY_UNITS 1
#define MAX_DELAY_UNITS 25

// Add random statistical noise to timing
void delay_jitter_entropy(void)
{
  // Collect entropy from various sources
  volatile int entropy = 0;

#ifdef __x86_64__
  uint64_t tsc;
  __asm__ volatile ("rdtsc":"=A" (tsc));
  entropy ^= (tsc & 0xFF);
#endif

  entropy ^= (rand() & 0xFF);
  entropy ^= (clock() & 0xFF);
  entropy ^= (getpid() & 0xFF);

  // Exponential backoff style delay
  volatile int delay = 1000;
  for (int i = 0; i < (entropy % 32); i++)
    {                           // Limit iterations
      delay = (int)((double)delay * 1.1);
      for (volatile int j = 0; j < delay; j++);
    }
}


// Self-calibrating timing randomization
void delay_adaptive_timing(int target_microseconds)
{
#if defined(__x86_64__) || defined(__i386__)
  // Calibrate loop length
  uint64_t start, end;
  __asm__ volatile ("rdtsc":"=A" (start));
  for (volatile int i = 0; i < 1000; i++);
  __asm__ volatile ("rdtsc":"=A" (end));

  uint64_t cycles_per_iter = (end - start) / 1000;
  uint64_t target_cycles = (uint64_t) target_microseconds * 2400;       // ~2.4GHz CPU

  __asm__ volatile ("rdtsc":"=A" (start));
  volatile uint64_t count = target_cycles / cycles_per_iter;
  for (volatile uint64_t i = 0; i < count; i++);
  __asm__ volatile ("rdtsc":"=A" (end));
#else
  // Fallback for non-x86
  usleep(target_microseconds);
#endif
}

// Individual delay methods (extracted from the switch)
void delay_busy_loop(int delay_units)
{
  volatile int counter = 0;
  for (int i = 0; i < delay_units * 10000; i++)
    {
      counter++;
    }
  (void)counter;
}

void delay_math_ops(int delay_units)
{
  volatile double result = 0.0;
  for (int i = 0; i < delay_units * 1000; i++)
    {
      result += sin((double)i) * cos((double)i);
      result -= tan((double)i);
    }
  (void)result;
}

void delay_syscall(int delay_units)
{
  for (int i = 0; i < delay_units; i++)
    {
      getpid();
    }
}

void delay_cache(int delay_units)
{
  // Cache-based timing (architecture dependent)
  static volatile uint8_t cache_array[4096 * 4] = { 0 };        // 16KB, static to persist
  volatile int index = (rand() % 4096) * 4;
  for (int i = 0; i < delay_units * 100; i++)
    {
      uint8_t val = cache_array[(index + i) % (4096 * 4)];
      (void)val;
    }
}

void delay_rdtsc(int delay_units)
{
#if defined(__x86_64__) || defined(__i386__)
  uint64_t start, current;
  uint64_t target_cycles = (uint64_t) delay_units * 100000;     // 100k cycles per unit
  __asm__ volatile ("rdtsc":"=A" (start));
  do
    {
      __asm__ volatile ("rdtsc":"=A" (current));
    }
  while ((current - start) < target_cycles);
#else
  // Fallback for non-x86
  delay_busy_loop(delay_units);
#endif
}

void delay_none(int delay_units)
{
  (void)delay_units;            // Do nothing, parameter unused
}

// Random delay that picks a method at runtime
void delay_random_timing(void)
{
  int delay_units =
    MIN_DELAY_UNITS + (rand() % (MAX_DELAY_UNITS - MIN_DELAY_UNITS));

  // Randomly select which delay method to use (0 to NUM_TIMING_MODES-1)
  int timing_mode = rand() % NUM_TIMING_MODES;

  switch (timing_mode)
    {
    case TIMING_NONE:
      delay_none(delay_units);
      break;

    case TIMING_BUSY_LOOP:
      delay_busy_loop(delay_units);
      break;

    case TIMING_MATH_OPS:
      delay_math_ops(delay_units);
      break;

    case TIMING_SYSCALL:
      delay_syscall(delay_units);
      break;

    case TIMING_CACHE:
      delay_cache(delay_units);
      break;

    case TIMING_RDTSC:
      delay_rdtsc(delay_units);
      break;

    case TIMING_JITTER:
      delay_jitter_entropy();
      break;

    case TIMING_ADAPTIVE:
      delay_adaptive_timing(delay_units * 1000);        // Convert to microseconds
      break;

    default:
      delay_busy_loop(delay_units);     // Fallback
      break;
    }
}


int noop()
{
  // Add random timing randomization BEFORE the NOP
  delay_random_timing();

  int noop_type = rand() % 3;

#if defined(__x86_64__) || defined(__i386__)
  // Vary which NOP based on noop_type
  switch (noop_type)
    {
    case 0:
      __asm__ volatile ("nop");
      break;
    case 1:
      __asm__ volatile ("xchg %%eax, %%eax":::"eax");
      break;
    case 2:
      __asm__ volatile ("lea (%%eax), %%eax":::"eax");
      break;
    }

#elif defined(__arm__) && !defined(__aarch64__)
  switch (noop_type)
    {
    case 0:
      __asm__ volatile ("mov r0, r0");
      break;
    case 1:
      __asm__ volatile ("nop");
      break;
    case 2:
      __asm__ volatile ("and r0, r0, r0");
      break;
    }

#elif defined(__aarch64__)
  switch (noop_type)
    {
    case 0:
      __asm__ volatile ("nop");
      break;
    case 1:
      __asm__ volatile ("hint #0");
      break;
    case 2:
      __asm__ volatile ("mov x0, x0");
      break;
    }

#elif defined(__powerpc__) || defined(__ppc__)
  switch (noop_type)
    {
    case 0:
      __asm__ volatile ("ori 0,0,0");
      break;
    case 1:
      __asm__ volatile ("nop");
      break;
    case 2:
      __asm__ volatile ("or 0,0,0");
      break;
    }

#else
#warning "No NOP defined for this architecture"
  return 1;
#endif

  // Add random timing randomization AFTER the NOP (could be different method)
  delay_random_timing();

  return 0;
}

int fake_noop()
{
  int noop_type = rand() % 10;

#if defined(__x86_64__) || defined(__i386__)
  // x86/x86-64: 10 different no-op equivalents with push/pop
  switch (noop_type)
    {
    case 0:
      __asm__ volatile ("nop");
      break;                    // No reg change

    case 1:
      __asm__ volatile (        // mov eax, eax
                         "push %%eax\n\t" "mov %%eax, %%eax\n\t" "pop %%eax":::"cc"     // only condition flags potentially affected
        );
      break;

    case 2:
      __asm__ volatile (        // lea (eax), eax
                         "push %%eax\n\t"
                         "lea (%%eax), %%eax\n\t" "pop %%eax":::"cc");
      break;

    case 3:
      __asm__ volatile (        // xchg eax, eax
                         "push %%eax\n\t"
                         "xchg %%eax, %%eax\n\t" "pop %%eax":::"cc");
      break;

    case 4:
      __asm__ volatile (        // add $0, eax
                         "push %%eax\n\t" "pushf\n\t"   // save flags
                         "add $0, %%eax\n\t" "popf\n\t" // restore flags
                         "pop %%eax":::);
      break;

    case 5:
      __asm__ volatile (        // sub $0, eax
                         "push %%eax\n\t"
                         "pushf\n\t"
                         "sub $0, %%eax\n\t" "popf\n\t" "pop %%eax":::);
      break;

    case 6:
      __asm__ volatile (        // and $0xFFFFFFFF, eax
                         "push %%eax\n\t"
                         "pushf\n\t"
                         "and $0xFFFFFFFF, %%eax\n\t"
                         "popf\n\t" "pop %%eax":::);
      break;

    case 7:
      __asm__ volatile (        // or $0, eax
                         "push %%eax\n\t"
                         "pushf\n\t"
                         "or $0, %%eax\n\t" "popf\n\t" "pop %%eax":::);
      break;

    case 8:
      __asm__ volatile (        // shl $0, eax
                         "push %%eax\n\t"
                         "pushf\n\t"
                         "shl $0, %%eax\n\t" "popf\n\t" "pop %%eax":::);
      break;

    case 9:
      __asm__ volatile (        // lea 0x00(eax), eax
                         "push %%eax\n\t"
                         "lea 0x00(%%eax), %%eax\n\t" "pop %%eax":::"cc");
      break;
    }

#elif defined(__arm__) && !defined(__aarch64__)
  // ARM 32-bit: save/restore r0 and condition flags
  switch (noop_type)
    {
    case 0:
      __asm__ volatile ("nop");
      break;                    // No reg change (ARMv6K+)

    case 1:
      __asm__ volatile (        // mov r0, r0
                         "push {r0}\n\t" "mov r0, r0\n\t" "pop {r0}");
      break;

    case 2:
      __asm__ volatile (        // and r0, r0, r0
                         "push {r0}\n\t" "push {r1}\n\t" "mrs r1, cpsr\n\t"     // save flags
                         "push {r1}\n\t" "and r0, r0, r0\n\t" "pop {r1}\n\t" "msr cpsr, r1\n\t" // restore flags
                         "pop {r1}\n\t" "pop {r0}");
      break;

    case 3:
      __asm__ volatile (        // orr r0, r0, #0
                         "push {r0}\n\t"
                         "push {r1}\n\t"
                         "mrs r1, cpsr\n\t"
                         "push {r1}\n\t"
                         "orr r0, r0, #0\n\t"
                         "pop {r1}\n\t"
                         "msr cpsr, r1\n\t" "pop {r1}\n\t" "pop {r0}");
      break;

    case 4:
      __asm__ volatile (        // eor r0, r0, #0
                         "push {r0}\n\t"
                         "push {r1}\n\t"
                         "mrs r1, cpsr\n\t"
                         "push {r1}\n\t"
                         "eor r0, r0, #0\n\t"
                         "pop {r1}\n\t"
                         "msr cpsr, r1\n\t" "pop {r1}\n\t" "pop {r0}");
      break;

    case 5:
      __asm__ volatile (        // add r0, r0, #0
                         "push {r0}\n\t"
                         "push {r1}\n\t"
                         "mrs r1, cpsr\n\t"
                         "push {r1}\n\t"
                         "add r0, r0, #0\n\t"
                         "pop {r1}\n\t"
                         "msr cpsr, r1\n\t" "pop {r1}\n\t" "pop {r0}");
      break;

    case 6:
      __asm__ volatile (        // sub r0, r0, #0
                         "push {r0}\n\t"
                         "push {r1}\n\t"
                         "mrs r1, cpsr\n\t"
                         "push {r1}\n\t"
                         "sub r0, r0, #0\n\t"
                         "pop {r1}\n\t"
                         "msr cpsr, r1\n\t" "pop {r1}\n\t" "pop {r0}");
      break;

    case 7:
      __asm__ volatile (        // bic r0, r0, #0
                         "push {r0}\n\t"
                         "push {r1}\n\t"
                         "mrs r1, cpsr\n\t"
                         "push {r1}\n\t"
                         "bic r0, r0, #0\n\t"
                         "pop {r1}\n\t"
                         "msr cpsr, r1\n\t" "pop {r1}\n\t" "pop {r0}");
      break;

    case 8:
      __asm__ volatile (        // mov r8, r8
                         "push {r8}\n\t" "mov r8, r8\n\t" "pop {r8}");
      break;

    case 9:
      __asm__ volatile (        // mov r0, r0 (alternate)
                         "push {r0}\n\t" "mov r0, r0\n\t" "pop {r0}");
      break;
    }

#elif defined(__aarch64__)
  // ARM 64-bit: save/restore x0 and condition flags
  switch (noop_type)
    {
    case 0:
      __asm__ volatile ("nop");
      break;                    // No reg change

    case 1:
      __asm__ volatile (        // mov x0, x0
                         "str x0, [sp, #-16]!\n\t"
                         "mov x0, x0\n\t" "ldr x0, [sp], #16");
      break;

    case 2:
      __asm__ volatile (        // and x0, x0, x0 (affects flags)
                         "str x0, [sp, #-16]!\n\t" "mrs x1, nzcv\n\t"   // save flags
                         "str x1, [sp, #-16]!\n\t" "and x0, x0, x0\n\t" "ldr x1, [sp], #16\n\t" "msr nzcv, x1\n\t"      // restore flags
                         "ldr x0, [sp], #16");
      break;

    case 3:
      __asm__ volatile (        // orr x0, x0, #0
                         "str x0, [sp, #-16]!\n\t"
                         "mrs x1, nzcv\n\t"
                         "str x1, [sp, #-16]!\n\t"
                         "orr x0, x0, #0\n\t"
                         "ldr x1, [sp], #16\n\t"
                         "msr nzcv, x1\n\t" "ldr x0, [sp], #16");
      break;

    case 4:
      __asm__ volatile (        // eor x0, x0, #0
                         "str x0, [sp, #-16]!\n\t"
                         "mrs x1, nzcv\n\t"
                         "str x1, [sp, #-16]!\n\t"
                         "eor x0, x0, #0\n\t"
                         "ldr x1, [sp], #16\n\t"
                         "msr nzcv, x1\n\t" "ldr x0, [sp], #16");
      break;

    case 5:
      __asm__ volatile (        // add x0, x0, #0
                         "str x0, [sp, #-16]!\n\t"
                         "mrs x1, nzcv\n\t"
                         "str x1, [sp, #-16]!\n\t"
                         "add x0, x0, #0\n\t"
                         "ldr x1, [sp], #16\n\t"
                         "msr nzcv, x1\n\t" "ldr x0, [sp], #16");
      break;

    case 6:
      __asm__ volatile (        // sub x0, x0, #0
                         "str x0, [sp, #-16]!\n\t"
                         "mrs x1, nzcv\n\t"
                         "str x1, [sp, #-16]!\n\t"
                         "sub x0, x0, #0\n\t"
                         "ldr x1, [sp], #16\n\t"
                         "msr nzcv, x1\n\t" "ldr x0, [sp], #16");
      break;

    case 7:
      __asm__ volatile (        // lsl x0, x0, #0
                         "str x0, [sp, #-16]!\n\t"
                         "mrs x1, nzcv\n\t"
                         "str x1, [sp, #-16]!\n\t"
                         "lsl x0, x0, #0\n\t"
                         "ldr x1, [sp], #16\n\t"
                         "msr nzcv, x1\n\t" "ldr x0, [sp], #16");
      break;

    case 8:
      __asm__ volatile (        // lsr x0, x0, #0
                         "str x0, [sp, #-16]!\n\t"
                         "mrs x1, nzcv\n\t"
                         "str x1, [sp, #-16]!\n\t"
                         "lsr x0, x0, #0\n\t"
                         "ldr x1, [sp], #16\n\t"
                         "msr nzcv, x1\n\t" "ldr x0, [sp], #16");
      break;

    case 9:
      __asm__ volatile (        // hint #0 (nop)
                         "hint #0");
      break;
    }

#elif defined(__powerpc__) || defined(__ppc__)
  // PowerPC: save/restore r0 and condition flags (CR0)
  switch (noop_type)
    {
    case 0:
      __asm__ volatile ("nop");
      break;                    // ori 0,0,0

    case 1:
      __asm__ volatile (        // or 0,0,0
                         "stw 0, -16(1)\n\t"    // save r0
                         "or 0,0,0\n\t" "lwz 0, -16(1)" // restore r0
        );
      break;

    case 2:
      __asm__ volatile (        // and 0,0,0 (affects CR0)
                         "stw 0, -16(1)\n\t" "mfcr 3\n\t"       // save condition register
                         "stw 3, -20(1)\n\t" "and 0,0,0\n\t" "lwz 3, -20(1)\n\t" "mtcr 3\n\t"   // restore condition register
                         "lwz 0, -16(1)":::"r3");
      break;

    case 3:
      __asm__ volatile (        // add 0,0,0
                         "stw 0, -16(1)\n\t" "add 0,0,0\n\t" "lwz 0, -16(1)");
      break;

    case 4:
      __asm__ volatile (        // sub 0,0,0
                         "stw 0, -16(1)\n\t" "sub 0,0,0\n\t" "lwz 0, -16(1)");
      break;

    case 5:
      __asm__ volatile (        // xor 0,0,0
                         "stw 0, -16(1)\n\t"
                         "mfcr 3\n\t"
                         "stw 3, -20(1)\n\t"
                         "xor 0,0,0\n\t"
                         "lwz 3, -20(1)\n\t"
                         "mtcr 3\n\t" "lwz 0, -16(1)":::"r3");
      break;

    case 6:
      __asm__ volatile (        // mr 0,0 (same as or)
                         "stw 0, -16(1)\n\t" "mr 0,0\n\t" "lwz 0, -16(1)");
      break;

    case 7:
      __asm__ volatile (        // ori 0,0,0 (nop)
                         "ori 0,0,0");
      break;

    case 8:
      __asm__ volatile (        // ori 31,31,0
                         "stw 31, -16(1)\n\t"
                         "ori 31,31,0\n\t" "lwz 31, -16(1)");
      break;

    case 9:
      __asm__ volatile (        // ori 0,0,0 (duplicate)
                         "ori 0,0,0");
      break;
    }

#else
#warning "No NOP defined for this architecture"
  return 1;
#endif

  return 0;
}


// Type definitions for function pointers
typedef void (*noop_func_t)(void);
typedef int (*junk_func_t)(void);       // if junk_inst returns int, adjust accordingly

// Randomized order version
int noop_action(int num_actions)
{
  for (int i = 0; i < num_actions; i++)
    {
      // Randomly choose the order of operations (3! = 6 possible orders)
      int order = rand() % 6;

      switch (order)
        {
        case 0:                // Original order
          noop();
          junk_inst();
          fake_noop();
          break;

        case 1:                // noop, fake_noop, junk
          noop();
          fake_noop();
          junk_inst();
          break;

        case 2:                // junk, noop, fake_noop
          junk_inst();
          noop();
          fake_noop();
          break;

        case 3:                // junk, fake_noop, noop
          junk_inst();
          fake_noop();
          noop();
          break;

        case 4:                // fake_noop, noop, junk
          fake_noop();
          noop();
          junk_inst();
          break;

        case 5:                // fake_noop, junk, noop
          fake_noop();
          junk_inst();
          noop();
          break;
        }
    }

  return 0;
}
