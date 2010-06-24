/** @file
 * @brief Test Environment: RGT chunked output.
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

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include "logger_defs.h"
#include "rgt_msg_fmt.h"
#include "rgt_co.h"

#define TABSTOP 2

/**********************************************************
 * MANAGER
 **********************************************************/
te_bool
rgt_co_mngr_valid(const rgt_co_mngr *mngr)
{
    return mngr != NULL &&
           mngr->tmp_dir != NULL &&
           mngr->used_mem <= mngr->max_mem;
}


rgt_co_mngr *
rgt_co_mngr_init(rgt_co_mngr *mngr, const char *tmp_dir, size_t max_mem)
{
    assert(mngr != NULL);
    assert(tmp_dir != NULL);
    assert(*tmp_dir != '\0');

    mngr->tmp_dir       = strdup(tmp_dir);
    mngr->max_mem       = max_mem;
    mngr->used_mem      = 0;
    mngr->first_used    = NULL;
    mngr->first_free    = NULL;

    return mngr;
}


static rgt_co_chunk *
add_chunk(rgt_co_mngr *mngr, rgt_co_chunk *prev, size_t depth)
{
    rgt_co_chunk  *chunk;

    assert(rgt_co_mngr_valid(mngr));
    assert(prev == NULL || rgt_co_chunk_valid(prev));

    /* If there are any free chunks */
    if (mngr->first_free != NULL)
    {
        /* Take a free chunk */
        chunk = mngr->first_free;
        mngr->first_free = chunk->next;
    }
    else
    {
        /* Allocate a new chunk */
        chunk = malloc(sizeof(*chunk));
        if (chunk == NULL)
            return NULL;
        chunk->mngr = mngr;
    }

    /* Initialize the chunk */
    rgt_co_strg_init(&chunk->strg);
    chunk->depth = depth;
    chunk->finished = FALSE;

    /* Link the new chunk */
    if (prev == NULL)
    {
        chunk->next = mngr->first_used;
        mngr->first_used = chunk;
    }
    else
    {
        chunk->next = prev->next;
        prev->next = chunk;
    }

    return chunk;
}


rgt_co_chunk *
rgt_co_mngr_add_first_chunk(rgt_co_mngr *mngr, size_t depth)
{
    assert(rgt_co_mngr_valid(mngr));
    return add_chunk(mngr, NULL, depth);
}


rgt_co_chunk *
rgt_co_mngr_add_chunk(rgt_co_chunk *prev, size_t depth)
{
    assert(rgt_co_chunk_valid(prev));
    return add_chunk(prev->mngr, prev, depth);
}


static void
del_chunk(rgt_co_mngr *mngr, rgt_co_chunk *prev)
{
    rgt_co_chunk *chunk;

    assert(rgt_co_mngr_valid(mngr));
    assert(prev == NULL || rgt_co_chunk_valid(prev));

    /* Retrieve and unlink the chunk */
    if (prev == NULL)
    {
        chunk = mngr->first_used;
        assert(chunk != NULL);
        mngr->first_used = chunk->next;
    }
    else
    {
        chunk = prev->next;
        assert(chunk != NULL);
        prev->next = chunk->next;
    }

    /* Cleanup the chunk */
    rgt_co_strg_clnp(&chunk->strg);

    /* Link the chunk to the free list */
    chunk->next = mngr->first_free;
    mngr->first_free = chunk;
}


void
rgt_co_mngr_del_first_chunk(rgt_co_mngr  *mngr)
{
    assert(rgt_co_mngr_valid(mngr));
    del_chunk(mngr, NULL);
}


void
rgt_co_mngr_del_chunk(rgt_co_chunk *prev)
{
    assert(rgt_co_chunk_valid(prev));
    del_chunk(prev->mngr, prev);
}


te_bool
rgt_co_mngr_finished(const rgt_co_mngr *mngr)
{
    assert(rgt_co_mngr_valid(mngr));

    return mngr->first_used != NULL &&
           rgt_co_chunk_finished(mngr->first_used) &&
           mngr->first_used->next == NULL;
}


void
rgt_co_mngr_clnp(rgt_co_mngr *mngr)
{
    rgt_co_chunk   *chunk;
    rgt_co_chunk   *next_chunk;

    assert(rgt_co_mngr_valid(mngr));

    /* Free the "free" chunks */
    for (chunk = mngr->first_free; chunk != NULL; chunk = next_chunk)
    {
        next_chunk = chunk->next;
        assert(rgt_co_chunk_is_void(chunk));
        free(chunk);
    }
    mngr->first_free = NULL;

    /* Free the "used" chunks */
    for (chunk = mngr->first_used; chunk != NULL; chunk = next_chunk)
    {
        next_chunk = chunk->next;
        rgt_co_strg_clnp(&chunk->strg);
        free(chunk);
    }
    mngr->first_used = NULL;

    /* Free the temporary directory */
    free(mngr->tmp_dir);
    mngr->tmp_dir = NULL;
}



