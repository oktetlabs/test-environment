/** @file 
 * @brief RGT core: I/O functions.
 *
 * Auxiliary functions for I/O operations.
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
 * @author Oleg N. Kravtsov  <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_RGT_CORE_IO_H__
#define __TE_RGT_CORE_IO_H__

#include <stdio.h>
#include <obstack.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "te_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Modes of reading raw log file */
typedef enum rgt_io_mode {
    RGT_IO_MODE_BLK, /**< Blocking mode. Read operation in this mode
                          blocks caller until all bytes read */
    RGT_IO_MODE_NBLK, /**< Nonblocking mode. If number of bytes caller
                           wants to read less than actually available
                           in file, it reads only them without waiting
                           for more data. */
} rgt_io_mode_t;

/**
 * Attempts to read up to count bytes from file descriptor fd into
 * the buffer starting at buf.
 * If read_mode equals to RMODE_BLOCKING and there is not enough data
 * in file it blocks until count bytes available for read.
 * If read_mode equals to RMODE_NONBLOCKING it doesn't block in any cases.
 *
 * @param  fd       File descriptor used for reading.
 * @param  buf      Pointer to the user specified buffer.
 * @param  count    Number of bytes user wants to be read.
 * @param  io_mode  Blocking or non-blocking mode of reading should be used.
 *
 * @return  Number of bytes read is returned.
 *
 * @retval n > 0 operation successfully complited.
 * @retval 0     An error occurs, or the end-of-file is reached.
 *               User has to check it with feof() or ferror().
 */
extern size_t universal_read(FILE *fd, void *buf, size_t count,
                             rgt_io_mode_t io_mode);

/**
 * Output a string, encoding XML special characters.
 *
 * @param obstk     Obstack structure for string output (if NULL, it will
 *                  be written to rgt_ctx.out_fd).
 * @param str       String to process and output.
 * @param attr_val  Whether the string is an attribute value of some TAG?
 */
extern void write_xml_string(struct obstack *obstk, const char *str,
                             te_bool attr_val);

#ifdef __cplusplus
}
#endif

#endif /* __TE_RGT_CORE_IO_H__ */

