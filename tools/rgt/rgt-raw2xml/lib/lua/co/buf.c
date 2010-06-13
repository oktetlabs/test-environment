/** @file 
 * @brief Test Environment: RGT - chunked output - buffer module
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

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define CO_BUF_NAME "co.buf"

typedef struct chunk chunk;

struct chunk {
    chunk  *next;
    size_t  size;
    size_t  len;
    char    buf[0];
};

static chunk *
chunk_new(size_t size)
{
    chunk  *c;

    c = malloc(sizeof(*c) + size);
    if (c != NULL)
    {
        c->next = NULL;
        c->size = size;
        c->len  = 0;
    }

    return c;
}


static inline void
chunk_free(chunk *c)
{
    free(c);
}


static chunk *
chunk_retension(chunk *c)
{
    c = realloc(c, sizeof(*c) + c->len);
    if (c == NULL)
        return NULL;

    c->size = c->len;

    return c;
}


typedef struct buf {
    chunk  *first;
    chunk  *pre_last;
    chunk  *last;
    size_t  len;
} buf;


#define BUF_DEFAULT_CHUNK_SIZE  (16384 - sizeof(chunk))
#define MAX(x, y) ((x) > (y) ? (x) : (y))


static bool
buf_valid(const buf *b)
{
    return b != NULL &&
           ((b->first == NULL && b->pre_last == NULL && b->last == NULL) ||
            (b->first != NULL && b->last != NULL));
}


static chunk *
buf_add_chunk(buf *b, size_t size)
{
    chunk  *c;

    assert(buf_valid(b));

    c = chunk_new(size);
    if (c == NULL)
        return NULL;

    if (b->first == NULL)
        b->first = c;
    else
        b->last->next = c;

    b->pre_last = b->last;
    b->last = c;

    return c;
}


static buf *
buf_init(buf *b, size_t size)
{
    b->first    = NULL;
    b->pre_last = NULL;
    b->last     = NULL;
    b->len      = 0;
    if (size > 0 && buf_add_chunk(b, size) == NULL)
        return NULL;
    assert(buf_valid(b));
    return b;
}


#if 0
static buf *
buf_new(size_t size)
{
    buf    *b;

    b = malloc(sizeof(*b));
    if (b == NULL)
        return NULL;

    return buf_init(b, size);
}
#endif


static size_t
buf_get_len(buf *b)
{
    assert(buf_valid(b));
    return b->len;
}


static bool
buf_retension(buf *b)
{
    chunk  *c;

    assert(buf_valid(b));

    c = b->last;
    if (c == NULL)
        return true;

    c = chunk_retension(c);
    if (c == NULL)
        return false;

    if (b->pre_last == NULL)
        b->first = c;
    else
        b->pre_last->next = c;
    b->last = c;

    return true;
}


static bool
buf_append(buf *b, const char *p, size_t s)
{
    chunk  *c   = b->last;
    size_t  r;

    assert(buf_valid(b));
    assert(p != NULL || s == 0);

    if (s == 0)
        return true;

    c = b->last;
    if (c == NULL)
    {
        c = buf_add_chunk(b, MAX(s, BUF_DEFAULT_CHUNK_SIZE));
        if (c == NULL)
            return false;
    }

    while (true)
    {
        r   = c->size - c->len;

        if (s <= r)
        {
            memcpy(c->buf + c->len, p, s);
            c->len += s;
            b->len += s;
            return true;
        }
        else
        {
            memcpy(c->buf + c->len, p, r);
            c->len += r;
            b->len += r;
            p += r;
            s -= r;

            c = buf_add_chunk(b, MAX(s, (b->len / 2)));
            if (c == NULL)
                return false;
        }
    }
}


static bool
buf_merge(buf *x, buf *y)
{
    assert(buf_valid(x));
    assert(buf_valid(y));

    /* If y is empty */
    if (y->last == NULL)
        return true;

    /* If x is empty */
    if (x->last == NULL)
    {
        x->first    = y->first;
        x->pre_last = y->pre_last;
        x->last     = y->last;
        x->len      = y->len;
    }
    else
    {
        /* Retension x to save memory */
        if (!buf_retension(x))
            return false;
        x->last->next   = y->first;
        x->pre_last     = (y->pre_last == NULL) ? x->last : y->pre_last;
        x->last         = y->last;
        x->len          += y->len;
    }

    /* Empty y */
    y->first    = NULL;
    y->pre_last = NULL;
    y->last     = NULL;
    y->len      = 0;

    return true;
}


static bool
buf_writeout(const buf *b, FILE *f)
{
    chunk  *c;

    assert(buf_valid(b));
    assert(f != NULL);
    
    for (c = b->first; c != NULL; c = c->next)
        if (fwrite(c->buf, c->len, 1, f) != 1)
            return false;

    return true;
}