/**
 * Collapse all finished memory-based chunk strips into files, until there
 * are no more of them, or enough memory is freed.
 * 
 * @param mngr          The manager.
 * @param req_chunk     The chunk requesting the memory.
 * @param psize         Location of/for amount of memory requested; will be
 *                      set to 0 if the requesting chunk is displaced.
 * @param psatisfied    Location for "satisfied" flag; will be set to TRUE
 *                      if enough memory was freed after displacing finished
 *                      strips, otherwise will be set to FALSE.
 * 
 * @return TRUE if displaced successfully, false otherwise.
 */
static te_bool
displace_finished_strips(rgt_co_mngr   *mngr,
                         rgt_co_chunk  *req_chunk,
                         size_t        *psize,
                         te_bool       *psatisfied)
{
    size_t          acceptable_mem;
    rgt_co_chunk   *prev;
    rgt_co_chunk   *chunk;
    rgt_co_chunk   *next;

    assert(rgt_co_mngr_valid(mngr));
    assert(rgt_co_chunk_valid(req_chunk));
    assert(psize != NULL);
    assert(psatisfied != NULL);

    acceptable_mem = mngr->max_mem * 3 / 4;
    prev = NULL;
    chunk = mngr->first_used;

    while (chunk != NULL && (mngr->used_mem + *psize) > acceptable_mem)
    {
        if (rgt_co_chunk_finished(chunk))
        {
            if (rgt_co_chunk_is_mem(chunk) &&
                !rgt_co_chunk_displace(chunk))
                return FALSE;

            while (TRUE)
            {
                if ((mngr->used_mem + *psize) <= acceptable_mem)
                    break;
                next = chunk->next;
                if (next == NULL || rgt_co_chunk_is_file(next))
                    break;

                if (!rgt_co_chunk_move_media(next, chunk))
                    return FALSE;
                /* Delete *this* (chunk) chunk */
                del_chunk(mngr, prev);

                chunk = next;

                if (!rgt_co_chunk_finished(chunk))
                {
                    if (chunk == req_chunk)
                        *psize = 0;
                    break;
                }
            }
        }
        prev = chunk;
        chunk = chunk->next;
    }

    *psatisfied = (mngr->used_mem + *psize) <= acceptable_mem;
    return TRUE;
}


static int
chunk_len_rcmp(const void *px, const void *py)
{
    size_t          xl  = rgt_co_chunk_get_len(*(rgt_co_chunk **)px);
    size_t          yl  = rgt_co_chunk_get_len(*(rgt_co_chunk **)py);

    return (xl < yl) ? 1 : ((xl == yl) ? 0 : -1);
}


/**
 * Displace all chunks to files (biggest first), until enough memory is
 * freed.
 *
 * @param mngr          The manager.
 * @param req_chunk     The chunk requesting the memory.
 * @param psize         Location of/for amount of memory requested; will be
 *                      set to 0 if the requesting chunk is displaced.
 *
 * @return TRUE if displaced successfully, false otherwise.
 */
static te_bool
displace_all(rgt_co_mngr   *mngr,
             rgt_co_chunk  *req_chunk,
             size_t        *psize)
{
    size_t          list_len;
    rgt_co_chunk   *chunk;
    rgt_co_chunk  **list;
    rgt_co_chunk  **pchunk;
    size_t          acceptable_mem;

    assert(rgt_co_mngr_valid(mngr));
    assert(rgt_co_chunk_valid(req_chunk));
    assert(psize != NULL);

    /* Count memory-based chunks */
    list_len = 0;
    for (chunk = mngr->first_used; chunk != NULL; chunk = chunk->next)
        if (rgt_co_chunk_is_mem(chunk))
            list_len++;

    if (list_len == 0)
        return TRUE;

    /* Allocate memory-based chunk list */
    list = malloc(sizeof(*list) * list_len);
    if (list == NULL)
        return FALSE;

    /* Fill-in the memory-based chunk list */
    for (pchunk = list, chunk = mngr->first_used;
         chunk != NULL; chunk = chunk->next)
        if (rgt_co_chunk_is_mem(chunk))
            *pchunk++ = chunk;

    /* Sort the chunk list */
    qsort(list, list_len, sizeof(*list), chunk_len_rcmp);

    /* Displace the chunks until half the memory is free */
    acceptable_mem = mngr->max_mem / 2;
    for (pchunk = list; list_len > 0; pchunk++, list_len--)
    {
        if (!rgt_co_chunk_displace(*pchunk))
        {
            free(list);
            return FALSE;
        }
        if (*pchunk == req_chunk)
            *psize = 0;
        if (mngr->used_mem + *psize <= acceptable_mem)
            break;
    }

    free(list);

    return TRUE;
}


