#include <stdio.h>
#include <string.h>

#include "transaction.h"
#include "unixlog.h"

#define tsn_verbose(fmt, ...) 

int tsn_init_active_transactions(struct tsn_active_transactions_struct *at,
    const char *logfile)
{
    int err;
    int ret = 0;

    at->tt_head_active = NULL;
    err = pthread_mutex_init(&at->tt_lock, NULL);
    if (err < 0) { ret = err; } 
    err = tsn_log_open_file(logfile, TSN_LOG_READWRITE, &at->tt_log);
    /* at->tt_next_tsn_id is initialized later, during the log scan */
    if (err < 0) { ret = err; }

    return ret;
}

int tsn_release_active_transactions(struct tsn_active_transactions_struct *at)
{
    int err;
    int ret = 0;

    /* should only be called if no other thread has references */
    err = pthread_mutex_destroy(&at->tt_lock);
    if (err < 0) { ret = err; } 
    err = tsn_log_close_file(&at->tt_log);
    if (err < 0) { ret = err; } 

    return ret;
}


static struct tsn_id_struct get_next_transaction_id(
    struct tsn_active_transactions_struct *tsns)
{
    struct tsn_id_struct next_tsn_id = {tsn_id: tsns->tt_next_tsn_id};
    tsns->tt_next_tsn_id++;
    return next_tsn_id;
}

static int add_transaction(struct tsn_active_transactions_struct *tsns, struct tsn_struct *tsn)
{
    tsn->tsn_next_active = tsns->tt_head_active;
    tsns->tt_head_active = tsn;

    return 1;
}

struct tsn_struct *tsn_find_tsn_by_id(struct tsn_active_transactions_struct
                                      *tsns, struct tsn_id_struct *id)
{
    struct tsn_struct *t;

    pthread_mutex_lock(&tsns->tt_lock);
    for (t = tsns->tt_head_active; t != NULL; t = t->tsn_next_active) {
        if (!memcmp(&t->tsn_id, id, sizeof(tsn_id_struct))) {
            break;
        }
    }
    pthread_mutex_unlock(&tsns->tt_lock);

    return t;
}

static struct tsn_struct *find_tsn_prev(struct tsn_active_transactions_struct
                                        *tsns, struct tsn_struct *tsn)
{
    struct tsn_struct *t;

    for (t = tsns->tt_head_active; t != NULL; t = t->tsn_next_active) {
        if (t->tsn_next_active == tsn) {
            break;
        }
    }

    return t;
}

static int del_transaction(struct tsn_active_transactions_struct *tsns, 
    struct tsn_struct *tsn)
{
    int ret = -1;
    struct tsn_struct *t;

    if (tsns->tt_head_active == tsn) {
        tsns->tt_head_active = tsn->tsn_next_active;
        ret = 0;
    } else {
        t = find_tsn_prev(tsns, tsn);

        /* now remove tsn from linkage */
        if (t) { 
            t->tsn_next_active = tsn->tsn_next_active;
            ret = 0;
        } 
    }

    return ret;
}

int tsn_init_transaction(struct tsn_active_transactions_struct *tsns,
    struct tsn_struct *tsn)
{
    pthread_mutex_lock(&tsns->tt_lock);

    /* transaction information */
    tsn->tsn_id = get_next_transaction_id(tsns);
    tsn->tsn_state = tsn_state_invalid;
    tsn->tsn_last_lsn = NULL_LSN;
    tsn->tsn_next_undo_lsn = NULL_LSN;

    /* linkage for transaction table */
    add_transaction(tsns, tsn);

    pthread_mutex_unlock(&tsns->tt_lock);

    return 1;
}

int tsn_release_transaction(struct tsn_active_transactions_struct *tsns,
    struct tsn_struct *tsn)
{
    int ret;

    pthread_mutex_lock(&tsns->tt_lock);
    ret = del_transaction(tsns, tsn);
    pthread_mutex_unlock(&tsns->tt_lock);
    return ret;
}

struct tsn_siteid_struct tsn_get_local_siteid(void)
{
    const struct tsn_siteid_struct my_siteid = {siteid: 0};
    return my_siteid;
}

static int str_tsn_log_record(char *const s, const size_t n,
                              const struct tsn_log_record_struct *const r)
{
    static const char *const tsn_op_names[] = {
        [tsn_op_invalid] = "tsn_op_invalid",
        [tsn_op_begin] = "tsn_op_begin",
        [tsn_op_abort] = "tsn_op_abort",
        [tsn_op_commit] = "tsn_op_commit",
        [tsn_op_set_savepoint] = "tsn_op_set_savepoint",
        [tsn_op_start_rollback] = "tsn_op_start_rollback",
        [tsn_op_finish_rollback] = "tsn_op_finish_rollback",

    };

//    char *kv_format = (char*) "lsn=%lu prev=%lu next_undo=%lu tsnid=%lu tsnid=%s";
    char *tbl_format = (char*) "%10lu P%10lu N%10lu %10lu %s";

    return snprintf(s, n,
                    tbl_format,
                    r->tsn_log_lsn.lsn,
                    r->tsn_log_prev_lsn.lsn,
                    r->tsn_log_next_undo_lsn.lsn, r->tsn_log_tsnid.tsn_id, 
                    tsn_op_names[r->tsn_log_op]);
}

