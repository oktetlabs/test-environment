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
#include <obstack.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "lua_rgt_co_chunk.h"
#include "lua_rgt_msg.h"

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free

/* Taken from lauxlib.c */
#define lua_abs_index(L, i) ((i) > 0 || (i) <= LUA_REGISTRYINDEX \
                                ? (i)                               \
                                : lua_gettop(L) + (i) + 1)

int
lua_rgt_co_chunk_wrap(lua_State *L, int mngr_idx, rgt_co_chunk *chunk)
{
    assert(L != NULL);
    assert(chunk != NULL);

    /* Convert mngr_idx to absolute value */
    mngr_idx = lua_abs_index(L, mngr_idx);

    /* Create rgt_co_chunk* user data with metatable */
    *(rgt_co_chunk **)lua_newuserdata(L, sizeof(chunk)) = chunk;
    luaL_getmetatable(L, LUA_RGT_CO_CHUNK_NAME);
    lua_setmetatable(L, -2);

    /* Create and assign environment table referencing the manager */
    lua_newtable(L);
    lua_pushliteral(L, "mngr");
    lua_pushvalue(L, mngr_idx);
    lua_rawset(L, -3);
    lua_setfenv(L, -2);

    return 1;
}


static int
l_finished(lua_State *L)
{
    rgt_co_chunk  **pchunk  = luaL_checkudata(L, 1, LUA_RGT_CO_CHUNK_NAME);
    lua_pushboolean(L, rgt_co_chunk_finished(*pchunk));
    return 1;
}


static int
l_is_void(lua_State *L)
{
    rgt_co_chunk  **pchunk  = luaL_checkudata(L, 1, LUA_RGT_CO_CHUNK_NAME);
    lua_pushboolean(L, rgt_co_chunk_is_void(*pchunk));
    return 1;
}


static int
l_is_file(lua_State *L)
{
    rgt_co_chunk  **pchunk  = luaL_checkudata(L, 1, LUA_RGT_CO_CHUNK_NAME);
    lua_pushboolean(L, rgt_co_chunk_is_file(*pchunk));
    return 1;
}


static int
l_is_mem(lua_State *L)
{
    rgt_co_chunk  **pchunk  = luaL_checkudata(L, 1, LUA_RGT_CO_CHUNK_NAME);
    lua_pushboolean(L, rgt_co_chunk_is_mem(*pchunk));
    return 1;
}


static int
l___len(lua_State *L)
{
    rgt_co_chunk  **pchunk  = luaL_checkudata(L, 1, LUA_RGT_CO_CHUNK_NAME);
    lua_pushinteger(L, rgt_co_chunk_get_len(*pchunk));
    return 1;
}


static int
l_fork(lua_State *L)
{
    rgt_co_chunk  **pchunk      = luaL_checkudata(L, 1,
                                                  LUA_RGT_CO_CHUNK_NAME);
    rgt_co_chunk   *chunk       = *pchunk;
    rgt_co_chunk   *new_chunk;
    rgt_cbuf       *cbuf;
    rgt_co_strg     strg        = RGT_CO_STRG_VOID;

    /* Retrieve manager reference from the chunk environment */
    lua_getfenv(L, 1);
    lua_getfield(L, -1, "mngr");

    /* Create new chunk with the same depth */
    new_chunk = rgt_co_mngr_add_chunk(chunk, chunk->depth);
    if (new_chunk == NULL)
        return luaL_error(L, "memory allocation failed");

    /* Create new chunked buffer */
    cbuf = rgt_cbuf_new(0);
    if (cbuf == NULL)
        return luaL_error(L, "memory allocation failed");

    /* Supply the new chunk with the chunked buffer */
    rgt_co_chunk_take(new_chunk, rgt_co_strg_take_mem(&strg, cbuf, 0));

    /* Wrap the new chunk and the manager into a userdata */
    return lua_rgt_co_chunk_wrap(L, -1, new_chunk);
}


static int
l_descend(lua_State *L)
{
    rgt_co_chunk  **pchunk  = luaL_checkudata(L, 1, LUA_RGT_CO_CHUNK_NAME);
    rgt_co_chunk_descend(*pchunk);
    lua_settop(L, 1);
    return 1;
}


static int
l_ascend(lua_State *L)
{
    rgt_co_chunk  **pchunk  = luaL_checkudata(L, 1, LUA_RGT_CO_CHUNK_NAME);
    rgt_co_chunk_ascend(*pchunk);
    lua_settop(L, 1);
    return 1;
}


static int
l_append(lua_State *L)
{
    rgt_co_chunk  **pchunk  = luaL_checkudata(L, 1, LUA_RGT_CO_CHUNK_NAME);
    size_t          len;
    const void     *ptr     = luaL_checklstring(L, 2, &len);

    if (!rgt_co_chunk_append(*pchunk, ptr, len))
        return luaL_error(L, "Failed to append to a chunk: %s",
                          strerror(errno));

    lua_settop(L, 1);
    return 1;
}