/**
 * Collapse finished file-based chunk strips.
 *
 * @param mngr          The manager.
 *
 * @return TRUE if collapsed successfully, FALSE otherwise.
 */
static te_bool
collapse_file_strips(rgt_co_mngr *mngr)
{
    rgt_co_chunk   *prev;
    rgt_co_chunk   *chunk;
    rgt_co_chunk   *next;

    assert(rgt_co_mngr_valid(mngr));

    prev = NULL;
    chunk = mngr->first_used;
    while (chunk != NULL)
    {
        if (rgt_co_chunk_finished(chunk) && rgt_co_chunk_is_file(chunk))
        {
            do {
                next = chunk->next;
                if (next == NULL || !rgt_co_chunk_is_file(chunk))
                    break;
                if (!rgt_co_chunk_move_media(next, chunk))
                    return FALSE;
                del_chunk(mngr, prev);
                chunk = next;
            } while (rgt_co_chunk_finished(chunk));
        }
        prev = chunk;
        chunk = chunk->next;
    }

    return TRUE;
}


static te_bool
request_mem(rgt_co_mngr *mngr, rgt_co_chunk *req_chunk, size_t size)
{
    assert(rgt_co_mngr_valid(mngr));
    assert(rgt_co_chunk_valid(req_chunk));
    assert(rgt_co_chunk_is_mem(req_chunk));
    assert(!rgt_co_chunk_finished(req_chunk));

    if (mngr->used_mem + size > mngr->max_mem)
    {
        te_bool first_attempt   = TRUE;
        te_bool satisfied;

        /*
         * Displace chunks to files until enough memory is freed
         */
        while (TRUE)
        {
            if (displace_finished_strips(mngr, req_chunk,
                                         &size, &satisfied) &&
                (satisfied || displace_all(mngr, req_chunk, &size)))
                break;
            else if ((errno != EMFILE && errno != ENFILE) || !first_attempt)
                return FALSE;

            if (!collapse_file_strips(mngr))
                return FALSE;
            first_attempt = FALSE;
        }
    }

    mngr->used_mem += size;
    assert(mngr->used_mem <= mngr->max_mem);

    return TRUE;
}


static inline te_bool
return_mem(rgt_co_mngr *mngr, rgt_co_chunk *ret_chunk, size_t size)
{
    assert(rgt_co_mngr_valid(mngr));
    assert(rgt_co_chunk_valid(ret_chunk));
    assert(!rgt_co_chunk_is_mem(ret_chunk));
    assert(size <= mngr->used_mem);

    mngr->used_mem -= size;

    return TRUE;
}


te_bool
rgt_co_mngr_dump(const rgt_co_mngr *mngr, FILE *file)
{
    const rgt_co_chunk *chunk;
    size_t              used_num;
    size_t              free_num;

    assert(rgt_co_mngr_valid(mngr));
    assert(file != NULL);

    if (fprintf(file, "Memory: %zu/%zu %zu%%\n",
                mngr->used_mem, mngr->max_mem,
                mngr->max_mem == 0
                    ? 100
                    : (mngr->used_mem * 100 / mngr->max_mem)) < 0)
        return FALSE;

    /* Count used chunks */
    used_num = 0;
    for (chunk = mngr->first_used; chunk != NULL; chunk = chunk->next)
        used_num++;

    /* Count free chunks */
    free_num = 0;
    for (chunk = mngr->first_free; chunk != NULL; chunk = chunk->next)
        free_num++;

    if (fprintf(file, "Chunks: %zu/%zu %zu%%\n",
                used_num, free_num,
                free_num == 0 ? 100 : (used_num * 100 / free_num)) < 0)
        return FALSE;

    for (chunk = mngr->first_used; chunk != NULL; chunk = chunk->next)
    {
        if (fprintf(file, "%8p %6s %10zu%s\n",
                    chunk,
                    rgt_co_chunk_is_mem(chunk)
                        ? "memory"
                        : rgt_co_chunk_is_file(chunk)
                            ? "file"
                            : "void",
                    rgt_co_chunk_get_len(chunk),
                    rgt_co_chunk_finished(chunk) ? " finished" : "") < 0)
            return FALSE;
    }

    return TRUE;
}


/**********************************************************
 * CHUNK
 **********************************************************/
te_bool
rgt_co_chunk_valid(const rgt_co_chunk *chunk)
{
    return chunk != NULL &&
           chunk->mngr != NULL &&
           rgt_co_strg_valid(&chunk->strg);
}


