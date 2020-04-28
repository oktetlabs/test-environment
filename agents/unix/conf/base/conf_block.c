/** @file
 * @brief Unix Test Agent
 *
 * Block devices management
 *
 *
 * Copyright (C) 2018 OKTET Labs
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Ushakov <Konstantin.Ushakov@oktetlabs.ru>
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
