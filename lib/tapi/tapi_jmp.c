/** @file
 * @brief Test API to use setjmp/longjmp.
 *
 * Thread safe stack of jump buffers and API to deal with it.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LOG_LEVEL    0xff
#define TE_LGR_USER     "TAPI Jumps"

#include "te_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#else
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#endif

#if HAVE_PTHREAD_H
#include <pthread.h>
#else
#error Required pthread.h not found
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "tapi_jmp.h"


/** List of jump points */
typedef LIST_HEAD(tapi_jmp_stack, tapi_jmp_point) tapi_jmp_stack;

/** Jumps context */
typedef struct tapi_jmp_ctx {
    tapi_jmp_stack  stack;      /**< Stack of jump points */
    tapi_jmp_stack  garbage;    /**< Stack of garbage jump points */
} tapi_jmp_ctx;


static pthread_once_t   jmp_once_control = PTHREAD_ONCE_INIT;
static pthread_key_t    jmp_key;


static tapi_jmp_ctx *tapi_jmp_get_ctx(te_bool create);


/**
 * Free garbage in the jumps context.
 *
 * @param ctx       Jumps context
 */
static void
tapi_jmp_ctx_free_garbage(tapi_jmp_ctx *ctx)
{
    tapi_jmp_point *p;

    assert(ctx != NULL);
    while ((p = ctx->garbage.lh_first) != NULL)
    {
        LIST_REMOVE(p, links);
        free(p);
    }
}

/**
 * Destructor of resources assosiated with thread.
 *
 * @param handle    Handle assosiated with thread
 */
static void
tapi_jmp_thread_ctx_destroy(void *handle)
{
    tapi_jmp_ctx   *ctx = handle;
    tapi_jmp_point *p;

    if (ctx == NULL)
        return;

    tapi_jmp_ctx_free_garbage(ctx);
    while ((p = ctx->stack.lh_first) != NULL)
    {
        LIST_REMOVE(p, links);
        ERROR("Jump point destructed: %s:%u", p->file, p->lineno);
        free(p);
    }
}

/**
 * Callback to be called at application exit.
 */
static void
tapi_jmp_atexit_callback(void)
{
    tapi_jmp_thread_ctx_destroy(tapi_jmp_get_ctx(FALSE));
}

/**
 * Only once called function to create key.
 */
static void
tapi_jmp_key_create(void)
{
    if (pthread_key_create(&jmp_key, tapi_jmp_thread_ctx_destroy) != 0)
    {
        ERROR("%s(): pthread_key_create() failed", __FUNCTION__);
        fprintf(stderr, "pthread_key_create() failed\n");
    }
    atexit(tapi_jmp_atexit_callback);
}

/**
 * Find thread context handle or initialize a new one.
 *
 * @return Context handle or NULL if some error occured
 */
static tapi_jmp_ctx *
tapi_jmp_get_ctx(te_bool create)
{
    tapi_jmp_ctx   *ctx;

    if (pthread_once(&jmp_once_control, tapi_jmp_key_create) != 0)
    {
        ERROR("%s(): pthread_once() failed", __FUNCTION__);
        return NULL;
    }

    ctx = pthread_getspecific(jmp_key);
    if (ctx == NULL && create)
    {
        ctx = calloc(1, sizeof(*ctx));
        if (ctx == NULL)
        {
            ERROR("%s(): calloc(1, %u) failed",
                  __FUNCTION__, sizeof(*ctx));
        }
        else
        {
            LIST_INIT(&ctx->stack);
            LIST_INIT(&ctx->garbage);
            if (pthread_setspecific(jmp_key, ctx) != 0)
            {
                ERROR("%s(): pthread_setspecific() failed", __FUNCTION__);
                free(ctx);
                ctx = NULL;
            }
        }
    }

    return ctx;
}


/* See description in tapi_jmp.h */
tapi_jmp_point *
tapi_jmp_push(const char *file, unsigned int lineno)
{
    tapi_jmp_ctx   *ctx = tapi_jmp_get_ctx(TRUE);
    tapi_jmp_point *p;

    if (ctx == NULL)
    {
        ERROR("%s(): No context");
        return NULL;
    }
    tapi_jmp_ctx_free_garbage(ctx);

    p = calloc(1, sizeof(*p));
    if (p == NULL)
    {
        ERROR("%s(): calloc(1, %u) failed", __FUNCTION__, sizeof(*p));
        return NULL;
    }
    LIST_INSERT_HEAD(&ctx->stack, p, links);
    p->file    = file;
    p->lineno  = lineno;

    INFO("Set jump point %s:%u", file, lineno);
    return p;
}

/* See description in tapi_jmp.h */
int
tapi_jmp_pop(const char *file, unsigned int lineno)
{
    tapi_jmp_ctx   *ctx;
    tapi_jmp_point *p;

    ctx = tapi_jmp_get_ctx(FALSE);
    if (ctx == NULL)
    {
        ERROR("%s(): No context", __FUNCTION__);
        return TE_RC(TE_TAPI, ENOMEM);
    }
    tapi_jmp_ctx_free_garbage(ctx);

    if ((p = ctx->stack.lh_first) == NULL)
    {
        ERROR("%s(): Jumps stack is empty", __FUNCTION__);
        return TE_RC(TE_TAPI, ENOENT);
    }
    LIST_REMOVE(p, links);

    INFO("Remove jump point %s:%u at %s:%u",
         p->file, p->lineno, file, lineno);

    free(p);

    return 0;
}

/* See description in tapi_jmp.h */
int
tapi_jmp_do(int val, const char *file, unsigned int lineno)
{
    tapi_jmp_ctx   *ctx;
    tapi_jmp_point *p;

    if (val < 0)
    {
        ERROR("%s(): Invalid return value %d for jump to do",
              __FUNCTION__, val);
        return TE_RC(TE_TAPI, EINVAL);
    }
    else if (val == 0)
    {
        val = TE_EOK;
    }

    ctx = tapi_jmp_get_ctx(FALSE);
    if (ctx == NULL)
    {
        ERROR("%s(): No context", __FUNCTION__);
        return TE_RC(TE_TAPI, ENOMEM);
    }
    tapi_jmp_ctx_free_garbage(ctx);

    if ((p = ctx->stack.lh_first) == NULL)
    {
        ERROR("%s(): Jumps stack is empty", __FUNCTION__);
        return TE_RC(TE_TAPI, ENOENT);
    }
    LIST_REMOVE(p, links);

    /* 
     * We can't free point here, since 'env' should be used
     * in the function which never returns.
     */
    LIST_INSERT_HEAD(&ctx->garbage, p, links);

    INFO("Jump from %s:%u to %s:%u rc=%x",
         file, lineno, p->file, p->lineno, (unsigned int)val);
    longjmp(p->env, val);

    /* Unreachable */
    assert(FALSE);
}

/* See description in tapi_jmp.h */
te_bool
tapi_jmp_stack_is_empty(void)
{
    tapi_jmp_ctx   *ctx;

    ctx = tapi_jmp_get_ctx(FALSE);

    return (ctx == NULL) || (ctx->stack.lh_first == NULL);
}
