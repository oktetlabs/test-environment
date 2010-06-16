/** @file
 * @brief Test Environment: RGT chunked buffer.
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
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
#include <string.h>
#include "rgt_cbuf.h"

rgt_cbuf_chunk *
rgt_cbuf_chunk_new(size_t size)
{
    rgt_cbuf_chunk  *c;

    c = malloc(sizeof(*c) + size);
    if (c != NULL)
    {
        c->next = NULL;
        c->size = size;
        c->len  = 0;
    }

    return c;
}


rgt_cbuf_chunk *
rgt_cbuf_chunk_retention(rgt_cbuf_chunk *c)
{
    c = realloc(c, sizeof(*c) + c->len);
    if (c == NULL)
        return NULL;

    c->size = c->len;

    return c;
}


#define DEFAULT_CHUNK_SIZE  (16384 - sizeof(rgt_cbuf_chunk))


te_bool
rgt_cbuf_valid(const rgt_cbuf *b)
{
    return b != NULL &&
           ((b->first == NULL && b->pre_last == NULL && b->last == NULL) ||
            (b->first != NULL && b->last != NULL));
}


static rgt_cbuf_chunk *
rgt_cbuf_add_chunk(rgt_cbuf *b, size_t size)
{
    rgt_cbuf_chunk  *c;

    assert(rgt_cbuf_valid(b));

    c = rgt_cbuf_chunk_new(size);
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


rgt_cbuf *
rgt_cbuf_init(rgt_cbuf *b, size_t size)
{
    b->first    = NULL;
    b->pre_last = NULL;
    b->last     = NULL;
    b->len      = 0;
    if (size > 0 && rgt_cbuf_add_chunk(b, size) == NULL)
        return NULL;
    assert(rgt_cbuf_valid(b));
    return b;
}


rgt_cbuf *
rgt_cbuf_new(size_t size)
{
    rgt_cbuf    *b;

    b = malloc(sizeof(*b));
    if (b == NULL)
        return NULL;

    return rgt_cbuf_init(b, size);
}


size_t
rgt_cbuf_get_len(const rgt_cbuf *b)
{
    assert(rgt_cbuf_valid(b));
    return b->len;
}


te_bool
rgt_cbuf_retention(rgt_cbuf *b)
{
    rgt_cbuf_chunk  *c;

    assert(rgt_cbuf_valid(b));

    c = b->last;
    if (c == NULL)
        return TRUE;

    c = rgt_cbuf_chunk_retention(c);
    if (c == NULL)
        return FALSE;

    if (b->pre_last == NULL)
        b->first = c;
    else
        b->pre_last->next = c;
    b->last = c;

    return TRUE;
}


te_bool
rgt_cbuf_append(rgt_cbuf *b, const void *p, size_t s)
{
    rgt_cbuf_chunk  *c   = b->last;
    size_t  r;

    assert(rgt_cbuf_valid(b));
    assert(p != NULL || s == 0);

    if (s == 0)
        return TRUE;

    c = b->last;
    if (c == NULL)
    {
        c = rgt_cbuf_add_chunk(b, MAX(s, DEFAULT_CHUNK_SIZE));
        if (c == NULL)
            return FALSE;
    }

    while (TRUE)
    {
        r   = c->size - c->len;

        if (s <= r)
        {
            memcpy(c->buf + c->len, p, s);
            c->len += s;
            b->len += s;
            return TRUE;
        }
        else
        {
            memcpy(c->buf + c->len, p, r);
            c->len += r;
            b->len += r;
            p += r;
            s -= r;

            c = rgt_cbuf_add_chunk(b, MAX(s, (b->len / 2)));
            if (c == NULL)
                return FALSE;
        }
    }
}


te_bool
rgt_cbuf_merge(rgt_cbuf *x, rgt_cbuf *y)
{
    assert(rgt_cbuf_valid(x));
    assert(rgt_cbuf_valid(y));

    /* If y is empty */
    if (y->last == NULL)
        return TRUE;

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
        if (!rgt_cbuf_retention(x))
            return FALSE;
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

    return TRUE;
}


te_bool
rgt_cbuf_writeout(const rgt_cbuf *b, FILE *f)
{
    rgt_cbuf_chunk  *c;

    assert(rgt_cbuf_valid(b));
    assert(f != NULL);
    
    for (c = b->first; c != NULL; c = c->next)
        if (fwrite(c->buf, c->len, 1, f) != 1)
            return FALSE;

    return TRUE;
}


size_t
rgt_cbuf_readin(rgt_cbuf *b, FILE *f)
{
    rgt_cbuf_chunk  *c       = b->last;
    size_t  r;
    size_t  rc;
    size_t  read    = 0;

    assert(rgt_cbuf_valid(b));
    assert(f != NULL);

    c = b->last;
    if (c == NULL)
    {
        c = rgt_cbuf_add_chunk(b, DEFAULT_CHUNK_SIZE);
        if (c == NULL)
            return read;
    }

    while (TRUE)
    {
        r = c->size - c->len;

        if (r == 0)
        {
            c = rgt_cbuf_add_chunk(b, b->len / 2);
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


void
rgt_cbuf_clear(rgt_cbuf *b)
{
    rgt_cbuf_chunk *c;
    rgt_cbuf_chunk *nc;

    assert(rgt_cbuf_valid(b));

    for (c = b->first; c != NULL; c = nc)
    {
        nc = c->next;
        rgt_cbuf_chunk_free(c);
    }

    b->first    = NULL;
    b->pre_last = NULL;
    b->last     = NULL;
    b->len      = 0;
}


void
rgt_cbuf_free(rgt_cbuf *b)
{
    if (b == NULL)
        return;

    assert(rgt_cbuf_valid(b));
    rgt_cbuf_clear(b);
    free(b);
}


