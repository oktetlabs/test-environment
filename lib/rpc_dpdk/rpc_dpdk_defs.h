/** @file
 * @brief DPDK RPC definitions
 *
 * DPDK RPC related definitions
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Igor Romanov <Igor.Romanov@oktetlabs.ru>
 */

#ifndef __RPC_DPDK_DEFS_H__
#define __RPC_DPDK_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define RPC_RTE_ETH_NAME_MAX_LEN 64

#define RPC_RTE_RETA_GROUP_SIZE 64

#define RPC_RSS_HASH_KEY_LEN_DEF 40

#define RPC_RTE_EPOLL_PER_THREAD -1


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __RPC_DPDK_DEFS_H__ */
