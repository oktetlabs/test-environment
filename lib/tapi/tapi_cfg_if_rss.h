/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Basic Configuration Model TAPI
 *
 * Definition of test API for Network Interface RSS settings
 * (storage/cm/cm_base.xml).
 *
 *
 * Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_CFG_IF_RSS_H__
#define __TE_TAPI_CFG_IF_RSS_H__

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Flow types whose RSS hash fields can be configured. */
typedef enum tapi_cfg_if_rss_flow {
    /** TCP over IPv4. */
    TAPI_CFG_IF_RSS_FLOW_TCP4,
    /** UDP over IPv4. */
    TAPI_CFG_IF_RSS_FLOW_UDP4,
    /** SCTP over IPv4. */
    TAPI_CFG_IF_RSS_FLOW_SCTP4,
    /** AH or ESP over IPv4. */
    TAPI_CFG_IF_RSS_FLOW_AH_ESP4,
    /** TCP over IPv6. */
    TAPI_CFG_IF_RSS_FLOW_TCP6,
    /** UDP over IPv6. */
    TAPI_CFG_IF_RSS_FLOW_UDP6,
    /** SCTP over IPv6. */
    TAPI_CFG_IF_RSS_FLOW_SCTP6,
    /** AH or ESP over IPv6. */
    TAPI_CFG_IF_RSS_FLOW_AH_ESP6,
} tapi_cfg_if_rss_flow;

/**
 * Fields that may be included in RSS hash of a flow type (flags of
 * a bitmask).
 */
typedef enum tapi_cfg_if_rss_field {
    /** Destination MAC address. */
    TAPI_CFG_IF_RSS_FIELD_BIT_L2DA = (1 << 0),
    /** VLAN tag. */
    TAPI_CFG_IF_RSS_FIELD_BIT_VLAN = (1 << 1),
    /** L3 protocol number. */
    TAPI_CFG_IF_RSS_FIELD_BIT_L3_PROTO = (1 << 2),
    /** Source IP address. */
    TAPI_CFG_IF_RSS_FIELD_BIT_IP_SRC = (1 << 3),
    /** Destination IP address. */
    TAPI_CFG_IF_RSS_FIELD_BIT_IP_DST = (1 << 4),
    /**
     * Bytes @c 0 and @c 1 of L4 header: the source port of TCP, UDP
     * and SCTP.
     */
    TAPI_CFG_IF_RSS_FIELD_BIT_L4_SRC = (1 << 5),
    /**
     * Bytes @c 2 and @c 3 of L4 header: the destination port of TCP,
     * UDP and SCTP.
     */
    TAPI_CFG_IF_RSS_FIELD_BIT_L4_DST = (1 << 6),
    /** GTP tunnel endpoint ID. */
    TAPI_CFG_IF_RSS_FIELD_BIT_GTP_TEID = (1 << 7),
    /** IPv6 flow label. */
    TAPI_CFG_IF_RSS_FIELD_BIT_IP6_FL = (1 << 8),
    /** Do not hash, discard all the packets of the flow type. */
    TAPI_CFG_IF_RSS_FIELD_BIT_DISCARD = (1 << 9),
} tapi_cfg_if_rss_field;