rgt_co_strg *
rgt_co_chunk_yield(rgt_co_strg *strg, rgt_co_chunk *chunk)
{
    size_t  returned;

    assert(rgt_co_strg_valid(strg));
    assert(rgt_co_strg_is_void(strg));
    assert(rgt_co_chunk_valid(chunk));

    returned = rgt_co_chunk_is_mem(chunk) ? rgt_co_chunk_get_len(chunk) : 0;

    memcpy(strg, &chunk->strg, sizeof(*strg));
    rgt_co_strg_void(&chunk->strg);
    if (returned != 0)
        (void)return_mem(chunk->mngr, chunk, returned);

    return strg;
}


te_bool
rgt_co_chunk_displace(rgt_co_chunk *chunk)
{
    rgt_co_strg strg        = RGT_CO_STRG_VOID;

    assert(rgt_co_chunk_valid(chunk));
    assert(rgt_co_chunk_is_mem(chunk));

    return rgt_co_strg_take_tmpfile(&strg, chunk->mngr->tmp_dir) &&
           rgt_co_strg_move_media(&chunk->strg, &strg) &&
           return_mem(chunk->mngr, chunk, rgt_co_chunk_get_len(chunk));
}


te_bool
rgt_co_chunk_move_media(rgt_co_chunk *dst, rgt_co_chunk *src)
{
    size_t  requested   = 0;
    size_t  returned    = 0;

    assert(rgt_co_chunk_valid(dst));
    assert(rgt_co_chunk_valid(src));
    assert(dst->mngr == src->mngr);

    if (rgt_co_chunk_is_mem(src))
    {
        /* If we're moving from file to memory */
        if (!rgt_co_chunk_is_mem(dst))
            requested = rgt_co_chunk_get_len(dst);
    }
    else
    {
        /* If we're moving from memory to file */
        if (rgt_co_chunk_is_mem(dst))
            returned = rgt_co_chunk_get_len(dst);
    }

    return (requested == 0 || request_mem(src->mngr, src, requested)) &&
           rgt_co_strg_move_media(&dst->strg, &src->strg) &&
           (returned == 0 || return_mem(dst->mngr, dst, returned));
}


te_bool
rgt_co_chunk_append(rgt_co_chunk *chunk, const void *ptr, size_t len)
{
    assert(rgt_co_chunk_valid(chunk));
    assert(!rgt_co_chunk_is_void(chunk));
    assert(!rgt_co_chunk_finished(chunk));
    assert(ptr != NULL || len == 0);

    return
        (!rgt_co_chunk_is_mem(chunk) ||
         request_mem(chunk->mngr, chunk, len)) &&
        rgt_co_strg_append(&chunk->strg, ptr, len);
}


te_bool
rgt_co_chunk_appendvf(rgt_co_chunk *chunk, const char   *fmt, va_list ap)
{
    va_list     ap_copy;
    int         rc;
    size_t      size;
    char       *buf;

    assert(rgt_co_chunk_valid(chunk));
    assert(fmt != NULL);

    va_copy(ap_copy, ap);
    rc = snprintf(NULL, 0, fmt, ap_copy);
    va_end(ap_copy);
    if (rc < 0)
        return FALSE;
    size = rc + 1;

    buf = alloca(size);
    snprintf(buf, size, fmt, ap);
    return rgt_co_chunk_append(chunk, buf, rc);
}


te_bool
rgt_co_chunk_appendf(rgt_co_chunk *chunk, const char *fmt, ...)
{
    va_list ap;
    te_bool result;

    assert(rgt_co_chunk_valid(chunk));
    assert(fmt != NULL);

    va_start(ap, fmt);
    result = rgt_co_chunk_appendvf(chunk, fmt, ap);
    va_end(ap);
    return result;
}


te_bool
rgt_co_chunk_append_span(rgt_co_chunk *chunk, char c, size_t n)
{
    char   buf[n];
    assert(rgt_co_chunk_valid(chunk));
    memset(buf, c, n);
    return rgt_co_chunk_append(chunk, buf, n);
}


te_bool
rgt_co_chunk_finish(rgt_co_chunk *chunk)
{
    rgt_co_mngr    *mngr;
    rgt_co_chunk   *next;

    assert(rgt_co_chunk_valid(chunk));

    mngr = chunk->mngr;
    chunk->finished = TRUE;

    /*
     * Relocate as much chunks as possible to the first chunk storage
     */
    for (chunk = mngr->first_used;
         rgt_co_chunk_finished(chunk);
         chunk = next)
    {
        if (!rgt_co_chunk_finished(chunk))
            break;
        next = chunk->next;
        if (next == NULL)
            break;
        if (!rgt_co_chunk_move_media(next, chunk))
            return FALSE;
        rgt_co_mngr_del_first_chunk(mngr);
    }

    return TRUE;
}


/**********************************************************
 * XML CHUNK
 **********************************************************/
static inline te_bool
append_indent(rgt_co_chunk *chunk)
{
    assert(rgt_co_chunk_valid(chunk));
    return rgt_co_chunk_append_span(chunk,
                                         ' ', chunk->depth * TABSTOP);
}


