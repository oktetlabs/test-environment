/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved. */

#include <obstack.h>
#include <string.h>

#include "memory.h"

/**
 * Pointer to an obstack that is used for allocation of log_msg data
 * structure.
 */
static struct obstack *log_msg_obstk = NULL;

/**
 * Pointer to an obstack that is used for allocation of node_info data
 * structure.
 */
static struct obstack *node_info_obstk = NULL;

static void
internal_obstack_alloc_failed(void)
{
    /* Go to the main procedure */
    THROW_EXCEPTION;
}

/**
 * These two definitions are for correct working of "obstack_init"
 * function.
 */
#define obstack_chunk_alloc malloc
#define obstack_chunk_free  free

/* See the description in memory.h */
struct obstack *
obstack_initialize(void)
{
    struct obstack *obstk;

    /* Allocate a new obstack */
    if ((obstk = (struct obstack *)malloc(sizeof(struct obstack))) == NULL)
        return NULL;

    obstack_alloc_failed_handler = &internal_obstack_alloc_failed;
    obstack_init(obstk);

    return obstk;
}

/* See the description in memory.h */
void
obstack_destroy(struct obstack *obstk)
{
    obstack_free(obstk, NULL);
    free(obstk);
}

/* See the description in memory.h */
void initialize_log_msg_pool(void)
{
    if (log_msg_obstk == NULL &&
        ((log_msg_obstk = obstack_initialize()) == NULL))
    {
        THROW_EXCEPTION;
    }
}

/* See the description in memory.h */
void destroy_log_msg_pool(void)
{
    if (log_msg_obstk != NULL)
    {
        obstack_destroy(log_msg_obstk);
        log_msg_obstk = NULL;
    }
}

/* See the description in memory.h */
log_msg *
alloc_log_msg(void)
{
    log_msg *msg;

    msg = (log_msg *)obstack_alloc(log_msg_obstk, sizeof(log_msg));
    memset(msg, 0, sizeof(*msg));
    msg->obstk = log_msg_obstk;

#ifdef BE_CAREFUL_WITH_POINTERS
    /*
     * It is not correct to do bzero, and think that all the pointers
     * got right NULL values.
     */
    msg->entity = NULL;
    msg->user = NULL;
    msg->level_str = NULL;
    msg->fmt_str = NULL;
    msg->args = NULL;
    msg->cur_arg = NULL;
    msg->txt_msg = NULL;
#endif

    return msg;
}

/* See the description in memory.h */
void
free_log_msg(log_msg *msg)
{
    if (msg != NULL)
        obstack_free(msg->obstk, msg);
}

/* See the description in memory.h */
void initialize_node_info_pool(void)
{
    if (node_info_obstk == NULL &&
        ((node_info_obstk = obstack_initialize()) == NULL))
    {
        THROW_EXCEPTION;
    }
}

/* See the description in memory.h */
void destroy_node_info_pool(void)
{
    if (node_info_obstk != NULL)
    {
        obstack_destroy(node_info_obstk);
        node_info_obstk = NULL;
    }
}

/* See the description in memory.h */
node_info_t *
alloc_node_info(void)
{
    return (node_info_t *)obstack_alloc(node_info_obstk,
                                        sizeof(node_info_t));
}

/* See the description in memory.h */
void
free_node_info(node_info_t *node)
{
    if (node != NULL)
        obstack_free(node_info_obstk, node);
}

/* See the description in memory.h */
void *
node_info_obstack_alloc(int size)
{
    return obstack_alloc(node_info_obstk, size);
}

/* See the description in memory.h */
void *
node_info_obstack_copy0(const void *address, int size)
{
    return obstack_copy0(node_info_obstk, address, size);
}

/* See the description in memory.h */
log_msg_ptr *
alloc_log_msg_ptr(void)
{
    log_msg_ptr *msg_ptr;

    msg_ptr = (log_msg_ptr *)calloc(1, sizeof(log_msg_ptr));
    if (msg_ptr == NULL)
    {
        fprintf(stderr, "%s\n", "Out of memory");
        THROW_EXCEPTION;
    }

    return msg_ptr;
}

/* See the description in memory.h */
void
free_log_msg_ptr(log_msg_ptr *msg_ptr)
{
    free(msg_ptr);
}
