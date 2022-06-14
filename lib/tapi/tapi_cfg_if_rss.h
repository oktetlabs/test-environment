/** @file
 * @brief Basic Configuration Model TAPI
 *
 * Definition of test API for Network Interface RSS settings
 * (storage/cm/cm_base.xml).
 *
 *
 * Copyright (C) 2022 OKTET Labs. All rights reserved.
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_TAPI_CFG_IF_RSS_H__ */
