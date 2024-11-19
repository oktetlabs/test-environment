/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2024 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAP interfaces configuration support
 *
 * Implementation of configuration nodes TAP interfaces.
 */

#define TE_LGR_USER     "Unix Conf TAP"

#include "te_config.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_alloc.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_string.h"

#include "logger_api.h"
#include "rcf_pch.h"
#include "ta_common.h"
#include "unix_internal.h"

#if HAVE_FCNTL_H && HAVE_LINUX_IF_TUN_H && HAVE_SYS_IOCTL_H
#include <fcntl.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>

#define TE_TUN_DEV "/dev/net/tun"

struct tap_entry {
    SLIST_ENTRY(tap_entry) links;
    char *name;
};

static SLIST_HEAD(, tap_entry) taps = SLIST_HEAD_INITIALIZER(taps);

static struct tap_entry *
tap_list_find(const char *name)
{
    struct tap_entry *p;

    SLIST_FOREACH(p, &taps, links)
    {
        if (strcmp(name, p->name) == 0)
            return p;
    }

    return NULL;
}

static struct tap_entry *
tap_entry_alloc(const char *ifname)
{
    struct tap_entry *tap;

    tap = TE_ALLOC(sizeof(*tap));
    tap->name = TE_STRDUP(ifname);

    return tap;
}

static void
tap_entry_free(struct tap_entry *tap)
{
    if (tap == NULL)
        return;

    free(tap->name);
    free(tap);
}

static te_errno
tap_add(unsigned int gid, const char *oid, const char *value,
        const char *ifname)
{
    int fd;
    struct ifreq ifr;
    int rc = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    if (tap_list_find(ifname) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if (!rcf_pch_rsrc_accessible("/agent:%s/interface:%s", ta_name, ifname))
    {
        ERROR("Failed to add TAP interface without TA resources");
        return TE_RC(TE_TA_UNIX, TE_EPERM);
    }

    fd = open(TE_TUN_DEV, O_RDWR);
    if (fd < 0)
    {
        rc = errno;
        ERROR("Failed to open '%s': %s", TE_TUN_DEV, strerror(rc));
        return TE_OS_RC(TE_TA_UNIX, rc);
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

#ifdef IFF_TUN_EXCL
    ifr.ifr_flags |= IFF_TUN_EXCL;
#endif

    if (ioctl(fd, TUNSETIFF, &ifr) < 0)
    {
        rc = errno;
        ERROR("Failed to create TAP interface '%s': %s", ifname, strerror(rc));
        goto out;
    }

    if (ioctl(fd, TUNSETPERSIST, 1))
    {
        rc = errno;
        ERROR("Failed to make iface '%s' persistent: %s", ifname, strerror(rc));
        goto out;
    }

    SLIST_INSERT_HEAD(&taps, tap_entry_alloc(ifname), links);

out:
    close(fd);

    return TE_OS_RC(TE_TA_UNIX, rc);
}

static te_errno
tap_del(unsigned int gid, const char *oid, const char *ifname)
{
    int fd;
    struct ifreq ifr;
    struct tap_entry *tap;
    int rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    tap = tap_list_find(ifname);
    if (tap == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    fd = open(TE_TUN_DEV, O_RDWR);
    if (fd < 0)
    {
        rc = errno;
        ERROR("Failed to open '%s': %s", TE_TUN_DEV, strerror(rc));
        return TE_OS_RC(TE_TA_UNIX, rc);
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    if (ioctl(fd, TUNSETIFF, &ifr) < 0)
    {
        rc = errno;
        ERROR("Prepare TAP interface '%s': %s", ifname, strerror(rc));
        goto out;
    }

    if (ioctl(fd, TUNSETPERSIST, 0) < 0)
    {
        rc = errno;
        ERROR("Failed to remove iface '%s': %s", ifname, strerror(rc));
        goto out;
    }

    SLIST_REMOVE(&taps, tap, tap_entry, links);
    tap_entry_free(tap);

out:
    close(fd);

    return TE_OS_RC(TE_TA_UNIX, rc);
}

static te_errno
tap_list(unsigned int gid, const char *oid, const char *sub_id, char **list)
{
    struct tap_entry *tap;
    te_string te_str = TE_STRING_INIT;
    te_bool first = true;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    SLIST_FOREACH(tap, &taps, links)
    {
        if (!rcf_pch_rsrc_accessible("/agent:%s/interface:%s",
                                     ta_name, tap->name))
            continue;

        te_string_append(&te_str, "%s%s", first ? "" : " ", tap->name);
        first = false;
    }

    *list = te_str.ptr;

    return 0;
}

RCF_PCH_CFG_NODE_RW_COLLECTION(node_tap, "tap", NULL, NULL,
                               NULL, NULL, tap_add, tap_del, tap_list, NULL);

/* See the description in conf_rule.h */
te_errno
ta_unix_conf_tap_init(void)
{
    te_errno rc;

    rc = rcf_pch_add_node("/agent/", &node_tap);
    if (rc != 0)
        return rc;

    return rcf_pch_rsrc_info("/agent/tap",
                             rcf_pch_rsrc_grab_dummy,
                             rcf_pch_rsrc_release_dummy);
}
#else
/* See the description in conf_rule.h */
te_errno
ta_unix_conf_tap_init(void)
{
   INFO("TAP interface configuration is not supported");

   return 0;
}
#endif /* HAVE_FCNTL_H && HAVE_LINUX_IF_TUN_H && HAVE_SYS_IOCTL_H */
