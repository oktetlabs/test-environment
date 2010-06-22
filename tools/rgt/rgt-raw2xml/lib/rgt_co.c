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
#include <sys/time.h>
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
           mngr->used_mem <= mngr->max_mem;
}


rgt_co_mngr *
rgt_co_mngr_init(rgt_co_mngr *mngr, size_t max_mem)
{
    assert(mngr != NULL);

    mngr->max_mem     = max_mem;
    mngr->used_mem    = 0;
    mngr->first_used  = NULL;
    mngr->first_free  = NULL;

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

    /* Cleanup the chunk ignoring any errors */
    (void)rgt_co_strg_clnp(&chunk->strg);

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
        /* Cleanup the chunk, ignoring any errors */
        (void)rgt_co_strg_clnp(&chunk->strg);
        free(chunk);
    }
    mngr->first_used = NULL;
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


te_bool
rgt_co_chunk_append(rgt_co_chunk *chunk, const void *ptr, size_t len)
{
    assert(rgt_co_chunk_valid(chunk));
    assert(ptr != NULL || len == 0);

    if (!rgt_co_strg_append(&chunk->strg, ptr, len))
        return FALSE;

    if (rgt_co_chunk_is_mem(chunk))
        chunk->mngr->used_mem += len;

    return TRUE;
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
        if (!rgt_co_strg_move_media(&next->strg, &chunk->strg))
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
    assert(list != NULL);

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
    return rgt_co_chunk_append_literal(chunk, ">\n");
}


static te_bool
append_start_tag(rgt_co_chunk              *chunk,
                 const char                *name,
                 const rgt_co_chunk_attr   *attr_list)
{
    assert(rgt_co_chunk_valid(chunk));
    assert(name != NULL);
    assert(*name != '\0');
    assert(attr_list != NULL);

    return append_start_tag_start(chunk, name) &&
           append_attr_list(chunk, attr_list) &&
           append_start_tag_end(chunk);
}


static inline te_bool
append_empty_tag_end(rgt_co_chunk *chunk)
{
    assert(rgt_co_chunk_valid(chunk));
    return rgt_co_chunk_append_literal(chunk, "/>\n");
}


static te_bool
append_empty_tag(rgt_co_chunk              *chunk,
                 const char                *name,
                 const rgt_co_chunk_attr   *attr_list)
{
    assert(rgt_co_chunk_valid(chunk));
    assert(name != NULL);
    assert(*name != '\0');
    assert(attr_list != NULL);

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
    assert(attr_list != NULL);

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
    assert(attr_list != NULL);
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


static te_bool
append_msg_cdata_spec(const char          **pspec,
                      size_t               *plen,
                      const rgt_msg_fld   **parg,
                      rgt_msg_fmt_out_fn   *out_fn,
                      void                 *out_data)
{
    assert(pspec != NULL);
    assert(*pspec != NULL);
    assert(plen != NULL);
    assert(parg != NULL);
    assert(*parg != NULL);
    assert(out_fn != NULL);

    return rgt_msg_fmt_spec_plain(pspec, plen, parg, out_fn, out_data);
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
    assert(rgt_co_chunk_valid(chunk));
    assert(rgt_msg_valid(msg));

    return append_indent(chunk) &&
           append_start_tag_start(chunk, "msg") &&
           append_safe_str_attr(chunk, "level",
                                te_log_level2str(msg->level)) &&
           append_attr(chunk, "entity",
                       msg->entity->buf, msg->entity->len) &&
           append_attr(chunk, "user",
                       msg->user->buf, msg->user->len) &&
           append_start_tag_end(chunk) &&
           append_msg_cdata(chunk, msg->fmt, msg->args) &&
           append_end_tag(chunk, "msg") &&
           append_newline(chunk);
}


