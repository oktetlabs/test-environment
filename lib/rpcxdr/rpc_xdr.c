/** @file
 * @brief RCF RPC encoding/decoding routines
 *
 * Implementation of routines used by RCF RPC to encode/decode RPC data.
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in the
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

#include "te_config.h"

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
    
    for (i = 0; rpc_functions[i].name != NULL; i++)
        if (strcmp(name, rpc_functions[i].name) == 0)
            return rpc_functions + i;
        
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
    xdrs.x_ops->x_putint32(&xdrs, (int32_t *)&len);
    xdrs.x_ops->x_putbytes(&xdrs, name, len);
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
    XDR xdrs;
    
    int32_t rc;
    
    rpc_info *info;

#ifdef RPC_XML    
    xdrxml_create(&xdrs, buf, buflen, rpc_xml_result, 
                  TRUE, name, XDR_DECODE);
    /* Decode  - how can we get the routine name? */
#else    
    /* Decode rc */
    xdrmem_create(&xdrs, buf, buflen, XDR_DECODE);
    xdrs.x_ops->x_getint32(&xdrs, &rc);
    
    if (rc == 0)
        return TE_RC(TE_RCF_RPC, TE_ESUNRPC);
#endif

    if ((info = rpc_find_info(name)) == NULL)
        return TE_RC(TE_RCF_RPC, TE_ENOENT);
    
    /* Decode argument */
    if (!info->out(&xdrs, objp))
        return TE_RC(TE_RCF_RPC, TE_ESUNRPC);
#ifdef RPC_XML
    rc = xdrxml_return_code(&xdrs);
    xdrxml_free(&xdrs);
    if (rc == FALSE)
        return TE_RC(TE_RCF_RPC, TE_ESUNRPC);
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
rpc_xdr_free(rpc_func func, void *objp)
{
    XDR xdrs;

    xdrmem_create(&xdrs, NULL, 0, XDR_FREE);
    func(&xdrs, objp);
}

#define XML_CALL_PREFIX         "<call name=\""
#define XML_CALL_PREFIX_LEN     strlen(XML_CALL_PREFIX)


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
    
    void     *objp;
    rpc_info *info;

#ifdef RPC_XML    
    char     *tmp;
    int       n;

    xdrxml_create(&xdrs, buf, buflen, rpc_xml_call, TRUE, name, XDR_DECODE);
    if (strncmp(XML_CALL_PREFIX, buf, XML_CALL_PREFIX_LEN) != 0 ||
        (tmp = strchr(buf + XML_CALL_PREFIX_LEN, '"')) == NULL ||
        (n = (tmp - buf) - XML_CALL_PREFIX_LEN) >= RCF_RPC_MAX_NAME)
    {
        return TE_ESUNRPC;
    }
    memcpy(name, buf + XML_CALL_PREFIX_LEN, n);
    name[n] = 0;
#else   
    uint32_t len = 0;
    
    /* Decode routine name */
#define BYTE(i) (unsigned int)(((char *)buf)[i])
    xdrmem_create(&xdrs, buf, buflen, XDR_DECODE);
    xdrs.x_ops->x_getint32(&xdrs, (int32_t *)&len);
    xdrs.x_ops->x_getbytes(&xdrs, name, len);
#endif

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
rpc_xdr_encode_result(char *name, te_bool rc, 
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
        int32_t rc_int32 = rc;
        
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
