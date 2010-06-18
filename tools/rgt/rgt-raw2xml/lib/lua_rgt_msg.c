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

#include <errno.h>
#include <string.h>
#include <lualib.h>
#include <lauxlib.h>
#include "lua_rgt_msg.h"

static int
l_is_tester_control(lua_State *L)
{
    rgt_msg   **pmsg    = luaL_checkudata(L, 1, LUA_RGT_MSG_NAME);

    lua_pushboolean(L, rgt_msg_is_tester_control(*pmsg));

    return 1;
}


static int
l_is_control(lua_State *L)
{
    rgt_msg   **pmsg    = luaL_checkudata(L, 1, LUA_RGT_MSG_NAME);

    lua_pushboolean(L, rgt_msg_is_control(*pmsg));

    return 1;
}


static int
l_parse_tester_control(lua_State *L)
{
    rgt_msg   **pmsg    = luaL_checkudata(L, 1, LUA_RGT_MSG_NAME);

    (void)pmsg;

    return 0;
}


static const luaL_Reg lib[] = {
  {"is_tester_control",     l_is_tester_control},
  {"is_control",            l_is_control},
  {"parse_tester_control",  l_parse_tester_control},
  {NULL, NULL}
};

int luaopen_rgt_msg(lua_State *L)
{
    /* Register the library */
    luaL_register(L, LUA_RGT_MSG_NAME, lib);

    /* Register the library table as the userdata metatable */
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, LUA_RGT_MSG_NAME);

    /* Make indexing userdata index metatable instead */
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    return 1;
}
