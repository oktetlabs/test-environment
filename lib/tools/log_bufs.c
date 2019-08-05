/** @file
 * @brief API to collect long log messages
 *
 * Implementation of API to collect long log messages.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Log Buffers"

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdio.h>
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

/** The number of characters in a sinle log buffer. */
#define LOG_BUF_LEN (1024 * 10)
/** The number of buffers in log buffer pool. */
#define LOG_BUF_NUM 10


/** Internal presentation of log buffer. */
struct te_log_buf {
    te_bool     used;   /**< Whether this buffer is already in use */
    te_string   str;    /**< Buffer data */
};

/** Statically allocated pool of log buffers. */
static te_log_buf te_log_bufs[LOG_BUF_NUM];

/** The index of the last freed buffer, or -1 if unknown. */
static int te_log_buf_last_freed = 0;

/** Mutex used to protect shred log buffer pool. */
static pthread_mutex_t te_log_buf_mutex = PTHREAD_MUTEX_INITIALIZER;

/** Check that 'ptr_' is a valid log buffer */
#define VALIDATE_LOG_BUF(ptr_) \
    do {                                                                \
        assert(((ptr_) >= te_log_bufs));                                \
        assert(((uintptr_t)(ptr_) - (uintptr_t)te_log_bufs) %           \
               sizeof(te_log_bufs[0]) == 0);                            \
        assert((ptr_ - te_log_bufs) < LOG_BUF_NUM);                     \
        assert((ptr_)->used == TRUE);                                   \
    } while (0)

/* See description in log_bufs.h */
te_log_buf *
te_log_buf_alloc()
{
    int i;
    int id;

    pthread_mutex_lock(&te_log_buf_mutex);

    if (te_log_buf_last_freed != -1)
    {
        assert(te_log_bufs[te_log_buf_last_freed].used == FALSE &&
               te_log_bufs[te_log_buf_last_freed].str.len == 0);

        id = te_log_buf_last_freed;
        te_log_buf_last_freed = -1;
        te_log_bufs[id].used = TRUE;
        te_log_bufs[id].str = (te_string)TE_STRING_INIT_RESERVE(LOG_BUF_LEN);
        pthread_mutex_unlock(&te_log_buf_mutex);

        return &te_log_bufs[id];
    }

    for (i = 0; i < LOG_BUF_NUM; i++)
    {
        if (!te_log_bufs[i].used)
        {
            assert(te_log_bufs[i].str.len == 0);

            te_log_bufs[i].used = TRUE;
            te_log_bufs[i].str = (te_string)TE_STRING_INIT_RESERVE(LOG_BUF_LEN);
            pthread_mutex_unlock(&te_log_buf_mutex);

            return &te_log_bufs[i];
        }
    }
    pthread_mutex_unlock(&te_log_buf_mutex);

    /* There is no available buffer, wait until one freed */
    do {
        pthread_mutex_lock(&te_log_buf_mutex);
        if (te_log_buf_last_freed != -1)
        {
            assert(te_log_bufs[te_log_buf_last_freed].used == FALSE &&
                   te_log_bufs[te_log_buf_last_freed].str.len == 0);

            id = te_log_buf_last_freed;
            te_log_buf_last_freed = -1;
            te_log_bufs[id].used = TRUE;
            te_log_bufs[id].str = (te_string)TE_STRING_INIT_RESERVE(
                LOG_BUF_LEN);
            pthread_mutex_unlock(&te_log_buf_mutex);

            return &te_log_bufs[id];
        }
        pthread_mutex_unlock(&te_log_buf_mutex);

        RING("Waiting for a tapi log buffer");
        sleep(1);
    } while (TRUE);

    assert(0);
    return NULL;
}

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

/* See description in log_bufs.h */
void
te_log_buf_free(te_log_buf *buf)
{
    if (buf == NULL)
        return;

    VALIDATE_LOG_BUF(buf);

    pthread_mutex_lock(&te_log_buf_mutex);
    buf->used = FALSE;
    te_string_free(&buf->str);
    te_log_buf_last_freed =
        (buf - te_log_bufs) / sizeof(te_log_bufs[0]);
    pthread_mutex_unlock(&te_log_buf_mutex);
}

const char *
te_bit_mask2log_buf(te_log_buf *buf, unsigned long long bit_mask,
                    const struct te_log_buf_bit2str *map)
{
    unsigned int        i;
    unsigned long long  mask;
    te_bool             added = FALSE;

    VALIDATE_LOG_BUF(buf);

    for (i = 0; map[i].str != NULL; ++i)
    {
        mask = (1ull << map[i].bit);
        if ((bit_mask & mask) != 0)
        {
            te_log_buf_append(buf, "%s%s", added ? "|" : "", map[i].str);
            added = TRUE;
            bit_mask &= ~mask;
        }
    }

    if (bit_mask != 0)
        te_log_buf_append(buf, "%s%#llx", added ? "|" : "", bit_mask);

    return te_log_buf_get(buf);
}

const char *
te_args2log_buf(te_log_buf *buf, int argc, const char **argv)
{
    int     i;

    for (i = 0; i < argc; ++i)
        te_log_buf_append(buf, "%s\"%s\"", i == 0 ? "" : ", ", argv[i]);

    return te_log_buf_get(buf);
}

const char *
te_ether_addr2log_buf(te_log_buf *buf, const uint8_t *mac_addr)
{
    if (mac_addr == NULL)
        te_log_buf_append(buf, "<NULL>");
    else
        te_log_buf_append(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
                          mac_addr[0], mac_addr[1],
                          mac_addr[2], mac_addr[3],
                          mac_addr[4], mac_addr[5]);

    return te_log_buf_get(buf);
}

const char *
te_ip_addr2log_buf(te_log_buf *buf, const void *ip_addr, int addr_str_len)
{
    char ip_addr_str[addr_str_len];
    int  af;

    switch (addr_str_len)
    {
        case INET_ADDRSTRLEN:
            af = AF_INET;
            break;
        case INET6_ADDRSTRLEN:
            af = AF_INET6;
            break;
        default:
            af = AF_UNSPEC;
            break;
    }

    inet_ntop(af, ip_addr, ip_addr_str, addr_str_len);

    te_log_buf_append(buf, "%s", ip_addr_str);

    return te_log_buf_get(buf);
}
