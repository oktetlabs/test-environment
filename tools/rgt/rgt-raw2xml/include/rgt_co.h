/** @file
 * @brief Test Environment: RGT chunked output.
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

#ifndef __TE_RGT_CO_H__
#define __TE_RGT_CO_H__

#include <string.h>
#include "rgt_co_strg.h"
#include "rgt_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************
 * MANAGER
 **********************************************************/
/** Forward declaration of the chunk */
typedef struct rgt_co_chunk rgt_co_chunk;

/** Chunked output manager */
typedef struct rgt_co_mngr {
    size_t          max_mem;    /**< Maximum memory for contents */
    size_t          used_mem;   /**< Memory used by chunk contents */
    rgt_co_chunk   *first_used; /**< First "used" chunk */
    rgt_co_chunk   *first_free; /**< First "free" chunk */
} rgt_co_mngr;

/**
 * Check if a manager is valid.
 *
 * @param mngr  Manager to check.
 *
 * @return TRUE if the manager is valid, FALSE otherwise.
 */
extern te_bool rgt_co_mngr_valid(const rgt_co_mngr *mngr);

/**
 * Initialize a manager.
 *
 * @param mngr      Manager to initialize.
 * @param max_mem   Maximium memory allowed for chunk contents.
 *
 * @return Initialized manager.
 */
extern rgt_co_mngr *rgt_co_mngr_init(rgt_co_mngr *mngr, size_t max_mem);

/**
 * Add a new first chunk to the chain.
 *
 * @param mngr  The manager.
 * @param depth The new chunk depth.
 *
 * @return New first chunk.
 */
extern rgt_co_chunk *rgt_co_mngr_add_first_chunk(rgt_co_mngr  *mngr,
                                                 size_t        depth);

/**
 * Delete first chunk from the chain.
 *
 * @param mngr  The manager to delete first chunk from.
 */
extern void rgt_co_mngr_del_first_chunk(rgt_co_mngr *mngr);
            
/**
 * Add a new (void) chunk to the chain.
 *
 * @param prev  The chunk after which the new chunk should be added, or
 *              NULL, if the new chunk should be first.
 * @param depth The new chunk depth.
 *
 * @return New chunk.
 */
extern rgt_co_chunk *rgt_co_mngr_add_chunk(rgt_co_chunk  *prev,
                                           size_t         depth);

/**
 * Delete a chunk from the chain.
 *
 * @param prev  The chunk after which the chunk being deleted is located, or
 *              NULL if the first chunk is deleted.
 */
extern void rgt_co_mngr_del_chunk(rgt_co_chunk *prev);

/**
 * Check if a manager is finished, in effect if it contains only one,
 * finished chunk.
 *
 * @param mngr  The manager to check.
 *
 * @return TRUE if the manager is finished, false otherwise.
 */
extern te_bool rgt_co_mngr_finished(const rgt_co_mngr *mngr);

/**
 * Cleanup a manager, removing all the chunks and ignoring any file closing
 * errors.
 *
 * @param mngr  The manager to cleanup.
 */
extern void rgt_co_mngr_clnp(rgt_co_mngr *mngr);

/**********************************************************
 * CHUNK
 **********************************************************/
/** Chunk */
struct rgt_co_chunk {
    rgt_co_chunk   *next;       /**< Next chunk */
    rgt_co_mngr    *mngr;       /**< Manager */
    rgt_co_strg     strg;       /**< Storage */
    size_t          depth;      /**< Nesting depth */
    te_bool         finished;   /**< "Finished" flag */
};

/**
 * Check if a chunk is valid.
 *
 * @param chunk    The chunk to check.
 *
 * @return TRUE if the chunk is valid, FALSE otherwise.
 */
extern te_bool rgt_co_chunk_valid(const rgt_co_chunk *chunk);

/**
 * Validate a chunk.
 *
 * @param chunk    The chunk to validate.
 *
 * @return The validate chunk.
 */
static inline const rgt_co_chunk *
rgt_co_chunk_validate(const rgt_co_chunk *chunk)
{
    assert(rgt_co_chunk_valid(chunk));
    return chunk;
}

/**
 * Check if a chunk is finished.
 *
 * @param chunk The chunk to check.
 *
 * @return TRUE if the chunk is finished, FALSE otherwise.
 */
static inline te_bool
rgt_co_chunk_finished(const rgt_co_chunk *chunk)
{
    assert(rgt_co_chunk_valid(chunk));
    return chunk->finished;
}

/**
 * Supply a chunk with a file as a storage media.
 *
 * @param chunk The chunk to supply with the file.
 * @param file  The file to supply with.
 * @param len   The file contents length.
 *
 * @return The chunk using the supplied media.
 */
static inline rgt_co_chunk *
rgt_co_chunk_take_file(rgt_co_chunk *chunk, FILE *file, size_t len)
{
    assert(rgt_co_chunk_valid(chunk));
    assert(rgt_co_strg_is_void(&chunk->strg));
    assert(file != NULL);
    rgt_co_strg_take_file(&chunk->strg, file, len);
    return chunk;
}

