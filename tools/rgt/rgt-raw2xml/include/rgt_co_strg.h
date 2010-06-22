/** @file
 * @brief Test Environment: RGT chunked output - storage.
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

#ifndef __RGT_CO_STRG_H__
#define __RGT_CO_STRG_H__

#include <assert.h>
#include <stdio.h>
#include "te_defs.h"
#include "rgt_cbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Storage type */
typedef enum rgt_co_strg_type {
    RGT_CO_STRG_TYPE_VOID,  /**< Void */
    RGT_CO_STRG_TYPE_MEM,   /**< Memory */
    RGT_CO_STRG_TYPE_FILE   /**< File */
} rgt_co_strg_type;

/** Storage */
typedef struct rgt_co_strg rgt_co_strg;

struct rgt_co_strg {
    rgt_co_strg_type    type;
    union {
        rgt_cbuf       *mem;
        FILE           *file;
    } media;
    size_t              len;
};

extern te_bool rgt_co_strg_valid(const rgt_co_strg *strg);

static inline const rgt_co_strg *
rgt_co_strg_validate(const rgt_co_strg *strg)
{
    assert(rgt_co_strg_valid(strg));
    return strg;
}


extern rgt_co_strg *rgt_co_strg_init(rgt_co_strg *strg);

extern rgt_co_strg *rgt_co_strg_take_file(rgt_co_strg *strg,
                                          FILE *file, size_t len);

extern rgt_co_strg *rgt_co_strg_take_mem(rgt_co_strg *strg,
                                         rgt_cbuf *mem, size_t len);

static inline te_bool
rgt_co_strg_is_void(const rgt_co_strg *strg)
{
    assert(rgt_co_strg_valid(strg));
    return strg->type == RGT_CO_STRG_TYPE_VOID;
}


static inline te_bool
rgt_co_strg_is_mem(const rgt_co_strg *strg)
{
    assert(rgt_co_strg_valid(strg));
    return strg->type == RGT_CO_STRG_TYPE_MEM;
}


static inline te_bool
rgt_co_strg_is_file(const rgt_co_strg *strg)
{
    assert(rgt_co_strg_valid(strg));
    return strg->type == RGT_CO_STRG_TYPE_FILE;
}


static inline void
rgt_co_strg_void(rgt_co_strg *strg)
{
    strg->type = RGT_CO_STRG_TYPE_VOID;
    strg->len = 0;
}


static inline te_bool
rgt_co_strg_append(rgt_co_strg *strg, const void *ptr, size_t size)
{
    assert(rgt_co_strg_valid(strg));
    assert(!rgt_co_strg_is_void(strg));

    if (size != 0 &&
        !(rgt_co_strg_is_mem(strg)
            ? rgt_cbuf_append(strg->media.mem, ptr, size)
            : fwrite(ptr, size, 1, strg->media.file) == 1))
        return FALSE;

    strg->len += size;

    return TRUE;
}


extern rgt_cbuf *rgt_co_strg_yield_mem(rgt_co_strg *strg, size_t *plen);

extern FILE *rgt_co_strg_yield_file(rgt_co_strg *strg, size_t *plen);

extern te_bool rgt_co_strg_clnp(rgt_co_strg *strg);

/**
 * Move the media from one storage to another, having destination storage
 * media content appended to the moved media.
 *
 * @param dst   Destination storage; its media content will be appended to
 *              the source storage media, after that the media will be
 *              freed/closed.
 * @param src   Source storage; its media will be removed (the storage will
 *              become void).
 *
 * @return TRUE if moved successfully, FALSE otherwise (consult errno for
 *         details).
 *
 * To illustrate the process:
 *    src           dst    
 *  (<xxx>)       ([yyy])   - initial state
 * (<xxxyyy>)      ([])     - dst contents relocated to src
 * (<xxxyyy>)       ()      - dst media freed
 *     ()        (<xxxyyy>) - src media moved to dst
 *
 * Legend:
 *  ()  - storage
 *  []  - dst media
 *  <>  - src media
 *  xxx - src contents
 *  yyy - dst contents
 */
extern te_bool rgt_co_strg_move_media(rgt_co_strg *dst,
                                      rgt_co_strg *src);

static inline te_bool
rgt_co_strg_retention(rgt_co_strg *strg)
{
    assert(rgt_co_strg_valid(strg));
    return !rgt_co_strg_is_mem(strg) ||
           rgt_cbuf_retention(strg->media.mem);
}


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__RGT_CO_STRG_H__ */
