/** @file
 * @brief TE project. Logger subsystem.
 *
 * This module provides data structures and logic for listener handling.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Viacheslav Galaktionov <Viacheslav.Galaktionov@oktetlabs.ru>
 */

#define TE_LGR_USER "Log streaming"

#include <sys/time.h>

#include "logger_listener.h"

log_listener    listeners[LOG_MAX_LISTENERS];
size_t          listeners_num;

/* See description in logger_listener.h */
te_errno
listener_process_plan(log_listener *listener, const refcnt_buffer *plan)
{
    fwrite(plan->buf, 1, plan->len, listener->file);
    fputs("\n", listener->file);
    fflush(listener->file);

    listener->state = LISTENER_GATHERING;
    return 0;
}

/* See description in logger_listener.h */
te_errno
listener_add_msg(log_listener *listener, const refcnt_buffer *msg)
{
    te_errno rc;

    rc = msg_buffer_add(&listener->buffer, msg);
    if (rc != 0)
        return rc;

    while (listener->buffer.n_items > 1 &&
           listener->buffer.total_length >
             listener->buffer_size * listener->buffers_num)
        msg_buffer_remove_first(&listener->buffer);

    return 0;
}

/* See description in logger_listener.h */
te_errno
listener_dump(log_listener *listener)
{
    refcnt_buffer *item;
    te_bool       first = TRUE;
    size_t        len = 0;

    fputc('[', listener->file);

    item = TAILQ_FIRST(&listener->buffer.items);
    while (item != NULL && (len == 0 || len < listener->buffer_size))
    {
        if (!first)
            fputc(',', listener->file);
        fwrite(item->buf, item->len, 1, listener->file);
        len += item->len;
        msg_buffer_remove_first(&listener->buffer);
        item = TAILQ_FIRST(&listener->buffer.items);
        first = FALSE;
    }

    fputs("]\n", listener->file);
    fflush(listener->file);

    gettimeofday(&listener->next_tv, NULL);
    /* Don't delay next dump if there are already enough messages for it */
    if (listener->buffer.total_length < listener->buffer_size)
        listener->next_tv.tv_sec += listener->interval;
    return 0;
}

/* See description in logger_listener.h */
te_errno
listener_finish(log_listener *listener)
{
    listener_free(listener);
    return 0;
}

/* See description in logger_listener.h */
void
listener_free(log_listener *listener)
{
    fclose(listener->file);
    msg_buffer_free(&listener->buffer);
    listener->state = LISTENER_FINISHED;
}
