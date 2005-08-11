/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of name resolution routines
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>
#include <ctype.h>
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
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#include <stdarg.h>

#include "tapi_rpc_internal.h"
#include "tapi_rpc_netdb.h"


/**
 * Convert hostent received via RPC to the host struct hostent.
 * Memory allocated by RPC library is partially used and rpc_he is
 * updated properly to avoid releasing of re-used memory.
 *
 * @param rpc_he        pointer to the field of RPC out structure
 *
 * @return pointer to struct hostent or NULL is memory allocation failed
 */
static struct hostent *
hostent_rpc2h(tarpc_hostent *rpc_he)
{
    struct hostent *he;

    if ((he = calloc(1, sizeof(*he))) == NULL)
        return NULL;

    if (rpc_he->h_aliases.h_aliases_val != NULL &&
        (he->h_aliases = calloc(rpc_he->h_aliases.h_aliases_len,
                                sizeof(char *))) == NULL)
    {
        free(he);
        return NULL;
    }

    if (rpc_he->h_addr_list.h_addr_list_val != NULL &&
        (he->h_addr_list = calloc(rpc_he->h_addr_list.h_addr_list_len,
                                  sizeof(char *))) == NULL)
    {
        free(he->h_aliases);
        free(he);
    }

    he->h_name = rpc_he->h_name.h_name_val;
    rpc_he->h_name.h_name_val = NULL;
    rpc_he->h_name.h_name_len = 0;

    if (he->h_aliases != NULL)
    {
        unsigned int i;

        for (i = 0; i < rpc_he->h_aliases.h_aliases_len; i++)
        {
            he->h_aliases[i] =
                rpc_he->h_aliases.h_aliases_val[i].name.name_val;
            rpc_he->h_aliases.h_aliases_val[i].name.name_val = NULL;
            rpc_he->h_aliases.h_aliases_val[i].name.name_len = 0;
        }
    }

    if (he->h_addr_list != NULL)
    {
        unsigned int i;

        for (i = 0; i < rpc_he->h_addr_list.h_addr_list_len; i++)
        {
            he->h_addr_list[i] =
                rpc_he->h_addr_list.h_addr_list_val[i].val.val_val;
            rpc_he->h_addr_list.h_addr_list_val[i].val.val_val = NULL;
            rpc_he->h_addr_list.h_addr_list_val[i].val.val_len = 0;
        }
    }

    he->h_length = rpc_he->h_length;
    he->h_addrtype = domain_rpc2h(rpc_he->h_addrtype);

    return he;
}

struct hostent *
rpc_gethostbyname(rcf_rpc_server *rpcs, const char *name)
{
    tarpc_gethostbyname_in  in;
    tarpc_gethostbyname_out out;

    struct hostent *res = NULL;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_PTR(gethostbyname, NULL);
    }

    in.name.name_val = (char *)name;
    in.name.name_len = (name == NULL) ? 0 : (strlen(name) + 1);

    rcf_rpc_call(rpcs, "gethostbyname", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && out.res.res_val != NULL)
    {
        if ((res = hostent_rpc2h(out.res.res_val)) == NULL)
            rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
    }

    TAPI_RPC_LOG("RPC (%s,%s): gethostbyname(%s) -> %p (%s)",
                 rpcs->ta, rpcs->name, name == NULL ? "" : name,
                 res, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_PTR(gethostbyname, res);
}

struct hostent *
rpc_gethostbyaddr(rcf_rpc_server *rpcs,
                  const char *addr, int len, rpc_socket_addr_family type)
{
    tarpc_gethostbyaddr_in  in;
    tarpc_gethostbyaddr_out out;

    struct hostent *res = NULL;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_PTR(gethostbyaddr, NULL);
    }

    in.type = type;

    if (addr != NULL)
    {
        in.addr.val.val_val = (char *)addr;
        in.addr.val.val_len = len;
    }

    rcf_rpc_call(rpcs, "gethostbyaddr",  &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && out.res.res_val != NULL)
    {
        if ((res = hostent_rpc2h(out.res.res_val)) == NULL)
            rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
    }

    TAPI_RPC_LOG("RPC (%s,%s): gethostbyaddr(%p, %d, %d) -> %p (%s)",
                 rpcs->ta, rpcs->name, addr, len, type,
                 res, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_PTR(gethostbyaddr, res);
}

/**
 * Convert RPC addrinfo to the host native one.
 *
 * @param rpc_ai        RPC addrinfo structure
 * @param ai            pre-allocated host addrinfo structure
 *
 * @return 0 in the case of success or -1 in the case of memory allocation
 * failure
 */
