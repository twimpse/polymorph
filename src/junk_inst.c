void junk_inst()
{
  // Randomly select a junk operation type (0-19 for more variety)
  int junk_type = rand() % 20;

  // Generate random values for the junk operations
  unsigned int junk_uint1 = rand();
  unsigned int junk_uint2 = rand();
  float junk_float1 = (float)rand() / (float)RAND_MAX;
  float junk_float2 = (float)rand() / (float)RAND_MAX;
  double junk_double1 = (double)rand() / (double)RAND_MAX;
  double junk_double2 = (double)rand() / (double)RAND_MAX;

  // Memory buffer for memory-based junk operations
  uint32_t junk_mem[16];
  for (int i = 0; i < 16; i++)
    {
      junk_mem[i] = rand();
    }

  volatile uint32_t junk_result32;
  volatile uint64_t junk_result64;
  volatile float junk_result_f;
  volatile double junk_result_d;

#if defined(__x86_64__) || defined(__i386__)
  // x86/x86-64: Integer, FPU, SSE, and memory junk operations
  switch (junk_type)
    {
      // ===== INTEGER OPERATIONS =====
    case 0:                    // Add two random numbers
      __asm__ volatile ("mov %1, %%eax\n\t"
                        "mov %2, %%ebx\n\t"
                        "add %%ebx, %%eax\n\t"
                        "mov %%eax, %0\n\t":"=r" (junk_result32):"r"
                        (junk_uint1), "r"(junk_uint2):"eax", "ebx", "cc");
      break;

    case 1:                    // Multiply then divide
      __asm__ volatile ("mov %2, %%eax\n\t"
                        "mov %3, %%ebx\n\t"
                        "mul %%ebx\n\t"
                        "div %%ebx\n\t"
                        "mov %%eax, %0\n\t":"=r" (junk_result32):"r"
                        (junk_uint1), "r"(junk_uint2), "r"(junk_uint2):"eax",
                        "ebx", "edx", "cc");
      break;

      // ===== FLOATING POINT (x87 FPU) =====
    case 2:                    // Float addition
      __asm__ volatile ("fld %2\n\t"
                        "fld %3\n\t"
                        "faddp\n\t"
                        "fstp %0\n\t":"=m" (junk_result_f):"m"(junk_float1),
                        "m"(junk_float2):"st", "st(1)");
      break;

    case 3:                    // Double multiplication with division
      __asm__ volatile ("fld %2\n\t"
                        "fld %3\n\t"
                        "fmulp\n\t"
                        "fld %4\n\t"
                        "fdivp\n\t"
                        "fstp %0\n\t":"=m" (junk_result_d):"m"(junk_double1),
                        "m"(junk_double2), "m"(junk_double2):"st", "st(1)",
                        "st(2)");
      break;

      // ===== SIMD (SSE) OPERATIONS =====
    case 4:                    // SSE packed float add
      __asm__ volatile ("movups %2, %%xmm0\n\t"
                        "movups %3, %%xmm1\n\t"
                        "addps %%xmm1, %%xmm0\n\t"
                        "movups %%xmm0, %0\n\t":"=m" (junk_mem[0]):"m"
                        (junk_float1), "m"(junk_float2):"xmm0", "xmm1",
                        "memory");
      break;

    case 5:                    // SSE2 packed integer multiply
      __asm__ volatile ("movdqu %2, %%xmm0\n\t"
                        "movdqu %3, %%xmm1\n\t"
                        "pmuludq %%xmm1, %%xmm0\n\t"
                        "movdqu %%xmm0, %0\n\t":"=m" (junk_mem[0]):"m"
                        (junk_mem[0]), "m"(junk_mem[4]):"xmm0", "xmm1",
                        "memory");
      break;

      // ===== MEMORY-BASED JUNK =====
    case 6:                    // Array access and modify
      __asm__ volatile ("mov %1, %%eax\n\t"
                        "and $0xF, %%eax\n\t"
                        "mov %2, %%ebx\n\t"
                        "and $0xF, %%ebx\n\t"
                        "mov %3[%%eax*4], %%ecx\n\t"
                        "mov %%ecx, %3[%%ebx*4]\n\t"
                        "mov %3[%%ebx*4], %0\n\t":"=r" (junk_result32):"r"
                        (junk_uint1), "r"(junk_uint2), "m"(junk_mem):"eax",
                        "ebx", "ecx", "memory", "cc");
      break;

    case 7:                    // Stack manipulation with computation
      __asm__ volatile ("push %1\n\t"
                        "push %2\n\t"
                        "pop %%eax\n\t"
                        "pop %%ebx\n\t"
                        "add %%ebx, %%eax\n\t"
                        "push %%eax\n\t"
                        "pop %0\n\t":"=r" (junk_result32):"r"(junk_uint1),
                        "r"(junk_uint2):"eax", "ebx", "memory", "cc");
      break;

    case 8:                    // String operation (rep movsb)
      {
        char src[64], dst[64];
        memcpy(src, junk_mem, 64);
        __asm__ volatile ("cld\n\t"
                          "mov %1, %%esi\n\t"
                          "mov %2, %%edi\n\t"
                          "mov $64, %%ecx\n\t"
                          "rep movsb\n\t"::"m" (src), "m"(dst):"esi", "edi",
                          "ecx", "memory", "cc");
      }
      break;

    case 9:                    // Atomic operation (harmless)
      __asm__ volatile ("lock addl $1, %0\n\t"
                        "lock subl $1, %0\n\t":"+m" (junk_mem[0])::"memory",
                        "cc");
      break;

      // ===== ADVANCED SIMD (AVX) =====
    case 10:                   // AVX vectorized operation
      __asm__ volatile ("vmovups %2, %%ymm0\n\t"
                        "vmovups %3, %%ymm1\n\t"
                        "vaddps %%ymm1, %%ymm0, %%ymm0\n\t"
                        "vmovups %%ymm0, %0\n\t"
                        "vzeroupper\n\t":"=m" (junk_mem[0]):"m"(junk_float1),
                        "m"(junk_float2):"ymm0", "ymm1", "memory");
      break;

      // ===== CRYPTO/HASH LIKE =====
    case 11:                   // CRC32 instruction (x86 SSE4.2)
      __asm__ volatile ("mov %1, %%eax\n\t"
                        "crc32 %2, %%eax\n\t"
                        "mov %%eax, %0\n\t":"=r" (junk_result32):"r"
                        (junk_uint1), "r"(junk_uint2):"eax", "cc");
      break;

      // ===== MISCELLANEOUS JUNK =====
    case 12:                   // RDTSC - read timestamp counter (just for time waste)
      __asm__ volatile ("rdtsc\n\t"
                        "mov %%eax, %0\n\t":"=r" (junk_result32)::"eax",
                        "edx", "cc");
      break;

    case 13:                   // CPUID (serializing instruction, wastes cycles)
      __asm__ volatile ("xor %%eax, %%eax\n\t"
                        "cpuid\n\t":::"eax", "ebx", "ecx", "edx");
      break;

    case 14:                   // FPU trig operation (sin/cos)
      __asm__ volatile ("fld %1\n\t"
                        "fsin\n\t"
                        "fstp %0\n\t":"=m" (junk_result_f):"m"
                        (junk_float1):"st", "cc");
      break;

    case 15:                   // FPU square root
      __asm__ volatile ("fld %1\n\t"
                        "fsqrt\n\t"
                        "fstp %0\n\t":"=m" (junk_result_f):"m"
                        (junk_double1):"st");
      break;

    case 16:                   // Bit scan forward
      __asm__ volatile ("mov %1, %%eax\n\t"
                        "bsf %%eax, %%eax\n\t"
                        "mov %%eax, %0\n\t":"=r" (junk_result32):"r"
                        (junk_uint1):"eax", "cc");
      break;

    case 17:                   // Byte swap
      __asm__ volatile ("mov %1, %%eax\n\t" "bswap %%eax\n\t" "bswap %%eax\n\t" // Swap back
                        "mov %%eax, %0\n\t":"=r" (junk_result32):"r"
                        (junk_uint1):"eax");
      break;

    case 18:                   // Cache line flush (if you really want to hurt performance)
      __asm__ volatile ("clflush %0\n\t"::"m" (junk_mem[0]):"memory");
      break;

    case 19:                   // Prefetch (harmless hint)
      __asm__ volatile ("prefetcht0 %0\n\t"::"m" (junk_mem[0]):"memory");
      break;
    }

#elif defined(__arm__) && !defined(__aarch64__)
  // ARM 32-bit: Integer, VFP, NEON, and memory junk
  switch (junk_type)
    {
      // ===== INTEGER OPERATIONS =====
    case 0:                    // Add with carry
      __asm__ volatile ("mov r0, %1\n\t"
                        "mov r1, %2\n\t"
                        "adds r2, r0, r1\n\t"
                        "mov %0, r2\n\t":"=r" (junk_result32):"r"(junk_uint1),
                        "r"(junk_uint2):"r0", "r1", "r2", "cc");
      break;

    case 1:                    // Multiply-accumulate
      __asm__ volatile ("mov r0, %1\n\t"
                        "mov r1, %2\n\t"
                        "mov r2, %3\n\t"
                        "mla r3, r0, r1, r2\n\t"
                        "mov %0, r3\n\t":"=r" (junk_result32):"r"(junk_uint1),
                        "r"(junk_uint2), "r"(junk_uint1):"r0", "r1", "r2",
                        "r3", "cc");
      break;

      // ===== VFP (Floating Point) =====
    case 2:                    // VFP float addition
      __asm__ volatile ("vldr s0, %1\n\t"
                        "vldr s1, %2\n\t"
                        "vadd.f32 s2, s0, s1\n\t"
                        "vstr s2, %0\n\t":"=m" (junk_result_f):"m"
                        (junk_float1), "m"(junk_float2):"s0", "s1", "s2");
      break;

    case 3:                    // VFP double multiply
      __asm__ volatile ("vldr d0, %1\n\t"
                        "vldr d1, %2\n\t"
                        "vmul.f64 d2, d0, d1\n\t"
                        "vstr d2, %0\n\t":"=m" (junk_result_d):"m"
                        (junk_double1), "m"(junk_double2):"d0", "d1", "d2");
      break;

      // ===== NEON SIMD =====
    case 4:                    // NEON vector add
      __asm__ volatile ("vld1.32 {q0}, %1\n\t"
                        "vld1.32 {q1}, %2\n\t"
                        "vadd.i32 q2, q0, q1\n\t"
                        "vst1.32 {q2}, %0\n\t":"=m" (junk_mem[0]):"m"(junk_mem
                                                                      [0]),
                        "m"(junk_mem[4]):"q0", "q1", "q2", "memory");
      break;

    case 5:                    // NEON multiply-accumulate
      __asm__ volatile ("vld1.32 {q0}, %1\n\t"
                        "vld1.32 {q1}, %2\n\t"
                        "vmla.i32 q0, q1, q0\n\t"
                        "vst1.32 {q0}, %0\n\t":"=m" (junk_mem[0]):"m"(junk_mem
                                                                      [0]),
                        "m"(junk_mem[4]):"q0", "q1", "memory");
      break;

      // ===== MEMORY-BASED JUNK =====
    case 6:                    // Load/Store multiple with permutation
      __asm__ volatile ("ldmia %1, {r0-r3}\n\t"
                        "stmia %2, {r0-r3}\n\t"::"r" (&junk_mem[0]),
                        "r"(&junk_mem[4]):"r0", "r1", "r2", "r3", "memory");
      break;

    case 7:                    // Push/pop with computation
      __asm__ volatile ("push {%1, %2}\n\t"
                        "pop {r0, r1}\n\t"
                        "add r0, r0, r1\n\t"
                        "mov %0, r0\n\t":"=r" (junk_result32):"r"(junk_uint1),
                        "r"(junk_uint2):"r0", "r1", "memory", "cc");
      break;

    case 8:                    // Bitfield extract and insert (ARMv7)
      __asm__ volatile ("mov r0, %1\n\t"
                        "ubfx r1, r0, #4, #8\n\t"
                        "bfi r0, r1, #4, #8\n\t"
                        "mov %0, r0\n\t":"=r" (junk_result32):"r"
                        (junk_uint1):"r0", "r1", "cc");
      break;

    case 9:                    // CRC32 (ARMv8-A CRC extension, some ARMv7 have it)
      __asm__ volatile ("mov r0, %1\n\t"
                        "crc32w r0, r0, %2\n\t"
                        "mov %0, r0\n\t":"=r" (junk_result32):"r"(junk_uint1),
                      "r"(junk_uint2):"r0", "cc");
      break;

    case 10:                   // Count leading zeros
      __asm__ volatile ("mov r0, %1\n\t"
                        "clz r0, r0\n\t"
                        "mov %0, r0\n\t":"=r" (junk_result32):"r"
                        (junk_uint1):"r0", "cc");
      break;

    case 11:                   // Reverse bits
      __asm__ volatile ("mov r0, %1\n\t"
                        "rbit r0, r0\n\t"
                        "rbit r0, r0\n\t"
                        "mov %0, r0\n\t":"=r" (junk_result32):"r"
                        (junk_uint1):"r0");
      break;

      // ===== VFP Square Root =====
    case 12:                   // Floating point sqrt
      __asm__ volatile ("vldr s0, %1\n\t"
                        "vsqrt.f32 s1, s0\n\t"
                        "vstr s1, %0\n\t":"=m" (junk_result_f):"m"
                        (junk_float1):"s0", "s1");
      break;

      // ===== NEON bitwise operations =====
    case 13:                   // NEON bitwise XOR
      __asm__ volatile ("vld1.32 {q0}, %1\n\t"
                        "vld1.32 {q1}, %2\n\t"
                        "veor q2, q0, q1\n\t"
                        "vst1.32 {q2}, %0\n\t":"=m" (junk_mem[0]):"m"(junk_mem
                                                                      [0]),
                        "m"(junk_mem[4]):"q0", "q1", "q2", "memory");
      break;

    case 14:                   // NEON shift
      __asm__ volatile ("vld1.32 {q0}, %1\n\t"
                        "vshr.u32 q1, q0, #4\n\t"
                        "vst1.32 {q1}, %0\n\t":"=m" (junk_mem[0]):"m"(junk_mem
                                                                      [0]):"q0",
                        "q1", "memory");
      break;

    case 15:                   // USAD8 - unsigned sum of absolute differences (for SIMD-like)
      __asm__ volatile ("mov r0, %1\n\t"
                        "mov r1, %2\n\t"
                        "usad8 r2, r0, r1\n\t"
                        "mov %0, r2\n\t":"=r" (junk_result32):"r"(junk_uint1),
                        "r"(junk_uint2):"r0", "r1", "r2", "cc");
      break;

    case 16:                   // PKHBT - pack halfword (ARMv6)
      __asm__ volatile ("mov r0, %1\n\t"
                        "mov r1, %2\n\t"
                        "pkhbt r2, r0, r1, lsl #16\n\t"
                        "mov %0, r2\n\t":"=r" (junk_result32):"r"(junk_uint1),
                        "r"(junk_uint2):"r0", "r1", "r2");
      break;

    case 17:                   // SMLAD - signed multiply accumulate dual
      __asm__ volatile ("mov r0, %1\n\t"
                        "mov r1, %2\n\t"
                        "mov r2, %3\n\t"
                        "smlad r3, r0, r1, r2\n\t"
                        "mov %0, r3\n\t":"=r" (junk_result32):"r"(junk_uint1),
                        "r"(junk_uint2), "r"(junk_uint1):"r0", "r1", "r2",
                        "r3", "cc");
      break;

    case 18:                   // WFE - wait for event (wastes cycles but might context switch)
      __asm__ volatile ("wfe\n\t":::"memory");
      break;

    case 19:                   // SEV - send event (no effect alone)
      __asm__ volatile ("sev\n\t":::"memory");
      break;
    }

#elif defined(__aarch64__)
  // ARM 64-bit: Integer, FP, NEON, SVE, and memory junk
  switch (junk_type)
    {
      // ===== INTEGER OPERATIONS =====
    case 0:                    // Add with flags
      __asm__ volatile ("mov x0, %1\n\t"
                        "mov x1, %2\n\t"
                        "adds x2, x0, x1\n\t"
                        "mov %0, x2\n\t":"=r" (junk_result64):"r"((uint64_t)
                                                                  junk_uint1),
                        "r"((uint64_t) junk_uint2):"x0", "x1", "x2", "cc");
      break;

    case 1:                    // Multiply-accumulate
      __asm__ volatile ("mov x0, %1\n\t"
                        "mov x1, %2\n\t"
                        "madd x2, x0, x1, x0\n\t"
                        "mov %0, x2\n\t":"=r" (junk_result64):"r"((uint64_t)
                                                                  junk_uint1),
                        "r"((uint64_t) junk_uint2):"x0", "x1", "x2", "cc");
      break;

      // ===== ADVANCED SIMD (NEON) =====
    case 2:                    // NEON vector add
      __asm__ volatile ("ld1 {v0.4s}, [%1]\n\t"
                        "ld1 {v1.4s}, [%2]\n\t"
                        "add v2.4s, v0.4s, v1.4s\n\t"
                        "st1 {v2.4s}, [%0]\n\t"::"r" (&junk_mem[0]),
                        "r"(&junk_mem[4]), "r"(&junk_mem[8]):"v0", "v1", "v2",
                        "memory");
      break;

    case 3:                    // NEON floating-point FMA
      __asm__ volatile ("ld1 {v0.2d}, [%1]\n\t"
                        "ld1 {v1.2d}, [%2]\n\t"
                        "fmla v0.2d, v1.2d, v0.2d\n\t"
                        "st1 {v0.2d}, [%0]\n\t"::"r" (&junk_mem[0]),
                        "r"(&junk_mem[4]), "r"(&junk_mem[8]):"v0", "v1",
                        "memory");
      break;

      // ===== FLOATING POINT =====
    case 4:                    // FP square root
      __asm__ volatile ("fmov s0, %1\n\t"
                        "fsqrt s1, s0\n\t"
                        "fmov %0, s1\n\t":"=r" (junk_result_f):"r"
                        (junk_float1):"s0", "s1");
      break;

    case 5:                    // FP absolute value
      __asm__ volatile ("fmov d0, %1\n\t"
                        "fabs d1, d0\n\t"
                        "fmov %0, d1\n\t":"=r" (junk_result_d):"r"
                        (junk_double1):"d0", "d1");
      break;

      // ===== MEMORY-BASED JUNK =====
    case 6:                    // Load/Store with pre/post-index
      __asm__ volatile ("mov x0, sp\n\t"
                        "sub sp, sp, #32\n\t"
                        "str %1, [sp]\n\t"
                        "ldr x1, [sp]\n\t"
                        "add sp, sp, #32\n\t"
                        "mov %0, x1\n\t":"=r" (junk_result64):"r"((uint64_t)
                                                                  junk_uint1):"x0",
                        "x1", "memory", "cc");
      break;

    case 7:                    // Atomic compare-and-swap (harmless)
      __asm__ volatile ("mov x0, #0\n\t"
                        "ldxr x1, [%1]\n\t"
                        "stxr w2, x0, [%1]\n\t"::"r" (&junk_mem[0]):"x0",
                        "x1", "w2", "memory", "cc");
      break;

      // ===== CRYPTO =====
    case 8:                    // AES single round (ARMv8 Crypto extension)
      __asm__ volatile ("ld1 {v0.16b}, [%1]\n\t"
                        "aese v0.16b, v0.16b\n\t"
                        "st1 {v0.16b}, [%0]\n\t"::"r" (&junk_mem[0]),
                        "r"(&junk_mem[4]):"v0", "memory");
      break;

    case 9:                    // SHA256 hash (ARMv8 Crypto)
      __asm__ volatile ("ld1 {v0.4s}, [%1]\n\t"
                        "ld1 {v1.4s}, [%2]\n\t"
                        "sha256h q0, q1, v0.4s\n\t"
                        "st1 {v0.4s}, [%0]\n\t"::"r" (&junk_mem[0]),
                        "r"(&junk_mem[4]), "r"(&junk_mem[8]):"v0", "v1",
                        "memory");
      break;

      // ===== SVE (Scalable Vector Extension) if available =====
    case 10:                   // SVE vector add
      __asm__ volatile ("ptrue p0.s\n\t"
                        "ld1w {z0.s}, p0/z, [%1]\n\t"
                        "ld1w {z1.s}, p0/z, [%2]\n\t"
                        "add z0.s, z0.s, z1.s\n\t"
                        "st1w {z0.s}, p0, [%0]\n\t"::"r" (&junk_mem[0]),
                        "r"(&junk_mem[4]), "r"(&junk_mem[8]):"p0", "z0", "z1",
                        "memory");
      break;

    case 11:                   // CRC32
      __asm__ volatile ("mov w0, %1\n\t"
                        "crc32w w0, w0, %2\n\t"
                        "mov %0, w0\n\t":"=r" (junk_result32):"r"(junk_uint1),
                        "r"(junk_uint2):"w0", "cc");
      break;

    case 12:                   // Population count
      __asm__ volatile ("mov x0, %1\n\t"
                        "cnt x0, x0\n\t"
                        "mov %0, x0\n\t":"=r" (junk_result64):"r"((uint64_t)
                                                                  junk_uint1):"x0");
      break;

    case 13:                   // Reverse bits
      __asm__ volatile ("mov x0, %1\n\t"
                        "rbit x0, x0\n\t"
                        "rbit x0, x0\n\t"
                        "mov %0, x0\n\t":"=r" (junk_result64):"r"((uint64_t)
                                                                  junk_uint1):"x0");
      break;

    case 14:                   // Data cache flush (address cleanup)
      __asm__ volatile ("dc civac, %0\n\t"::"r" (&junk_mem[0]):"memory");
      break;

    case 15:                   // Instruction barrier
      __asm__ volatile ("isb\n\t":::"memory");
      break;

    case 16:                   // Yield instruction (low power)
      __asm__ volatile ("yield\n\t":::);
      break;

    case 17:                   // Floating point round to integer
      __asm__ volatile ("fmov d0, %1\n\t"
                        "frinti d1, d0\n\t"
                        "fmov %0, d1\n\t":"=r" (junk_result_d):"r"
                        (junk_double1):"d0", "d1");
      break;

    case 18:                   // NEON table lookup
      __asm__ volatile ("ld1 {v0.16b}, [%1]\n\t"
                        "ld1 {v1.16b}, [%2]\n\t"
                        "tbl v2.16b, {v0.16b}, v1.16b\n\t"
                        "st1 {v2.16b}, [%0]\n\t"::"r" (&junk_mem[0]),
                        "r"(&junk_mem[4]), "r"(&junk_mem[8]):"v0", "v1", "v2",
                        "memory");
      break;

    case 19:                   // Dot product (NEON)
      __asm__ volatile ("ld1 {v0.4s}, [%1]\n\t"
                        "ld1 {v1.4s}, [%2]\n\t"
                        "sdov2.4s, v0.16b, v1.16b\n\t"
                        "st1 {v2.4s}, [%0]\n\t"::"r" (&junk_mem[0]),
                        "r"(&junk_mem[4]), "r"(&junk_mem[8]):"v0", "v1", "v2",
                        "memory");
      break;
    }

#elif defined(__powerpc__) || defined(__ppc__)
  // PowerPC: Integer, FPU, AltiVec/VMX, VSX, and memory junk
  switch (junk_type)
    {
      // ===== INTEGER OPERATIONS =====
    case 0:                    // Multiply-accumulate
      __asm__ volatile ("mr 3, %1\n\t"
                        "mr 4, %2\n\t"
                        "mullw 5, 3, 4\n\t"
                        "add 6, 5, 3\n\t"
                        "mr %0, 6\n\t":"=r" (junk_result32):"r"(junk_uint1),
                        "r"(junk_uint2):"r3", "r4", "r5", "r6", "cc");
      break;

    case 1:                    // Count leading zeros
      __asm__ volatile ("mr 3, %1\n\t"
                        "cntlzw 3, 3\n\t"
                        "mr %0, 3\n\t":"=r" (junk_result32):"r"
                        (junk_uint1):"r3", "cc");
      break;

      // ===== FLOATING POINT =====
    case 2:                    // FP multiply-add
      __asm__ volatile ("lfs 1, %1\n\t"
                        "lfs 2, %2\n\t"
                        "fmul 3, 1, 2\n\t"
                        "fadd 4, 3, 1\n\t"
                        "stfs 4, %0\n\t":"=m" (junk_result_f):"m"
                        (junk_float1), "m"(junk_float2):"fr1", "fr2", "fr3",
                        "fr4", "memory");
      break;

    case 3:                    // FP square root
      __asm__ volatile ("lfd 1, %1\n\t"
                        "fsqrt 2, 1\n\t"
                        "stfd 2, %0\n\t":"=m" (junk_result_d):"m"
                        (junk_double1):"fr1", "fr2", "memory");
      break;

      // ===== AltiVec/VMX (SIMD) =====
    case 4:                    // AltiVec vector add
      __asm__ volatile ("lvx 0, 0, %1\n\t"
                        "lvx 1, 0, %2\n\t"
                        "vaddubm 2, 0, 1\n\t"
                        "stvx 2, 0, %0\n\t"::"r" (&junk_mem[0]),
                        "r"(&junk_mem[4]), "r"(&junk_mem[8]):"v0", "v1", "v2",
                        "memory");
      break;

    case 5:                    // AltiVec multiply
      __asm__ volatile ("lvx 0, 0, %1\n\t"
                        "lvx 1, 0, %2\n\t"
                      "vmulouw 2, 0, 1\n\t"
                        "stvx 2, 0, %0\n\t"::"r" (&junk_mem[0]),
                        "r"(&junk_mem[4]), "r"(&junk_mem[8]):"v0", "v1", "v2",
                        "memory");
      break;

      // ===== VSX (Vector Scalar Extension) =====
    case 6:                    // VSX vector multiply
      __asm__ volatile ("lxvd2x 0, 0, %1\n\t"
                        "lxvd2x 1, 0, %2\n\t"
                        "xvmuldp 2, 0, 1\n\t"
                        "stxvd2x 2, 0, %0\n\t"::"r" (&junk_mem[0]),
                        "r"(&junk_mem[4]), "r"(&junk_mem[8]):"vs0", "vs1",
                        "vs2", "memory");
      break;

    case 7:                    // VSX permute
      __asm__ volatile ("lxvd2x 0, 0, %1\n\t"
                        "lxvd2x 1, 0, %2\n\t"
                        "xxpermdi 2, 0, 1, 0\n\t"
                        "stxvd2x 2, 0, %0\n\t"::"r" (&junk_mem[0]),
                        "r"(&junk_mem[4]), "r"(&junk_mem[8]):"vs0", "vs1",
                        "vs2", "memory");
      break;

      // ===== MEMORY-BASED JUNK =====
    case 8:                    // Load/Store multiple with update
      __asm__ volatile ("lmw 3, 0(%1)\n\t"
                        "stmw 3, 0(%2)\n\t"::"r" (&junk_mem[0]),
                        "r"(&junk_mem[4]):"r3", "r4", "r5", "r6", "r7", "r8",
                        "r9", "r10", "r11", "r12", "r13", "memory");
      break;

    case 9:                    // Atomic compare-and-swap (lwarx/stwcx)
      __asm__ volatile ("lwarx 3, 0, %1\n\t"
                        "stwcx. 3, 0, %1\n\t"::"r" (&junk_mem[0]):"r3", "cc",
                        "memory");
      break;

    case 10:                   // DCBT (data cache block touch - prefetch)
      __asm__ volatile ("dcbt 0, %0\n\t"::"r" (&junk_mem[0]):"memory");
      break;

    case 11:                   // DCBF (data cache block flush)
      __asm__ volatile ("dcbf 0, %0\n\t"::"r" (&junk_mem[0]):"memory");
      break;

    case 12:                   // SYNC (heavy barrier)
      __asm__ volatile ("sync\n\t":::"memory");
      break;

    case 13:                   // ISYNC (instruction sync)
      __asm__ volatile ("isync\n\t":::"memory");
      break;

    case 14:                   // Rotate and mask (bitfield op)
      __asm__ volatile ("mr 3, %1\n\t"
                        "rlwinm 4, 3, 16, 0, 31\n\t"
                        "rlwinm 5, 4, 16, 0, 31\n\t"
                        "mr %0, 5\n\t":"=r" (junk_result32):"r"
                        (junk_uint1):"r3", "r4", "r5", "cc");
      break;

    case 15:                   // Population count
      __asm__ volatile ("mr 3, %1\n\t"
                        "popcntw 3, 3\n\t"
                        "mr %0, 3\n\t":"=r" (junk_result32):"r"
                        (junk_uint1):"r3", "cc");
      break;

    case 16:                   // Parity calculation
      __asm__ volatile ("mr 3, %1\n\t"
                        "prtyw 4, 3\n\t"
                        "mr %0, 4\n\t":"=r" (junk_result32):"r"
                        (junk_uint1):"r3", "r4", "cc");
      break;

    case 17:                   // Extend sign byte
      __asm__ volatile ("mr 3, %1\n\t"
                        "extsb 4, 3\n\t"
                        "mr %0, 4\n\t":"=r" (junk_result32):"r"
                        (junk_uint1):"r3", "r4", "cc");
      break;

    case 18:                   // Wait instruction
      __asm__ volatile ("wait\n\t":::);
      break;

    case 19:                   // Move from time base (lower)
      __asm__ volatile ("mftb 3\n\t"
                        "mr %0, 3\n\t":"=r" (junk_result32)::"r3");
      break;
    }

#else
#warning "No junk instructions defined for this architecture"
  return;
#endif

  // ===== COMPILER OPTIMIZATION PREVENTION =====
  // Force the compiler to believe all computed results are actually used
  // This prevents the optimizer from removing the "dead" computations

  // Use compiler barriers and fake dependencies
  __asm__ volatile (""::"r" (junk_result32), "r"(junk_result64),
                    "r"(junk_result_f), "r"(junk_result_d),
                    "m"(junk_mem):"memory");

  // Also use a compiler-level fake read of the volatile variables
  (void)junk_result32;
  (void)junk_result64;
  (void)junk_result_f;
  (void)junk_result_d;

  // Force memory barrier to prevent reordering
  __asm__ volatile ("":::"memory");
}