static size_t
buf_readin(buf *b, FILE *f)
{
    chunk  *c       = b->last;
    size_t  r;
    size_t  rc;
    size_t  read    = 0;

    assert(buf_valid(b));
    assert(f != NULL);

    c = b->last;
    if (c == NULL)
    {
        c = buf_add_chunk(b, BUF_DEFAULT_CHUNK_SIZE);
        if (c == NULL)
            return read;
    }

    while (true)
    {
        r = c->size - c->len;

        if (r == 0)
        {
            c = buf_add_chunk(b, b->len / 2);
            if (c == NULL)
                return read;
        }

        rc = fread(c->buf + c->len, 1, r, f);
        if (rc == 0)
            return read;
        read += rc;
        c->len += rc;
        b->len += rc;
    }
}


static void
buf_clear(buf *b)
{
    chunk *c;
    chunk *nc;

    assert(buf_valid(b));

    for (c = b->first; c != NULL; c = nc)
    {
        nc = c->next;
        chunk_free(c);
    }

    b->first    = NULL;
    b->pre_last = NULL;
    b->last     = NULL;
    b->len      = 0;
}


#if 0
static void
buf_free(buf *b)
{
    if (b == NULL)
        return;

    assert(buf_valid(b));
    buf_clear(b);
    free(b);
}
#endif


static int
l___call(lua_State *L)
{
    lua_Integer size;

    luaL_checktype(L, 1, LUA_TTABLE);
    size = luaL_optinteger(L, 2, 0);

    if (buf_init(lua_newuserdata(L, sizeof(buf)), size) == NULL)
        luaL_error(L, "memory allocation failed");
    lua_pushvalue(L, 1);
    lua_setmetatable(L, -2);

    return 1;
}


static int
l_append(lua_State *L)
{
    buf        *b       = luaL_checkudata(L, 1, CO_BUF_NAME);
    size_t      len;
    const char *str     = luaL_checklstring(L, 2, &len);

    if (!buf_append(b, str, len))
        luaL_error(L, "memory allocation failed");

    lua_settop(L, 1);
    return 1;
}


static int
l___tostring(lua_State *L)
{
    buf            *b       = luaL_checkudata(L, 1, CO_BUF_NAME);
    luaL_Buffer     s;
    chunk          *c;

    luaL_buffinit(L, &s);

    for (c = b->first; c != NULL; c = c->next)
        luaL_addlstring(&s, c->buf, c->len);

    luaL_pushresult(&s);
    return 1;
}


static int
l___len(lua_State *L)
{
    buf            *b       = luaL_checkudata(L, 1, CO_BUF_NAME);

    lua_pushinteger(L, buf_get_len(b));

    return 1;
}


static int
l_merge(lua_State *L)
{
    buf    *x       = luaL_checkudata(L, 1, CO_BUF_NAME);
    buf    *y       = luaL_checkudata(L, 2, CO_BUF_NAME);

    if (!buf_merge(x, y))
        luaL_error(L, "memory allocation failed");

    lua_settop(L, 1);
    return 1;
}


static int
l_readin(lua_State *L)
{
    buf        *b       = luaL_checkudata(L, 1, CO_BUF_NAME);
    FILE      **pf      = luaL_checkudata(L, 2, LUA_FILEHANDLE);
    size_t      read;

    luaL_argcheck(L, (*pf != NULL), 2, "closed file");

    clearerr(*pf);
    read = buf_readin(b, *pf);
    if (ferror(*pf))
        luaL_error(L, "failed reading a file into the buffer: %s",
                   strerror(errno));

    lua_pushinteger(L, read);
    return 1;
}


static int
l_writeout(lua_State *L)
{
    buf        *b       = luaL_checkudata(L, 1, CO_BUF_NAME);
    FILE      **pf      = luaL_checkudata(L, 2, LUA_FILEHANDLE);

    luaL_argcheck(L, (*pf != NULL), 2, "closed file");

    if (!buf_writeout(b, *pf))
        luaL_error(L, "failed writing the buffer to a file: %s",
                   strerror(errno));

    lua_settop(L, 1);
    return 1;
}


static int
l_retension(lua_State *L)
{
    buf    *b       = luaL_checkudata(L, 1, CO_BUF_NAME);

    if (!buf_retension(b))
        luaL_error(L, "memory allocation failed");

    lua_settop(L, 1);
    return 1;
}


static int
l_clear(lua_State *L)
{
    buf_clear(luaL_checkudata(L, 1, CO_BUF_NAME));
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
  {"retension",     l_retension},
  {"clear",         l_clear},
  {"__gc",          l_clear},
  {NULL, NULL}
};

int luaopen_co_buf(lua_State *L)
{
    /* Register the library */
    luaL_register(L, CO_BUF_NAME, lib);

    /* Create and assign library metatable */
    lua_newtable(L);
    lua_pushcfunction(L, l___call);
    lua_setfield(L, -2, "__call");
    lua_setmetatable(L, -2);

    /* Register the library table as the userdata metatable */
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, CO_BUF_NAME);

    /* Make indexing userdata index metatable instead */
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    return 1;
}

