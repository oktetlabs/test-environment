/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RCF RPC encoding/decoding routines
 *
 * Implementation of routines used by RCF RPC to encode/decode RPC data.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WINDOWS
/*
 * It's not possible to define it in config.h and include it explicitly
 * because a lot of shit like HAVE_UNISTD_H is defined there automatically.
 */
#define x_int32_arg_t long
#define xdr_int16_t xdr_short
#define xdr_int32_t xdr_int
#define xdr_int8_t xdr_char
#define xdr_uint16_t xdr_u_short
#define xdr_uint32_t xdr_u_int
#define xdr_uint64_t xdr_u_int64_t
#define xdr_uint8_t xdr_u_char
#endif

#include <stdio.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <rpc/types.h>
#include <rpc/xdr.h>

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"

#include "rpc_xdr.h"
#ifdef RPC_XML
#include "xml_xdr.h"
#endif

/**
 * Find information corresponding to RPC function by its name.
 *
 * @param name  base function name
 *
 * @return function information structure address or NULL
 */
rpc_info *
rpc_find_info(const char *name)
{
    int i;

    for (i = 0; tarpc_functions[i].name != NULL; i++)
        if (strcmp(name, tarpc_functions[i].name) == 0)
            return tarpc_functions + i;

    return NULL;
}

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
int
rpc_xdr_encode_call(const char *name, void *buf, size_t *buflen, void *objp)
{
    XDR xdrs;

    rpc_info *info;
    uint32_t  len;

    if ((info = rpc_find_info(name)) == NULL)
        return TE_RC(TE_RCF_RPC, TE_ENOENT);

    len = strlen(name) + 1;

#ifdef RPC_XML
    xdrxml_create(&xdrs, buf, *buflen, rpc_xml_call,
                  TRUE, name, XDR_ENCODE);
    /* Put header */
#else
    /* Encode routine name */
    xdrmem_create(&xdrs, buf, *buflen, XDR_ENCODE);
    xdrs.x_ops->x_putint32(&xdrs, (x_int32_arg_t *)&len);
    xdrs.x_ops->x_putbytes(&xdrs, (caddr_t)name, len);
#endif

    /* Encode argument */
    if (!info->in(&xdrs, objp))
        return TE_RC(TE_RCF_RPC, TE_ESUNRPC);

#ifdef RPC_XML
    xdrxml_free(&xdrs);
    *buflen = strlen(buf) + 1;
#else
    *buflen = xdrs.x_ops->x_getpostn(&xdrs);
#endif

    return 0;
}

static te_errno
decode_result_start(XDR *xdrp, const void *buf, size_t buflen)
{
    x_int32_arg_t   rc;
#ifdef RPC_XML
    xdrxml_create(xdrp, (void *)buf, buflen, rpc_xml_result,
                  TRUE, name, XDR_DECODE);
    /* Decode  - how can we get the routine name? */
    rc = xdrxml_return_code(xdrp);
#else
    /* Decode rc */
    xdrmem_create(xdrp, (void *)buf, buflen, XDR_DECODE);
    xdrp->x_ops->x_getint32(xdrp, &rc);
#endif

    if (rc == 0)
        return TE_RC(TE_RCF_RPC, TE_ESUNRPC);
    return 0;
}

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
int
rpc_xdr_decode_result(const char *name, void *buf, size_t buflen,
                      void *objp)
{
    te_errno rc;
    XDR xdrs;
    rpc_info *info;

    rc = decode_result_start(&xdrs, buf, buflen);
    if (rc != 0)
        return rc;

    if ((info = rpc_find_info(name)) == NULL)
        return TE_RC(TE_RCF_RPC, TE_ENOENT);

    /* Decode argument */
    if (!info->out(&xdrs, objp))
        return TE_RC(TE_RCF_RPC, TE_ESUNRPC);

#ifdef RPC_XML
    xdrxml_free(&xdrs);
#endif

    return 0;
}

/**
 * Free RPC C structure.
 *
 * @param func  XDR function
 * @param objp  C structure pointer
 */
void
rpc_xdr_free(rpc_arg_func func, void *objp)
{
    XDR xdrs;

    xdrmem_create(&xdrs, NULL, 0, XDR_FREE);
    func(&xdrs, objp);
}

#define XML_CALL_PREFIX         "<call name=\""
#define XML_CALL_PREFIX_LEN     strlen(XML_CALL_PREFIX)