/**
 * Supply a chunk with a buffer as a storage media.
 *
 * @param chunk The chunk to supply with the buffer.
 * @param mem   The buffer to supply with.
 * @param len   The buffer contents length.
 *
 * @return The chunk using the supplied media.
 */
static inline rgt_co_chunk *
rgt_co_chunk_take_mem(rgt_co_chunk *chunk, rgt_cbuf *mem, size_t len)
{
    assert(rgt_co_chunk_valid(chunk));
    assert(rgt_co_strg_is_void(&chunk->strg));
    assert(rgt_cbuf_valid(mem));
    rgt_co_strg_take_mem(&chunk->strg, mem, len);
    return chunk;
}

/**
 * Check if a chunk storage is void.
 *
 * @param chunk The chunk to check.
 *
 * @return TRUE if the chunk storage is void, FALSE otherwise.
 */
static inline te_bool
rgt_co_chunk_is_void(const rgt_co_chunk *chunk)
{
    assert(rgt_co_chunk_valid(chunk));
    return rgt_co_strg_is_void(&chunk->strg);
}

/**
 * Check if a chunk storage uses file media.
 *
 * @param chunk The chunk to check.
 *
 * @return TRUE if the chunk media is a file, FALSE otherwise.
 */
static inline te_bool
rgt_co_chunk_is_file(const rgt_co_chunk *chunk)
{
    assert(rgt_co_chunk_valid(chunk));
    return rgt_co_strg_is_file(&chunk->strg);
}

/**
 * Check if a chunk storage uses buffer media.
 *
 * @param chunk The chunk to check.
 *
 * @return TRUE if the chunk media is a buffer, FALSE otherwise.
 */
static inline te_bool
rgt_co_chunk_is_mem(const rgt_co_chunk *chunk)
{
    assert(rgt_co_chunk_valid(chunk));
    return rgt_co_strg_is_mem(&chunk->strg);
}

/**
 * Relocate chunk storage contents to another (file) media.
 *
 * @param chunk The chunk to relocate.
 * @param file  The file to relocate to.
 * @param len   The file contents length.
 *
 * @return The chunk relocated to the specified media.
 */
static inline te_bool
rgt_co_chunk_relocate_to_file(rgt_co_chunk *chunk, FILE *file, size_t len)
{
    rgt_co_strg  strg;

    assert(rgt_co_chunk_valid(chunk));
    assert(file != NULL);

    return
        rgt_co_strg_move_media(
                &chunk->strg,
                rgt_co_strg_take_file(
                    rgt_co_strg_init(&strg), file, len));
}

/**
 * Relocate chunk storage contents to another (buffer) media.
 *
 * @param chunk The chunk to relocate.
 * @param mem   The buffer to relocate to.
 * @param len   The buffer contents length.
 *
 * @return The chunk relocated to the specified media.
 */
static inline te_bool
rgt_co_chunk_relocate_to_mem(rgt_co_chunk *chunk, rgt_cbuf *mem, size_t len)
{
    rgt_co_strg  strg;

    assert(rgt_co_chunk_valid(chunk));
    assert(rgt_cbuf_valid(mem));

    return
        rgt_co_strg_move_media(
                &chunk->strg,
                rgt_co_strg_take_mem(
                    rgt_co_strg_init(&strg), mem, len));
}

/**
 * Take the file media from a finished chunk; the chunk storage becomes
 * void.
 *
 * @param chunk The chunk to retrieve the media from.
 * @param plen  Location for the contents length, or NULL.
 *
 * @return The chunk file media.
 */
static inline FILE *
rgt_co_chunk_yield_file(rgt_co_chunk *chunk, size_t *plen)
{
    assert(rgt_co_chunk_valid(chunk));
    assert(rgt_co_chunk_finished(chunk));
    assert(rgt_co_chunk_is_file(chunk));
    return rgt_co_strg_yield_file(&chunk->strg, plen);
}

/**
 * Take the buffer media from a finished chunk; the chunk storage becomes
 * void.
 *
 * @param chunk The chunk to retrieve the media from.
 * @param plen  Location for the contents length, or NULL.
 *
 * @return The chunk buffer media.
 */
