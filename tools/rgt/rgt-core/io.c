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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "io.h"

/**
 * Attempts to read up to count bytes from file descriptor fd into
 * the buffer starting at buf.
 * If read_mode equals to RMODE_BLOCKING and there is not enough data in file
 * it blocks until count bytes available for read.
 * If read_mode equals to RMODE_NONBLOCKING it doesn't block in any cases.
 *
 * @param  fd      File descriptor used for reading.
 * @param  buf     Pointer to the user specified buffer.
 * @param  count   Number of bytes user wants to be read.
 * @param  rmode   Blocking or non-blocking mode of reading should be used.
 *
 * @return  Number of bytes read is returned.
 *
 * @retval n > 0 operation successfully complited.
 * @retval 0     An error occurs, or the end-of-file is reached.
 *               User has to check it with feof() or ferror().
 */
size_t
universal_read(FILE *fd, void *buf, size_t count, enum read_mode rmode)
{
    size_t r_count;

    do {
        r_count = fread(buf, 1, count, fd);

        if ((r_count == count) || (rmode == RMODE_NONBLOCKING))
        {
            /*
             * All required number of bytes have been read, or
             * the number of bytes available for read is less than required
             * and the function was called with RMODE_NONBLOCKING mode.
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
