/** @file
 * @brief Test Environment: RGT chunked output - chunk.
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

#ifndef __TE_RGT_CO_CHUNK_H__
#define __TE_RGT_CO_CHUNK_H__

#include "rgt_co_strg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rgt_co_chunk rgt_co_chunk;

struct rgt_co_chunk {
    rgt_co_chunk   *next;       /**< Next chunk */
    rgt_co_strg     strg;       /**< Storage */
    te_bool         finished;   /**< "Finished" flag */
    size_t          depth;      /**< Nesting depth */
};

/**
 * Check if a chunk is valid.
 *
 * @param c Chunk to check.
 * 
 * @return TRUE if the chunk is valid, FALSE otherwise.
 */
extern te_bool rgt_co_chunk_valid(const rgt_co_chunk *c);

/**
 * Validate a chunk.
 *
 * @param c Chunk to validate.
 *
 * @return Validated chunk.
 */
static inline const rgt_co_chunk*
rgt_co_chunk_validate(const rgt_co_chunk *c)
{
    assert(rgt_co_chunk_valid(c));
    return c;
}

/**
 * Initialize a chunk with void storage.
 *
 * @param c     Chunk to initialize.
 * @param depth Chunk nesting depth.
 *
 * @return Initialized chunk.
 */
extern rgt_co_chunk *rgt_co_chunk_init(rgt_co_chunk    *c,
                                       size_t           depth);

/**
 * Cleanup a chunk.
 *
 * @param c The chunk to cleanup.
 *
 * @return TRUE if cleaned up successfully, FALSE if file media is used in
 *         the storage and it failed to close.
 */
extern te_bool rgt_co_chunk_clnp(rgt_co_chunk *c);

/**
 * Check if a chunk is finished.
 *
 * @param c The chunk to check.
 *
 * @return TRUE if the chunk is finished, FALSE otherwise.
 */
static inline te_bool
rgt_co_chunk_finished(const rgt_co_chunk *c)
{
    assert(rgt_co_chunk_valid(c));
    return c->finished;
}

/**
 * Mark chunk as finished.
 *
 * @param c The chunk to finish.
 *
 * @return TRUE if the chunk finished successfully, FALSE otherwise.
 *
 * @note The chunk may get freed as a result.
 */
static inline te_bool
rgt_co_chunk_finish(rgt_co_chunk *c)
{
    assert(!rgt_co_chunk_finished(c));
    c->finished = TRUE;
    return rgt_co_strg_retention(&c->strg);
}

/**
 * Supply a chunk with a file as a storage media.
 *
 * @param c     The chunk to supply with the file.
 * @param file  The file to supply with.
 * @param len   The file contents length.
 *
 * @return The chunk using the supplied media.
 */
static inline rgt_co_chunk *
rgt_co_chunk_take_file(rgt_co_chunk *c, FILE *file, size_t len)
{
    assert(rgt_co_chunk_valid(c));
    assert(rgt_co_strg_is_void(&c->strg));
    assert(file != NULL);
    rgt_co_strg_take_file(&c->strg, file, len);
    return c;
}

/**
 * Supply a chunk with a buffer as a storage media.
 *
 * @param c     The chunk to supply with the buffer.
 * @param mem   The buffer to supply with.
 * @param len   The buffer contents length.
 *
 * @return The chunk using the supplied media.
 */
static inline rgt_co_chunk *
rgt_co_chunk_take_mem(rgt_co_chunk *c, rgt_cbuf *mem, size_t len)
{
    assert(rgt_co_chunk_valid(c));
    assert(rgt_co_strg_is_void(&c->strg));
    assert(rgt_cbuf_valid(mem));
    rgt_co_strg_take_mem(&c->strg, mem, len);
    return c;
}

/**
 * Check if a chunk storage is void.
 *
 * @param c The chunk to check.
 *
 * @return TRUE if the chunk storage is void, FALSE otherwise.
 */
static inline te_bool
rgt_co_chunk_is_void(const rgt_co_chunk *c)
{
    assert(rgt_co_chunk_valid(c));
    return rgt_co_strg_is_void(&c->strg);
}

/**
 * Check if a chunk storage uses file media.
 *
 * @param c The chunk to check.
 *
 * @return TRUE if the chunk media is a file, FALSE otherwise.
 */
