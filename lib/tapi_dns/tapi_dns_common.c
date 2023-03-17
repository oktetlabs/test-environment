/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Common methods for tapi_dns lib.
 *
 * Common methods for tapi_dns lib.
 */

#include "te_string.h"
#include "tapi_file.h"
#include "tapi_cfg_base.h"
#include "tapi_dns_common.h"

char *
tapi_dns_gen_filepath(const char *ta,
                      const char *base_dir,
                      const char *filename)
{
    char *result = NULL;
    char *ta_dir = NULL;

    if ((filename != NULL) && (filename[0] == '/'))
        return strdup(filename);

    if (base_dir == NULL)
    {
        ta_dir = tapi_cfg_base_get_ta_dir(ta, TAPI_CFG_BASE_TA_DIR_TMP);
        base_dir = ta_dir;
    }
    result = tapi_file_join_pathname(NULL, base_dir, filename, NULL);

    free(ta_dir);
    return result;
}