static inline te_bool
append_newline(rgt_co_chunk *chunk)
{
    assert(rgt_co_chunk_valid(chunk));
    return rgt_co_chunk_append_char(chunk, '\n');
}


static inline te_bool
append_start_tag_start(rgt_co_chunk *chunk, const char *name)
{
    assert(rgt_co_chunk_valid(chunk));
    assert(name != NULL);
    assert(*name != '\0');

    return rgt_co_chunk_append_char(chunk, '<') &&
           rgt_co_chunk_append_str(chunk, name);
}


static te_bool
append_attr_value(rgt_co_chunk *chunk, const uint8_t *ptr, size_t len)
{
    static const char       xd[]        = "0123456789abcdef";
    static char             enc_buf[]   = "&lt;0xXX&gt;";
    static char * const     enc_pos1    = enc_buf + 6;  /* first X */
    static char * const     enc_pos2    = enc_buf + 7;  /* second X */

    const uint8_t  *prev_p;
    const uint8_t  *p;
    uint8_t         c;
    const char     *r;
    size_t          l;

    assert(rgt_co_chunk_valid(chunk));
    assert(ptr != NULL || len == 0);

    for (p = ptr, prev_p = ptr; len > 0; len--)
    {
        c = *p;
        switch (c)
        {
            case '<':
                r = "&lt;";
                l = 4;
                break;
            case '>':
                r = "&gt;";
                l = 4;
                break;
            case '&':
                r = "&amp;";
                l = 5;
                break;
            case '"':
                r = "&quot;";
                l = 6;
                break;
            case '\r':
                r = "&#13;";
                l = 5;
                break;
            case '\n':
                r = "&#10;";
                l = 5;
                break;
            default:
                if ((c >= ' ' && c < '\x7F') || c == '\t')
                {
                    p++;
                    continue;
                }

                *enc_pos1 = xd[c >> 4];
                *enc_pos2 = xd[c & 0xF];
                r = enc_buf;
                l = sizeof(enc_buf) - 1;
                break;
        }

        if ((p > prev_p &&
             !rgt_co_chunk_append(chunk, prev_p, (p - prev_p))) ||
            !rgt_co_chunk_append(chunk, r, l))
            return FALSE;
        p++;
        prev_p = p;
    }

    return p == prev_p ||
           rgt_co_chunk_append(chunk, prev_p, (p - prev_p));
}


static te_bool
append_attr(rgt_co_chunk   *chunk,
            const char     *name,
            const void     *value_ptr,
            size_t          value_len)
{
    size_t  name_len;
    size_t  buf_len;
    char   *buf;
    char   *p;

    assert(rgt_co_chunk_valid(chunk));
    assert(name != NULL);
    assert(*name != '\0');
    assert(value_ptr != NULL || value_len == 0);

    /*
     * Construct " name=\""
     */
    name_len = strlen(name);
    buf_len = name_len + 3;
    buf = alloca(buf_len);
    p = buf;
    *p++ = ' ';
    memcpy(p, name, name_len);
    p += name_len;
    *p++ = '=';
    *p = '"';

    return rgt_co_chunk_append(chunk, buf, buf_len) &&
           append_attr_value(chunk,
                             (const uint8_t *)value_ptr, value_len) &&
           rgt_co_chunk_append_char(chunk, '"');
}


static inline te_bool
append_str_attr(rgt_co_chunk   *chunk,
                const char     *name,
                const char     *value)
{
    assert(rgt_co_chunk_valid(chunk));
    assert(name != NULL);
    assert(*name != '\0');
    assert(value != NULL);

    return append_attr(chunk, name, value, strlen(value));
}


static te_bool
append_safe_attr(rgt_co_chunk   *chunk,
                 const char     *name,
                 const void     *value_ptr,
                 size_t          value_len)
{
    size_t  name_len;
    size_t  buf_len;
    char   *buf;
    char   *p;

    assert(rgt_co_chunk_valid(chunk));
    assert(name != NULL);
    assert(*name != '\0');
    assert(value_ptr != NULL || value_len == 0);

    /*
     * Construct " name=\"value\""
     */
    name_len = strlen(name);
    buf_len = name_len + value_len + 4;
    buf = alloca(buf_len);
    p = buf;
    *p++ = ' ';
    memcpy(p, name, name_len);
    p += name_len;
    *p++ = '=';
    *p++ = '"';
    memcpy(p, value_ptr, value_len);
    p += value_len;
    *p = '"';

    return rgt_co_chunk_append(chunk, buf, buf_len);
}


static inline te_bool
append_safe_str_attr(rgt_co_chunk  *chunk,
                     const char    *name,
                     const char    *value)
{
    assert(rgt_co_chunk_valid(chunk));
    assert(name != NULL);
    assert(*name != '\0');
    assert(value != NULL);

    return append_safe_attr(chunk, name, value, strlen(value));
}


