/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief IOMMU status
 *
 * IOMMU status
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include <sys/types.h>
#include <dirent.h>

#include "rcf_pch_tree.h"

static te_errno
pci_iommu_get(const ta_conf_ctx *ctx, te_string *val)
{
    const char *dirname = "/sys/class/iommu";
    DIR *dir;
    int dirs_nb;

    UNUSED(ctx);

    /* Check if directory is empty, i.e. contains only '.' and '..' entries */
    dir = opendir(dirname);
    if (dir == NULL)
        return TE_OS_RC(TE_TA_UNIX, errno);
    dirs_nb = 0;
    while (readdir(dir) != NULL)
    {
        dirs_nb++;
        if (dirs_nb > 2)
            break;
    }

    te_string_append(val, "%s", dirs_nb > 2 ? "on" : "off");

    (void)closedir(dir);

    return 0;
}

static const ta_conf_node *const node_pci_iommu =
    TA_CONF_RO_STR("iommu", pci_iommu_get);

te_errno
ta_unix_conf_iommu_init(void)
{
    return ta_conf_register("/agent/hardware", node_pci_iommu);
}
