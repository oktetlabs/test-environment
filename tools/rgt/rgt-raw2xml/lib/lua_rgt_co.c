/** @file 
 * @brief Test Environment: RGT chunked output Lua interface.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Nikolai Kondrashov <Nikolai.Kondrashov@oktetlabs.ru>
 *
 * $Id$
 */

#include <lua.h>
#include <lauxlib.h>
#include "lua_rgt_co.h"

static const luaL_Reg lib[] = {
  {NULL, NULL}
};

int
luaopen_rgt_co(lua_State *L)
{
    /* Register the library */
    luaL_register(L, LUA_RGT_CO_NAME, lib);

    /* Load rgt.co.mngr */
    luaopen_rgt_co_mngr(L);
    lua_setfield(L, -2, "mngr");

    /* Load rgt.co.chunk */
    luaopen_rgt_co_chunk(L);
    lua_setfield(L, -2, "chunk");

    return 1;
}



