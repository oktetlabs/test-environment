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

#include "rgt_co_chunk.h"

te_bool
rgt_co_chunk_valid(const rgt_co_chunk *c)
{
    return c != NULL &&
           rgt_co_strg_valid(&c->strg);
}

rgt_co_chunk *
rgt_co_chunk_init(rgt_co_chunk *c, size_t depth)
{
    assert(c != NULL);

    rgt_co_strg_init(&c->strg);
    c->depth = depth;
    c->finished = FALSE;

    return (rgt_co_chunk *)rgt_co_chunk_validate(c);
}

te_bool
rgt_co_chunk_clnp(rgt_co_chunk *c)
{
    assert(rgt_co_chunk_valid(c));

    return rgt_co_strg_clnp(&c->strg);
}
