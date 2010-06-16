/** @file
 * @brief Test Environment: RGT chunked output - storage.
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

#include <string.h>
#include "rgt_co_strg.h"

#define RELOCATE_BUF_SIZE   32768

te_bool
rgt_co_strg_valid(const rgt_co_strg *strg)
{
    return strg != NULL &&
           ((strg->type == RGT_CO_STRG_TYPE_VOID &&
             strg->len == 0) ||
            (strg->type == RGT_CO_STRG_TYPE_MEM &&
             strg->media.mem != NULL &&
             rgt_cbuf_get_len(strg->media.mem) == strg->len) ||
            (strg->type == RGT_CO_STRG_TYPE_FILE &&
             strg->media.file != NULL));
}


rgt_co_strg *
rgt_co_strg_init(rgt_co_strg *strg)
{
    assert(strg != NULL);

    strg->type          = RGT_CO_STRG_TYPE_VOID;
    strg->len           = 0;

    return (rgt_co_strg *)rgt_co_strg_validate(strg);
}


rgt_co_strg *
rgt_co_strg_take_file(rgt_co_strg *strg, FILE *file, size_t len)
{
    assert(rgt_co_strg_valid(strg));
    assert(file != NULL);

    strg->type          = RGT_CO_STRG_TYPE_FILE;
    strg->media.file    = file;
    strg->len           = len;

    return (rgt_co_strg *)rgt_co_strg_validate(strg);
}


rgt_co_strg *
rgt_co_strg_take_mem(rgt_co_strg *strg, rgt_cbuf *mem, size_t len)
{
    assert(rgt_co_strg_valid(strg));
    assert(rgt_cbuf_valid(mem));

    strg->type      = RGT_CO_STRG_TYPE_MEM;
    strg->media.mem = mem;
    strg->len       = len;

    return (rgt_co_strg *)rgt_co_strg_validate(strg);
}


rgt_cbuf *
rgt_co_strg_yield_mem(rgt_co_strg *strg, size_t *plen)
{
    rgt_cbuf   *mem;

    assert(rgt_co_strg_valid(strg));
    assert(rgt_co_strg_is_mem(strg));

    mem = strg->media.mem;
    if (plen != NULL)
        *plen = strg->len;

    rgt_co_strg_void(strg);

    return mem;
}


FILE *
rgt_co_strg_yield_file(rgt_co_strg *strg, size_t *plen)
{
    FILE   *file;

    assert(rgt_co_strg_valid(strg));
    assert(rgt_co_strg_is_file(strg));

    file = strg->media.file;
    if (plen != NULL)
        *plen = strg->len;

    rgt_co_strg_void(strg);

    return file;
}


te_bool
rgt_co_strg_clnp(rgt_co_strg *strg)
{
    assert(rgt_co_strg_valid(strg));

    if (rgt_co_strg_is_void(strg))
        return TRUE;

    if (rgt_co_strg_is_mem(strg))
        rgt_cbuf_free(strg->media.mem);
    else
    {
        if (fclose(strg->media.file) != 0)
            return FALSE;
    }

    rgt_co_strg_void(strg);

    return TRUE;
}


static te_bool
relocate_to_mem(rgt_co_strg *dst, rgt_co_strg *src)
{
    assert(rgt_co_strg_valid(dst));
    assert(rgt_co_strg_is_mem(dst));
    assert(rgt_co_strg_valid(src));
    assert(!rgt_co_strg_is_void(src));

    /* If the source is memory-based */
    if (rgt_co_strg_is_mem(src))
    {
        if (!rgt_cbuf_merge(dst->media.mem, src->media.mem))
            return FALSE;
        rgt_cbuf_free(src->media.mem);
        return TRUE;
    }
    else
    {
        /* Rewind the source file */
        if (fseek(src->media.file, -src->len, SEEK_CUR) != 0)
            return FALSE;

        /* Read the file contents into the buffer */
        if (rgt_cbuf_readin(dst->media.mem, src->media.file) != src->len)
            return FALSE;

        /* Close the file */
        if (fclose(src->media.file) != 0)
            return FALSE;

        return TRUE;
    }
}


static te_bool
relocate_to_file(rgt_co_strg *dst, rgt_co_strg *src)
{
    assert(rgt_co_strg_valid(dst));
    assert(rgt_co_strg_is_file(dst));
    assert(rgt_co_strg_valid(src));
    assert(!rgt_co_strg_is_void(src));

    /* If the source is memory-based */
    if (rgt_co_strg_is_mem(src))
    {
        if (!rgt_cbuf_writeout(src->media.mem, dst->media.file))
            return FALSE;
        rgt_cbuf_free(src->media.mem);
        return TRUE;
    }
    else
    {
        te_bool     result;
        void       *buf;

        /* Allocate the transfer buffer */
        buf = malloc(RELOCATE_BUF_SIZE);
        if (buf == NULL)
            return FALSE;

        result = FALSE;

        /* Rewind the source file */
        if (fseek(src->media.file, -src->len, SEEK_CUR) != 0)
            goto cleanup;

        /* Transfer the source file contents via the buffer */
        while (TRUE)
        {
            if (fread(buf, RELOCATE_BUF_SIZE, 1, src->media.file) != 1)
            {
                if (ferror(src->media.file))
                    goto cleanup;
                else
                    break;
            }

            if (fwrite(buf, RELOCATE_BUF_SIZE, 1, dst->media.file) != 1)
                goto cleanup;
        }

        /* Close the file */
        if (fclose(src->media.file) != 0)
            goto cleanup;

        result = TRUE;

cleanup:

        free(buf);

        return result;
    }
}


te_bool
rgt_co_strg_move_media(rgt_co_strg *dst, rgt_co_strg *src)
{
    assert(rgt_co_strg_valid(dst));
    assert(rgt_co_strg_valid(src));

    if (rgt_co_strg_is_void(src))
        return rgt_co_strg_clnp(dst);

    if (rgt_co_strg_is_void(dst))
    {
        memcpy(dst, src, sizeof(*src));
        rgt_co_strg_void(src);
        return TRUE;
    }

    if (rgt_co_strg_is_void(dst))
        memcpy(dst, src, sizeof(*src));
    else
    {
        if (rgt_co_strg_is_mem(src))
        {
            if (!relocate_to_mem(src, dst))
                return FALSE;
            dst->type = RGT_CO_STRG_TYPE_MEM;
            dst->media.mem = src->media.mem;
        }
        else
        {
            if (!relocate_to_file(src, dst))
                return FALSE;
            dst->type = RGT_CO_STRG_TYPE_FILE;
            dst->media.file = src->media.file;
        }
        dst->len += src->len;
    }

    rgt_co_strg_void(src);
    assert(rgt_co_strg_valid(dst));

    return TRUE;
}


