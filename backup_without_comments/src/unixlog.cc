#include <stdio.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <cassert>
#include "unixlog.h"

#if 1
#define tsn_verbose(msg, ...)
#else
#define tsn_verbose(msg, ...) printf(msg, __VA_ARGS__)
#endif

static struct tsn_logdatum_struct *open_logs = NULL;

static pthread_mutex_t open_log_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * XXX:  belongs in private header file?
 */
static int unixfile_log_write(struct tsn_log_struct *log,
                              const void *buf, ssize_t n,
                              struct tsn_lsn_struct *out_lsn);
static int unixfile_log_flush(struct tsn_log_struct *log,
                              const struct tsn_lsn_struct *to_lsn,
                              bool force_now,
                              struct tsn_lsn_struct *max_durable_lsn);
static int unixfile_log_read(struct tsn_log_struct *log,
                             const struct tsn_lsn_struct *lsn,
                             void *buf, ssize_t n);

const struct tsn_log_operations unixfile_log_operations =
{
    l_write : unixfile_log_write,
    l_flush : unixfile_log_flush,
    l_read  : unixfile_log_read,
}; 
/*
static int logfile_find_oldest_lsn(struct tsn_log_struct *logfile)
{
    // returns the oldest LSN known to the (stable) log, in that all
    // all other LSN's are larger than this LSN, i.e.
    // 
    // tsn_lsn_lessthan(oldest_lsn, lsn) == TRUE 
    //     for all lsn in stable log and lsn != oldest_lsn
    //
    // This should probably return the NULL_LSN for a newly created log 

    return -1;
}
*/
/*
static int logfile_find_newest_lsn(struct tsn_log_struct *logfile)
{
    // return the largest LSN in the stable log such that
    // 
    // tsn_lsn_lessthan(lsn, newest_lsn) == TRUE 
    //     for all lsn in stable log and lsn != newest_lsn
     

    return -1;
}
*/
static int lsn_to_offset(struct tsn_logdatum_struct *tlg, 
                         struct tsn_lsn_struct t_lsn)
{
    const int log_start_offset = tlg->logfile_start_offset;
    const int log_size = tlg->logfile_max_size;

    return log_start_offset + (t_lsn.lsn % log_size);
}

static void init_empty_log_header(struct tsn_log_header *hd)
{
    hd->magic = TSN_LOG_HEADER_MAGIC;
    /* XXX: Should probably add a creation time in here */
    hd->last_checkpoint_lsn = NULL_LSN; /* never checkpointed */
    hd->logfile_start_offset = LOGFILE_HEADER_SIZE;
    hd->start_lsn.lsn = LOGFILE_DEFAULT_SIZE;
    hd->logfile_max_size = LOGFILE_DEFAULT_SIZE;
}

static int check_log_header(struct tsn_log_header *hd)
{
    int ret;

    if (hd->magic != TSN_LOG_HEADER_MAGIC) {
        ret = -1;
        goto out;
    }

    ret = 0;
out:
    return ret;
}

/*
 * Initializes the fields of a log_datum from the log file
 *
 * Accesses fd, so caller must hold the fd_mutex.
 */
static int load_log_header(struct tsn_logdatum_struct *tlg)
{
    struct tsn_log_header hd;
    int ret;

    ret = read(tlg->fd, &hd, sizeof(hd));
    if (ret != sizeof(hd)) {
        ret = -1;
        goto out;
    }

    ret = check_log_header(&hd);
    if (ret != 0) {
        goto out;
    }

    tlg->start_lsn = hd.start_lsn;
    tlg->last_checkpoint_lsn = hd.last_checkpoint_lsn;
    tlg->logfile_start_offset = hd.logfile_start_offset;
    tlg->logfile_max_size = hd.logfile_max_size;

out:
    return ret;

}

static int write_zeroes(int fd, int count)
{
    char buf[count];
    int bytes;

    memset(buf, 0, count);

    bytes = write(fd, buf, count);

    return bytes;
}