static inline te_bool
rgt_co_chunk_is_file(const rgt_co_chunk *c)
{
    assert(rgt_co_chunk_valid(c));
    return rgt_co_strg_is_file(&c->strg);
}

/**
 * Check if a chunk storage uses buffer media.
 *
 * @param c The chunk to check.
 *
 * @return TRUE if the chunk media is a buffer, FALSE otherwise.
 */
static inline te_bool
rgt_co_chunk_is_mem(const rgt_co_chunk *c)
{
    assert(rgt_co_chunk_valid(c));
    return rgt_co_strg_is_mem(&c->strg);
}

/**
 * Relocate chunk storage contents to another (file) media.
 *
 * @param c     The chunk to relocate.
 * @param file  The file to relocate to.
 * @param len   The file contents length.
 *
 * @return The chunk relocated to the specified media.
 */
static inline te_bool
rgt_co_chunk_relocate_to_file(rgt_co_chunk *c, FILE *file, size_t len)
{
    rgt_co_strg  strg;

    assert(rgt_co_chunk_valid(c));
    assert(file != NULL);

    return
        rgt_co_strg_move_media(
                &c->strg,
                rgt_co_strg_take_file(
                    rgt_co_strg_init(&strg), file, len));
}

/**
 * Relocate chunk storage contents to another (buffer) media.
 *
 * @param c     The chunk to relocate.
 * @param mem   The buffer to relocate to.
 * @param len   The buffer contents length.
 *
 * @return The chunk relocated to the specified media.
 */
static inline te_bool
rgt_co_chunk_relocate_to_mem(rgt_co_chunk *c, rgt_cbuf *mem, size_t len)
{
    rgt_co_strg  strg;

    assert(rgt_co_chunk_valid(c));
    assert(rgt_cbuf_valid(mem));

    return
        rgt_co_strg_move_media(
                &c->strg,
                rgt_co_strg_take_mem(
                    rgt_co_strg_init(&strg), mem, len));
}

/**
 * Take the file media from a finished chunk; the chunk storage becomes
 * void.
 *
 * @param c     The chunk to retrieve the media from.
 * @param plen  Location for the contents length, or NULL.
 *
 * @return The chunk file media.
 */
static inline FILE *
rgt_co_chunk_yield_file(rgt_co_chunk *c, size_t *plen)
{
    assert(rgt_co_chunk_valid(c));
    assert(rgt_co_chunk_finished(c));
    assert(rgt_co_chunk_is_file(c));
    return rgt_co_strg_yield_file(&c->strg, plen);
}

/**
 * Take the buffer media from a finished chunk; the chunk storage becomes
 * void.
 *
 * @param c     The chunk to retrieve the media from.
 * @param plen  Location for the contents length, or NULL.
 *
 * @return The chunk buffer media.
 */
static inline rgt_cbuf *
rgt_co_chunk_yield_mem(rgt_co_chunk *c, size_t *plen)
{
    assert(rgt_co_chunk_valid(c));
    assert(rgt_co_chunk_finished(c));
    assert(rgt_co_chunk_is_mem(c));
    return rgt_co_strg_yield_mem(&c->strg, plen);
}

/**
 * Append a memory chunk to a chunk.
 *
 * @param c     The chunk to append to.
 * @param ptr   The memory chunk pointer to append.
 * @param len   The memory chunk length to append.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
static inline te_bool
rgt_co_chunk_append(rgt_co_chunk *c, const void *ptr, size_t len)
{
    assert(rgt_co_chunk_valid(c));
    assert(!rgt_co_chunk_finished(c));
    assert(ptr != NULL || len == 0);
    return rgt_co_strg_append(&c->strg, ptr, len);
}

/**
 * Merge two chunks.
 *
 * @alg The destination chunk storage contents gets appended to the source
 *      chunk storage, the destination chunk storage media is freed and the
 *      source chunk storage media is moved to the destination chunk (the
 *      source storage becomes void).
 *
 * @param dst   The destination chunk.
 * @param src   The source chunk.
 *
 * @return TRUE if merged successfully, FALSE otherwise.
 */
static inline te_bool
rgt_co_chunk_merge(rgt_co_chunk *dst, rgt_co_chunk *src)
{
    assert(rgt_co_chunk_valid(dst));
    assert(rgt_co_chunk_valid(src));

    return rgt_co_strg_move_media(&dst->strg, &src->strg);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_RGT_CO_CHUNK_H__ */