void print_log_record(const char *s, 
                             const struct tsn_log_record_struct *const r)
{
    const int buflen = 256;
    char buf[buflen];

    str_tsn_log_record(buf, buflen, r);
    printf("%24s: %s\n", s, buf);
}

static int write_simple_record(struct tsn_active_transactions_struct *tsns,
    struct tsn_struct *t, enum tsn_op_enum logged_op, 
    struct tsn_lsn_struct *lsn)
{
    int ret;
    const struct tsn_log_operations *const ops = tsns->tt_log.log_ops;
    struct tsn_log_record_struct rec;

    memset(&rec, 0, sizeof(rec));
    rec.tsn_log_prev_lsn = t->tsn_last_lsn;
    rec.tsn_log_siteid = tsn_get_local_siteid();
    rec.tsn_log_tsnid = t->tsn_id;
    rec.tsn_log_op = logged_op;
    /* simple records have no next_undo_lsn or savepoint id */

    /* write the record */
    ret = ops->l_write(&tsns->tt_log, &rec, sizeof(rec), &t->tsn_last_lsn);
    if (ret != sizeof(rec)) {
        ret = -1;
    }
    /* update the (in-memory) LSN field of the record */
    rec.tsn_log_lsn = t->tsn_last_lsn;

    /* save LSN for caller */
    *lsn = t->tsn_last_lsn;

        print_log_record(__FUNCTION__, &rec);

    tsn_verbose("%s: returning %d\n", __FUNCTION__, ret);
    return ret;
}

static int is_savepoint_op(enum tsn_op_enum o)
{
    if ((o == tsn_op_set_savepoint) || (o == tsn_op_start_rollback)
        || (o == tsn_op_finish_rollback)) {
        return 1;
    } else {
        return 0;
    }
}

static int write_savepoint_record(struct tsn_active_transactions_struct *tsns,
    struct tsn_struct *t, enum tsn_op_enum logged_op, 
    uint64_t savepoint_id,
    struct tsn_lsn_struct *lsn)
{
    int ret;
    const struct tsn_log_operations *const ops = tsns->tt_log.log_ops;
    struct tsn_log_record_struct rec;

    memset(&rec, 0, sizeof(rec));
    rec.tsn_log_prev_lsn = t->tsn_last_lsn;
    rec.tsn_log_siteid = tsn_get_local_siteid();
    rec.tsn_log_tsnid = t->tsn_id;
    rec.tsn_log_op = logged_op;
    if (is_savepoint_op(logged_op)) {
        rec.tsn_log_savepoint_id = savepoint_id;
    }
    if (logged_op == tsn_op_set_savepoint) {
        rec.tsn_log_next_undo_lsn = t->tsn_next_undo_lsn;
    }

    /* write the record */
    ret = ops->l_write(&tsns->tt_log, &rec, sizeof(rec), &t->tsn_last_lsn);
    if (ret != sizeof(rec)) {
        ret = -1;
    }
    /* update the (in-memory) LSN field of the record */
    rec.tsn_log_lsn = t->tsn_last_lsn;

    /* save LSN for caller */
    *lsn = t->tsn_last_lsn;

        print_log_record(__FUNCTION__, &rec);

    return ret;
}


/*
 * essentially, the user-visible constructor for the transaction
 */
int tsn_begin_transaction(struct tsn_active_transactions_struct *tsns, 
                          struct tsn_struct *t)
{
    int ret;
    struct tsn_lsn_struct lsn;

    ret = tsn_init_transaction(tsns, t);
    if (ret < 0) {
        goto out;
    }

    ret = write_simple_record(tsns, t, tsn_op_begin, &lsn);
    if (ret < 0) {
        goto out;
    }

    t->tsn_state = tsn_state_forward_rolling;
    t->tsn_next_undo_lsn = lsn;

  out:
    return ret;
}

/*
 * roughly, one of two user-accessible destructors
 */
int tsn_commit_transaction(struct tsn_active_transactions_struct *tsns,
                           struct tsn_struct *t)
{
    int ret = -1;
    const struct tsn_log_operations *const log_ops = tsns->tt_log.log_ops;
    struct tsn_lsn_struct lsn;
    struct tsn_lsn_struct max_stable_lsn;

    ret = write_simple_record(tsns, t, tsn_op_commit, &lsn);
    if (ret < 0) { goto out; }

    /* now flush the log */
    ret = log_ops->l_flush(&tsns->tt_log, &lsn, 0, &max_stable_lsn);
    if (ret < 0) {
        /* uh-oh...  something bad happened while trying to flush the log */
        goto out;
    }

    /* update in-memory state */
    if (t->tsn_state == tsn_state_forward_rolling) {
        t->tsn_state = tsn_state_committed;
    } else {
        t->tsn_state = tsn_state_rolled_back;
    }
    ret = tsn_release_transaction(tsns, t);

out:
    return ret;
}

