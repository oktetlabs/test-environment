/** @file
 * @brief IOMMU status
 *
 * IOMMU status
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#include <sys/types.h>
#include <dirent.h>

#include "rcf_ch_api.h"
#include "rcf_pch.h"

static te_errno
pci_iommu_get(unsigned int gid, const char *oid, char *value,
              const char *unused1, const char *unused2, const char *unused3)
{
    const char *dirname = "/sys/class/iommu";
    DIR *dir;
    int dirs_nb;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(unused1);
    UNUSED(unused2);
    UNUSED(unused3);

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

    if (dirs_nb > 2)
        strcpy(value, "on");
    else
        strcpy(value, "off");

    (void)closedir(dir);

    return 0;
}

RCF_PCH_CFG_NODE_RO_COLLECTION(node_pci_iommu, "iommu", NULL, NULL,
                               pci_iommu_get, NULL);

te_errno
ta_unix_conf_iommu_init(void)
{
    return rcf_pch_add_node("/agent/hardware", &node_pci_iommu);
}