static te_bool
append_attr_list(rgt_co_chunk *chunk, const rgt_co_chunk_attr *list)
{
    assert(rgt_co_chunk_valid(chunk));

    if (list == NULL)
        return TRUE;

    for (; list->name != NULL; list++)
        if (!append_attr(chunk,
                         list->name, list->value_ptr, list->value_len))
            return FALSE;

    return TRUE;
}


static inline te_bool
append_start_tag_end(rgt_co_chunk *chunk)
{
    assert(rgt_co_chunk_valid(chunk));
    return rgt_co_chunk_append_literal(chunk, ">");
}


static te_bool
append_start_tag(rgt_co_chunk              *chunk,
                 const char                *name,
                 const rgt_co_chunk_attr   *attr_list)
{
    assert(rgt_co_chunk_valid(chunk));
    assert(name != NULL);
    assert(*name != '\0');

    return append_start_tag_start(chunk, name) &&
           append_attr_list(chunk, attr_list) &&
           append_start_tag_end(chunk);
}


static inline te_bool
append_empty_tag_end(rgt_co_chunk *chunk)
{
    assert(rgt_co_chunk_valid(chunk));
    return rgt_co_chunk_append_literal(chunk, "/>");
}


static te_bool
append_empty_tag(rgt_co_chunk              *chunk,
                 const char                *name,
                 const rgt_co_chunk_attr   *attr_list)
{
    assert(rgt_co_chunk_valid(chunk));
    assert(name != NULL);
    assert(*name != '\0');

    return append_start_tag_start(chunk, name) &&
           append_attr_list(chunk, attr_list) &&
           append_empty_tag_end(chunk);
}


static te_bool
append_cdata(rgt_co_chunk *chunk, const uint8_t *ptr, size_t len)
{
    static const char       xd[]        = "0123456789abcdef";
    static char             enc_buf[]   = "&lt;0xXX&gt;";
    static char * const     enc_pos1    = enc_buf + 6;  /* first X */
    static char * const     enc_pos2    = enc_buf + 7;  /* second X */

    const uint8_t  *prev_p;
    const uint8_t  *p;
    uint8_t         c;
    const char     *r;
    size_t          l;

    assert(rgt_co_chunk_valid(chunk));
    assert(ptr != NULL || len == 0);

    for (p = ptr, prev_p = ptr; len > 0;)
    {
        c = *p;
        switch (c)
        {
            case '<':
                r = "&lt;";
                l = 4;
                break;
            case '>':
                r = "&gt;";
                l = 4;
                break;
            case '&':
                r = "&amp;";
                l = 5;
                break;
            case '\r':
                r = "<br/>";
                l = 5;
                /* If it is not "\r\n" */
                if (len == 1 || *(p + 1) != '\n')
                    break;

                /* It is "\r\n" */
                if ((p > prev_p &&
                     !rgt_co_chunk_append(chunk,
                                               prev_p, (p - prev_p))) ||
                    !rgt_co_chunk_append(chunk, r, l))
                    return FALSE;
                p += 2;
                len -= 2;
                prev_p = p;
                continue;
            case '\n':
                r = "<br/>";
                l = 5;
                break;
            default:
                if ((c >= ' ' && c < '\x7F') || c == '\t')
                {
                    p++;
                    len--;
                    continue;
                }

                *enc_pos1 = xd[c >> 4];
                *enc_pos2 = xd[c & 0xF];
                r = enc_buf;
                l = sizeof(enc_buf) - 1;
                break;
        }

        if ((p > prev_p &&
             !rgt_co_chunk_append(chunk, prev_p, (p - prev_p))) ||
            !rgt_co_chunk_append(chunk, r, l))
            return FALSE;
        p++;
        len--;
        prev_p = p;
    }

    return p == prev_p ||
           rgt_co_chunk_append(chunk, prev_p, (p - prev_p));
}


static te_bool
append_end_tag(rgt_co_chunk *chunk, const char *name)
{
    size_t  name_len;
    size_t  buf_len;
    char   *buf;
    char   *p;

    assert(rgt_co_chunk_valid(chunk));
    assert(name != NULL);
    assert(*name != '\0');

    /*
     * Construct "</name>"
     */
    name_len = strlen(name);
    buf_len = name_len + 3;
    buf = alloca(buf_len);
    p = buf;
    *p++ = '<';
    *p++ = '/';
    memcpy(p, name, name_len);
    p += name_len;
    *p = '>';

    return rgt_co_chunk_append(chunk, buf, buf_len);
}


