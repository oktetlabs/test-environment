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

#include "rgt_common.h"
#include "log_msg.h"
#include "log_format.h"

/**
 * Determines RLF version of the format and returns pointer to the function
 * that should be used for extracting log messages from a raw log file.
 *
 * @param  fd   Raw log file descriptor.
 * @param  err  Pointer to the pointer on string that can be set to an error 
 *              message string if the function returns NULL.
 *
 * @return  pointer to the log message extracting function.
 * 
 * @retval  !NULL   Pointer to the log message extracting function.
 * @retval  NULL    Unknown format of RLF.
 */
f_fetch_log_msg
rgt_define_rlf_format(FILE *fd, char **err)
{
    uint8_t version;

    /* The first byte of Raw log file contains raw log file version */
    if (universal_read(fd, &version, 1, rgt_rmode) != 1)
    {
        /* Postponed mode: File has zero size */
        if (*err != NULL)
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