/*
 * Creates a new log file.  
 *
 * Returns < 0 on error, >= 0 on success.
 *
 * No locks or anything necessary, since we don't
 * touch anything global, other than the file system.  It's possible
 * that we could race on file creation, but I don't expect this will
 * occur under most circumstances.
 */
int tsn_log_create_file(const char *logfilename)
{
    int ret;
    int fd;
    mode_t mode;
    struct tsn_log_header new_log_header;
    int flags;

    flags = O_CREAT | O_WRONLY | O_TRUNC | O_EXCL;
    mode = S_IRUSR | S_IWUSR;
    fd = open(logfilename, flags, mode);
    if (fd < 0) {
        printf("what? %d\n", fd);
        printf("file: %s\n", logfilename);
        ret = fd;
        goto out;
    }

    init_empty_log_header(&new_log_header);

    /* write the new log header to the log file */
    ret = write(fd, &new_log_header, sizeof(new_log_header));
    if (ret != sizeof(new_log_header)) {
        /* should print a warning or something here */
        goto out_close;
    }

    /* pad the rest with NULL's */
    ret =
        write_zeroes(fd,
                     new_log_header.logfile_start_offset -
                     sizeof(new_log_header));
    printf("sdfasdf %d\n", ret);
    getchar();
out_close:
    close(fd);
out:
    return ret;
}

int tsn_log_destroy_file(const char *logfilename)
{
    return unlink(logfilename);
}

static int get_open_flags(const int flags)
{
    static int fflags;

    if (flags == TSN_LOG_READ) {
        fflags = O_RDONLY;
    } else if (flags == TSN_LOG_WRITE) { 
        /* XXX: There is probably no legitimate use for this flag... */
        fflags = O_WRONLY;
    } else if (flags == TSN_LOG_READWRITE) {
        fflags = O_RDWR;
    }

    return fflags;
}

static void set_tlg_refcnt(struct tsn_logdatum_struct *tlg, int count)
{
    pthread_mutex_lock(&open_log_lock);
    tlg->refcnt = count;
    pthread_mutex_unlock(&open_log_lock);
}

static struct tsn_logdatum_struct *alloc_tlg(void)
{
    struct tsn_logdatum_struct *new_tlg;

    new_tlg = (tsn_logdatum_struct*)malloc(sizeof(*new_tlg));
    set_tlg_refcnt(new_tlg, 0);
    return new_tlg;
}

static void free_tlg(struct tsn_logdatum_struct *tlg)
{
    /* XXX: Warn if refcnt is nonzero */
    free(tlg);
}
/*
static void get_tlg_ref(struct tsn_logdatum_struct *tlg)
{
    // Until we have atomics, we're just going to use the mutex to ensure atomicity here
    pthread_mutex_lock(&open_log_lock);
    ++tlg->refcnt;
    pthread_mutex_unlock(&open_log_lock);
}
*/

static int tsn_log_close(struct tsn_logdatum_struct *tlg)
{
    int ret;

    free(tlg->logfile_name);
    tlg->logfile_name = NULL;

    /* Holding tlg->fd_mutex is unnecessary, since we shouldn't have
     * called tsn_log_close() if there are multiple references to the
     * log file, but I'm leaving the thing in there, in case that
     * changes in the future. */
    pthread_mutex_lock(&tlg->fd_mutex);
    ret = fsync(tlg->fd);
    /* XXX: Should do something with the error code from fsync() */
    ret = close(tlg->fd);
    pthread_mutex_unlock(&tlg->fd_mutex);

    free_tlg(tlg);

    return ret;
}

static struct tsn_logdatum_struct *find_open_log_prev(struct
                                                      tsn_logdatum_struct
                                                      *tlg)
{
    struct tsn_logdatum_struct *t;

    for (t = open_logs; t != NULL; t = tlg->next_open_log)
    {
        if (t->next_open_log == tlg) { 
            break;
        }
    }

    return t;
}

/*
 * removes a log from the log list
 *
 * You must hold the open_log_lock when calling this.
 */
