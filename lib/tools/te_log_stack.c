/** @file
 * @brief API to stack log messages that are to be flushed later on.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Konstantin Ushakov <Konstantin.Ushakov@oktetlabs.ru>
 *
  */

#define TE_LGR_USER "Log Stack"

#include "te_config.h"

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#include "te_defs.h"
#include "te_log_stack.h"

#define LOG_STACK_DEPTH 10
#define LOG_STACK_ELEMENT_LEN 256

static __thread te_bool init_done
                           __attribute__((tls_model("global-dynamic")));
static __thread const char *top_point
                           __attribute__((tls_model("global-dynamic")));
static __thread char *log_stack[LOG_STACK_DEPTH]
                           __attribute__((tls_model("global-dynamic")));
static __thread unsigned log_stack_ptr
                           __attribute__((tls_model("global-dynamic")));

void
te_log_stack_init_here(const char *point)
{
    unsigned i;

    /* We won't reinit even if you ask us to. */
    if (init_done)
        return;

    for (i = 0; i < LOG_STACK_DEPTH; i++)
        log_stack[i] = calloc(1, LOG_STACK_ELEMENT_LEN);

    top_point = strdup(point);
    init_done = TRUE;
}

void
te_log_stack_push_under(const char *user,
                        const char *fmt, ...)
{
    va_list ap;
    int offset;

    if (!init_done)
        return;

    if (log_stack_ptr == LOG_STACK_DEPTH)
        return;

    va_start(ap, fmt);

    offset = snprintf(log_stack[log_stack_ptr], LOG_STACK_ELEMENT_LEN,
                      "[%s] ", user != NULL ? user : TE_LGR_USER);
    vsnprintf(log_stack[log_stack_ptr] + offset,
              LOG_STACK_ELEMENT_LEN - offset, fmt, ap);
    log_stack_ptr++;
    va_end(ap);
}

char *
te_log_stack_pop(void)
{
    if (!init_done || log_stack_ptr == 0)
        return NULL;

    return strdup(log_stack[--log_stack_ptr]);
}

void
te_log_stack_dump(te_log_level log_level)
{
    int i;

    if (!init_done)
        return;

    for (i = log_stack_ptr - 1; i >= 0; i--)
    {
        LGR_MESSAGE(log_level, TE_LGR_USER, "STACK[%u]: %s",
                    i, log_stack[i]);
    }
}

void
te_log_stack_maybe_reset(const char *point)
{
    if (top_point != NULL && strcmp(point, top_point) == 0)
        log_stack_ptr = 0;
}
