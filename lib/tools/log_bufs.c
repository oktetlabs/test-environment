/** @file
 * @brief API to collect long log messages
 *
 * Implementation of API to collect long log messages.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 */

#define TE_LGR_USER     "Log Buffers"

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#endif

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#if HAVE_PTHREAD_H
#include <pthread.h>
#else
#error Required pthread.h not found
#endif

#include "te_defs.h"
#include "te_string.h"
#include "logger_api.h"
#include "log_bufs.h"
#include "te_queue.h"
#include "te_alloc.h"
#include "te_sockaddr.h"

/** The number of characters in a sinle log buffer. */
#define LOG_BUF_LEN (1024 * 10)


/** Internal presentation of log buffer. */
struct te_log_buf {
    te_string   str;    /**< Buffer data - must be the first member */
    te_bool     used;   /**< Whether this buffer is already in use */

    TAILQ_ENTRY(te_log_buf) links; /**< Queue links */
};

/** Type of buffers queue head */
typedef TAILQ_HEAD(te_log_bufs, te_log_buf) te_log_bufs;

/** Head of the queue of log buffers */
static te_log_bufs bufs_queue = TAILQ_HEAD_INITIALIZER(bufs_queue);

/** Mutex used to protect shred log buffer pool. */
static pthread_mutex_t te_log_buf_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Release function which will be called from te_string_free().
 *
 * @param str     Pointer to TE string obtained with
 *                te_log_str_alloc().
 */
static te_string_free_func te_log_str_free;
static void
te_log_str_free(te_string *str)
{
    if (str == NULL)
        return;

    te_log_buf_free((te_log_buf *)str);
}

te_log_buf *
te_log_buf_alloc(void)
{
    te_log_buf *buf;
    te_log_buf *prev_buf;

    pthread_mutex_lock(&te_log_buf_mutex);

    buf = TAILQ_LAST(&bufs_queue, te_log_bufs);
    if (buf == NULL || buf->used)
    {
        buf = TE_ALLOC(sizeof(te_log_buf));
        if (buf == NULL)
        {
            pthread_mutex_unlock(&te_log_buf_mutex);
            return NULL;
        }
        buf->str = (te_string)TE_STRING_INIT_RESERVE_FREE(LOG_BUF_LEN,
                                                          &te_log_str_free);
        TAILQ_INSERT_HEAD(&bufs_queue, buf, links);
    }
    else
    {
        /*
         * Make sure that all the buffers which are in use are at the
         * beginning of the queue.
         */
        prev_buf = TAILQ_PREV(buf, te_log_bufs, links);
        if (prev_buf != NULL && !prev_buf->used)
        {
            TAILQ_REMOVE(&bufs_queue, buf, links);
            TAILQ_INSERT_HEAD(&bufs_queue, buf, links);
        }
    }

    buf->used = TRUE;
    pthread_mutex_unlock(&te_log_buf_mutex);

    return buf;
}

te_string *
te_log_str_alloc(void)
{
    te_log_buf *buf;

    buf = te_log_buf_alloc();
    if (buf == NULL)
        return NULL;

    return &buf->str;
}

void
te_log_buf_free(te_log_buf *buf)
{
    te_log_buf *next_buf;

    if (buf == NULL)
        return;

    pthread_mutex_lock(&te_log_buf_mutex);
    te_string_reset(&buf->str);
    buf->used = FALSE;

    /*
     * Make sure that all the free buffers are at the
     * tail of the queue.
     */
    next_buf = TAILQ_NEXT(buf, links);
    if (next_buf != NULL)
    {
        TAILQ_REMOVE(&bufs_queue, buf, links);
        TAILQ_INSERT_TAIL(&bufs_queue, buf, links);
    }

    pthread_mutex_unlock(&te_log_buf_mutex);
}

void
te_log_bufs_cleanup(void)
{
    te_log_buf *buf;
    te_log_buf *buf_aux;

    pthread_mutex_lock(&te_log_buf_mutex);

    TAILQ_FOREACH_SAFE(buf, &bufs_queue, links, buf_aux)
    {
        TAILQ_REMOVE(&bufs_queue, buf, links);
        free(buf->str.ptr);
        free(buf);
    }

    pthread_mutex_unlock(&te_log_buf_mutex);
}

/** Check that 'ptr_' is a valid log buffer */
#define VALIDATE_LOG_BUF(ptr_) \
    do {                                                                \
        assert((ptr_)->used == TRUE);                                   \
    } while (0)

