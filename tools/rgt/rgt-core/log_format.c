/** @file
 * @brief Test Environment: Format independent functions.
 * 
 * Implementation of a function that determines raw log file version.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
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
            if (err != NULL)
                *err = "File format isn't recognized\n";
            break;
    }

    return NULL;
}
