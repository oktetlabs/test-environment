/** @file
 * @brief Unix Test Agent
 *
 * Block devices management
 *
 *
 * Copyright (C) 2018-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Conf Block"

#include "te_config.h"
#include "config.h"

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#include<string.h>

#include "te_stdint.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "unix_internal.h"
#include "conf_common.h"

static te_bool
ta_block_is_mine(const char *block_name, void *data)
{
    UNUSED(data);
    return rcf_pch_rsrc_accessible("/agent:%s/block:%s", ta_name, block_name);
}

static te_errno
block_dev_list(unsigned int gid, const char *oid,
               const char *sub_id, char **list)
{
    char buf[4096];

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

#ifdef __linux__
    {
        te_errno rc;
        rc = get_dir_list("/sys/block", buf, sizeof(buf), FALSE,
                          &ta_block_is_mine, NULL);
        if (rc != 0)
            return rc;
    }
#else
    UNUSED(list);
    ERROR("%s(): getting list of block devices "
          "is supported only for Linux", __FUNCTION__);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

RCF_PCH_CFG_NODE_COLLECTION(node_block_dev, "block",
                            NULL, NULL,
                            NULL, NULL, block_dev_list, NULL);

te_errno
ta_unix_conf_block_dev_init(void)
{
    te_errno rc;

    rc = rcf_pch_add_node("/agent", &node_block_dev);
    if (rc != 0)
        return rc;

    return rcf_pch_rsrc_info("/agent/block",
                             rcf_pch_rsrc_grab_dummy,
                             rcf_pch_rsrc_release_dummy);
}