/* See description in log_bufs.h */
int
te_log_buf_append(te_log_buf *buf, const char *fmt, ...)
{
    te_errno rc;
    size_t   old_len = buf->str.len;
    va_list  ap;

    VALIDATE_LOG_BUF(buf);

    va_start(ap, fmt);
    rc = te_string_append_va(&buf->str, fmt, ap);
    va_end(ap);
    if (rc != 0)
        return -1;

    /* Must be at worst the same */
    assert(buf->str.len >= old_len);

    return buf->str.len - old_len;
}

const char *
te_log_buf_get(te_log_buf *buf)
{
    VALIDATE_LOG_BUF(buf);

    if (buf->str.len == 0)
    {
        assert(te_string_append(&buf->str, "") == 0);
    }

    return buf->str.ptr;
}

static te_errno
te_bit_mask_or_flag2te_str(te_string *te_str, unsigned long long bit_mask,
                           const struct te_log_buf_bit2str *bit_map,
                           const struct te_log_buf_flag2str *flag_map,
                           unsigned long long *left_bit_mask,
                           te_bool append_left, te_bool *added_out)
{
    unsigned int        i;
    te_bool             added = FALSE;
    te_bool             append;
    uint64_t            bit_or_flag;
    const char         *str;

    te_errno rc;

    for (i = 0; (str = bit_map != NULL ? bit_map[i].str : flag_map[i].str)
                != NULL; ++i)
    {
        if (bit_map != NULL)
        {
            bit_or_flag = (UINT64_C(1) << bit_map[i].bit);
            append = (bit_mask & bit_or_flag) != 0;
        }
        else
        {
            bit_or_flag = flag_map[i].flag;
            append = (bit_mask & flag_map[i].mask) == bit_or_flag;
        }

        if (append)
        {
            rc = te_string_append(te_str, "%s%s", added ? "|" : "", str);
            if (rc != 0)
                return rc;
            added = TRUE;
            bit_mask &= ~bit_or_flag;
        }
    }

    if (bit_mask != 0 && append_left)
    {
        rc = te_string_append(te_str, "%s%#llx", added ? "|" : "",
                              bit_mask);
        if (rc != 0)
            return rc;
    }

    if (left_bit_mask != NULL)
        *left_bit_mask = bit_mask;

    if (added_out != NULL)
        *added_out = added;

    return 0;
}

te_errno
te_bit_mask2te_str(te_string *str,
                   unsigned long long bit_mask,
                   const te_bit2str *map)
{
    return te_bit_mask_or_flag2te_str(str, bit_mask, map, NULL, NULL, TRUE,
                                      NULL);
}

const char *
te_bit_mask2log_buf(te_log_buf *buf, unsigned long long bit_mask,
                    const struct te_log_buf_bit2str *map)
{
    te_bit_mask2te_str(&buf->str, bit_mask, map);

    return te_log_buf_get(buf);
}

te_errno
te_extended_bit_mask2te_str(te_string *str,
                            unsigned long long bit_mask,
                            const te_bit2str *bm,
                            const te_flag2str *fm)
{
    unsigned long long left;
    te_bool added;
    te_errno rc;

    rc = te_bit_mask_or_flag2te_str(str, bit_mask, bm, NULL, &left,
                                    FALSE, &added);
    if (rc != 0)
        return rc;

    if (added)
    {
        rc = te_string_append(str, "|");
        if (rc != 0)
            return rc;
    }

    rc = te_bit_mask_or_flag2te_str(str, left, NULL, fm, &left, FALSE,
                                    added ? NULL : &added);
    if (rc != 0)
        return rc;

    if (left != 0)
        return te_string_append(str, "%s%#llx", added ? "|" : "", left);

    return 0;
}

const char *
te_extended_bit_mask2log_buf(te_log_buf *buf, unsigned long long bit_mask,
                             const struct te_log_buf_bit2str *bm,
                             const struct te_log_buf_flag2str *fm)
{
    te_extended_bit_mask2te_str(&buf->str, bit_mask, bm, fm);

    return te_log_buf_get(buf);
}

te_errno
te_args2te_str(te_string *str, int argc, const char **argv)
{
    int i;
    te_errno rc;

    for (i = 0; i < argc; ++i)
    {
        rc = te_string_append(str, "%s\"%s\"", i == 0 ? "" : ", ", argv[i]);
        if (rc != 0)
            return rc;
    }

    return 0;
}

const char *
te_args2log_buf(te_log_buf *buf, int argc, const char **argv)
{
    te_args2te_str(&buf->str, argc, argv);

    return te_log_buf_get(buf);
}

const char *
te_ether_addr2log_buf(te_log_buf *buf, const uint8_t *mac_addr)
{
    te_mac_addr2te_str(&buf->str, mac_addr);

    return te_log_buf_get(buf);
}