te_bool
rgt_co_chunk_append_start_tag(rgt_co_chunk             *chunk,
                              const char               *name,
                              const rgt_co_chunk_attr  *attr_list)
{
    te_bool success;
    assert(rgt_co_chunk_valid(chunk));
    assert(name != NULL);
    assert(*name != '\0');

    success = append_indent(chunk) &&
              append_start_tag(chunk, name, attr_list) &&
              append_newline(chunk);

    if (success)
        rgt_co_chunk_descend(chunk);

    return success;
}


te_bool
rgt_co_chunk_append_cdata(rgt_co_chunk *chunk, const void *ptr, size_t len)
{
    assert(rgt_co_chunk_valid(chunk));
    assert(ptr != NULL || len == 0);

    return append_indent(chunk) &&
           append_cdata(chunk, (const uint8_t *)ptr, len) &&
           append_newline(chunk);
}


te_bool
rgt_co_chunk_append_end_tag(rgt_co_chunk *chunk, const char *name)
{
    assert(rgt_co_chunk_valid(chunk));
    assert(name != NULL);
    assert(*name != '\0');

    rgt_co_chunk_ascend(chunk);

    return append_indent(chunk) &&
           append_end_tag(chunk, name) &&
           append_newline(chunk);
}


te_bool
rgt_co_chunk_append_element(rgt_co_chunk               *chunk,
                            const char                 *name,
                            const rgt_co_chunk_attr    *attr_list,
                            const void                 *content_ptr,
                            size_t                      content_len)
{
    assert(rgt_co_chunk_valid(chunk));
    assert(name != NULL);
    assert(*name != '\0');
    assert(content_ptr != NULL || content_len == 0);

    if (content_len == 0)
        return append_indent(chunk) &&
               append_empty_tag(chunk, name, attr_list) &&
               append_newline(chunk);
    else
        return append_indent(chunk) &&
               append_start_tag(chunk, name, attr_list) &&
               append_cdata(chunk,
                            (const uint8_t *)content_ptr, content_len) &&
               append_end_tag(chunk, name) &&
               append_newline(chunk);
}


/**********************************************************
 * MSG CHUNK
 **********************************************************/
static te_bool append_msg_cdata_out(void       *data,
                                    const void *ptr,
                                    size_t      len)
{
    rgt_co_chunk   *chunk   = (rgt_co_chunk *)data;

    assert(data != NULL);
    assert(rgt_co_chunk_valid(chunk));
    assert(ptr != NULL || len == 0);

    return append_cdata(data, ptr, len);
}


/**
 * Read a %Tm format specifier arguments.
 *
 * @param pp    Location of/for the format string pointer.
 * @param pl    Location of/for the remaining format string length.
 * @param prl   Location for the row length in elements.
 * @param pel   Location for the element length in bytes.
 *
 * @return TRUE if parsed successfully, FALSE otherwise.
 *
 * @note This is crazy manual work, I know. This is rather due to the need
 *       to process format strings containing 0's (which is due to
 *       reluctance to terminate the format strings read from the raw log)
 *       and an attempt to avoid catering for a sscanf, which needs copying
 *       and termination.
 */
static te_bool
parse_memdump_spec_args(const char    **pp,
                        size_t         *pl,
                        size_t         *prl,
                        size_t         *pel)
{
    const char *p;
    char        c;
    size_t      l;
    size_t      rl;
    size_t      el;

    assert(pp != NULL);
    assert(*pp != NULL);
    assert(pl != NULL);
    assert(prl != NULL);
    assert(pel != NULL);

    p = *pp;
    l = *pl;

    /*
     * The format regex is \[\[\d+\]\.\[\d+\]\].
     * Example: [[16].[4]]
     */

    /* Match \[\[\d */
    if (l-- == 0 || *p++ != '[' ||
        l-- == 0 || *p++ != '[' ||
        l-- == 0 || (c = *p++) < '0' || c > '9')
        return FALSE;

    /* Match \d*\] */
    rl = c - '0';
    while (TRUE)
    {
        if (l-- == 0)
            return FALSE;
        c = *p++;
        if (c == ']')
            break;
        if (c < '0' || c > '9')
            return FALSE;
        rl = rl * 10 + (c - '0');
    }

    /* Match \.\[\d */
    if (l-- == 0 || *p++ != '.' ||
        l-- == 0 || *p++ != '[' ||
        l-- == 0 || (c = *p++) < '0' || c > '9')
        return FALSE;

    /* Match \d*\] */
    el = c - '0';
    while (TRUE)
    {
        if (l-- == 0)
            return FALSE;
        c = *p++;
        if (c == ']')
            break;
        if (c < '0' || c > '9')
            return FALSE;
        el = el * 10 + (c - '0');
    }

    /* Match \] */
    if (l-- == 0 || *p++ != ']')
        return FALSE;

    *pp = p;
    *pl = l;
    *prl = rl;
    *pel = el;

    return TRUE;
}


