/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Block Device Configuration Model TAPI
 *
 * Definition of test API for block devices configuration model
 * (doc/cm/cm_block.yml).
 */

#define TE_LGR_USER "Configuration TAPI"

#include "te_config.h"

#include "te_errno.h"
#include "te_str.h"
#include "logger_api.h"
#include "conf_api.h"

#include "tapi_cfg_modules.h"
#include "tapi_cfg_block.h"

#define CFG_BLOCK_DEVICE_FMT "/agent:%s/block:%s"
#define CFG_BLOCK_DEVICE_LOOP_FMT CFG_BLOCK_DEVICE_FMT "/loop:"
#define CFG_BLOCK_RSRC_NAME "/agent:%s/rsrc:block:%s"

#define LOOP_BLOCK_KMOD "loop"

te_errno
tapi_cfg_block_initialize_loop(const char *ta)
{
    return tapi_cfg_module_add(ta, LOOP_BLOCK_KMOD, TRUE);
}

te_errno
tapi_cfg_block_grab(const char *ta, const char *block_dev)
{
    char block_oid[CFG_OID_MAX];
    te_errno rc;

    TE_SPRINTF(block_oid, CFG_BLOCK_DEVICE_FMT, ta, block_dev);

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, block_oid),
                              CFG_BLOCK_RSRC_NAME, ta, block_dev);
    if (rc != 0)
        ERROR("Failed to reserve resource '%s': %r", block_oid, rc);

    return rc;
}

te_bool
tapi_cfg_block_is_loop(const char *ta, const char *block_dev)
{
    int is_loop = 0;
    te_errno rc = cfg_get_instance_int_fmt(&is_loop,
                                           CFG_BLOCK_DEVICE_LOOP_FMT,
                                           ta, block_dev);

    return rc == 0 && is_loop != 0;
}

te_errno
tapi_cfg_block_loop_get_backing_file(const char *ta, const char *block_dev,
                                     char **filename)
{
    te_errno rc;

    rc = cfg_get_instance_string_fmt(filename,
                                     CFG_BLOCK_DEVICE_LOOP_FMT "/backing_file:",
                                     ta, block_dev);
    if (rc != 0)
        return rc;

    if (**filename == '\0')
    {
        free(*filename);
        *filename = NULL;
    }

    return 0;
}

te_errno
tapi_cfg_block_loop_set_backing_file(const char *ta, const char *block_dev,
                                     const char *filename)
{
    if (filename == NULL)
        filename = "";

    return cfg_set_instance_fmt(CFG_VAL(STRING, filename),
                                CFG_BLOCK_DEVICE_LOOP_FMT "/backing_file:",
                                ta, block_dev);
}
