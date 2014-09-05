#ifndef _LOG_H
#define _LOG_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>

#define TSN_LOG_READ  1
#define TSN_LOG_WRITE 2
#define TSN_LOG_READWRITE 3

/*
 * clients should treat this value as opaque
 * the log manager is free to use this field in any way it sees fit, 
 * so long as the API semantics are preserved.
 */
struct tsn_lsn_struct {
    uint64_t lsn;
};

struct tsn_siteid_struct {
    uint64_t siteid;
};

struct tsn_logrecord_header {
    /* the UNIX file offset is encoded in the LSN, so this gives us a
     * crude way of validating entries, and a crude way of finding the
     * end of the log.  */
    struct tsn_lsn_struct h_lsn;
    int64_t h_this_offset;
    uint64_t h_next_offset;
};

#define LOGFILE_HEADER_SIZE  4096
#define LOGFILE_DEFAULT_SIZE 262144

#define NULL_LSN ((struct tsn_lsn_struct) { lsn: 0 })

/* Just a random number... */
#define TSN_LOG_HEADER_MAGIC 0xbe92a001450cb267ULL

/*
 * tsn_logdatum_struct
 *
 * info for a particular log file, instances of tsn_log_struct reference
 * this.
 */
struct tsn_logdatum_struct {

    /* LSN's for normal forward processing */
    struct tsn_lsn_struct durable_lsn;
    struct tsn_lsn_struct last_lsn;
    struct tsn_lsn_struct next_lsn;

    /* LSN's used for restart processing */
    struct tsn_lsn_struct start_lsn;
    struct tsn_lsn_struct last_checkpoint_lsn;

    /* Fields describing the on-disk layout */
    char *logfile_name;
    int logfile_start_offset;           /* first byte containing data */
    int logfile_max_size;               

    int fd;
    pthread_mutex_t fd_mutex;           /* any *operations* on fd need this */
    pthread_mutex_t lsn_mutex;          /* locks LSN fields */

    void *log_buffer;
    size_t log_buffer_size;

    /* linkage for open_logs maintained by log manager */
    struct tsn_logdatum_struct *next_open_log;
    int refcnt;
};

struct tsn_log_header {
    uint64_t magic;

    struct tsn_lsn_struct start_lsn;    /* start of the log */

    /* LSN of last successful checkpoint */
    struct tsn_lsn_struct last_checkpoint_lsn;
                                       
    /* physical format of the log */
    int logfile_start_offset;           /* first byte containing data */
    int logfile_max_size;               
};

/*
 * tsn_log_struct
 * 
 * an instance of an open log file.  (analogy log:logdatum :: file:inode)
 */
struct tsn_log_struct {
    struct tsn_logdatum_struct *my_log;
    struct tsn_log_operations *log_ops;
};

struct tsn_log_operations {
    /* insert n bytes of log record (buf) into the log buffer
     * placing the record's LSN into out_lsn
     * returns < 0 on failure */
    int (*l_write) (struct tsn_log_struct *log,
                    const void *buf, ssize_t n,
                    struct tsn_lsn_struct *out_lsn);

    /* wait until the log is flushed (at least) until to_lsn, 
     * (returns when until to_lsn < durable_lsn)
     *
     * force_now schedules the log buffer for immediate writing, otherwise
     * we block until the log is written out through the usual writeback
     */
    int (*l_flush) (struct tsn_log_struct *log,
                    const struct tsn_lsn_struct *to_lsn, bool force_now,
                    struct tsn_lsn_struct *max_durable_lsn);

    /* read the log record at location lsn into the buffer buf
     * returns < 0 on failure */
    int (*l_read) (struct tsn_log_struct *log,
                   const struct tsn_lsn_struct *lsn, void *buf,
                   ssize_t n);
};

/* constructor, basically.  opens a UNIX-style log file */
int tsn_log_open_file(const char *logfilename, const int flags, 
                      struct tsn_log_struct *logfile);
/* destructor, closes a UNIX log file */
int tsn_log_close_file(struct tsn_log_struct *logfile);

/* initialize a new (empty) log file */
int tsn_log_create_file(const char *logfilename);
/* destroy an existing log file */
int tsn_log_destroy_file(const char *logfilename);

static inline bool tsn_lsn_lessthan(struct tsn_lsn_struct *lsn1,
    struct tsn_lsn_struct *lsn2)
{
    return lsn1->lsn < lsn2->lsn;
}

static inline struct tsn_lsn_struct *tsn_lsn_min(struct tsn_lsn_struct *lsn1,
                                                 struct tsn_lsn_struct *lsn2)
{
    if (tsn_lsn_lessthan(lsn1, lsn2)) { return lsn1; } else { return lsn2; }
}

#endif /* _LOG_H */
