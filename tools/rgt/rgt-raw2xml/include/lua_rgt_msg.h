/** @file
 * @brief Test Environment: RGT message Lua interface.
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

#ifndef __LUA_RGT_MSG_H__
#define __LUA_RGT_MSG_H__

#include <lauxlib.h>
#include "rgt_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LUA_RGT_MSG_NAME "rgt.msg"

static inline void
lua_rgt_msg_wrap_idx(lua_State *L, int meta_idx, rgt_msg *msg)
{
    lua_newuserdata(L, sizeof(msg));
    lua_setmetatable(L, meta_idx);
}

static inline void
lua_rgt_msg_wrap(lua_State *L, rgt_msg *msg)
{
    lua_newuserdata(L, sizeof(msg));
    luaL_getmetatable(L, LUA_RGT_MSG_NAME);
    lua_setmetatable(L, -2);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__LUA_RGT_MSG_H__ */
