/** @file
 * @brief RGT core: Raw read interface
 *
 * Implementation of "read" function for regular files that 
 * supports blocking mode.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Oleg N. Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "io.h"

/* See the description in io.h */
size_t
universal_read(FILE *fd, void *buf, size_t count, rgt_io_mode_t io_mode)
{
    size_t r_count;

    do {
        r_count = fread(buf, 1, count, fd);

        if ((r_count == count) || (io_mode == RGT_IO_MODE_NBLK))
        {
            /*
             * All required number of bytes have been read, or
             * the number of bytes available for read is less than required
             * and the function was called with RGT_IO_MODE_NBLK mode.
             */

            /* Return number of butes successfully read */
            return r_count;
        }

        count -= r_count;

        /* Wait for a while may be some data comes */
        sleep(1);
    } while (1);

    /* This code is never reached: just for fixing Compiler Warning */
    return 0;
}
