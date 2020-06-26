/** @file
 * @brief Test Environment: splitting raw log.
 *
 * Commong functions for splitting raw log into fragments and
 * merging fragments back into raw log.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
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

/**
 * Check value of an expression evaluating to te_errno; if
 * it is not zero, print error and terminate the tool.
 *
 * @param _expr     Expression to check.
 */
#define CHECK_TE_RC(_expr) \
    do {                                                  \
        te_errno _rc;                                     \
                                                          \
        _rc = (_expr);                                    \
        if (_rc != 0)                                     \
        {                                                 \
            ERROR("%s failed, rc=%r", #_expr, _rc);       \
            exit(EXIT_FAILURE);                           \
        }                                                 \
    } while (0)


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_RGT_LOG_BUNDLE_COMMON_H__ */