static int remove_open_log(struct tsn_logdatum_struct *tlg)
{
    int ret = 0;

    if (open_logs == tlg) {
        open_logs = tlg->next_open_log;
    } else {
        struct tsn_logdatum_struct *prev_tlg = find_open_log_prev(tlg);
        if (prev_tlg == NULL) {
            /* we didn't find it! abort, and run away! */
            ret = -1;
        } else {
            prev_tlg->next_open_log = tlg->next_open_log;
        }
    }

    return ret;
}

static void put_tlg_ref(struct tsn_logdatum_struct *tlg)
{
    int needs_close = 0;

    pthread_mutex_lock(&open_log_lock);
    --tlg->refcnt;
    if (!(tlg->refcnt)) {
        remove_open_log(tlg);
        needs_close = 1;
    }
    pthread_mutex_unlock(&open_log_lock);

    if (needs_close) {
        tsn_log_close(tlg);
    }
}

static void add_new_open_log(struct tsn_logdatum_struct *tlg)
{
    /* now put us on the list of open logs */
    pthread_mutex_lock(&open_log_lock);
    tlg->next_open_log = open_logs;
    open_logs = tlg;
    pthread_mutex_unlock(&open_log_lock);
}

static int log_header_is_ok(struct tsn_logdatum_struct *tlg,
                            struct tsn_logrecord_header *lh)
{
    if (lh->h_lsn.lsn == 0) { return 0; }
    if (lsn_to_offset(tlg, lh->h_lsn) != lh->h_this_offset) { return 0; }

    return 1;
}

/*
 * XXX:  Ick, all sorts of confusion between uint64 and off_t here...
 *
 * Routine is used in finding the end of the log.  It scans a header,
 * then seeks to the next header, returning 
 *   0 when no more headers are found, 
 *   < 0 on an error from read() or lseek(), and
 *   > 0 on success.
 */
static int read_log_record(struct tsn_logdatum_struct *tlg,
                           struct tsn_logrecord_header *out_lh)
{
    const int fd = tlg->fd;
    ssize_t bytes = -1;
    off_t where;
    struct tsn_logrecord_header lh;

    if (out_lh == NULL) {
        goto out;
    } else {
        memset(out_lh, 0, sizeof(*out_lh));
    }

    /* read the header */
    bytes = read(fd, &lh, sizeof(lh));
    if (bytes == 0) {
        /* reached end of file -- no more records */
        goto out;
    } else if (bytes != sizeof(lh)) {
        /* some kind of read error */
        bytes = -1;
        goto out;
    }

    /* now validate the log entry */
    if (log_header_is_ok(tlg, &lh)) {
        /* ok, we'll return it to the caller */
        memcpy(out_lh, &lh, sizeof(*out_lh));
    } else {
        /* the entry is invalid, interpret this as the end of the log */
        bytes = 0;
        goto out;
    }

    /* skip to the next record */
    where = lseek(fd, lh.h_next_offset, SEEK_SET);
    if (where == ((off_t) (-1))) {
        /* some kind of error -- punt */
        bytes = -1;
        goto out;
    } else if (where != (off_t) lh.h_next_offset) {
        /* how can this happen? -- punt, panic? */
        bytes = -1;
        goto out;
    } 

    bytes += lh.h_next_offset - lsn_to_offset(tlg, lh.h_lsn);

out:
    tsn_verbose("%s: returning %d\n", __func__, (int) bytes);
    return bytes;
}

/*
 * scan the log.  We're trying to find the last complete log record.
 *
 * initialize last_lsn to point to the last known record 
 *     (NULL_LSN if there isn't one, and the log is brand new.)
 * initialize next_lsn to point to the place where we'll start writing
 *     new log records: file offset of last_lsn + size of that record
 * XXX: we might want to increase the wrap count on the log as well. Although
 *     it shouldn't happen, on the off chance that we gave out an LSN before
 *     the last crash.
 * XXX: Along the same lines, we might keep a generation count and/or a
 *     startup (open) count inside the log master record.
 * initialize durable_lsn to last_lsn
 *
 */
