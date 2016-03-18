/** @file
 * @brief Test Environment: splitting raw log.
 *
 * Commong functions for splitting raw log into fragments and
 * merging fragments back into raw log.
 *
 *
 * Copyright (C) 2015 Test Environment authors (see file AUTHORS in the
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
 * @author Dmitry Izbitsky  <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */

#include <stdlib.h>
#include <stdio.h>
#include <popt.h>

#include "te_config.h"
#include "te_defs.h"
#include "logger_api.h"

/* See the description in rgt_log_bundle_common.h */
int
file2file(FILE *out_f, FILE *in_f,
          off_t out_offset,
          off_t in_offset, off_t length)
{
#define BUF_SIZE  4096
    char   buf[BUF_SIZE];
    size_t bytes_read;

    if (out_offset >= 0)
        fseeko(out_f, out_offset, SEEK_SET);
    if (in_offset >= 0)
        fseeko(in_f, in_offset, SEEK_SET);

    while (length > 0)
    {
        bytes_read = (length > BUF_SIZE ? BUF_SIZE : length);
        bytes_read = fread(buf, 1, bytes_read, in_f);

        if (bytes_read > 0)
        {
            fwrite(buf, 1, bytes_read, out_f);
            length -= bytes_read;
        }

        if (feof(in_f))
            break;
    }

    if (length > 0)
    {
        ERROR("Failed to copy last %llu bytes to file",
              (long long unsigned int)length);
        return -1;
    }

    return 0;
#undef BUF_SIZE
}
