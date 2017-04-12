#ifndef _MACHINE_SPECIFIC
#define _MACHINE_SPECIFIC

///////////////////////////////////////////////////////////////////////////
// 
// For preserving stack
#ifdef _LP64
# define STACK_SIZE 2097152+16384 /* AMODE 64 */
#else
# define STACK_SIZE 16384         /* AMODE 32 */
#endif

#ifdef __x86_64__
# define eax rax
# define ebx rbx
# define ecx rcx
# define edx rax
# define ebp rbp
# define esi rsi
# define edi rdi
# define esp rsp
# define TRIM_64BIT(args...) TRIM_64BIT_HELPER(args)
# define TRIM_64BIT_HELPER(args...) #args
#elif __i386__
# define TRIM_64BIT(args...) #args
#else
# define TRIM_64BIT(args...) "UNDEFINED_MACRO"
#endif
///////////////////////////////////////////////////////////////////////////

#if 0
static inline
void GetStackPtr(void **sp)
{
#if defined(__i386__) || defined(__x86_64__)
  asm volatile (TRIM_64BIT(mov %%esp,%0)
    : "=g" (*sp)
                : : "memory");
#elif defined(__arm__) || defined(__aarch64__)
  asm volatile ("mov %0,sp"
    : "=r" (*sp)
                : : "memory");
#else
# error "Error: No Definition"
#endif
}
#endif

struct StackEntry {
  unsigned int    stack_size_;
  void           *sp_;
  void           *shadow_stack_;
  void           *preserved_stack_;
  StackEntry(void) : stack_size_(0), sp_(NULL), preserved_stack_(NULL)
  {
    shadow_stack_ = malloc(STACK_SIZE);
  }
  ~StackEntry(void) { free(shadow_stack_); }
};

#endif
