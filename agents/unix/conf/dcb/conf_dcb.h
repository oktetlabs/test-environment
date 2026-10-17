/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief Unix TA Traffic Control DCB configuration support
 *
 * Registration of ETS and DCBX subtrees for DCB node.
 */

#ifndef __TE_AGENTS_UNIX_CONF_DCB_CONF_DCB_H_
#define __TE_AGENTS_UNIX_CONF_DCB_CONF_DCB_H_

#include "te_defs.h"
#include "te_enum.h"
#include "te_errno.h"
#include "te_vector.h"
#include "rcf_pch_tree.h"

/** Number of Traffic Classes represented by the IEEE 802.1Q. */
#define TA_UNIX_CONF_DCB_MAX_TCS 8

/** Type of DCB data to retrieve or configure. */
typedef enum ta_unix_conf_dcb_type {
     /** IEEE 802.1Qbb Priority-based Flow Control configuration. */
    TA_UNIX_CONF_DCB_TYPE_PFC,
    /** IEEE 802.1Qaz Enhanced Transmission Selection configuration. */
    TA_UNIX_CONF_DCB_TYPE_ETS,
    /** DCBX mode represented by a bitmask of DCB_CAP_DCBX_* flags. */
    TA_UNIX_CONF_DCB_TYPE_DCBX,
} ta_unix_conf_dcb_type;

/** DCB type to string mapping. */
static const te_enum_map ta_unix_conf_dcb_type_map[] = {
    {.name = "PFC",  .value = TA_UNIX_CONF_DCB_TYPE_PFC},
    {.name = "ETS",  .value = TA_UNIX_CONF_DCB_TYPE_ETS},
    {.name = "DCBX", .value = TA_UNIX_CONF_DCB_TYPE_DCBX},
    TE_ENUM_MAP_END
};

/**
 * Retrieve DCB data for a network interface.
 *
 * The type of object referenced by @p data must correspond to @p type:
 * - @ref TA_UNIX_CONF_DCB_TYPE_PFC: pointer to @c ieee_pfc;
 * - @ref TA_UNIX_CONF_DCB_TYPE_ETS: pointer to @c ieee_ets;
 * - @ref TA_UNIX_CONF_DCB_TYPE_DCBX: pointer to @c uint8_t.
 *
 * @p data must point to writable storage large enough for the
 * object selected by @p type.
 *
 * @param[in] if_name   Interface name.
 * @param[in] type      Type of DCB data to retrieve.
 * @param[out] data     Destination for the retrieved data.
 *
 * @return Status code.
 * @retval 0             Success.
 * @retval TE_EINVAL     Invalid argument or DCB data type.
 * @retval TE_ENOTCONN   Netlink socket is not available.
 * @retval TE_EOPNOTSUPP DCB is not supported by the interface, or
 *                       the driver has no DCBX getter (for
 *                       @ref TA_UNIX_CONF_DCB_TYPE_DCBX).
 * @retval TE_ENODEV     The interface does not exist.
 * @retval TE_ENODATA    The driver provides no data of @p type: its getter
 *                       is absent or failed.
 * @retval TE_EPROTO     Unexpected response.
 * @retval Other         Error returned by the kernel, converted from errno.
 */
extern te_errno ta_unix_conf_dcb_data_get(const char *if_name,
                                          ta_unix_conf_dcb_type type,
                                          void *data);

/**
 * Retrieve DCB data for a network interface, asking the driver once per
 * configuration group.
 *
 * The first call for @p if_name and @p type in a group reads the data
 * from the driver (see ta_unix_conf_dcb_data_get()); later calls in the
 * same group return the same data and status. A Configurator
 * synchronization is one group, so all the values it reads come from
 * one snapshot. A request outside a group is a group of its own.
 *
 * @param[in] gid       Group identifier.
 * @param[in] if_name   Interface name.
 * @param[in] type      Type of DCB data to retrieve.
 * @param[out] data     Destination for the retrieved data.
 *
 * @return Status code, as for ta_unix_conf_dcb_data_get().
 */
extern te_errno ta_unix_conf_dcb_data_get_group(unsigned int gid,
                                                const char *if_name,
                                                ta_unix_conf_dcb_type type,
                                                void *data);

/**
 * Configure DCB data for a network interface.
 *
 * The type of object referenced by @p data must correspond to @p type:
 * - @ref TA_UNIX_CONF_DCB_TYPE_PFC: pointer to @c ieee_pfc;
 * - @ref TA_UNIX_CONF_DCB_TYPE_ETS: pointer to @c ieee_ets;
 * - @ref TA_UNIX_CONF_DCB_TYPE_DCBX: pointer to @c uint8_t containing a
 *   bitmask of @c DCB_CAP_DCBX_* flags.
 *
 * @param[in] if_name   Network interface name.
 * @param[in] type      Type of DCB data to configure.
 * @param[in] data      DCB data to apply.
 *
 * @return Status code.
 * @retval 0             Success.
 * @retval TE_EINVAL     Invalid argument, data, or DCB data type.
 * @retval TE_ENOTCONN   Netlink socket is not available.
 * @retval TE_EOPNOTSUPP Requested DCB feature is not supported.
 * @retval TE_EPROTO     The response has no driver status.
 * @retval TE_EFAIL      DCBX setter failed with a non-errno status.
 * @retval Other         Error returned by the kernel or by the driver
 *                       setter, converted from errno.
 */
extern te_errno ta_unix_conf_dcb_data_set(const char *if_name,
                                          ta_unix_conf_dcb_type type,
                                          const void *data);

/**
 * Get the list of instances of a singleton node backed by DCB data.
 *
 * The node is listed (with an empty instance name) if DCB data of
 * @p type can be retrieved for the interface. It is not listed if
 * the interface does not support DCB, is gone, or its driver provides
 * no data of @p type (the kernel does not allow to distinguish
 * a missing driver getter from a failed one). On other errors the node
 * is listed, so that getting its value reports the error.
 *
 * The data is read with ta_unix_conf_dcb_data_get_group().
 *
 * @param[in] ctx       Context of the list request.
 * @param[in] type      Type of DCB data backing the node.
 * @param[out] names    Vector to append the instance names to.
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_dcb_list(const ta_conf_ctx *ctx,
                                      ta_unix_conf_dcb_type type,
                                      te_vec *names);

/**
 * Parse an instance name of an indexed DCB collection (Traffic Class or
 * priority).
 *
 * @param[in]  instance  Instance name.
 * @param[out] index     Parsed index, from @c 0 to
 *                       @c TA_UNIX_CONF_DCB_MAX_TCS - 1.
 *
 * @return Status code.
 * @retval 0          Success.
 * @retval TE_EINVAL  Not a number, or out of range.
 */
extern te_errno ta_unix_conf_dcb_index_parse(const char *instance,
                                             unsigned int *index);

/**
 * List the fixed instances (@c 0 to @c 7) of an indexed DCB collection:
 * the list handler of the ETS and PFC arrays.
 */
extern ta_conf_list_fn ta_unix_conf_dcb_index_list;

/**
 * Register the DCB (PFC + ETS + DCBX) configuration subtrees.
 *
 * The netlink socket is opened on the first DCB request.
 *
 * @return Status code.
 */
extern te_errno ta_unix_conf_dcb_init(void);

/**
 * Release the resources of the DCB configuration subtrees: close the
 * netlink socket. The subtrees stay registered.
 */
extern void ta_unix_conf_dcb_fini(void);

#endif /* __TE_AGENTS_UNIX_CONF_DCB_CONF_DCB_H_ */
