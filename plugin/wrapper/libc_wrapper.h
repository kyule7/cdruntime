#include <stdlib.h>
#include <malloc.h>
#include <string.h>

/**
 *  Our approach to log libc library is using dlsym(RTLD_NEXT, symbol)
 *  FT_symbol is our format of function pointer for the real symbol, 
 *  and the address is calculated by dlsym in Initialize() phase.
 *  Currently, Initialize() is invoked before main as constructor of static
 *  object. 
 *
 *  disabled flag is important to play a role in on/off switch for logging.
 *  Any libc call before main() must not be recorded, but just be invoked as it
 *  is. Therefore, disabled is 1 very initially, and once app starts, it changes
 *  to 0. For this, it provides Initialize() interface for user to control it.
 *  When this wrapper is integrated into CD runtime, Initialize() should be
 *  called inside CD runtime's initialization phase. (In CD_Init())
 *
 *  This Initialize() should be invoked for the first place, because the other
 *  things may call libc library before that, may call our wrapper which did not
 *  yet calculate function pointer. 
 *
 *
 *  One way to avoid it is to do dlsym in the wrapper. It lazily calculate
 *  function pointer when the libc function is called by application. 
 *
 *  The following items are issues regarding our approach for libc logging.
 *
 *  1) Recursive calloc/dlsym: In order to wrap calloc, it calls dlsym. dlsym
 *     calls calloc in it, and calloc call invokes our wrapper which calls dlsym
 *     in it, never actually returning calloc function pointer from dlsym. We can
 *     use other memory allocation calls, but dlsym must return the function pointer
 *     of the real function beforehand. Unless we do not support a wrapper for a
 *     certain memory allocation call, it still remains.
 * 
 *     Resolution: dlsym allocates 32 Byte using calloc, and static array is
 *     returned for dlsym, instead of calloc. By test, this 32 Byte memory is not
 *     freed, it is safe to use static array. 
 * 
 *  2) malloc_hooks(https://www.gnu.org/software/libc/manual/html_node/Hooks-for-Malloc.html):
 *     libopen-pal.so which is a part of Open MPI actually uses this.
 *     
 */



namespace log {

enum {
  kFreed = 0x1,
};

//typedef void*(*FType_malloc)(size_t size);
//typedef void*(*FType_calloc)(size_t numElem, size_t size);
//typedef void*(*FType_valloc)(size_t size);
//typedef void*(*FType_realloc)(void *ptr, size_t size);
//typedef void*(*FType_memalign)(size_t boundary, size_t size);
//typedef int(*FType_posix_memalign)(void **memptr, size_t alignment, size_t size);
//typedef void(*FType_free)(void *ptr);

}
