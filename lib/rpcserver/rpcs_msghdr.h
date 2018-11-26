/** @file
 * @brief API for processing struct msghdr in RPC calls
 *
 * API for processing struct msghdr in RPC calls
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#ifndef __RPCSERVER_RPCS_MSGHDR_H__
#define __RPCSERVER_RPCS_MSGHDR_H__

#include "rpc_server.h"

/**
 * Helper structure used when converting tarpc_msghdr to struct msghdr
 * and vice versa.
 */
typedef struct rpcs_msghdr_helper {
    uint8_t                  *addr_data;        /**< Pointer to address
                                                     buffer */
    struct sockaddr          *addr;             /**< Pointer to address
                                                     to be placed in
                                                     msg_name */
    socklen_t                 addr_len;         /**< Value to set for
                                                     msg_namelen */

    int                       orig_msg_flags;   /**< Original value of
                                                     msg_flags */
    uint8_t                  *orig_control;     /**< Where original content
                                                     of msg_control is
                                                     stored */
    size_t                    orig_controllen;  /**< Original value of
                                                     msg_controllen */
} rpcs_msghdr_helper;

/**
 * Convert tarpc_msghdr to struct msghdr (@b rpcs_msghdr_h2tarpc() should
 * be used for reverse conversion after RPC call).
 *
 * @param recv_call   Pass @c TRUE if receive function call is prepared,
 *                    @c FALSE otherwise.
 * @param tarpc_msg   tarpc_msghdr value to convert.
 * @param helper      Helper structure storing auxiliary data for
 *                    converted value.
 * @param msg         Where to save converted value.
 * @param arglist     Pointer to list of variable-length arguments which
 *                    are checked after target function call (to ensure
 *                    that the target function changes only what it is
 *                    supposed to).
 * @param name_fmt    Format string for base name for arguments which will
 *                    be added to @p arglist ("msg", "msgs[3]", etc).
 * @param ...         Format string arguments.
 *
 * @return Status code.
 *
 * @sa rpcs_msghdr_h2tarpc, rpcs_msghdr_helper_clean
 */
extern te_errno rpcs_msghdr_tarpc2h(
                               te_bool recv_call,
                               const struct tarpc_msghdr *tarpc_msg,
                               rpcs_msghdr_helper *helper,
                               struct msghdr *msg,
                               checked_arg_list *arglist,
                               const char *name_fmt, ...)
                                  __attribute__((format(printf, 6, 7)));

/**
 * Convert struct msghdr back to tarpc_msghdr (i.e. this function should
 * be used after @b rpcs_msghdr_tarpc2h()).
 *
 * @param msg         msghdr to convert.
 * @param helper      Helper structure passed to @b rpcs_msghdr_tarpc2h()
 *                    for this @p msg.
 * @param tarpc_msg   Where to save converted value.
 *
 * @return Status code.
 *
 * @sa rpcs_msghdr_tarpc2h
 */
extern te_errno rpcs_msghdr_h2tarpc(const struct msghdr *msg,
                                    const rpcs_msghdr_helper *helper,
                                    struct tarpc_msghdr *tarpc_msg);

/**
 * Release memory allocated for rpcs_msghdr_helper and struct msghdr after
 * calling @b rpcs_msghdr_tarpc2h().
 *
 * @param h         Pointer to rpcs_msghdr_helper.
 * @param msg       Pointer to struct msghdr.
 *
 * @sa rpcs_msghdr_tarpc2h
 */
extern void rpcs_msghdr_helper_clean(rpcs_msghdr_helper *h,
                                     struct msghdr *msg);

/**
 * Convert array of tarpc_mmsghdr structures to array of
 * mmsghdr structures.
 *
 * @note Use @b rpcs_mmsghdrs_helpers_clean() to release memory
 *       after calling this function.
 *
 * @param recv_call     Pass @c TRUE if receive function call is prepared,
 *                      @c FALSE otherwise.
 * @param tarpc_mmsgs   Pointer to array of tarpc_mmsghdr structures.
 * @param num           Number of elements in @p tarpc_mmsgs.
 * @param helpers       Where to save pointer to array of helper structures
 *                      storing auxiliary data for converted values.
 * @param mmsgs         Where to save pointer to array of mmsghdr
 *                      structures.
 * @param arglist       Pointer to list of variable-length arguments which
 *                      are checked after target function call (to ensure
 *                      that the target function changes only what it is
 *                      supposed to).
 *
 * @return Status code.
 *
 * @sa rpcs_mmsghdrs_h2tarpc, rpcs_mmsghdrs_helpers_clean
 */
extern te_errno rpcs_mmsghdrs_tarpc2h(te_bool recv_call,
                                      const tarpc_mmsghdr *tarpc_mmsgs,
                                      unsigned int num,
                                      rpcs_msghdr_helper **helpers,
                                      struct mmsghdr **mmsgs,
                                      checked_arg_list *arglist);

/**
 * Convert array of mmsghdr structures back to array of
 * tarpc_mmsghdr structures.
 *
 * @note This function should be used after @b rpcs_mmsghdrs_tarpc2h().
 *
 * @param mmsgs         Pointer to array of mmsghdr structures.
 * @param helpers       Pointer to array of helper structures storing
 *                      auxiliary data for converted values.
 * @param tarpc_mmsgs   Pointer to array of tarpc_mmsghdr structures.
 * @param num           Number of elements in arrays.
 *
 * @return Status code.
 *
 * @sa rpcs_mmsghdrs_tarpc2h, rpcs_mmsghdrs_helpers_clean
 */
extern te_errno rpcs_mmsghdrs_h2tarpc(const struct mmsghdr *mmsgs,
                                      const rpcs_msghdr_helper *helpers,
                                      struct tarpc_mmsghdr *tarpc_mmsgs,
                                      unsigned int num);

/**
 * Release memory allocated for arrays after calling
 * @b rpcs_mmsghdrs_tarpc2h().
 *
 * @param helpers         Pointer to array of rpcs_msghdr_helper structures.
 * @param mmsgs           Pointer to array of mmsghdr structures.
 * @param num             Number of elements in arrays.
 */
extern void rpcs_mmsghdrs_helpers_clean(rpcs_msghdr_helper *helpers,
                                        struct mmsghdr *mmsgs,
                                        unsigned int num);

#endif /* __RPCSERVER_RPCS_MSGHDR_H__ */