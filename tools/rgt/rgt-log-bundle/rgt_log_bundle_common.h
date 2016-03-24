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

#ifndef __TE_RGT_LOG_BUNDLE_COMMON_H__
#define __TE_RGT_LOG_BUNDLE_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <popt.h>

#include "te_config.h"
#include "te_defs.h"
#include "logger_api.h"

/**
 * Generic string length used for strings containing
 * raw log fragment file names and strings from index files
*/
#define DEF_STR_LEN 512

/**
 * Copy data from one file to another.
 *
 * @param out_f       Destination file pointer
 * @param in_ f       Source file pointer
 * @param out_offset  At which offset to write data in
 *                    destination file (if < 0, then at
 *                    current positon)
 * @param in_offset   At which offset to read data from
 *                    source file (if < 0, then at
 *                    current positon)
 * @param length      Length of data to be copied
 *
 * @return 0 on success, -1 on failure
 */
extern int file2file(FILE *out_f, FILE *in_f,
                     off_t out_offset,
                     off_t in_offset, off_t length);

static inline void
usage(poptContext optCon, int exitcode, char *error, char *addl)
{
    poptPrintUsage(optCon, stderr, 0);
    if (error)
    {
        fprintf(stderr, "%s", error);
        if (addl != NULL)
            fprintf(stderr, ": %s", addl);
        fprintf(stderr, "\n");
    }

    poptFreeContext(optCon);

    exit(exitcode);
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_RGT_LOG_BUNDLE_COMMON_H__ */
