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

#ifndef __LUA_RGT_CO_MNGR_H__
#define __LUA_RGT_CO_MNGR_H__

#include "rgt_co.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LUA_RGT_CO_MNGR_NAME "rgt.co.mngr"

extern int luaopen_rgt_co_mngr(lua_State *L);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__LUA_RGT_CO_MNGR_H__ */
