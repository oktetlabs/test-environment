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
 * @param ta            Test Agent name
 * @param if_name       Network interface name
 * @param rss_context   RSS context
 * @param buf           Key to set
 * @param len           Length of the key in bytes
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_if_rss_hash_key_set_local(
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

/** Maximum length of RSS hash function name */
#define TAPI_CFG_IF_RSS_HFUNC_NAME_LEN 128

/** Information about RSS hash function */
typedef struct tapi_cfg_if_rss_hfunc {
    char name[TAPI_CFG_IF_RSS_HFUNC_NAME_LEN];  /**< Function name */
    te_bool enabled; /**< Whether function is enabled */
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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_CFG_IF_RSS_H__ */
