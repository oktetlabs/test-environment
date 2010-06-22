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

#include <inttypes.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <lualib.h>
#include <lauxlib.h>
#include "rgt_msg_fmt.h"
#include "lua_rgt_msg.h"

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free

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


#define SET_FIELD(_type, _name, _args...) \
    do {                                    \
        lua_pushliteral(L, #_name);         \
        lua_push##_type(L, ##_args);        \
        lua_rawset(L, -3);                  \
    } while (0)


#define SKIP_SPACE(_p) \
    do {                                \
        for (; isspace(*(_p)); (_p)++); \
    } while (0)


#define PARSE_FAIL(_p, _object) \
    do {                                                        \
        lua_pushfstring(L,                                      \
                        "Failed to extract %s from control "    \
                        "message format string at \"%s\"",      \
                       _object, _p);                            \
        return FALSE;                                           \
    } while (0)


static char *
fmt_spec(struct obstack *obs, const char **pp, const rgt_msg_fld **parg)
{
    size_t  len     = strlen(*pp);
    char   *str;

    if (!rgt_msg_fmt_spec_plain(pp, &len, parg,
                                rgt_msg_fmt_out_obstack, obs))
    {
        obstack_free(obs, obstack_finish(obs));
        return NULL;
    }

    len = obstack_object_size(obs);
    obstack_1grow(obs, '\0');
    str = obstack_finish(obs);
    str[len] = '\0';

    return str;
}


static te_bool
parse_quoted_string(char              **pstr,
                    size_t             *plen,
                    struct obstack     *obs,
                    const char        **pp,
                    const rgt_msg_fld **parg)
{
    const char     *p;
    const char     *next_p;

    assert(pstr != NULL);
    assert(plen != NULL);
    assert(obs != NULL);
    assert(pp != NULL);
    assert(*pp != NULL);
    assert(parg != NULL);
    assert(*parg != NULL);

    p = *pp;

    if (*p != '"')
    {
        errno = EINVAL;
        return FALSE;
    }
    p++;

    /* Seek closing " or \0 */
    for (next_p = p; *next_p != '"' && *next_p != '\0'; next_p++);
    if (*next_p == '\0')
    {
        errno = EINVAL;
        return FALSE;
    }

    if (!rgt_msg_fmt_plain_obstack(obs, p, next_p - p, parg))
    {
        obstack_free(obs, obstack_finish(obs));
        return FALSE;
    }
    *plen = obstack_object_size(obs);
    *pstr = obstack_finish(obs);

    p++;
    *pp = p;
    return TRUE;
}


static te_bool
parse_quoted_escaped_string(char              **pstr,
                            size_t             *plen,
                            struct obstack     *obs,
                            const char        **pp)
{
    const char     *p;
    const char     *next_p;

    assert(pstr != NULL);
    assert(plen != NULL);
    assert(obs != NULL);
    assert(pp != NULL);
    assert(*pp != NULL);

    p = *pp;

    if (*p != '"')
    {
        errno = EINVAL;
        return FALSE;
    }
    p++;

    while (TRUE)
    {
        for (next_p = p;
             *next_p != '\\' && *next_p != '"' && *next_p != '\0';
             next_p++);
        if (*next_p == '\0')
        {
            errno = EINVAL;
            obstack_free(obs, obstack_finish(obs));
            return FALSE;
        }
        obstack_grow(obs, p, next_p - p);
        if (*next_p == '"')
            break;
        p = next_p + 1;
        obstack_1grow(obs, *p);
        p++;
    }

    *pp = p + 1;
    *plen = obstack_object_size(obs);
    *pstr = obstack_finish(obs);

    return TRUE;
}


static te_bool
parse_non_space(char              **pstr,
                size_t             *plen,
                struct obstack     *obs,
                const char        **pp,
                const rgt_msg_fld **parg)
{
    const char     *next_p;

    assert(pstr != NULL);
    assert(plen != NULL);
    assert(obs != NULL);
    assert(pp != NULL);
    assert(*pp != NULL);
    assert(parg != NULL);
    assert(*parg != NULL);

    /* While non-space */
    for (next_p = *pp; *next_p != '\0' && !isspace(*next_p); next_p++);

    if (!rgt_msg_fmt_plain_obstack(obs, *pp, next_p - *pp, parg))
    {
        obstack_free(obs, obstack_finish(obs));
        return FALSE;
    }
    *plen = obstack_object_size(obs);
    *pstr = obstack_finish(obs);

    return TRUE;
}


static te_bool
parse_tag_tin(lua_State            *L,
              struct obstack       *obs,
              const char          **pp,
              const rgt_msg_fld   **parg)
{
    char   *str;
    size_t  len;

    assert(obs != NULL);
    assert(pp != NULL);
    assert(*pp != NULL);
    assert(parg != NULL);
    assert(*parg != NULL);

    SKIP_SPACE(*pp);
    if (!parse_non_space(&str, &len, obs, pp, parg))
        PARSE_FAIL(*pp, "TIN");
    SET_FIELD(lstring, tin, str, len);

    return TRUE;
}


static te_bool
parse_tag_page(lua_State           *L,
               struct obstack      *obs,
               const char         **pp,
               const rgt_msg_fld  **parg)
{
    char   *str;
    size_t  len;

    assert(obs != NULL);
    assert(pp != NULL);
    assert(*pp != NULL);
    assert(parg != NULL);
    assert(*parg != NULL);

    SKIP_SPACE(*pp);
    if (!parse_non_space(&str, &len, obs, pp, parg))
        PARSE_FAIL(*pp, "page");
    SET_FIELD(lstring, page, str, len);

    return TRUE;
}


static te_bool
parse_tag_authors(lua_State            *L,
                  struct obstack       *obs,
                  const char          **pp,
                  const rgt_msg_fld   **parg)
{
    static const char   mailto_str[]    = "mailto:";
#define mailto_len  (sizeof(mailto_str) - 1)

    char           *p;
    char           *next_p;
    lua_Integer     i;

    assert(obs != NULL);
    assert(pp != NULL);
    assert(*pp != NULL);
    assert(parg != NULL);
    assert(*parg != NULL);

    SKIP_SPACE(*pp);
    p = fmt_spec(obs, pp, parg);
    if (p == NULL)
        PARSE_FAIL(*pp, "authors");

    /* Push authors field name */
    lua_pushliteral(L, "authors");
    /* Create authors table */
    lua_newtable(L);
    /* Lua counts from one */
    i = 1;

    while (TRUE)
    {
        SKIP_SPACE(p);
        if (*p == '\0')
            break;

        if (strncmp(p, mailto_str, mailto_len) != 0)
        {
            lua_pushfstring(L, "Invalid e-mail address "
                               "in an authors string at \"%s\"", p);
            return FALSE;
        }
        p += mailto_len;

        for (next_p = p; *next_p != '\0' && !isspace(*next_p); next_p++);
        if (next_p == p)
        {
            lua_pushfstring(L, "Empty e-mail address "
                               "in an authors string at \"%s\"", p);
            return FALSE;
        }

        lua_pushinteger(L, i);
        lua_pushlstring(L, p, next_p - p);
        lua_rawset(L, -3);

        i++;
    }

    /* Set authors field */
    lua_rawset(L, -3);

    return TRUE;
}


static te_bool
parse_tag_args(lua_State           *L,
               struct obstack      *obs,
               const char         **pp,
               const rgt_msg_fld  **parg)
{
    char           *p;
    char           *next_p;
    lua_Integer     i;
    char           *str;
    size_t          len;

    assert(obs != NULL);
    assert(pp != NULL);
    assert(*pp != NULL);
    assert(parg != NULL);
    assert(*parg != NULL);

    SKIP_SPACE(*pp);
    p = fmt_spec(obs, pp, parg);
    if (p == NULL)
        PARSE_FAIL(*pp, "args");

    /* Push args field name */
    lua_pushliteral(L, "args");
    /* Create argument list table */
    lua_newtable(L);
    /* Lua counts from one */
    i = 1;

    while (TRUE)
    {
        SKIP_SPACE(p);
        if (*p == '\0')
            break;

        for (next_p = p; *next_p != '\0' && *next_p != '='; next_p++);
        if (next_p == p)
        {
            lua_pushfstring(L, "Empty argument name "
                               "in an argument list string at \"%s\"", p);
            return FALSE;
        }

        /* Push argument index */
        lua_pushinteger(L, i);
        /* Create argument table */
        lua_newtable(L);

        /* Set argument name */
        lua_pushinteger(L, 1);
        lua_pushlstring(L, p, next_p - p);
        lua_rawset(L, -3);

        p = next_p + 1;
        /* Parse argument value */
        if (!parse_quoted_escaped_string(&str, &len, obs,
                                         (const char **)&p))
        {
            lua_pushfstring(L, "Failed unescaping argument value "
                               "in an argument list string at \"%s\"", p);
            return FALSE;
        }

        /* Set argument value */
        lua_pushinteger(L, 2);
        lua_pushlstring(L, str, len);
        lua_rawset(L, -3);

        /* Add argument into the list */
        lua_rawset(L, -3);

        i++;
    }

    /* Set args field */
    lua_rawset(L, -3);

    return TRUE;
}


te_bool
parse_node_start_tags(lua_State            *L,
                      struct obstack       *obs,
                      const char           *fmt,
                      const rgt_msg_fld    *arg)
{
    const char *p           = fmt;
    const char *next_p;
    char       *tag;
    te_bool     success;

    assert(L != NULL);
    assert(obs != NULL);
    assert(fmt != NULL);
    assert(arg != NULL);

    while (TRUE)
    {
        SKIP_SPACE(p);
        /* While alpha */
        for (next_p = p; isalpha(*next_p); next_p++);
        if (next_p == p)
            break;
        tag = obstack_copy0(obs, p, next_p - p);

        if (strcasecmp(tag, "tin") == 0)
            success = parse_tag_tin(L, obs, &p, &arg);
        else if (strcasecmp(tag, "page") == 0)
            success = parse_tag_page(L, obs, &p, &arg);
        else if (strcasecmp(tag, "authors") == 0)
            success = parse_tag_authors(L, obs, &p, &arg);
        else if (strcasecmp(tag, "args") == 0)
            success = parse_tag_args(L, obs, &p, &arg);
        else
        {
            lua_pushfstring(L, "Unknown tag \"%s\" encountered in message "
                               "format string at \"%s\"", tag, p);
            return FALSE;
        }

        if (!success)
            return FALSE;
    }

    return TRUE;
}


static te_bool
parse_node_start(lua_State         *L,
                 struct obstack    *obs,
                 te_bool            named,
                 const char        *fmt,
                 const rgt_msg_fld *arg)
{
    const char *p       = fmt;
    char       *str;
    size_t      len;

    assert(L != NULL);
    assert(obs != NULL);
    assert(fmt != NULL);
    assert(arg != NULL);

    if (named)
    {
        /* Extract name */
        SKIP_SPACE(p);
        if (!parse_non_space(&str, &len, obs, &p, &arg) || len == 0)
            PARSE_FAIL(p, "name");
        SET_FIELD(lstring, name, str, len);

        /* Extract objective */
        SKIP_SPACE(p);
        if (!parse_quoted_string(&str, &len, obs, &p, &arg))
            PARSE_FAIL(p, "objective");
        SET_FIELD(lstring, objective, str, len);
    }

    return parse_node_start_tags(L, obs, p, arg);
}


static te_bool
parse_tester_control(lua_State         *L,
                     struct obstack    *obs,
                     const char        *fmt,
                     const rgt_msg_fld *arg)
{
    uint32_t    parent_id;
    uint32_t    id;
    const char *p;
    int         read;
    const char *next_p;
    char       *event;

    assert(L != NULL);
    assert(obs != NULL);
    assert(fmt != NULL);
    assert(arg != NULL);

    /* Start parsing */
    p = fmt;

    /* Extract parent and node IDs */
    if (sscanf(p, "%" PRIu32 " %" PRIu32 " %n", &parent_id, &id, &read) < 2)
        PARSE_FAIL(p, "parent and node IDs");
    p += read;
    SET_FIELD(number, parent_id, parent_id);
    SET_FIELD(number, id, id);

    /* Extract the event */
    for (next_p = p; isalnum(*next_p); next_p++);
    if (next_p == p)
        PARSE_FAIL(p, "event");
    SET_FIELD(lstring, event, p, next_p - p);
    event = obstack_copy0(obs, p, next_p - p);
    p = next_p;

    /* If it's a package or test start */
    if (strcasecmp(event, "package") == 0 ||
        strcasecmp(event, "test") == 0)
        /* Parse a named node start */
        return parse_node_start(L, obs, TRUE, p, arg);
    /* Else if it's a session start */
    else if (strcasecmp(event, "session") == 0)
        /* Parse an unnamed node start */
        return parse_node_start(L, obs, FALSE, p, arg);
    /* Otherwise it is a node end */
    else
    {
        char   *str;
        size_t  len;

        /* Extract error message */
        SKIP_SPACE(p);
        if (!parse_quoted_string(&str, &len, obs, &p, &arg))
            PARSE_FAIL(p, "error message");
        SET_FIELD(lstring, err, str, len);
    }

    return TRUE;
}

#undef PARSE_FAIL
#undef SKIP_SPACE
#undef SET_FIELD


static int
l_parse_tester_control(lua_State *L)
{
    rgt_msg           **pmsg        = luaL_checkudata(L, 1, LUA_RGT_MSG_NAME);
    rgt_msg            *msg         = *pmsg;
    struct obstack      obs;
    char               *fmt;
    te_bool             success;

    assert(msg != NULL);

    obstack_init(&obs);

    /* Terminate formatting string - assume it doesn't contain 0's */
    fmt = obstack_copy0(&obs, msg->fmt->buf, msg->fmt->len);

    /* Create the output table */
    lua_newtable(L);

    /* Parse the message into the table */
    success = parse_tester_control(L, &obs, fmt, msg->args);

    obstack_free(&obs, NULL);

    return success ? 1 : lua_error(L);
}


static int
l_get_id(lua_State *L)
{
    rgt_msg   **pmsg    = luaL_checkudata(L, 1, LUA_RGT_MSG_NAME);
    lua_pushnumber(L, (*pmsg)->id);
    return 1;
}


static int
l_get_ts(lua_State *L)
{
    rgt_msg   **pmsg    = luaL_checkudata(L, 1, LUA_RGT_MSG_NAME);

    /* Create an instance of rgt.ts */
    lua_getglobal(L, "require");
    lua_pushliteral(L, "rgt.ts");
    lua_call(L, 1, 1);
    lua_pushnumber(L, (*pmsg)->ts_secs);
    lua_pushnumber(L, (*pmsg)->ts_usecs);
    lua_call(L, 2, 1);

    return 1;
}


static te_bool
rgt_msg_fmt_out_lua_buffer(void        *data,
                           const void  *ptr,
                           size_t       len)
{
    assert(data != NULL);
    assert(ptr != NULL || len == 0);

    luaL_addlstring((luaL_Buffer *)data, ptr, len);

    return TRUE;
}


static int
l_get_text(lua_State *L)
{
    rgt_msg           **pmsg        = luaL_checkudata(L, 1,
                                                      LUA_RGT_MSG_NAME);
    rgt_msg            *msg         = *pmsg;
    const rgt_msg_fld  *arg         = msg->args;
    luaL_Buffer         buf;
    te_bool             success;

    luaL_buffinit(L, &buf);
    success = rgt_msg_fmt_plain((const char *)msg->fmt->buf, msg->fmt->len,
                                &arg, rgt_msg_fmt_out_lua_buffer, &buf);
    luaL_pushresult(&buf);
    if (!success)
        return luaL_error(L, "Failed formatting message text: %s",
                          strerror(errno));

    return 1;
}


static const luaL_Reg lib[] = {
  {"is_tester_control",     l_is_tester_control},
  {"is_control",            l_is_control},
  {"parse_tester_control",  l_parse_tester_control},
  {"get_id",                l_get_id},
  {"get_ts",                l_get_ts},
  {"get_text",              l_get_text},
  {NULL, NULL}
};


int
luaopen_rgt_msg(lua_State *L)
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


