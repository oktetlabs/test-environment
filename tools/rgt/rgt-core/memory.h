/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved. */

#ifndef __TE_RGT_MEMORY_H__
#define __TE_RGT_MEMORY_H__

#include "log_msg.h"

/** Allocate and initialize a new obstack structure */
struct obstack *obstack_initialize(void);
/** Deallocate an obstack structure */
void obstack_destroy(struct obstack *obstk);

/**
 * Initialize the log message pool.
 *
 * Throws if it's already initialized or on failure.
 */
void initialize_log_msg_pool(void);
/** Free the log message pool */
void destroy_log_msg_pool(void);

/** Allocate a log message buffer from the pool  */
log_msg *alloc_log_msg(void);
/** Return a log message buffer to the pool */
void free_log_msg(log_msg *msg);

/** Allocate memory for log_msg_ptr structure */
extern log_msg_ptr *alloc_log_msg_ptr(void);
/** Release memory occupied by log_msg_ptr structure */
extern void free_log_msg_ptr(log_msg_ptr *msg_ptr);

/**
 * Initialize the node_info pool.
 *
 * Throws if it's already initialized or on failure.
 */
void initialize_node_info_pool(void);
/** Free the node_info pool */
void destroy_node_info_pool(void);

/** Allocate a node_info structure from the pool */
node_info_t *alloc_node_info(void);
/** Return a node_info structure to the pool */
void free_node_info(node_info_t *node);

/** Allocate a buffer of given size from the node_info pool */
void *node_info_obstack_alloc(int size);

/**
 * Allocate a copy of user-supplied data from the node_info pool.
 *
 * This function adds an extra null byte at the end of the buffer it allocates.
 * This means that if the user is copying a C string, they do not have to
 * include the null terminator. In other words, the result of strlen can be
 * passed as the size argument without any modification.
 *
 * @param address       data buffer
 * @param size          size of data
 *
 * @returns Pointer to the allocated buffer
 */
void *node_info_obstack_copy0(const void *address, int size);

#endif /* __TE_RGT_MEMORY_H__ */
