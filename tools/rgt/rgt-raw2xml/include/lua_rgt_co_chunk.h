/** @file 
 * @brief Test Environment: RGT chunked output chunk Lua interface.
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __LUA_RGT_CO_CHUNK_H__
#define __LUA_RGT_CO_CHUNK_H__

#include "rgt_co.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LUA_RGT_CO_CHUNK_NAME "rgt.co.chunk"

extern int lua_rgt_co_chunk_wrap(lua_State     *L,
                                 int            mngr_idx,
                                 rgt_co_chunk  *chunk);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__LUA_RGT_CO_CHUNK_H__ */
