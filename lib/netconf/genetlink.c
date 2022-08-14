/** @file
 * @brief Device management using netconf library
 *
 * Implementation of internal API for using Generic Netlink.
 *
 * Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "Netconf genetlink"

#include "config.h"
#include "te_config.h"
#include "netconf.h"
#include "netconf_internal.h"
#include "netconf_internal_genetlink.h"
#include "logger_api.h"

#if HAVE_LINUX_GENETLINK_H

#include "linux/genetlink.h"

/* Callback for obtaining devlink family ID from message attribute */
static te_errno
family_attr_cb(struct nlattr *na, void *cb_data)
{
    uint16_t val;
    te_errno rc;
    int *family = (int *)cb_data;

    if (na->nla_type == CTRL_ATTR_FAMILY_ID)
    {
        rc = netconf_get_uint16_attr(na, &val);
        if (rc != 0)
            return rc;

        *family = val;
    }

    return 0;
}

/* Callback for processing netlink message containing devlink family ID */
static int
family_msg_cb(struct nlmsghdr *h, netconf_list *list, void *cookie)
{
    te_errno rc;

    UNUSED(list);
    UNUSED(cookie);

    rc = netconf_gn_process_attrs(h, family_attr_cb, cookie);
    if (rc != 0)
        return -1;

    return 0;
}

#endif /* HAVE_LINUX_GENETLINK_H */

/* See description in netconf_internal_genetlink.h */
te_errno
netconf_gn_process_attrs(struct nlmsghdr *h,
                         netconf_attr_cb cb,
                         void *cb_data)
{
#ifndef HAVE_LINUX_GENETLINK_H
    UNUSED(h);
    UNUSED(cb);
    UNUSED(cb_data);

    return TE_ENOENT;
#else
    return netconf_process_hdr_attrs(h, GENL_HDRLEN,
                                     cb, cb_data);
#endif
}

/* See description in netconf_internal_genetlink.h */
te_errno
netconf_gn_init_hdrs(void *req, size_t max_len, uint16_t nlmsg_type,
                     uint16_t nlmsg_flags, uint8_t cmd, uint8_t version,
                     netconf_handle nh)
{
#ifndef HAVE_LINUX_GENETLINK_H
    UNUSED(req);
    UNUSED(max_len);
    UNUSED(nlmsg_type);
    UNUSED(nlmsg_flags);
    UNUSED(cmd);
    UNUSED(version);
    UNUSED(nh);

    return TE_ENOENT;
#else
    struct nlmsghdr *h;
    struct genlmsghdr *gh;

    if (max_len < NLMSG_LENGTH(GENL_HDRLEN))
    {
        ERROR("%s(): not enough space for headers", __FUNCTION__);
        return TE_ENOBUFS;
    }

    h = (struct nlmsghdr *)req;

    memset(h, 0, sizeof(*h));
    h->nlmsg_type = nlmsg_type;
    h->nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    h->nlmsg_flags = nlmsg_flags;
    h->nlmsg_seq = ++nh->seq;

    gh = NLMSG_DATA(h);
    memset(gh, 0, GENL_HDRLEN);
    gh->cmd = cmd;
    gh->version = version;

    return 0;
#endif
}

/* See description in netconf_internal_genetlink.h */
te_errno
netconf_gn_get_family(netconf_handle nh, const char *family_name,
                      uint16_t *family_id)
{
#ifndef HAVE_LINUX_GENETLINK_H
    UNUSED(nh);
    UNUSED(family_name);
    UNUSED(family_id);

    return TE_ENOENT;
#else
    char req[NETCONF_MAX_REQ_LEN] = { 0, };
    te_errno rc;
    int os_rc;
    struct nlmsghdr *h;
    int family = -1;

    h = (struct nlmsghdr *)req;

    rc = netconf_gn_init_hdrs(req, sizeof(req), GENL_ID_CTRL, NLM_F_REQUEST,
                              CTRL_CMD_GETFAMILY, 0x1, nh);
    if (rc != 0)
        return rc;

    rc = netconf_append_attr(req, sizeof(req), CTRL_ATTR_FAMILY_NAME,
                             family_name, strlen(family_name) + 1);
    if (rc != 0)
        return rc;

    os_rc = netconf_talk(nh, req, h->nlmsg_len, family_msg_cb,
                         &family, NULL);
    if (os_rc < 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("%s(): failed to obtain generic netlink family ID for "
              "'%s', rc=%r", __FUNCTION__, family_name, rc);
        return rc;
    }

    if (family < 0)
    {
        ERROR("%s(): family ID was not found for '%s'", __FUNCTION__,
              family_name);
        return TE_ENOENT;
    }

    *family_id = family;
    return 0;
#endif
}
