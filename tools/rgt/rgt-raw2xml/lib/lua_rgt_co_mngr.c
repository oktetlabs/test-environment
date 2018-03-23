/** @file 
 * @brief Test Environment: RGT chunked output manager Lua interface.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
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
    const char     *tmp_dir;
    lua_Integer     max_mem;

    luaL_checktype(L, 1, LUA_TTABLE);
    tmp_dir = luaL_optstring(L, 2, "/tmp");
    max_mem = luaL_optinteger(L, 3, 0);

    if (rgt_co_mngr_init(lua_newuserdata(L, sizeof(rgt_co_mngr)),
                         tmp_dir, max_mem) == NULL)
        luaL_error(L, "memory allocation failed");
    lua_pushvalue(L, 1);
    lua_setmetatable(L, -2);

    return 1;
}


static int
l_take_file(lua_State *L)
{
    rgt_co_mngr    *mngr    = luaL_checkudata(L, 1, LUA_RGT_CO_MNGR_NAME);
    FILE          **pfile   = luaL_checkudata(L, 2, LUA_FILEHANDLE);
    rgt_co_chunk   *chunk;
    rgt_co_strg     strg    = RGT_CO_STRG_VOID;

    luaL_argcheck(L, *pfile != NULL, 2, "closed");

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
    rgt_co_chunk_take(chunk, rgt_co_strg_take_file(&strg, *pfile, 0));

    lua_rgt_co_chunk_wrap(L, 1, chunk);

    return 1;
}


static int
l_yield_file(lua_State *L)
{
    rgt_co_mngr    *mngr    = luaL_checkudata(L, 1, LUA_RGT_CO_MNGR_NAME);
    size_t          len;
    rgt_co_strg     strg    = RGT_CO_STRG_VOID;

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
    rgt_co_strg_yield_file(rgt_co_chunk_yield(&strg, mngr->first_used),
                           &len);
    lua_pushinteger(L, len);

    return 2;
}


static int
l___gc(lua_State *L)
{
    rgt_co_mngr    *mngr    = luaL_checkudata(L, 1, LUA_RGT_CO_MNGR_NAME);
    rgt_co_strg     strg    = RGT_CO_STRG_VOID;
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
            rgt_co_chunk_yield(&strg, mngr->first_used);
        }
    }
    rgt_co_mngr_clnp(mngr);
    return 0;
}


static int
l_dump(lua_State *L)
{
    rgt_co_mngr    *mngr    = luaL_checkudata(L, 1, LUA_RGT_CO_MNGR_NAME);
    FILE          **pfile   = luaL_checkudata(L, 2, LUA_FILEHANDLE);

    luaL_argcheck(L, *pfile != NULL, 2, "closed");

    if (!rgt_co_mngr_dump(mngr, *pfile))
        return luaL_error(L, "Failed dumping a manager: %s",
                          strerror(errno));

    lua_settop(L, 1);
    return 1;
}


static const luaL_Reg lib[] = {
  {"__gc",          l___gc},
  {"take_file",     l_take_file},
  {"yield_file",    l_yield_file},
  {"dump",          l_dump},
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


