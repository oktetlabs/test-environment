/** @file
 * @brief RCF RPC encoding/decoding routines
 *
 * Definition of the API used by RCF RPC to encode/decode RPC data.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_RPC_XDR_H__
#define __TE_RPC_XDR_H__

#ifdef __cplusplus
extern "C" {
#endif

/** Usual RPC buffer length */
#define RCF_RPC_BUF_LEN          2048

/** 
 * Huge RPC buffer length - if encode fails with this buffer length, 
 * it's assumed that error is not related to lack of space.
 */
#define RCF_RPC_HUGE_BUF_LEN     (1024 * 1024)

/** Maximum length of the RPC */
#define RCF_RPC_MAX_NAME        64

typedef te_bool (*rpc_func)(void *xdrs,...);

/** Information about the RPC function */
typedef struct {
    char    *name;    /**< Name of RPC function, i.g. "bind" */
    rpc_func rpc;     /**< Address of the RPC function */
    rpc_func in;      /**< Address of input argument encoder/decoder */
    int      in_len;  /**< Size of the input argument structure */
    rpc_func out;     /**< Address of output argument encoder/decoder */
    int      out_len; /**< Size of the output argument structure */
} rpc_info;

/** RPC functions table; generated automatically */
extern rpc_info rpc_functions[];

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
 * @retval ENOENT       No such function
 * @retval ETESUNRPC    Buffer is too small or another encoding error
 *                      ocurred
 */
extern int rpc_xdr_encode_call(const char *name, char *buf, int *buflen, 
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
 * @retval ETESUNRPC    Buffer is too small or another encoding error
 *                      ocurred
 */
extern int rpc_xdr_encode_result(char *name, te_bool rc, 
                                 char *buf, int *buflen, void *objp);


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
extern int rpc_xdr_decode_call(char *buf, int buflen,
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
extern int rpc_xdr_decode_result(const char *name, char *buf, 
                                 int buflen, void *objp);

/**
 * Free RPC C structure.
 *
 * @param func  XDR function
 * @param objp  C structure pointer
 */
extern void rpc_xdr_free(rpc_func func, void *objp);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_RPC_XDR_H__ */
