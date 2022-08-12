/** @file
 * @brief TE project. Logger subsystem.
 *
 * This module provides different buffers for log messages.
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 *
 */

#define TE_LGR_USER "Buffers"

#include "te_defs.h"
#include "logger_api.h"
#include "logger_bufs.h"
#include "te_alloc.h"

/* See description in logger_bufs.h */
te_errno
refcnt_buffer_init(refcnt_buffer *rbuf, void *data, size_t len)
{
    int *refcount;

    refcount = TE_ALLOC(sizeof(*refcount));
    if (refcount == NULL)
        return TE_ENOMEM;
    *refcount = 1;

    memset(rbuf, 0, sizeof(*rbuf));
    rbuf->buf = data;
    rbuf->len = len;
    rbuf->refcount = refcount;

    return 0;
}

/* See description in logger_bufs.h */
te_errno
refcnt_buffer_init_copy(refcnt_buffer *rbuf, const void *data, size_t len)
{
    te_errno  rc;
    void     *buf;

    buf = TE_ALLOC(len);
    if (buf == NULL)
        return TE_ENOMEM;
    memcpy(buf, data, len);

    rc = refcnt_buffer_init(rbuf, buf, len);
    if (rc != 0)
        free(buf);

    return rc;
}

/* See description in logger_bufs.h */
void
refcnt_buffer_copy(refcnt_buffer *dest, const refcnt_buffer *src)
{
    if (dest->buf != NULL)
        refcnt_buffer_free(dest);

    memset(dest, 0, sizeof(*dest));
    dest->buf = src->buf;
    dest->len = src->len;
    dest->refcount = src->refcount;

    (*dest->refcount)++;
}


/* See description in logger_bufs.h */
void
refcnt_buffer_free(refcnt_buffer *rbuf)
{
    *rbuf->refcount -= 1;
    if (*rbuf->refcount == 0)
    {
        free(rbuf->buf);
        free(rbuf->refcount);
    }

    rbuf->buf = NULL;
    rbuf->len = 0;
    rbuf->refcount = NULL;
}

/* See description in logger_bufs.h */
void
msg_buffer_init(msg_buffer *buf)
{
    TAILQ_INIT(&buf->items);
    buf->n_items = 0;
    buf->total_length = 0;
}

/* See description in logger_bufs.h */
te_errno
msg_buffer_add(msg_buffer *buf, const refcnt_buffer *msg)
{
    refcnt_buffer *item;

    item = TE_ALLOC(sizeof(*item));
    if (item == NULL)
        return TE_ENOMEM;

    refcnt_buffer_copy(item, msg);
    TAILQ_INSERT_TAIL(&buf->items, item, links);

    buf->n_items += 1;
    buf->total_length += msg->len;
    return 0;
}

/* See description in logger_bufs.h */
void
msg_buffer_remove_first(msg_buffer *buf)
{
    refcnt_buffer *item;

    item = TAILQ_FIRST(&buf->items);
    if (item != NULL)
    {
        TAILQ_REMOVE(&buf->items, item, links);
        buf->n_items -= 1;
        buf->total_length -= item->len;
        refcnt_buffer_free(item);
        free(item);
    }
}

/* See description in logger_bufs.h */
void
msg_buffer_free(msg_buffer *buf)
{
    refcnt_buffer *item;
    refcnt_buffer *tmp;

    if (!TAILQ_EMPTY(&buf->items))
    {
        WARN("Not all messages have been processed");
        TAILQ_FOREACH_SAFE(item, &buf->items, links, tmp)
        {
            refcnt_buffer_free(item);
            free(item);
        }
    }

    msg_buffer_init(buf);
}
