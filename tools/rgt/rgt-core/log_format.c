/** @file
 * @brief Test Environment: Format independent functions.
 * 
 * Implementation of a function that determines raw log file version.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
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
 *
 * @author  Oleg N. Kravtsov  <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#include <obstack.h>

#include "log_format.h"

/* The description see in log_format.h */
f_fetch_log_msg
rgt_define_rlf_format(rgt_gen_ctx_t *ctx, char **err)
{
    uint8_t version;

    /* The first byte of Raw log file contains raw log file version */
    if (universal_read(ctx->rawlog_fd, &version, 1, ctx->io_mode) != 1)
    {
        /* Postponed mode: File has zero size */
        if (err != NULL)
            *err = "Raw log file is too short to extract version number\n";
        return NULL;
    }

    switch (version)
    {
        case RGT_RLF_V1:
            return fetch_log_msg_v1;

        default:
            if (*err != NULL)
                *err = "File format isn't recognized\n";
            break;
    }

    return NULL;
}
