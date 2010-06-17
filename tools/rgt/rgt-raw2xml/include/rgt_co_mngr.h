/** @file
 * @brief Test Environment: RGT chunked output - manager.
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
#include "rgt_co_chunk.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Chunked output manager */
typedef struct rgt_co_mngr {
    size_t          max_mem;    /**< Maximum memory for chunk contents */
    size_t          used_mem;   /**< Memory used by chunk contents */
    rgt_co_chunk   *first;      /**< First "live" chunk */
    rgt_co_chunk   *free;       /**< First free chunk */
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
 * Add a new (void) chunk to the chain.
 *
 * @param mngr  Manager.
 * @param prev  The chunk after which the new chunk should be added, or
 *              NULL, if the new chunk should be first.
 * @param depth The new chunk depth.
 *
 * @return New chunk.
 */
extern rgt_co_chunk *rgt_co_mngr_add_chunk(rgt_co_mngr     *mngr,
                                           rgt_co_chunk    *prev,
                                           size_t           depth);

/**
 * Delete a chunk from the chain.
 *
 * @param mngr  Manager.
 * @param prev  The chunk after which the chunk being deleted is located, or
 *              NULL if the first chunk is deleted.
 */
extern void rgt_co_mngr_del_chunk(rgt_co_mngr  *mngr,
                                  rgt_co_chunk *prev);

/**
 * Append a memory chunk to a chunk contents.
 *
 * @param mngr  Manager.
 * @param chunk Chunk to append to.
 * @param pgr   Memory chunk pointer.
 * @param len   Memory chunk length.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
extern te_bool rgt_co_mngr_chunk_append(rgt_co_mngr    *mngr,
                                        rgt_co_chunk   *chunk,
                                        const void     *ptr,
                                        size_t          len);

/**
 * Append a string to a memory chunk.
 *
 * @param mngr  Manager.
 * @param chunk Chunk to append to.
 * @param str   String to append.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
static inline te_bool
rgt_co_mngr_chunk_append_str(rgt_co_mngr    *mngr,
                             rgt_co_chunk   *chunk,
                             const char     *str)
{
    assert(rgt_co_mngr_valid(mngr));
    assert(rgt_co_chunk_valid(chunk));
    assert(str != NULL);
    return rgt_co_mngr_chunk_append(mngr, chunk, str, strlen(str));
}

/**
 * Append a literal to a memory chunk.
 *
 * @param _mngr     Manager.
 * @param _chunk    Chunk to append to.
 * @param _literal  Literal string to append.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
#define rgt_co_mngr_chunk_append_literal(_mngr, _chunk, _literal) \
    rgt_co_mngr_chunk_append(_mngr, _chunk, _literal, sizeof(_literal) - 1)

/**
 * Append a formatted string to a chunk.
 *
 * @param mngr  The chunk manager.
 * @param chunk The chunk to append to.
 * @param fmt   Format string.
 * @param ...   Format arguments.
 *
 * @note Uses stack space for output buffer.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
extern te_bool rgt_co_mngr_chunk_appendf(rgt_co_mngr   *mngr,
                                         rgt_co_chunk  *chunk,
                                         const char    *fmt,
                                         ...);

/**
 * Append a formatted string to a chunk, va_list version.
 *
 * @param mngr  The chunk manager.
 * @param chunk The chunk to append to.
 * @param fmt   Format string.
 * @param ap    Format arguments.
 *
 * @note Uses stack space for output buffer.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
extern te_bool rgt_co_mngr_chunk_appendvf(rgt_co_mngr  *mngr,
                                          rgt_co_chunk *chunk,
                                          const char   *fmt,
                                          va_list      ap);

/**
 * Append a single character to a chunk.
 *
 * @param chunk The chunk to append to.
 * @param c     The character to append.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
static inline te_bool
rgt_co_mngr_chunk_append_char(rgt_co_mngr  *mngr,
                              rgt_co_chunk *chunk,
                              char          c)
{
    assert(rgt_co_mngr_valid(mngr));
    assert(rgt_co_chunk_valid(chunk));
    return rgt_co_mngr_chunk_append(mngr, chunk, &c, sizeof(c));
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
extern te_bool rgt_co_mngr_chunk_append_span(rgt_co_mngr   *mngr,
                                             rgt_co_chunk  *chunk,
                                             char           c,
                                             size_t         n);

/** XML attribute */
typedef struct rgt_co_mngr_attr {
    const char *name;       /**< Name */
    void       *value_ptr;  /**< Value pointer */
    size_t      value_len;  /**< Value length */
} rgt_co_mngr_attr;

/**
 * Append an XML start tag.
 *
 * @param mngr      Manager.
 * @param chunk     The chunk to append to.
 * @param name      XML element name.
 * @param attr_list Attribute list, terminated by an attribute with NULL name.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
extern te_bool rgt_co_mngr_chunk_append_start_tag(
                                        rgt_co_mngr            *mngr,
                                        rgt_co_chunk           *chunk,
                                        const char             *name,
                                        const rgt_co_mngr_attr *attr_list);

/**
 * Append an XML element CDATA content.
 *
 * @param mngr  Manager.
 * @param chunk The chunk to append to.
 * @param ptr   The content pointer.
 * @param len   The content length.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
extern te_bool rgt_co_mngr_chunk_append_cdata(rgt_co_mngr  *mngr,
                                              rgt_co_chunk *chunk,
                                              const void   *ptr,
                                              size_t        len);
/**
 * Append an XML end tag.
 *
 * @param mngr  Manager.
 * @param chunk The chunk to append to.
 * @param name  XML element name.
 *
 * @return TRUE if appended successfully, FALSE otherwise.
 */
extern te_bool rgt_co_mngr_chunk_append_end_tag(rgt_co_mngr   *mngr,
                                                rgt_co_chunk  *chunk,
                                                const char    *name);

/**
 * Append an XML element.
 *
 * @param mngr          Manager.
 * @param chunk         The chunk to append to.
 * @param name          XML element name.
 * @param attr_list     Attribute list, terminated by an attribute with NULL name.
 * @param content_ptr   Content pointer.
 * @param content_len   Content length.
 */
extern te_bool rgt_co_mngr_chunk_append_element(
                                    rgt_co_mngr            *mngr,
                                    rgt_co_chunk           *chunk,
                                    const char             *name,
                                    const rgt_co_mngr_attr *attr_list,
                                    const void             *content_ptr,
                                    size_t                  content_len);

/**
 * Finish a chunk.
 *
 * @param mngr  Manager.
 * @param chunk The chunk to finish.
 *
 * @return TRUE if finished successfully, FALSE otherwise.
 */
extern te_bool rgt_co_mngr_chunk_finish(rgt_co_mngr    *mngr,
                                        rgt_co_chunk   *chunk);

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
 * Cleanup a manager, removing all the chunks.
 *
 * @param mngr  The manager to cleanup.
 *
 * @return TRUE if cleaned up successfully, FALSE if a file chunk failed to
 *         close (flush).
 */
extern te_bool rgt_co_mngr_clnp(rgt_co_mngr *mngr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_RGT_CO_H__ */