static int scan_log(struct tsn_logdatum_struct *tlg)
{
    int ret;
    struct tsn_lsn_struct lsn;
    struct tsn_logrecord_header lh;
    ssize_t bytes;
    ssize_t offset;

    /* load the last checkpoint address, and the log start pointer */
    ret = load_log_header(tlg);
    if (ret < 0) {
        goto out;
    }

    /* first seek to the start address */
    ret = lseek(tlg->fd, tlg->logfile_start_offset, SEEK_SET);
    if (ret < 0) {
        goto out;
    }

    offset = tlg->logfile_start_offset;
    lsn = NULL_LSN;
    while (1) {
        bytes = read_log_record(tlg, &lh);
        if (bytes < 0) {
            ret = (int) bytes;
            goto out;
        } else if (bytes == 0) {
            /* EOF -- finished processing */
            break;
        }

        offset += bytes;
    }

    /* read_log_record should have positioned the file pointer to the
     * right place, and lsn should be the last good LSN in the log */

    if (offset == tlg->logfile_start_offset) {
        /* no log records */

        /* set next_lsn to hd.start_lsn */
        tlg->next_lsn = tlg->start_lsn;
        tlg->last_lsn = NULL_LSN; /* any old records were garbage collected */
        tlg->durable_lsn = NULL_LSN; /* old records were garbage collected */

// tsn_verbose("%s: tlg->next_lsn = %lu\n", __func__, tlg->next_lsn.lsn);
// tsn_verbose("%s: lsn_to_offset(tlg, tlg->next_lsn) = %d\n", __func__, lsn_to_offset(tlg, tlg->next_lsn));
// tsn_verbose("%s: offset = %lu\n", __func__, offset);

        /* verify that the offset for start_lsn matches */
        if (lsn_to_offset(tlg, tlg->next_lsn) != offset) {
            ret = -1;
            goto out;
        }
    }

  out:
// tsn_verbose("%s: returning %d\n", __func__, ret);
    return ret;
}

static struct tsn_logdatum_struct *tsn_log_open(const char *logfilename,
                                                const int tsn_log_flags)
{
    int fflags = get_open_flags(tsn_log_flags);
    int fd;
    int ret;

    struct tsn_logdatum_struct *tlg = alloc_tlg();
    if (!tlg) { goto out; }
    
    fd = open(logfilename, fflags);
    if (fd < 0) {
        ret = fd;
        goto out_free_tlg;
    }


    /* caller is the first reference. */
    set_tlg_refcnt(tlg, 1);
    tlg->fd = fd;
    pthread_mutex_init(&tlg->fd_mutex, NULL);
    pthread_mutex_init(&tlg->lsn_mutex, NULL);
    tlg->logfile_name = (char*)malloc(strlen(logfilename));
    if (!tlg->logfile_name) {
        /* sigh... */
        goto out_zero_refcnt;
    }

    pthread_mutex_lock(&tlg->fd_mutex);
    pthread_mutex_lock(&tlg->lsn_mutex);

    /* zero-out the forward processing fields, we should initialize these
     * to the correct values later, once we've really had a good look at
     * the actual log file and started scanning records */

    /* durable_lsn records the highest LSN that (as far as we know) is 
     * durable.  We update durable_lsn when the log is flushed (fsync'd),
     * rather than on write, since the kernel may not have written the
     * corresponding buffers out to disk. */
    tlg->durable_lsn = NULL_LSN;        
    /* next_lsn is analagous to an end-of-file pointer, 
     * the next record we write is assigned this LSN.  */
    tlg->next_lsn = NULL_LSN;           
    /* last_lsn, so we can backwards chain the log records */
    tlg->last_lsn = NULL_LSN;           

    /* now scan the log */
    ret = scan_log(tlg);
    if (ret < 0) {
        goto out_unlock;
    }

    pthread_mutex_unlock(&tlg->lsn_mutex);
    pthread_mutex_unlock(&tlg->fd_mutex);

    /* Expect we'll use a separate lock for this, eventually. */
    tlg->log_buffer = NULL;
    tlg->log_buffer_size = 0;

    add_new_open_log(tlg);

    /* all done */
    goto out;

out_unlock:
    pthread_mutex_unlock(&tlg->fd_mutex);
    pthread_mutex_destroy(&tlg->fd_mutex);
out_zero_refcnt:
    /* unnecessary, since no one should have been able to obtain a reference
     * to it, but what the heck */
    set_tlg_refcnt(tlg, 0);
    close(fd);
out_free_tlg:
    free_tlg(tlg);
    tlg = NULL;
out:
    return tlg;
}