/**
 * Get number of available RX queues.
 *
 * @param ta            Test Agent name
 * @param if_name       Network interface name
 * @param rx_queues     Where to save number of RX queues
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rss_rx_queues_get(
                                    const char *ta,
                                    const char *if_name,
                                    int *rx_queues);

/**
 * Get current RSS hash key.
 *
 * @param ta            Test Agent name
 * @param if_name       Network interface name
 * @param rss_context   RSS context
 * @param buf           Where to save pointer to the key
 *                      (should be released by the caller)
 * @param len           Length of the key in bytes
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rss_hash_key_get(const char *ta,
                                             const char *if_name,
                                             unsigned int rss_context,
                                             uint8_t **buf, size_t *len);

/**
 * Set RSS hash key (change should be committed with
 * tapi_cfg_if_rss_hash_indir_commit()).
 *
 * @param ta            Test Agent name.
 * @param if_name       Network interface name.
 * @param rss_context   RSS context.
 * @param buf           Key to set.
 * @param len           Length of the key in bytes (you can use
 *                      tapi_cfg_if_rss_hash_key_get() to find out
 *                      supported key length).
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rss_hash_key_set_local(
                                             const char *ta,
                                             const char *if_name,
                                             unsigned int rss_context,
                                             const uint8_t *buf, size_t len);

/**
 * Set RSS hash key.
 *
 * @param ta            Test Agent name
 * @param if_name       Network interface name
 * @param rss_context   RSS context
 * @param buf           Key to set
 * @param len           Length of the key in bytes
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rss_hash_key_set(
                                             const char *ta,
                                             const char *if_name,
                                             unsigned int rss_context,
                                             const uint8_t *buf, size_t len);

/**
 * Set current size of RSS indirection table.
 *
 * @param ta            Test Agent name
 * @param if_name       Network interface name
 * @param rss_context   RSS context
 * @param size          Where to save table size
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rss_indir_table_size(const char *ta,
                                                 const char *if_name,
                                                 unsigned int rss_context,
                                                 unsigned int *size);

/**
 * Get value stored in a entry of RSS indirection table.
 *
 * @param ta            Test Agent name
 * @param if_name       Network interface name
 * @param rss_context   RSS context
 * @param idx           Index of the entry
 * @param val           Where to save obtained value
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rss_indir_get(const char *ta,
                                          const char *if_name,
                                          unsigned int rss_context,
                                          unsigned int idx,
                                          int *val);

/**
 * Set value stored in a entry of RSS indirection table
 * (change should be committed with
 * tapi_cfg_if_rss_hash_indir_commit()).
 *
 * @param ta            Test Agent name
 * @param if_name       Network interface name
 * @param rss_context   RSS context
 * @param idx           Index of the entry
 * @param val           Value to set
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rss_indir_set_local(
                                          const char *ta,
                                          const char *if_name,
                                          unsigned int rss_context,
                                          unsigned int idx,
                                          int val);

/**
 * Request resetting indirection table of default RSS context
 * to default state (reset will happen after
 * tapi_cfg_if_rss_hash_indir_commit() is called).
 *
 * @param ta            Test Agent name
 * @param if_name       Network interface name
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rss_indir_default_set_local(
                                              const char *ta,
                                              const char *if_name);

/**
 * Request resetting indirection table of default RSS context
 * to default state.
 *
 * @param ta            Test Agent name
 * @param if_name       Network interface name
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rss_indir_default_set(
                                              const char *ta,
                                              const char *if_name);

/**
 * Commit changes related to hash key, hash functions and
 * indirection table.
 *
 * @param ta            Test Agent name
 * @param if_name       Network interface name
 * @param rss_context   RSS context
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rss_hash_indir_commit(const char *ta,
                                                  const char *if_name,
                                                  unsigned int rss_context);

/**
 * Fill RSS hash indirection table by assigning to all its entries
 * queues from [@p queue_from, @p queue_to]. Made changes should be
 * committed with tapi_cfg_if_rss_hash_indir_commit().
 *
 * Example: if queue_from = 2 and queue_to = 3, then table will look like:
 * entry 0: 2
 * entry 1: 3
 * entry 2: 2
 * entry 3: 3
 * ...
 *
 * @param ta            Test Agent name
 * @param if_name       Network interface name
 * @param rss_context   RSS context
 * @param queue_from    The first RX queue number
 * @param queue_to      The second RX queue number (if it is less than the
 *                      first one, queues are assigned in backwards order)
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rss_fill_indir_table(
                                          const char *ta,
                                          const char *if_name,
                                          unsigned int rss_context,
                                          unsigned int queue_from,
                                          unsigned int queue_to);

/**
 * Print to log RSS hash indirection table.
 *
 * @param ta            Test Agent name
 * @param if_name       Network interface name
 * @param rss_context   RSS context
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rss_print_indir_table(
                                          const char *ta,
                                          const char *if_name,
                                          unsigned int rss_context);

/** Maximum length of RSS hash function name */
#define TAPI_CFG_IF_RSS_HFUNC_NAME_LEN 128

/** Information about RSS hash function */
typedef struct tapi_cfg_if_rss_hfunc {
    char name[TAPI_CFG_IF_RSS_HFUNC_NAME_LEN];  /**< Function name */
    bool enabled; /**< Whether function is enabled */
} tapi_cfg_if_rss_hfunc;

