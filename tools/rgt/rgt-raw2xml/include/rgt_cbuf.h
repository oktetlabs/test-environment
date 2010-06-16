/** @file
 * @brief Test Environment: RGT chunked buffer.
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
 * 
 * $Id$
 */

#ifndef __TE_RGT_CBUF_H__
#define __TE_RGT_CBUF_H__

#include <stdio.h>
#include <stdlib.h>
#include "te_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Buffer chunk */
typedef struct rgt_cbuf_chunk rgt_cbuf_chunk;

struct rgt_cbuf_chunk {
    rgt_cbuf_chunk  *next;
    size_t  size;
    size_t  len;
    char    buf[0];
};

/**
 * Create a new chunk.
 *
 * @param size  Chunk contents size to allocate.
 *
 * @return New chunk or NULL, if failed to allocate memory.
 */
extern rgt_cbuf_chunk *rgt_cbuf_chunk_new(size_t size);

/**
 * Retension a chunk, freeing unused memory.
 *
 * @param c The chunk to retention.
 *
 * @return The retentioned chunk or NULL, if failed to re-allocate memory.
 */
extern rgt_cbuf_chunk *rgt_cbuf_chunk_retention(rgt_cbuf_chunk *c);

static inline void
rgt_cbuf_chunk_free(rgt_cbuf_chunk *c)
{
    free(c);
}


/** Buffer */
typedef struct rgt_cbuf rgt_cbuf;

struct rgt_cbuf {
    rgt_cbuf_chunk *first;
    rgt_cbuf_chunk *pre_last;
    rgt_cbuf_chunk *last;
    size_t          len;
};


/**
 * Check if a buffer is valid.
 *
 * @param b The buffer to check.
 *
 * @return TRUE if the buffer is valid, FALSE otherwise.
 */
extern te_bool rgt_cbuf_valid(const rgt_cbuf *b);

/**
 * Initialize a buffer.
 *
 * @param b     The buffer to initialize.
 * @param size  Amount of memory to pre-allocate, or 0 for none.
 *
 * @return Initialized buffer, or NULL if failed to allocate memory.
 */
extern rgt_cbuf *rgt_cbuf_init(rgt_cbuf *b, size_t size);

/**
 * Create a new buffer (allocate and initialize).
 *
 * @param size  Amount of memory to pre-allocate, or 0 for none.
 *
 * @return New buffer, or NULL if failed to allocate memory.
 */
extern rgt_cbuf *rgt_cbuf_new(size_t size);

/**
 * Retrieve buffer contents length.
 *
 * @param b     The buffer to retrieve length of.
 *
 * @return The buffer contents length.
 */
extern size_t rgt_cbuf_get_len(const rgt_cbuf *b);

/**
 * Retention a buffer, freeing unused memory.
 *
 * @param b The buffer to retention.
 *
 * @return TRUE if retention succeeded, FALSE if memory reallocation
 *         failed.
 */
extern te_bool rgt_cbuf_retention(rgt_cbuf *b);

/**
 * Append a memory chunk to a buffer.
 *
 * @param b The buffer to append to.
 * @param p Memory chunk pointer to append.
 * @param s Memory chunk size to append.
 *
 * @return TRUE if appended successfully, FALSE if failed to allocate
 *         memory.
 */
extern te_bool rgt_cbuf_append(rgt_cbuf *b, const void *p, size_t s);

/**
 * Move a buffer contents to the end of another buffer.
 *
 * @param x The buffer to merge to; will be retentioned first.
 * @param y The buffer to have contents removed.
 *
 * @return TRUE if merged successfully, FALSE if failed to reallocate
 *         memory.
 */
extern te_bool rgt_cbuf_merge(rgt_cbuf *x, rgt_cbuf *y);

/**
 * Write a buffer contents to a file.
 *
 * @param b The buffer to write out.
 * @param f The file to write to.
 *
 * @return TRUE if written successfully, FALSE otherwise (see errno).
 */
extern te_bool rgt_cbuf_writeout(const rgt_cbuf *b, FILE *f);

/**
 * Read a file contents into a buffer.
 *
 * @param b The buffer to have file's contents appended to.
 * @param f The file to read from.
 *
 * @return Number of bytes read, see ferror(3) and errno for errors.
 */
extern size_t rgt_cbuf_readin(rgt_cbuf *b, FILE *f);

/**
 * Clear (remove) a buffer contents.
 *
 * @param b The buffer to clear.
 */
extern void rgt_cbuf_clear(rgt_cbuf *b);

/**
 * Free a buffer.
 *
 * @param b The buffer to free, could be NULL.
 */
extern void rgt_cbuf_free(rgt_cbuf *b);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_RGT_CBUF_H__ */
