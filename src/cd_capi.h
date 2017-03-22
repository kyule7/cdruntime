#ifndef _CD_CAPI_H 
#define _CD_CAPI_H
// C interface
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <ucontext.h>
#include <setjmp.h>
#include <cd_def_common.h>

struct cd_handle;
typedef struct cd_handle cd_handle_t;
struct regenobject;

#if CD_TUNING_ENABLED

  
void cd_begin(cd_handle_t *c_handle, const char *label, int collective, uint64_t sys_err_vec);

# define cd_begin0(X) \
    internal_begin((X), NO_LABEL, 0, 0);

# define cd_begin1(X,Y) \
    if(ctxt_prv_mode((X)) == kExcludeStack) setjmp(*jmp_buffer((X))); \
    else getcontext(ctxt((X))); \
    commit_preserve_buff((X)); internal_begin((X),(Y), 0, 0);

# define cd_begin2(X,Y,Z) \
    if(ctxt_prv_mode((X)) == kExcludeStack) setjmp(*jmp_buffer((X))); \
    else getcontext(ctxt((X))); \
    commit_preserve_buff((X)); internal_begin((X),(Y),(Z), 0);

# define cd_begin3(X,Y,Z,W) \
    if(ctxt_prv_mode((X)) == kExcludeStack) setjmp(*jmp_buffer((X))); \
    else getcontext(ctxt((X))); \
    commit_preserve_buff((X)); internal_begin((X),(Y),(Z),(W));

#else
# define cd_begin(...) FOUR_ARGS_MACRO(__VA_ARGS__, cd_begin3, cd_begin2, cd_begin1, cd_begin0)(__VA_ARGS__)
// Macros for setjump / getcontext
// So users should call this in their application, not call cd_handle->Begin().
# define cd_begin0(X) \
    if(ctxt_prv_mode((X)) == kExcludeStack) setjmp(*jmp_buffer((X))); \
    else getcontext(ctxt((X))); \
    commit_preserve_buff((X)); \
    internal_begin((X), NO_LABEL, 0, 0);

# define cd_begin1(X,Y) \
    if(ctxt_prv_mode((X)) == kExcludeStack) setjmp(*jmp_buffer((X))); \
    else getcontext(ctxt((X))); \
    commit_preserve_buff((X)); internal_begin((X),(Y), 0, 0);

# define cd_begin2(X,Y,Z) \
    if(ctxt_prv_mode((X)) == kExcludeStack) setjmp(*jmp_buffer((X))); \
    else getcontext(ctxt((X))); \
    commit_preserve_buff((X)); internal_begin((X),(Y),(Z), 0);

# define cd_begin3(X,Y,Z,W) \
    if(ctxt_prv_mode((X)) == kExcludeStack) setjmp(*jmp_buffer((X))); \
    else getcontext(ctxt((X))); \
    commit_preserve_buff((X)); internal_begin((X),(Y),(Z),(W));

#endif

cd_handle_t *cd_init(int num_tasks, 
                     int my_task, 
                     int prv_medium);

void cd_finalize();

//SZ: should always specify # of children to create
cd_handle_t *cd_create(cd_handle_t *c_handle,
                      uint32_t  num_children,
                      const char *name, 
                      int cd_type, uint32_t error_name);

cd_handle_t *cd_create_customized(cd_handle_t *c_handle,
                                 uint32_t color,
                                 uint32_t task_in_color,
                                 uint32_t  numchildren,
                                 const char *name,
                                 int cd_type, uint32_t error_name);

void cd_destroy(cd_handle_t*);

//void cd_begin(cd_handle_t*, int collective, const char *label);
int ctxt_prv_mode(cd_handle_t*);
jmp_buf *jmp_buffer(cd_handle_t *);
ucontext_t *ctxt(cd_handle_t *);
void commit_preserve_buff(cd_handle_t *);
void internal_begin(cd_handle_t *, const char *label, int collective, uint64_t sys_err_vec);

void cd_complete(cd_handle_t*);

//FIXME: for now only supports this one preservation
int cd_preserve(cd_handle_t*, 
                   void *data_ptr,
                   uint64_t len,
                   uint32_t preserve_mask,
                   const char *my_name, 
                   const char *ref_name);

int cd_preserve_serdes(cd_handle_t *c_handle, 
                   void *data_ptr,
                   uint32_t preserve_mask,
                   const char *my_name, 
                   const char *ref_name);

cd_handle_t *getcurrentcd(void);
cd_handle_t *getleafcd(void);
void cd_detect(cd_handle_t *);






#ifdef __cplusplus
}
#endif



#endif
