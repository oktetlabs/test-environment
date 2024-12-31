/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI RPC for BPF
 *
 * Definitions of TAPI RPC for BPF
 */

#ifndef __TE_TAPI_RPC_BPF_H__
#define __TE_TAPI_RPC_BPF_H__

#include "te_config.h"

#include "te_defs.h"
#include "rcf_rpc.h"
#include "te_rpc_bpf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create UMEM.
 *
 * @param rpcs          RPC server
 * @param umem_area     Memory allocated for UMEM
 * @param size          Size of the memory
 * @param config        UMEM configuration parameters
 *                      (if @c NULL, default values will be used by libxdp)
 * @param umem          RPC pointer to created UMEM
 *
 * @return @c 0 on success, @c -1 on failure.
 */
extern int rpc_xsk_umem__create(rcf_rpc_server *rpcs, rpc_ptr umem_area,
                                uint64_t size,
                                tarpc_xsk_umem_config *config,
                                rpc_ptr *umem);

/**
 * Destroy UMEM.
 *
 * @param rpcs          RPC server
 * @param umem          RPC pointer to UMEM
 *
 * @return @c 0 on success, @c -1 on failure.
 */
extern int rpc_xsk_umem__delete(rcf_rpc_server *rpcs, rpc_ptr umem);

/**
 * Create AF_XDP socket.
 *
 * @param rpcs          RPC server
 * @param if_name       Interface name
 * @param queue_id      Rx queue id
 * @param umem          RPC pointer to UMEM
 * @param shared_umem   If @c true, UMEM is shared with other sockets
 * @param config        Configuration parameters
 * @param sock          RPC pointer to created socket structure on TA
 *
 * @return Nonnegative socket FD on success, @c -1 on failure.
 */
extern int rpc_xsk_socket__create(rcf_rpc_server *rpcs, const char *if_name,
                                  uint32_t queue_id, rpc_ptr umem,
                                  bool shared_umem,
                                  tarpc_xsk_socket_config *config,
                                  rpc_ptr *sock);

/**
 * Destroy AF_XDP socket on TA.
 *
 * @param rpcs      RPC server
 * @param sock      RPC pointer to socket structure on TA
 *
 * @return @c 0 on success, @c -1 on failure.
 */
extern int rpc_xsk_socket__delete(rcf_rpc_server *rpcs, rpc_ptr sock);

/**
 * Set an entry of XSK map to FD of AF_XDP socket.
 *
 * @note This operation cannot be done via Configurator by editing
 *       BPF map entries in configuration tree. XSKMAP is not supported
 *       like other map types by Linux kernel. Configurator cannot even
 *       list values of its entries because map lookup does not work.
 *       The set values are file descriptors which are valid only in the
 *       RPC server which created them, not in TA process handling
 *       Configuration tree changes.
 *
 * @sa rpc_bpf_obj_get()
 *
 * @param rpcs      RPC server
 * @param map_fd    File descriptor of the map
 * @param key       Key in the map
 * @param fd        Value to set (nonnegative value means FD of
 *                  AF_XDP socket; negative value means "remove existing
 *                  entry")
 *
 * @return @c 0 on success, @c -1 on failure.
 */
extern int rpc_xsk_map_set(rcf_rpc_server *rpcs, int map_fd,
                           unsigned int key, int fd);

/**
 * Add specified number of frame buffers to FILL ring of UMEM associated
 * with a given AF_XDP socket. It will enable receiving data into these
 * buffers.
 *
 * @param rpcs        RPC server
 * @param umem        RPC pointer to UMEM
 * @param if_name     Interface name
 * @param queue_id    Rx queue id
 * @param frames_cnt  Number of frame buffers
 *
 * @return Number of added buffers on success, @c -1 on failure.
 */
extern ssize_t rpc_xsk_rx_fill_simple(rcf_rpc_server *rpcs, rpc_ptr umem,
                                      const char *if_name,
                                      uint32_t queue_id,
                                      size_t frames_cnt);

/**
 * Obtain a single packet from RX ring of AF_XDP socket.
 *
 * @note This function is meant to be used with rpc_xsk_rx_fill_simple().
 *       After reading data from frame buffer, the buffer is again added
 *       to FILL ring so that it can be reused.
 *
 * @param rpcs        RPC server
 * @param sock        RPC pointer to socket structure on TA
 * @param buf         Buffer for received data
 * @param len         Buffer length
 *
 * @return Actual number of received bytes on success, @c -1 on failure.
 */
extern ssize_t rpc_xsk_receive_simple(rcf_rpc_server *rpcs, rpc_ptr sock,
                                      void *buf, size_t len);

/**
 * Send a single packet from AF_XDP socket.
 *
 * @note This function chooses for sending one of frame buffers
 *       not added to FILL ring by rpc_xsk_rx_fill_simple().
 *       After adding packet buffer to TX ring, it waits until
 *       completion can be read from COMPLETION ring, and marks
 *       frame buffer as free for future use.
 *
 * @param rpcs        RPC server
 * @param sock        RPC pointer to socket structure on TA
 * @param buf         Buffer with data to send
 * @param len         Data length
 *
 * @return @c 0 on success, @c -1 on failure.
 */
extern int rpc_xsk_send_simple(rcf_rpc_server *rpcs, rpc_ptr sock,
                               void *buf, size_t len);

/**
 * Open a file in BPF file system, obtain BPF object file descriptor.
 *
 * @note This function can be used to obtain from RPC process FD of XSK map
 *       pinned via configuration tree from TA process.
 *
 * @sa rpc_xsk_map_set()
 *
 * @param rpcs        RPC server
 * @param path        Path to BPF object file
 *
 * @return nonnegative FD on success, @c -1 on failure.
 */
extern int rpc_bpf_obj_get(rcf_rpc_server *rpcs, const char *path);

/**
 * Pin BPF map to a file in BPFFS in TA process via configuration tree
 * (if it is not pinned yet; otherwise use the currently pinned file).
 * Then open that file with bpf_obj_get() to obtain FD of the map in RPC
 * server process.
 *
 * @param rpcs        RPC server
 * @param bpf_id      BPF object ID
 * @param map_name    BPF map name
 * @param fd          Location for obtained file descriptor
 *
 * @return Status code.
 */
extern te_errno tapi_bpf_map_pin_get_fd(rcf_rpc_server *rpcs,
                                        unsigned int bpf_id,
                                        const char *map_name, int *fd);
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_RPC_BPF_H__ */