static te_errno
decode_call_start(XDR *xdrp, char *name, const void *buf, size_t buflen)
{
#ifdef RPC_XML
    char     *tmp;
    int       n;

    xdrxml_create(xdrp, (void *)buf, buflen, rpc_xml_call, TRUE,
                  name, XDR_DECODE);
    if (strncmp(XML_CALL_PREFIX, buf, XML_CALL_PREFIX_LEN) != 0 ||
        (tmp = strchr(buf + XML_CALL_PREFIX_LEN, '"')) == NULL ||
        (n = (tmp - buf) - XML_CALL_PREFIX_LEN) >= RCF_RPC_MAX_NAME)
    {
        return TE_RC(TE_RCF_RPC, TE_ESUNRPC);
    }
    memcpy(name, buf + XML_CALL_PREFIX_LEN, n);
    name[n] = 0;
#else
    x_int32_arg_t len = 0;

    /* Decode routine name */
    xdrmem_create(xdrp, (void *)buf, buflen, XDR_DECODE);
    if (!xdrp->x_ops->x_getint32(xdrp, &len))
        return TE_RC(TE_RCF_RPC, TE_ESUNRPC);
    if (!xdrp->x_ops->x_getbytes(xdrp, name, len))
        return TE_RC(TE_RCF_RPC, TE_ESUNRPC);
#endif
    return 0;
}


/**
 * Decode RPC call.
 *
 * @param buf      buffer with encoded data
 * @param buflen   length of the data
 * @param name     RPC name location
 * @param objp_p   location for C structure for input parameters to be
 *                 allocated and filled
 *
 * @return Status code
 */
int
rpc_xdr_decode_call(void *buf, size_t buflen, char *name, void **objp_p)
{
    XDR xdrs;
    te_errno rc;

    void     *objp;
    rpc_info *info;

    rc = decode_call_start(&xdrs, name, buf, buflen);
    if (rc != 0)
        return rc;

    if ((info = rpc_find_info(name)) == NULL)
    {
        printf("%s %d: Cannot find info for %s\n",
               __FUNCTION__, __LINE__, name);
        return TE_RC(TE_RCF_RPC, TE_ENOENT);
    }

    /* Allocate memory for the argument */
    if ((objp = calloc(1, info->in_len)) == NULL)
    {
        return TE_RC(TE_RCF_RPC, TE_ENOMEM);
    }

    /* Encode argument */
    if (!info->in(&xdrs, objp))
    {
        free(objp);
        return TE_RC(TE_RCF_RPC, TE_ESUNRPC);
    }
#ifdef RPC_XML
    xdrxml_free(&xdrs);
#endif

    *objp_p = objp;

    return 0;
}

/**
 * Encode RPC result.
 *
 * @param name          RPC name
 * @param buf           buffer for encoded data
 * @param buflen        length of the buf (IN) / length of the data (OUT)
 * @param rc            value returned by RPC
 * @param objp          output parameters structure, for example
 *                      pointer to structure tarpc_bind_out
 *
 * @return Status code
 * @retval TE_ESUNRPC   Buffer is too small or another encoding error
 *                      ocurred
 */
int
rpc_xdr_encode_result(const char *name, te_bool rc,
                      void *buf, size_t *buflen, void *objp)
{
    XDR xdrs;

    rpc_info *info;

    if ((info = rpc_find_info(name)) == NULL)
    {
        printf("%s %d: Cannot find info for %s\n",
               __FUNCTION__, __LINE__, name);
        rc = FALSE;
    }

#ifdef RPC_XML
    xdrxml_create(&xdrs, buf, *buflen, rpc_xml_result,
                  rc, name, XDR_ENCODE);
#else
    xdrmem_create(&xdrs, buf, *buflen, XDR_ENCODE);
    /* Encode return code */
    {
        x_int32_arg_t   rc_int32 = rc;

        xdrs.x_ops->x_putint32(&xdrs, &rc_int32);
    }
#endif
    /* Encode argument */
    if (rc && !info->out(&xdrs, objp))
        return TE_RC(TE_RCF_RPC, TE_ESUNRPC);

#ifdef RPC_XML
    *buflen = strlen(buf) + 1;
    xdrxml_free(&xdrs);
#else
    *buflen = xdrs.x_ops->x_getpostn(&xdrs);
#endif
    return 0;
}

te_errno
rpc_xdr_inspect_call(const void *buf, size_t buflen, char *name,
                     struct tarpc_in_arg *common)
{
    XDR xdrs;
    te_errno rc = 0;

    rc = decode_call_start(&xdrs, name, buf, buflen);
    if (rc != 0)
        return rc;

    if (!xdr_tarpc_in_arg(&xdrs, common))
        return TE_RC(TE_RCF_RPC, TE_ESUNRPC);

#ifdef RPC_XML
    xdrxml_free(&xdrs);
#endif

    return rc;
}

te_errno
rpc_xdr_inspect_result(const void *buf, size_t buflen,
                       struct tarpc_out_arg *common)
{
    XDR xdrs;
    te_errno rc = 0;

    rc = decode_result_start(&xdrs, buf, buflen);
    if (rc != 0)
        return rc;

    if (!xdr_tarpc_out_arg(&xdrs, common))
        return TE_RC(TE_RCF_RPC, TE_ESUNRPC);

#ifdef RPC_XML
    xdrxml_free(&xdrs);
#endif

    return rc;
}
