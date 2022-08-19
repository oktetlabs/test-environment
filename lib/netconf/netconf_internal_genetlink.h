/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Device management using netconf library
 *
 * Declarations for internal API to work with Generic Netlink.
 *
 * Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __NETCONF_INTERNAL_GENETLINK_H__
#define __NETCONF_INTERNAL_GENETLINK_H__

#include "config.h"
#include "te_config.h"
#include "netconf.h"
#include "netconf_internal.h"

/**
 * Get Generic Netlink family ID. The same module can have different
 * IDs on different hosts, so it is required to obtain its family ID
 * before accessing it via Generic Netlink.
 *
 * @param nh            Netconf handle
 * @param family_name   Family name
 * @param family_id     Where to save requested family ID
 *
 * @return Status code.
 */
te_errno netconf_gn_get_family(netconf_handle nh, const char *family_name,
                               uint16_t *family_id);

/**
 * Process attributes of Generic Netlink message.
 *
 * @param h         Pointer to message header
 * @param cb        Callback for processing attributes
 * @param cb_data   User data passed to the callback
 *
 * @return Status code.
 */
extern te_errno netconf_gn_process_attrs(struct nlmsghdr *h,
                                         netconf_attr_cb cb,
                                         void *cb_data);

/**
 * Initialize headers for Generic Netlink message.
 *
 * @param req           Pointer to the buffer where to initialize headers
 * @param max_len       Maximum space available in the buffer
 * @param nlmsg_type    Message type (usually netlink family ID is passed
 *                      here)
 * @param nlmsg_flags   Netlink flags
 * @param cmd           Command ID
 * @param version       Family-specific version
 * @param nh            Netconf handle
 *
 * @return Status code.
 */
extern te_errno netconf_gn_init_hdrs(void *req, size_t max_len,
                                     uint16_t nlmsg_type,
                                     uint16_t nlmsg_flags, uint8_t cmd,
                                     uint8_t version, netconf_handle nh);

#endif /* __NETCONF_INTERNAL_GENETLINK_H__ */