static inline rgt_cbuf *
rgt_co_chunk_yield_mem(rgt_co_chunk *chunk, size_t *plen)
{
    assert(rgt_co_chunk_valid(chunk));
    assert(rgt_co_chunk_finished(chunk));
    assert(rgt_co_chunk_is_mem(chunk));
    return rgt_co_strg_yield_mem(&chunk->strg, plen);
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

/**
 * Increase chunk nesting depth.
 *
 * @param chunk The chunk to descend.
 */
static inline void
rgt_co_chunk_descend(rgt_co_chunk *chunk)
{
    assert(rgt_co_chunk_valid(chunk));
    chunk->depth++;
}

/**
 * Decrease chunk nesting depth.
 *
 * @param chunk The chunk to ascend.
 */
static inline void
rgt_co_chunk_ascend(rgt_co_chunk *chunk)
{
    assert(rgt_co_chunk_valid(chunk));
    assert(chunk->depth > 0);
    chunk->depth--;
}

/**
 * Append a memory chunk to a chunk contents.
 *
 * @param chunk Managed chunk to append to.
 * @param pgr   Memory chunk pointer.
 * @param len   Memory chunk length.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
extern te_bool rgt_co_chunk_append(rgt_co_chunk  *chunk,
                                   const void    *ptr,
                                   size_t         len);

/**
 * Append a string to a chunk.
 *
 * @param chunk Managed chunk to append to.
 * @param str   String to append.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
static inline te_bool
rgt_co_chunk_append_str(rgt_co_chunk *chunk,
                        const char   *str)
{
    assert(rgt_co_chunk_valid(chunk));
    assert(str != NULL);
    return rgt_co_chunk_append(chunk, str, strlen(str));
}

/**
 * Append a literal to a chunk.
 *
 * @param _chunk    Managed chunk to append to.
 * @param _literal  Literal string to append.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
#define rgt_co_chunk_append_literal(_chunk, _literal) \
    rgt_co_chunk_append(_chunk, _literal, sizeof(_literal) - 1)

/**
 * Append a formatted string to a chunk.
 *
 * @param chunk The chunk to append to.
 * @param fmt   Format string.
 * @param ...   Format arguments.
 *
 * @note Uses stack space for output buffer.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
extern te_bool rgt_co_chunk_appendf(rgt_co_chunk   *chunk,
                                    const char     *fmt,
                                    ...);

/**
 * Append a formatted string to a chunk, va_list version.
 *
 * @param chunk The chunk to append to.
 * @param fmt   Format string.
 * @param ap    Format arguments.
 *
 * @note Uses stack space for output buffer.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
extern te_bool rgt_co_chunk_appendvf(rgt_co_chunk    *chunk,
                                     const char      *fmt,
                                     va_list          ap);

/**
 * Append a single character to a chunk.
 *
 * @param chunk The chunk to append to.
 * @param c     The character to append.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
static inline te_bool
rgt_co_chunk_append_char(rgt_co_chunk    *chunk,
                         char             c)
{
    assert(rgt_co_chunk_valid(chunk));
    return rgt_co_chunk_append(chunk, &c, sizeof(c));
}

/**
 * Append a repeated character to a chunk.
 *
 * @param chunk The chunk to append to.
 * @param c     The character to append.
 * @param n     Number of times the character should be repeated.
 *
 * @note Uses stack space for output buffer.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
extern te_bool rgt_co_chunk_append_span(rgt_co_chunk   *chunk,
                                        char            c,
                                        size_t          n);

/**
 * Finish a chunk.
 *
 * @param chunk The chunk to finish.
 *
 * @return TRUE if finished successfully, FALSE otherwise.
 */
extern te_bool rgt_co_chunk_finish(rgt_co_chunk *chunk);

/**********************************************************
 * XML CHUNK
 **********************************************************/
/** XML attribute */
typedef struct rgt_co_chunk_attr {
    const char *name;       /**< Name */
    void       *value_ptr;  /**< Value pointer */
    size_t      value_len;  /**< Value length */
} rgt_co_chunk_attr;

/**
 * Append an XML start tag to a chunk.
 *
 * @param chunk     The chunk to append to.
 * @param name      XML element name.
 * @param attr_list Attribute list, terminated by an attribute with NULL name.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
extern te_bool rgt_co_chunk_append_start_tag(
                                    rgt_co_chunk               *chunk,
                                    const char                 *name,
                                    const rgt_co_chunk_attr    *attr_list);

/**
 * Append an XML element CDATA content to a chunk.
 *
 * @param chunk The chunk to append to.
 * @param ptr   The content pointer.
 * @param len   The content length.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
extern te_bool rgt_co_chunk_append_cdata(rgt_co_chunk  *chunk,
                                         const void    *ptr,
                                         size_t         len);

/**
 * Append an XML end tag to a chunk.
 *
 * @param chunk The chunk to append to.
 * @param name  XML element name.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
extern te_bool rgt_co_chunk_append_end_tag(rgt_co_chunk    *chunk,
                                           const char      *name);

/**
 * Append an XML element to a chunk.
 *
 * @param chunk         The chunk to append to.
 * @param name          XML element name.
 * @param attr_list     Attribute list, terminated by an attribute with NULL name.
 * @param content_ptr   Content pointer.
 * @param content_len   Content length.
 */
extern te_bool rgt_co_chunk_append_element(
                                rgt_co_chunk               *chunk,
                                const char                 *name,
                                const rgt_co_chunk_attr    *attr_list,
                                const void                 *content_ptr,
                                size_t                      content_len);

/**********************************************************
 * MSG CHUNK
 **********************************************************/
/**
 * Append a message XML element to a chunk.
 *
 * @param chunk The chunk to append to.
 * @param msg   The message to append.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
extern te_bool rgt_co_chunk_append_msg(rgt_co_chunk    *chunk,
                                       const rgt_msg   *msg);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_RGT_CO_H__ */