int tsn_log_open_file(const char *logfilename, const int flags, 
                      struct tsn_log_struct *logfile)
{
    struct tsn_logdatum_struct *my_log;
    int ret = -1;

    memset(logfile, 0, sizeof(*logfile));

    my_log = tsn_log_open(logfilename, flags);
    if (my_log == NULL) {
        goto out;
    }

    logfile->my_log = my_log;
    logfile->log_ops = (struct tsn_log_operations *) &unixfile_log_operations;
    ret = 0;

out:
    return ret;
}

int tsn_log_close_file(struct tsn_log_struct *logfile)
{
    put_tlg_ref(logfile->my_log);
    return 0;
}

static int unixfile_log_write(struct tsn_log_struct *logfile,
                          const void *buf, ssize_t n,
                          struct tsn_lsn_struct *out_lsn)
{
    struct tsn_lsn_struct new_lsn;
    struct tsn_logrecord_header lr;
    struct tsn_logdatum_struct *const my_log = logfile->my_log;
    ssize_t hdr_bytes;
    ssize_t bytes = -1;

    tsn_verbose("%s: enter\n", __func__);

    if (out_lsn == NULL) {
        goto out;
    }


    pthread_mutex_lock(&my_log->lsn_mutex);

    /* take tlg->next_lsn as our LSN */
    new_lsn = my_log->next_lsn;
    /* save next_lsn as last_lsn */
    my_log->last_lsn = my_log->next_lsn;
    /* now find the next LSN */
    my_log->next_lsn.lsn += n + sizeof(struct tsn_logrecord_header);

    pthread_mutex_lock(&my_log->fd_mutex);
    
    lr.h_lsn = new_lsn;
    lr.h_this_offset = lsn_to_offset(my_log, lr.h_lsn);
    lr.h_next_offset = lsn_to_offset(my_log, my_log->next_lsn);

    /* write the header */
    tsn_verbose("%s: write(%d, %p, %ld)\n", __func__, my_log->fd, &lr, sizeof(lr));
    hdr_bytes = write(my_log->fd, &lr, sizeof(lr));
    if (hdr_bytes < 0) {
        perror("write");
        goto out_unlock;
    } else if (hdr_bytes != sizeof(lr)) {        
        /* XXX: short write, we should handle this */
        goto out_unlock;
    }

    /* now write the record */
    tsn_verbose("%s: write(%d, %p, %ld)\n", __func__, my_log->fd, buf, n);
    bytes = write(my_log->fd, buf, n);
    if (bytes < 0) {
        perror("write");
    } else if (bytes != n) {
        /* XXX: short write, we should handle this */
        bytes = -1;
    }

out_unlock:
    pthread_mutex_unlock(&my_log->fd_mutex);
    pthread_mutex_unlock(&my_log->lsn_mutex);

    /* so the obvious thing to do here is... */
    if (bytes >= 0) {
        memcpy(out_lsn, &new_lsn, sizeof(*out_lsn));
    }

out:
    tsn_verbose("%s: returning %ld\n", __func__, bytes);
    return bytes;
}

static int unixfile_log_flush(struct tsn_log_struct *logfile,
                          const struct tsn_lsn_struct *to_lsn,
                          bool force_now,
                          struct tsn_lsn_struct *max_durable_lsn)
{
    int ret = -1;
    struct tsn_logdatum_struct *const my_log = logfile->my_log;