static rgt_co_chunk_attr *
table_to_attr_list(struct obstack *obs, lua_State *L, int idx)
{
    size_t              num;
    rgt_co_chunk_attr  *list;
    size_t              attr_idx;
    rgt_co_chunk_attr  *attr;
    const char         *ptr;
    size_t              len;

    /* Convert table index to absolute value */
    idx = lua_abs_index(L, idx);

    /* Allocate the list */
    num = lua_isnoneornil(L, idx) ? 0 : lua_objlen(L, idx);
    list = obstack_alloc(obs, sizeof(*list) * (num + 1));

    /* Fill-in the list */
    for (attr_idx = 1, attr = list; attr_idx <= num; attr_idx++, attr++)
    {
        /* Retrieve name/value pair table */
        lua_pushinteger(L, attr_idx);
        lua_rawget(L, idx);

        /* Transfer name */
        lua_pushinteger(L, 1);
        lua_rawget(L, -2);
        ptr = lua_tolstring(L, -1, &len);
        attr->name = obstack_copy0(obs, ptr, len);
        lua_pop(L, 1);

        /* Transfer value */
        lua_pushinteger(L, 2);
        lua_rawget(L, -2);
        ptr = lua_tolstring(L, -1, &len);
        attr->value_ptr = obstack_copy(obs, ptr, len);
        attr->value_len = len;
        lua_pop(L, 1);

        /* Pop name/value pair table */
        lua_pop(L, 1);
    }

    /* Terminate the list */
    attr->name = NULL;

    return list;
}


static int
l_append_start_tag(lua_State *L)
{
    rgt_co_chunk      **pchunk  = luaL_checkudata(L, 1,
                                                  LUA_RGT_CO_CHUNK_NAME);
    const char         *name    = luaL_checkstring(L, 2);

    struct obstack      obs;
    te_bool             success;
    int                 append_errno;

    if (!lua_isnoneornil(L, 3))
        luaL_checktype(L, 3, LUA_TTABLE);

    obstack_init(&obs);
    success = rgt_co_chunk_append_start_tag(*pchunk, name,
                                            table_to_attr_list(&obs, L, 3));
    append_errno = errno;
    obstack_free(&obs, NULL);

    if (!success)
        return luaL_error(L, "Failed to append start tag to a chunk: %s",
                          strerror(append_errno));

    lua_settop(L, 1);
    return 1;
}


static int
l_append_cdata(lua_State *L)
{
    rgt_co_chunk      **pchunk  = luaL_checkudata(L, 1,
                                                  LUA_RGT_CO_CHUNK_NAME);
    size_t              len;
    const void         *ptr     = luaL_checklstring(L, 2, &len);

    if (!rgt_co_chunk_append_cdata(*pchunk, ptr, len))
        return luaL_error(L, "Failed to append cdata to a chunk: %s",
                          strerror(errno));

    lua_settop(L, 1);
    return 1;
}


static int
l_append_end_tag(lua_State *L)
{
    rgt_co_chunk      **pchunk  = luaL_checkudata(L, 1,
                                                  LUA_RGT_CO_CHUNK_NAME);
    const char         *name    = luaL_checkstring(L, 2);

    if (!rgt_co_chunk_append_end_tag(*pchunk, name))
        return luaL_error(L, "Failed to append end tag to a chunk: %s",
                          strerror(errno));

    lua_settop(L, 1);
    return 1;
}


static int
l_append_element(lua_State *L)
{
    rgt_co_chunk      **pchunk  = luaL_checkudata(L, 1,
                                                  LUA_RGT_CO_CHUNK_NAME);
    const char         *name    = luaL_checkstring(L, 2);

    size_t              content_len;
    const void         *content_ptr     = luaL_optlstring(L, 4, NULL,
                                                          &content_len);

    struct obstack      obs;
    te_bool             success;
    int                 append_errno;

    if (!lua_isnoneornil(L, 3))
        luaL_checktype(L, 3, LUA_TTABLE);

    obstack_init(&obs);
    success = rgt_co_chunk_append_element(*pchunk, name,
                                          table_to_attr_list(&obs, L, 3),
                                          content_ptr, content_len);
    append_errno = errno;
    obstack_free(&obs, NULL);

    if (!success)
        return luaL_error(L, "Failed to append start tag to a chunk: %s",
                          strerror(append_errno));

    lua_settop(L, 1);
    return 1;
}


static int
l_append_msg(lua_State *L)
{
    rgt_co_chunk      **pchunk  = luaL_checkudata(L, 1,
                                                  LUA_RGT_CO_CHUNK_NAME);
    rgt_msg           **pmsg    = luaL_checkudata(L, 2,
                                                  LUA_RGT_MSG_NAME);

    if (!rgt_co_chunk_append_msg(*pchunk, *pmsg))
        return luaL_error(L, "Failed to append message element "
                             "to a chunk: %s", strerror(errno));

    lua_settop(L, 1);
    return 1;
}


static int
l_finish(lua_State *L)
{
    rgt_co_chunk  **pchunk  = luaL_checkudata(L, 1,
                                              LUA_RGT_CO_CHUNK_NAME);
    if (!rgt_co_chunk_finish(*pchunk))
        return luaL_error(L, "Failed to finish a chunk: %s",
                          strerror(errno));

    lua_settop(L, 1);
    return 1;
}


static const luaL_Reg lib[] = {
  {"finished",          l_finished},
  {"is_void",           l_is_void},
  {"is_file",           l_is_file},
  {"is_mem",            l_is_mem},
  {"__len",             l___len},
  {"fork",              l_fork},
  {"descend",           l_descend},
  {"ascend",            l_ascend},
  {"append",            l_append},
  {"append_start_tag",  l_append_start_tag},
  {"append_cdata",      l_append_cdata},
  {"append_end_tag",    l_append_end_tag},
  {"append_element",    l_append_element},
  {"append_msg",        l_append_msg},
  {"finish",            l_finish},
  {NULL, NULL}
};

int
luaopen_rgt_co_chunk(lua_State *L)
{
    /* Register the library */
    luaL_register(L, LUA_RGT_CO_CHUNK_NAME, lib);

    /* Register the library table as the userdata metatable */
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, LUA_RGT_CO_CHUNK_NAME);

    /* Make indexing userdata index metatable instead */
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    return 1;
}


