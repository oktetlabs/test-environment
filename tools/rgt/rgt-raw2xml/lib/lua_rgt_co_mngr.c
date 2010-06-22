/** @file 
 * @brief Test Environment: RGT chunked output manager Lua interface.
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

#include <errno.h>
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "lua_rgt_co_chunk.h"
#include "lua_rgt_co_mngr.h"

static int
l___call(lua_State *L)
{
    lua_Integer max_mem;

    luaL_checktype(L, 1, LUA_TTABLE);
    max_mem = luaL_optinteger(L, 2, 0);

    if (rgt_co_mngr_init(lua_newuserdata(L, sizeof(rgt_co_mngr)),
                         max_mem) == NULL)
        luaL_error(L, "memory allocation failed");
    lua_pushvalue(L, 1);
    lua_setmetatable(L, -2);

    return 1;
}


static int
l_take_file(lua_State *L)
{
    rgt_co_mngr    *mngr    = luaL_checkudata(L, 1, LUA_RGT_CO_MNGR_NAME);
    FILE           *file    = luaL_checkudata(L, 2, LUA_FILEHANDLE);
    rgt_co_chunk   *chunk;

    /*
     * Store the file in the environment to prevent garbage collection
     */
    lua_getfenv(L, 1);
    if (!lua_isnil(L, -1))
    {
        lua_getfield(L, -1, "file");
        luaL_argcheck(L, lua_isnil(L, -1), 1, "has file already");
        lua_pop(L, 1);
    }
    else
    {
        lua_pop(L, 1);
        lua_newtable(L);
    }
    lua_pushvalue(L, 2);
    lua_setfield(L, -2, "file");
    lua_setfenv(L, 1);

    /*
     * Create the first chunk and supply it with the file
     */
    chunk = rgt_co_mngr_add_first_chunk(mngr, 0);
    if (chunk == NULL)
        luaL_error(L, "memory allocation failed");
    rgt_co_chunk_take_file(chunk, file, 0);

    lua_rgt_co_chunk_wrap(L, 1, chunk);

    return 1;
}


static int
l_yield_file(lua_State *L)
{
    rgt_co_mngr    *mngr    = luaL_checkudata(L, 1, LUA_RGT_CO_MNGR_NAME);
    size_t          len;

    luaL_argcheck(L, rgt_co_mngr_finished(mngr), 1, "not finished");

    /*
     * Retrieve/remove the file from the environment
     */
    lua_getfenv(L, 1);
    luaL_argcheck(L, !lua_isnil(L, -1), 1, "no environment");
    lua_getfield(L, -1, "file");
    luaL_argcheck(L, !lua_isnil(L, -1), 1, "no file in the environment");
    lua_pushnil(L);
    lua_setfield(L, -3, "file");

    /*
     * Take the file from the chunk
     */
    rgt_co_chunk_yield_file(mngr->first_used, &len);
    lua_pushinteger(L, len);

    return 2;
}


static int
l___gc(lua_State *L)
{
    rgt_co_mngr    *mngr    = luaL_checkudata(L, 1, LUA_RGT_CO_MNGR_NAME);
    /*
     * Remove the file from the first chunk, since it doesn't belong to us.
     */
    lua_getfenv(L, 1);
    if (!lua_isnil(L, -1))
    {
        lua_getfield(L, -1, "file");
        if (!lua_isnil(L, -1))
        {
            lua_pushnil(L);
            lua_setfield(L, -2, "file");
            rgt_co_chunk_yield_file(mngr->first_used, NULL);
        }
    }
    rgt_co_mngr_clnp(mngr);
    return 0;
}


static const luaL_Reg lib[] = {
  {"__gc",          l___gc},
  {"take_file",     l_take_file},
  {"yield_file",    l_yield_file},
  {NULL, NULL}
};

int
luaopen_rgt_co_mngr(lua_State *L)
{
    /* Register the library */
    luaL_register(L, LUA_RGT_CO_MNGR_NAME, lib);

    /* Create and assign library metatable */
    lua_newtable(L);
    lua_pushcfunction(L, l___call);
    lua_setfield(L, -2, "__call");
    lua_setmetatable(L, -2);

    /* Register the library table as the userdata metatable */
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, LUA_RGT_CO_MNGR_NAME);

    /* Make indexing userdata index metatable instead */
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    return 1;
}


