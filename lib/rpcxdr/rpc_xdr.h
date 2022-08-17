/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RCF RPC encoding/decoding routines
 *
 * Definition of the API used by RCF RPC to encode/decode RPC data.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_RPC_XDR_H__
#define __TE_RPC_XDR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "te_errno.h"
#include "tarpc.h"

#ifdef HAVE_RPC_TYPES_H
#include <rpc/types.h>
#endif

/** Usual RPC buffer length */
#define RCF_RPC_BUF_LEN          2048

/**
 * Huge RPC buffer length - if encode fails with this buffer length,
 * it's assumed that error is not related to lack of space.
 */
#define RCF_RPC_HUGE_BUF_LEN     (20 * 1024 * 1024)

/** Maximum length of the RPC */
#define RCF_RPC_MAX_NAME        64

typedef bool_t (*rpc_func)(void *in, void *out, void *rqstp);
typedef bool_t (*rpc_arg_func)(void *xdrs, void *arg);

/** Information about the RPC function */
typedef struct {
    const char   *name;    /**< Name of RPC function, i.g. "bind" */
    rpc_func      rpc;     /**< Address of the RPC function */
    rpc_arg_func  in;      /**< Address of input argument encoder/decoder */
    int           in_len;  /**< Size of the input argument structure */
    rpc_arg_func  out;     /**< Address of output argument encoder/decoder */
    int           out_len; /**< Size of the output argument structure */
} rpc_info;

/** RPC functions table; generated automatically
 *  @note This will be soon moved to another library
 */
extern rpc_info tarpc_functions[];

/**
 * Find information corresponding to RPC function by its name.
 *
 * @param name  base function name
 *
 * @return function information structure address or NULL
 */
extern rpc_info *rpc_find_info(const char *name);

/**
 * Encode RPC call with specified name.
 *
 * @param name          RPC name
 * @param buf           buffer for encoded data
 * @param buflen        length of the buf (IN) / length of the data (OUT)
 * @param objp          input parameters structure, for example
 *                      pointer to structure tarpc_bind_in
 *
 * @return Status code
 * @retval TE_ENOENT       No such function
 * @retval TE_ESUNRPC   Buffer is too small or another encoding error
 *                      ocurred
 */
extern int rpc_xdr_encode_call(const char *name, void *buf, size_t *buflen,
                               void *objp);

/**
 * Encode RPC result.
 *
 * @param name          RPC name
 * @param buf           buffer for encoded data
 * @param buflen        length of the buf (IN) / length of the (OUT)
 * @param rc            value returned by RPC
 * @param objp          output parameters structure, for example
 *                      pointer to structure tarpc_bind_out
 *
 * @return Status code
 * @retval TE_ESUNRPC   Buffer is too small or another encoding error
 *                      ocurred
 */
extern int rpc_xdr_encode_result(const char *name, te_bool rc,
                                 void *buf, size_t *buflen, void *objp);


/**
 * Decode RPC call.
 *
 * @param buf     buffer with encoded data
 * @param buflen  length of the data
 * @param name    RPC name location (length >= RCF_RPC_MAX_NAME)
 * @param objp    C structure for input parameters to be allocated
 *                and filled
 *
 * @return Status code
 */
extern int rpc_xdr_decode_call(void *buf, size_t buflen,
                               char *name, void **objp);

/**
 * Decode RPC result.
 *
 * @param name    RPC name
 * @param buf     buffer with encoded data
 * @param buflen  length of the data
 * @param objp    C structure for input parameters to be filled
 *
 * @return Status code (if rc attribute of result is FALSE, an error should
 *         be returned)
 */
extern int rpc_xdr_decode_result(const char *name, void *buf,
                                 size_t buflen, void *objp);


/**
 * Decode only the common part of the RPC call
 *
 * @param buf     buffer with encoded data
 * @param buflen  length of the data
 * @param name    RPC name location (length >= RCF_RPC_MAX_NAME)
 * @param common  Decode common input parameters
 * @note Unlike rpc_xdr_decode_call(), this is a pointer to memory
 * provided by the caller, not allocated by the function
 *
 * @return Status code
 */
extern te_errno rpc_xdr_inspect_call(const void *buf, size_t buflen,
                                     char *name, struct tarpc_in_arg *common);

/**
 * Decode only the common part of the RPC result.
 *
 * @param buf     buffer with encoded data
 * @param buflen  length of the data
 * @param common  Decode common output parameters
 *
 * @return Status code (if rc attribute of result is FALSE, an error should
 *         be returned)
 */
extern te_errno rpc_xdr_inspect_result(const void *buf, size_t buflen,
                                       struct tarpc_out_arg *common);



/**
 * Free RPC C structure.
 *
 * @param func  XDR function
 * @param objp  C structure pointer
 */
extern void rpc_xdr_free(rpc_arg_func func, void *objp);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_RPC_XDR_H__ */
