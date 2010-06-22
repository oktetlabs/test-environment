/** @file 
 * @brief Test Environment: RGT chunked buffer Lua interface.
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
#include "lua_rgt_cbuf.h"

static int
l___call(lua_State *L)
{
    lua_Integer size;

    luaL_checktype(L, 1, LUA_TTABLE);
    size = luaL_optinteger(L, 2, 0);

    if (rgt_cbuf_init(lua_newuserdata(L, sizeof(rgt_cbuf)), size) == NULL)
        return luaL_error(L, "memory allocation failed");
    lua_pushvalue(L, 1);
    lua_setmetatable(L, -2);

    return 1;
}


static int
l_append(lua_State *L)
{
    rgt_cbuf   *b       = luaL_checkudata(L, 1, LUA_RGT_CBUF_NAME);
    size_t      len;
    const char *str     = luaL_checklstring(L, 2, &len);

    if (!rgt_cbuf_append(b, str, len))
        return luaL_error(L, "memory allocation failed");

    lua_settop(L, 1);
    return 1;
}


static int
l___tostring(lua_State *L)
{
    rgt_cbuf       *b   = luaL_checkudata(L, 1, LUA_RGT_CBUF_NAME);
    luaL_Buffer     s;
    rgt_cbuf_chunk *c;

    luaL_buffinit(L, &s);

    for (c = b->first; c != NULL; c = c->next)
        luaL_addlstring(&s, c->buf, c->len);

    luaL_pushresult(&s);
    return 1;
}


static int
l___len(lua_State *L)
{
    rgt_cbuf   *b       = luaL_checkudata(L, 1, LUA_RGT_CBUF_NAME);

    lua_pushinteger(L, rgt_cbuf_get_len(b));

    return 1;
}


static int
l_merge(lua_State *L)
{
    rgt_cbuf   *x   = luaL_checkudata(L, 1, LUA_RGT_CBUF_NAME);
    rgt_cbuf   *y   = luaL_checkudata(L, 2, LUA_RGT_CBUF_NAME);

    if (!rgt_cbuf_merge(x, y))
        return luaL_error(L, "memory allocation failed");

    lua_settop(L, 1);
    return 1;
}


static int
l_readin(lua_State *L)
{
    rgt_cbuf   *b       = luaL_checkudata(L, 1, LUA_RGT_CBUF_NAME);
    FILE      **pf      = luaL_checkudata(L, 2, LUA_FILEHANDLE);
    size_t      read;

    luaL_argcheck(L, (*pf != NULL), 2, "closed file");

    clearerr(*pf);
    read = rgt_cbuf_readin(b, *pf);
    if (ferror(*pf))
        return luaL_error(L, "failed reading a file into the buffer: %s",
                          strerror(errno));

    lua_pushinteger(L, read);
    return 1;
}


static int
l_writeout(lua_State *L)
{
    rgt_cbuf   *b   = luaL_checkudata(L, 1, LUA_RGT_CBUF_NAME);
    FILE      **pf  = luaL_checkudata(L, 2, LUA_FILEHANDLE);

    luaL_argcheck(L, (*pf != NULL), 2, "closed file");

    if (!rgt_cbuf_writeout(b, *pf))
        return luaL_error(L, "failed writing the buffer to a file: %s",
                          strerror(errno));

    lua_settop(L, 1);
    return 1;
}


static int
l_retention(lua_State *L)
{
    rgt_cbuf   *b   = luaL_checkudata(L, 1, LUA_RGT_CBUF_NAME);

    if (!rgt_cbuf_retention(b))
        return luaL_error(L, "memory allocation failed");

    lua_settop(L, 1);
    return 1;
}


static int
l_clear(lua_State *L)
{
    rgt_cbuf_clear(luaL_checkudata(L, 1, LUA_RGT_CBUF_NAME));
    lua_settop(L, 1);
    return 1;
}


static const luaL_Reg lib[] = {
  {"__tostring",    l___tostring},
  {"__len",         l___len},
  {"append",        l_append},
  {"merge",         l_merge},
  {"readin",        l_readin},
  {"writeout",      l_writeout},
  {"retention",     l_retention},
  {"clear",         l_clear},
  {"__gc",          l_clear},
  {NULL, NULL}
};

int
luaopen_rgt_cbuf(lua_State *L)
{
    /* Register the library */
    luaL_register(L, LUA_RGT_CBUF_NAME, lib);

    /* Create and assign library metatable */
    lua_newtable(L);
    lua_pushcfunction(L, l___call);
    lua_setfield(L, -2, "__call");
    lua_setmetatable(L, -2);

    /* Register the library table as the userdata metatable */
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, LUA_RGT_CBUF_NAME);

    /* Make indexing userdata index metatable instead */
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    return 1;
}