static int
ai_rpc2h(struct tarpc_ai *ai_rpc, struct addrinfo *ai)
{
    ai->ai_flags = ai_flags_rpc2h(ai_rpc->flags);
    ai->ai_family = domain_rpc2h(ai_rpc->family);
    ai->ai_socktype = socktype_rpc2h(ai_rpc->socktype);
    ai->ai_protocol = proto_rpc2h(ai_rpc->protocol);
    ai->ai_addrlen = ai_rpc->addrlen + SA_COMMON_LEN;

    if (ai_rpc->addr.sa_data.sa_data_val != NULL)
    {
        if ((ai->ai_addr = calloc(1, ai_rpc->addr.sa_data.sa_data_len +
                                  SA_COMMON_LEN)) == NULL)
        {
            return -1;
        }
        ai->ai_addr->sa_family = addr_family_rpc2h(ai_rpc->addr.sa_family);
        memcpy(ai->ai_addr->sa_data, ai_rpc->addr.sa_data.sa_data_val,
               ai_rpc->addr.sa_data.sa_data_len);
    }

    if (ai_rpc->canonname.canonname_val != NULL)
    {
        ai->ai_canonname = ai_rpc->canonname.canonname_val;
        ai_rpc->canonname.canonname_val = NULL;
        ai_rpc->canonname.canonname_len = 0;
    }

    return 0;
}


int
rpc_getaddrinfo(rcf_rpc_server *rpcs,
                const char *node, const char *service,
                const struct addrinfo *hints,
                struct addrinfo **res)
{
    tarpc_getaddrinfo_in  in;
    tarpc_getaddrinfo_out out;

    struct addrinfo *list = NULL;
    struct tarpc_ai  rpc_hints;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(getaddrinfo, -1);
    }

    if (node != NULL)
    {
        in.node.node_val = (char *)node;
        in.node.node_len = strlen(node) + 1;
    }

    if (service != NULL)
    {
        in.service.service_val = (char *)service;
        in.service.service_len = strlen(service) + 1;
    }

    if (hints != NULL)
    {
        rpc_hints.flags = ai_flags_h2rpc(hints->ai_flags);
        rpc_hints.family = addr_family_h2rpc(hints->ai_family);
        rpc_hints.socktype = socktype_h2rpc(hints->ai_socktype);
        rpc_hints.protocol = proto_h2rpc(hints->ai_protocol);
        rpc_hints.addrlen = hints->ai_addrlen - SA_COMMON_LEN;

        if (hints->ai_addr != NULL)
        {
            rpc_hints.addr.sa_family =
                addr_family_h2rpc(hints->ai_addr->sa_family);
            rpc_hints.addr.sa_data.sa_data_val = hints->ai_addr->sa_data;
            rpc_hints.addr.sa_data.sa_data_len =
                hints->ai_addrlen > SA_COMMON_LEN ?
                hints->ai_addrlen - SA_COMMON_LEN : 0;
        }

        if (hints->ai_canonname != NULL)
        {
            rpc_hints.canonname.canonname_val = hints->ai_canonname;
            rpc_hints.canonname.canonname_len =
                strlen(hints->ai_canonname) + 1;
        }

        in.hints.hints_val = &rpc_hints;
        in.hints.hints_len = 1;
    }

    rcf_rpc_call(rpcs, "getaddrinfo", &in, &out);

    if (RPC_IS_CALL_OK(rpcs) && out.res.res_val != NULL)
    {
        int i;

        if ((list = calloc(1, out.res.res_len * sizeof(*list) +
                           sizeof(int))) == NULL)
        {
            rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
            RETVAL_INT(getaddrinfo, -1);
        }
        *(int *)list = out.mem_ptr;
        list = (struct addrinfo *)((int *)list + 1);

        for (i = 0; i < (int)out.res.res_len; i++)
        {
            if (ai_rpc2h(out.res.res_val, list + i) < 0)
            {
                for (i--; i >= 0; i--)
                {
                    free(list[i].ai_addr);
                    free(list[i].ai_canonname);
                }
                free((int *)list - 1);
                rpcs->_errno = TE_RC(TE_RCF, TE_ENOMEM);
                RETVAL_INT(getaddrinfo, -1);
            }
            list[i].ai_next = (i == (int)out.res.res_len - 1) ?
                                  NULL : list + i + 1;
        }
        *res = list;
    }

    CHECK_RETVAL_VAR_IS_ZERO_OR_MINUS_ONE(getaddrinfo, out.retval);

    TAPI_RPC_LOG("RPC (%s,%s): getaddrinfo(%s, %s, %p, %p) -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 node, service, hints, res,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(getaddrinfo, out.retval);
}

void
rpc_freeaddrinfo(rcf_rpc_server *rpcs,
                 struct addrinfo *res)
{
    tarpc_freeaddrinfo_in  in;
    tarpc_freeaddrinfo_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_VOID(freeaddrinfo);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;

    if (res != NULL)
        in.mem_ptr = *((int *)res - 1);

    rcf_rpc_call(rpcs, "freeaddrinfo",  &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): freeaddrinfo(%p) -> (%s)",
                 rpcs->ta, rpcs->name, res,
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    free((int *)res - 1);

    RETVAL_VOID(freeaddrinfo);
}
