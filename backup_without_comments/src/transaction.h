#ifndef _TRANSACTION_H
#define _TRANSACTION_H

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "unixlog.h"

enum tsn_state_enum {
    tsn_state_invalid,
    tsn_state_forward_rolling,
    tsn_state_backward_rolling,
    tsn_state_committed,  
    tsn_state_rolled_back,
};

enum tsn_op_enum {
    tsn_op_invalid,
    tsn_op_begin,
    tsn_op_abort,
    tsn_op_commit,
    tsn_op_set_savepoint,
    tsn_op_start_rollback,
    tsn_op_finish_rollback,
};

struct tsn_id_struct {
    uint64_t tsn_id;
};

struct tsn_log_record_struct {
    struct tsn_lsn_struct tsn_log_lsn;
   /* tsn_log_prev_lsn links all records for a given transaction through a
    * linked list */
    struct tsn_lsn_struct tsn_log_prev_lsn;    

    struct tsn_siteid_struct tsn_log_siteid;
    struct tsn_id_struct tsn_log_tsnid;
    enum tsn_op_enum tsn_log_op;

    /* 
     * These next two fields (savepoint id, and next undo LSN) 
     * are valid for tsn_op_set_savepoint records only
     */
    uint64_t tsn_log_savepoint_id; 
   /* tsn_next_undo_lsn points to the next record to be undone for this 
    * transaction */
    struct tsn_lsn_struct tsn_log_next_undo_lsn;
};

struct tsn_struct {
    struct tsn_id_struct tsn_id;
    enum tsn_state_enum tsn_state;
    struct tsn_lsn_struct tsn_last_lsn;
    struct tsn_lsn_struct tsn_next_undo_lsn;

    /* linking these things together, don't mess with this unless you hold
     * the lock on tsn_active_transactions_struct */
    struct tsn_struct *tsn_next_active;
};

struct tsn_active_transactions_struct
{
    uint64_t tt_next_tsn_id;
    struct tsn_struct *tt_head_active;

    /* log file */
    struct tsn_log_struct tt_log;

    /* lock to protect table structures */
    pthread_mutex_t tt_lock;
};

static inline bool tsn_state_is_terminated(enum tsn_state_enum t)
{
    return ((t == tsn_state_committed) || (t == tsn_state_rolled_back));
}

static inline bool tsn_state_is_active(enum tsn_state_enum t)
{
    return ((t == tsn_state_forward_rolling) || 
            (t == tsn_state_backward_rolling));
}

static inline bool tsn_state_is_aborted(enum tsn_state_enum t)
{
    return ((t == tsn_state_rolled_back) || (t == tsn_state_backward_rolling));
}

extern int tsn_init_active_transactions(struct tsn_active_transactions_struct *at, const char *filename);
extern int tsn_release_active_transactions(struct tsn_active_transactions_struct *at);
extern int tsn_init_transaction(struct tsn_active_transactions_struct *tsns, struct tsn_struct *tsn);
extern int tsn_release_transaction(struct tsn_active_transactions_struct *tsns, struct tsn_struct *tsn);


extern int tsn_begin_transaction(struct tsn_active_transactions_struct *tsns, struct tsn_struct *tsn);
extern int tsn_commit_transaction(struct tsn_active_transactions_struct *tsns, struct tsn_struct *tsn);
extern int tsn_abort_transaction(struct tsn_active_transactions_struct *tsns, struct tsn_struct *tsn);
extern int tsn_rollback_transaction(struct tsn_active_transactions_struct *tsns, struct tsn_struct *tsn);
extern int tsn_rollback_to_savepoint(struct tsn_active_transactions_struct *tsns, struct tsn_struct *tsn, uint64_t savepoint_id);

/* writing savepoint records */
extern int tsn_set_savepoint(struct tsn_active_transactions_struct *tsns,
                             struct tsn_struct *tsn, uint64_t savepoint_id);
extern int tsn_start_rollback_savepoint(struct tsn_active_transactions_struct
                                        *tsns, struct tsn_struct *t,
                                        uint64_t savepoint_id);
extern int tsn_finish_rollback_savepoint(struct tsn_active_transactions_struct
                                         *tsns, struct tsn_struct *t,
                                         uint64_t savepoint_id);


extern struct tsn_struct *tsn_find_tsn_by_id(struct tsn_active_transactions_struct
                                      *tsns, struct tsn_id_struct *id);


#endif /* _TRANSACTION_H */
