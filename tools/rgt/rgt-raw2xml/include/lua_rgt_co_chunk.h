/** @file 
 * @brief Test Environment: RGT chunked output chunk Lua interface.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
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

extern int luaopen_rgt_co_chunk(lua_State *L);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__LUA_RGT_CO_CHUNK_H__ */