    pthread_mutex_lock(&my_log->lsn_mutex);
    if (to_lsn->lsn > my_log->durable_lsn.lsn) {
        /* this was written already */
        ret = 0;
    } else {
        /* not written yet */
        pthread_mutex_lock(&my_log->fd_mutex);
        ret = fsync(my_log->fd);
        pthread_mutex_unlock(&my_log->fd_mutex);
    }

    if (ret == 0) {
        my_log->durable_lsn = my_log->last_lsn;
        if (max_durable_lsn) {
            *max_durable_lsn = my_log->last_lsn;
        }
    }

    pthread_mutex_unlock(&my_log->lsn_mutex);

    return ret;
}

static size_t get_record_length(struct tsn_logrecord_header *lh)
{
    /* XXX:  Needs to handle wrapping */

    return lh->h_next_offset - lh->h_this_offset - sizeof(struct tsn_logrecord_header);
}

static inline uint64_t min_u64(uint64_t a, uint64_t b)
{
    if (a<b) { return a; } return b;
}

static int unixfile_log_read(struct tsn_log_struct *logfile,
                         const struct tsn_lsn_struct *lsn, void *buf,
                         ssize_t n)
{
    struct tsn_logdatum_struct *const my_log = logfile->my_log;
    struct tsn_logrecord_header lh;
    ssize_t len;
    ssize_t bytes = -1;
    off_t old_offset;
    off_t lsn_offset;
    off_t sys_offset;

    if ((logfile == NULL) || (lsn == NULL) || (buf == NULL)) { goto out; }

    pthread_mutex_lock(&my_log->fd_mutex);

    /* suppose we could store this, rather than use an extra system call. */
    old_offset = lseek(my_log->fd, 0, SEEK_CUR);
    if (old_offset < 0) {
        goto out_unlock;
    }

    /* XXX:  This should validate the offset, otherwise we could wind up
     * looking for something that's not in the log.  (We check for this
     * later, but this would avoid the expense */

    lsn_offset = lsn_to_offset(my_log, *lsn);
    sys_offset = lseek(my_log->fd, lsn_offset, SEEK_SET);
    if (sys_offset < 0) {
        perror("seek");
        goto out_lseek;
    } else if (sys_offset != lsn_offset) {
        goto out_lseek;
    }

    /* read the header */
    tsn_verbose("%s: read(%d, %p, %ld)\n", __func__, my_log->fd, &lh, sizeof(lh));
    bytes = read(my_log->fd, &lh, sizeof(lh));
    if (bytes == 0) {
        /* end of file, return an error */
        goto out_lseek;
    } else if (bytes < 0) {
        /* some kind of read error */
        perror("read");
        bytes = -1;
        goto out_lseek;
    } else if (bytes != sizeof(lh)) {
        /* short read */
    }

    /* validate the log entry */
    if (!log_header_is_ok(my_log, &lh)) {
        tsn_verbose("%s: invalid header\n", __func__);
        bytes = -1;
        goto out_lseek;
    }

    /* verify that the LSN in the header matches what we asked for */
    if (lh.h_lsn.lsn != lsn->lsn) {
        tsn_verbose("%lu != %lu\n", lh.h_lsn.lsn, lsn->lsn);
        goto out_lseek;
    }

    /* now, read out the record */
    len = get_record_length(&lh);
    len = min_u64(len, n);

    tsn_verbose("%s: read(%d, %p, %ld)\n", __func__, my_log->fd, buf, len);
    bytes = read(my_log->fd, buf, len);
    if (bytes < 0) { perror("read"); }
    if (bytes != len) { bytes = -1;  /* short read */ }

out_lseek:
    /* now seek back */
    sys_offset = lseek(my_log->fd, old_offset, SEEK_SET);
    if (sys_offset < 0) {
        perror("lseek");
    } else if (sys_offset != old_offset) {
        /* warn? */
        ;
    }
out_unlock:
    pthread_mutex_unlock(&my_log->fd_mutex);
out:
    tsn_verbose("%s: returning %ld\n", __func__, bytes);
    return bytes;
}
