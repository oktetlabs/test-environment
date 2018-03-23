/** @file
 * @brief Test Environment: RGT message Lua interface.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
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
lua_rgt_msg_wrap(lua_State *L, rgt_msg *msg)
{
    *(rgt_msg **)lua_newuserdata(L, sizeof(msg)) = msg;
    luaL_getmetatable(L, LUA_RGT_MSG_NAME);
    assert(!lua_isnil(L, -1));
    lua_setmetatable(L, -2);
}

extern int luaopen_rgt_msg(lua_State *L);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__LUA_RGT_MSG_H__ */
