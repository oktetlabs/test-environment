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

#include "rgt_co_chunk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rgt_co_mngr {
    size_t          max_mem;
    size_t          used_mem;
    rgt_co_chunk   *first;
    rgt_co_chunk   *free;
} rgt_co_mngr;

extern rgt_co_mngr *rgt_co_mngr_init(rgt_co_mngr *mngr, size_t max_mem);

extern rgt_co_chunk *rgt_co_mngr_add_chunk(rgt_co_mngr     *mngr,
                                           rgt_co_chunk    *prev,
                                           size_t           depth);

extern void rgt_co_mngr_del_chunk(rgt_co_mngr  *mngr,
                                  rgt_co_chunk *prev,
                                  rgt_co_chunk *chunk);

extern te_bool rgt_co_mngr_chunk_append(rgt_co_mngr    *mngr,
                                        rgt_co_chunk   *chunk,
                                        const void     *ptr,
                                        size_t          len);

extern te_bool rgt_co_mngr_chunk_finish(rgt_co_mngr    *mngr,
                                        rgt_co_chunk   *chunk);

extern te_bool rgt_co_mngr_finished(const rgt_co_mngr *mngr);

extern te_bool rgt_co_mngr_clnp(rgt_co_mngr *mngr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_RGT_CO_H__ */