/**
 * Get information about all RSS hash functions for a given interface.
 *
 * @param ta            Test Agent name
 * @param if_name       Network interface name
 * @param rss_context   RSS context
 * @param hfuncs        Where to save pointer to array of
 *                      tapi_cfg_if_rss_hfunc structures
 * @param hfuncs_num    Where to save number of hash functions
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rss_hfuncs_get(
                          const char *ta,
                          const char *if_name,
                          unsigned int rss_context,
                          tapi_cfg_if_rss_hfunc **hfuncs,
                          unsigned int *hfuncs_num);

/**
 * Set locally the state of RSS hash function.
 * This change should be committed with
 * tapi_cfg_if_rss_hash_indir_commit().
 *
 * @param ta            Test Agent name
 * @param if_name       Network interface name
 * @param rss_context   RSS context
 * @param func_name     Function name
 * @param state         @c 0 - disabled,
 *                      @c 1 - enabled
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rss_hfunc_set_local(
                                             const char *ta,
                                             const char *if_name,
                                             unsigned int rss_context,
                                             const char *func_name,
                                             int state);

/**
 * Enable specified hash function, disable all the other functions.
 * This change should be committed with
 * tapi_cfg_if_rss_hash_indir_commit().
 *
 * @param ta            Test Agent name
 * @param if_name       Network interface name
 * @param rss_context   RSS context
 * @param func_name     Function name
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rss_hfunc_set_single_local(
                                             const char *ta,
                                             const char *if_name,
                                             unsigned int rss_context,
                                             const char *func_name);

/**
 * Get the fields included in RSS hash of a flow type.
 *
 * The fields are synchronized with the Test Agent first, so they are
 * the ones the driver reports. Do not call the function between
 * tapi_cfg_if_rss_flow_hash_set_local() and
 * tapi_cfg_if_rss_flow_hash_commit() for the same flow type:
 * the synchronization discards values that are not committed.
 *
 * @param[in]  ta           Test Agent name
 * @param[in]  if_name      Network interface name
 * @param[in]  rss_context  RSS context
 * @param[in]  flow         Flow type
 * @param[out] fields       Bitmask of @ref tapi_cfg_if_rss_field flags
 *                          of the fields included in the hash
 *
 * @return Status code.
 * @retval TE_ENOENT    The interface does not report hash fields for
 *                      the flow type.
 */
extern te_errno tapi_cfg_if_rss_flow_hash_get(const char *ta,
                                              const char *if_name,
                                              unsigned int rss_context,
                                              tapi_cfg_if_rss_flow flow,
                                              unsigned int *fields);

/**
 * Set locally the fields included in RSS hash of a flow type: the fields
 * present in @p fields are included, all the other ones are excluded.
 * This change should be committed with
 * tapi_cfg_if_rss_flow_hash_commit().
 *
 * @param[in] ta            Test Agent name
 * @param[in] if_name       Network interface name
 * @param[in] rss_context   RSS context
 * @param[in] flow          Flow type
 * @param[in] fields        Bitmask of @ref tapi_cfg_if_rss_field flags
 *                          of the fields to include in the hash
 *
 * @return Status code.
 * @retval TE_ENOENT        The interface does not report hash fields for
 *                          the flow type.
 * @retval TE_EOPNOTSUPP    A field of @p fields is not known to
 *                          the Test Agent.
 */
extern te_errno tapi_cfg_if_rss_flow_hash_set_local(
                                              const char *ta,
                                              const char *if_name,
                                              unsigned int rss_context,
                                              tapi_cfg_if_rss_flow flow,
                                              unsigned int fields);

/**
 * Commit changes of the fields included in RSS hash of a flow type, and
 * synchronize them with the Test Agent, so that
 * tapi_cfg_if_rss_flow_hash_get() returns what the driver has applied.
 *
 * @param[in] ta            Test Agent name
 * @param[in] if_name       Network interface name
 * @param[in] rss_context   RSS context
 * @param[in] flow          Flow type
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rss_flow_hash_commit(
                                              const char *ta,
                                              const char *if_name,
                                              unsigned int rss_context,
                                              tapi_cfg_if_rss_flow flow);

/**
 * Set the fields included in RSS hash of a flow type and commit
 * the change (see tapi_cfg_if_rss_flow_hash_set_local() and
 * tapi_cfg_if_rss_flow_hash_commit()).
 *
 * @param[in] ta            Test Agent name
 * @param[in] if_name       Network interface name
 * @param[in] rss_context   RSS context
 * @param[in] flow          Flow type
 * @param[in] fields        Bitmask of @ref tapi_cfg_if_rss_field flags
 *                          of the fields to include in the hash
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rss_flow_hash_set(const char *ta,
                                              const char *if_name,
                                              unsigned int rss_context,
                                              tapi_cfg_if_rss_flow flow,
                                              unsigned int fields);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_CFG_IF_RSS_H__ */