static te_bool
append_msg_cdata_spec(const char          **pspec,
                      size_t               *plen,
                      const rgt_msg_fld   **parg,
                      rgt_msg_fmt_out_fn   *out_fn,
                      void                 *out_data)
{
    const char         *p;
    char                c;
    size_t              l;
    rgt_co_chunk       *chunk;
    const rgt_msg_fld  *arg;

    assert(pspec != NULL);
    assert(*pspec != NULL);
    assert(plen != NULL);
    assert(parg != NULL);
    assert(*parg != NULL);
    assert(out_fn != NULL);

    p = *pspec;
    l = *plen;

    /* If it is not our format specifier or there are no more arguments */
    if (l < 3 ||
        *p++ != '%' || *p++ != 'T' ||
        ((c = *p) != 'f' && c != 'm') ||
        rgt_msg_fld_is_term(*parg))
        return rgt_msg_fmt_spec_plain(pspec, plen, parg, out_fn, out_data);

    l -= 3;
    chunk = (rgt_co_chunk *)out_data;
    arg = *parg;

    p++;

    if (c == 'f')
    {
        if (!append_start_tag_start(chunk, "file") ||
            !append_safe_attr(chunk, "name", "TODO", 4))
            return FALSE;

        if (arg->len > 0)
        {
            if (!append_start_tag_end(chunk) ||
                !append_cdata(chunk, arg->buf, arg->len) ||
                !append_end_tag(chunk, "file"))
                return FALSE;
        }
        else if (!append_empty_tag_end(chunk))
                return FALSE;
    }
    else
    {
        static const char   xd[]    = "0123456789ABCDEF";

        size_t          rl;
        size_t          el;
        size_t          i;
        const uint8_t  *bp;
        uint8_t         b;
        char            bs[2];

        if (!parse_memdump_spec_args(&p, &l, &rl, &el))
        {
            rl = 16;
            el = 1;
        }
        else if (el == 0 || arg->len % el != 0)
        {
            errno = EINVAL;
            return FALSE;
        }

        if (!append_start_tag(chunk, "mem-dump", NULL))
            return FALSE;

        for (bp = arg->buf, i = 0; i < arg->len; bp++)
        {
            if (i % (el * rl) == 0 && !append_start_tag(chunk, "row", NULL))
                return FALSE;

            if (i % el == 0 && !append_start_tag(chunk, "elem", NULL))
                return FALSE;

            b = *bp;
            bs[0] = xd[b >> 4];
            bs[1] = xd[b & 0xF];

            if (!rgt_co_chunk_append(chunk, bs, 2))
                return FALSE;

            i++;

            if (i % el == 0 && !append_end_tag(chunk, "elem"))
                return FALSE;

            if (i % (el * rl) == 0 && !append_end_tag(chunk, "row"))
                return FALSE;
        }

        if ((i % (el * rl) != 0 && !append_end_tag(chunk, "row")) ||
            !append_end_tag(chunk, "mem-dump"))
            return FALSE;
    }

    *parg = rgt_msg_fld_next(arg);
    *pspec = p;
    *plen = l;
    return TRUE;
}


static te_bool
append_msg_cdata(rgt_co_chunk      *chunk,
                 const rgt_msg_fld *fmt,
                 const rgt_msg_fld *args)
{
    assert(rgt_co_chunk_valid(chunk));
    assert(fmt != NULL);
    assert(args != NULL);

    return rgt_msg_fmt((const char *)fmt->buf, fmt->len, &args,
                       append_msg_cdata_spec,
                       append_msg_cdata_out, chunk);
}


te_bool
rgt_co_chunk_append_msg(rgt_co_chunk *chunk, const rgt_msg *msg)
{
    time_t      ts;
    struct tm  *tm;
    char        ts_buf[16];
    size_t      ts_len;

    assert(rgt_co_chunk_valid(chunk));
    assert(rgt_msg_valid(msg));

    ts = msg->ts_secs;
    tm = localtime(&ts);
    ts_len = strftime(ts_buf, sizeof(ts_buf), "%H:%M:%S", tm);
    ts_len += snprintf(ts_buf + ts_len, sizeof(ts_buf) - ts_len,
                       " %u ms", msg->ts_usecs / 1000);

    return append_indent(chunk) &&
           append_start_tag_start(chunk, "msg") &&
           append_safe_str_attr(chunk, "level",
                                te_log_level2str(msg->level)) &&
           append_attr(chunk, "entity",
                       msg->entity->buf, msg->entity->len) &&
           append_attr(chunk, "user",
                       msg->user->buf, msg->user->len) &&
           append_safe_attr(chunk, "ts", ts_buf, ts_len) &&
           append_start_tag_end(chunk) &&
           append_msg_cdata(chunk, msg->fmt, msg->args) &&
           append_end_tag(chunk, "msg") &&
           append_newline(chunk);
}


