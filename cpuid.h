// This file is not supposed to be distributed.
//intel processor CPUID
//eax:
#define PERF_MONITOR_VERSIONID_MASK     (0xff)
#define NUM_OF_GP_COUNTER_MASK          (0xff << 8)
#define WIDTH_OF_GP_COUNTER_MASK        (0xff << 16)
#define LENGTH_OF_EBX_BIT_VECTOR_MASK   (0xff << 24)

#define PERF_MONITOR_VERSIONID(EAX)     ((EAX & PERF_MONITOR_VERSIONID_MASK))
#define NUM_OF_GP_COUNTER(EAX)          ((EAX & NUM_OF_GP_COUNTER_MASK) >> 8)

//ebx: not avaible if the bit is 1
#define CORE_CYCLE_EVT          1
#define INST_RETIRED_EVT        2
#define REF_CYCLES_EVT          4
#define LL_CACHE_REF_EVT        8            /*last level cache references*/
#define LL_CACHE_MISS_EVT       16           /*last level cache misses*/
#define BR_INST_RETIRED_EVT     32
#define BR_MISPRED_RETIRED_EVT  64

//edx: fixed function counters
#define NUM_OF_FIXED_PERF_COUNTER_MASK      0x0f
#define WIDTH_OF_FIXED_PERF_COUNTER_MASK    0xff0
#define NUM_OF_FIXED_PERF_COUNTER(EDX)      (EDX &	\
					     NUM_OF_FIXED_PERF_COUNTER_MASK)
#define WIDTH_OF_FIXED_PERF_COUNTER(EDX)    ((EDX & WIDTH_OF_FIXED_PERF_COUNTER_MASK) \
					     >> 4)
//opcode: 0x0fa2
#define CPUID(EAX, EBX, ECX, EDX)   ({			\
      __asm__ volatile ("cpuid"                         \ 
: "=a"(EAX), "=b"(EBX), "=c"(ECX), "=d"(EDX) 		\
    : "0" (EAX)						\
    );})