/*
 * tsn_abort_transaction()
 * - changes the transaction state to tsn_state_backward_rolling
 * - doesn't perform the rollback!
 */
int tsn_abort_transaction(struct tsn_active_transactions_struct *tsns,
                          struct tsn_struct *t)
{
    int ret = -1;
    struct tsn_lsn_struct lsn;

    ret = write_simple_record(tsns, t, tsn_op_abort, &lsn);
    if (ret >= 0) {
        t->tsn_state = tsn_state_backward_rolling;
    }

    return ret;
}

/*
 * tsn_set_savepoint()
 * - writes a savepoint record
 */
int tsn_set_savepoint(struct tsn_active_transactions_struct *tsns,
                      struct tsn_struct *t, uint64_t savepoint_id)
{
    int ret = -1;
    struct tsn_lsn_struct lsn;

    ret = write_savepoint_record(tsns, t, tsn_op_set_savepoint, savepoint_id, &lsn);
    if (ret >= 0) {
        t->tsn_next_undo_lsn = lsn;
    }

    return ret;
}

/*
 * tsn_start_rollback_savepoint()
 * - log beginning of rollback to a savepoint
 */
int tsn_start_rollback_savepoint(struct tsn_active_transactions_struct *tsns,
                      struct tsn_struct *t, uint64_t savepoint_id)
{
    int ret;
    struct tsn_lsn_struct lsn;

    ret = write_savepoint_record(tsns, t, tsn_op_start_rollback, savepoint_id, &lsn);

    return ret;
}

/*
 * tsn_finish_rollback_savepoint()
 * - log completion of rollback to a savepoint
 */
int tsn_finish_rollback_savepoint(struct tsn_active_transactions_struct *tsns,
                      struct tsn_struct *t, uint64_t savepoint_id)
{
    int ret;
    struct tsn_lsn_struct lsn;

    ret = write_savepoint_record(tsns, t, tsn_op_finish_rollback, savepoint_id, &lsn);

    return ret;
}

static int get_tsn_record(struct tsn_active_transactions_struct *tsns,
        const struct tsn_lsn_struct lsn, struct tsn_log_record_struct *r)
{    
    int ret;

    ret = tsns->tt_log.log_ops->l_read(&tsns->tt_log, &lsn, r, sizeof(*r));
    /* store the LSN in the record for the caller */
    r->tsn_log_lsn = lsn;

    if (ret != sizeof(*r) || 1) {
        print_log_record(__FUNCTION__, r);
    }

    return ret;
}

/*
 * tsn_rollback_transaction()
 * - log completion of rollback to a savepoint
 */
int tsn_rollback_transaction(struct tsn_active_transactions_struct *tsns,
                      struct tsn_struct *t)
{
    int ret = -1;
    struct tsn_lsn_struct next;
    struct tsn_log_record_struct rec;

    ret = tsn_abort_transaction(tsns, t);
    if (ret < 0) {
        goto out;
    }

    next = t->tsn_next_undo_lsn;
    ret = get_tsn_record(tsns, next, &rec);
    if (ret != sizeof(rec)) { goto out; }

    while (1) {
        /* loop termination */
        if (rec.tsn_log_op == tsn_op_begin) { break; }

        /* perform undo processing */
        if (rec.tsn_log_op == tsn_op_set_savepoint) {
            t->tsn_next_undo_lsn = rec.tsn_log_next_undo_lsn;
        }
    
        next = t->tsn_next_undo_lsn;
        ret = get_tsn_record(tsns, next, &rec);
        if (ret != sizeof(rec)) { goto out; }
    }

    ret = tsn_commit_transaction(tsns, t);
    if (ret < 0) { 
        goto out;
    }

out:
    return ret;
}

/*
 * tsn_rollback_transaction()
 * - log completion of rollback to a savepoint
 */
int tsn_rollback_to_savepoint(struct tsn_active_transactions_struct *tsns,
                              struct tsn_struct *t, uint64_t savepoint_id)
{
    int ret = -1;
    struct tsn_lsn_struct next;
    struct tsn_log_record_struct rec;

    ret = tsn_start_rollback_savepoint(tsns, t, savepoint_id);
    if (ret < 0) {
        goto out;
    }

    next = t->tsn_next_undo_lsn;
    ret = get_tsn_record(tsns, next, &rec);
    if (ret != sizeof(rec)) { goto out; }

    while (1) {
        /* loop termination */
        if (rec.tsn_log_op == tsn_op_set_savepoint) {
            if (rec.tsn_log_savepoint_id == savepoint_id) { 
                /* we're done undoing this savepoint */
                break; 
            } else {
                /* another savepoint was established in between, undo it */
                t->tsn_next_undo_lsn = rec.tsn_log_next_undo_lsn;
            }
        }
    
        next = t->tsn_next_undo_lsn;
        ret = get_tsn_record(tsns, next, &rec);
        if (ret != sizeof(rec)) { goto out; }
    }

    ret = tsn_finish_rollback_savepoint(tsns, t, savepoint_id);
    if (ret < 0) { 
        goto out;
    }

out:
    return ret;
}
