/** @file
 * @brief RCF RPC definitions
 *
 * Definition of functions to create/destroy RPC servers on Test
 * Agents and to set/get RPC server context parameters.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_RCF_RPC_DEFS_H__
#define __TE_RCF_RPC_DEFS_H__

#include <stdio.h>
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include "te_errno.h"


/** RPC errno values */
typedef enum {
    RPC_EOK = 0,
    RPC_EPERM = TE_RC(TE_RPC, TE_EPERM),
    RPC_ENOENT = TE_RC(TE_RPC, TE_ENOENT),
    RPC_ESRCH = TE_RC(TE_RPC, TE_ESRCH),
    RPC_EINTR = TE_RC(TE_RPC, TE_EINTR),
    RPC_EIO = TE_RC(TE_RPC, TE_EIO),
    RPC_ENXIO = TE_RC(TE_RPC, TE_ENXIO),
    RPC_E2BIG = TE_RC(TE_RPC, TE_E2BIG),
    RPC_ENOEXEC = TE_RC(TE_RPC, TE_ENOEXEC),
    RPC_EBADF = TE_RC(TE_RPC, TE_EBADF),
    RPC_ECHILD = TE_RC(TE_RPC, TE_ECHILD),
    RPC_EAGAIN = TE_RC(TE_RPC, TE_EAGAIN),
    RPC_ENOMEM = TE_RC(TE_RPC, TE_ENOMEM),
    RPC_EACCES = TE_RC(TE_RPC, TE_EACCES),
    RPC_EFAULT = TE_RC(TE_RPC, TE_EFAULT),
    RPC_ENOTBLK = TE_RC(TE_RPC, TE_ENOTBLK),
    RPC_EBUSY = TE_RC(TE_RPC, TE_EBUSY),
    RPC_EEXIST = TE_RC(TE_RPC, TE_EEXIST),
    RPC_EXDEV = TE_RC(TE_RPC, TE_EXDEV),
    RPC_ENODEV = TE_RC(TE_RPC, TE_ENODEV),
    RPC_ENOTDIR = TE_RC(TE_RPC, TE_ENOTDIR),
    RPC_EISDIR = TE_RC(TE_RPC, TE_EISDIR),
    RPC_EINVAL = TE_RC(TE_RPC, TE_EINVAL),
    RPC_ENFILE = TE_RC(TE_RPC, TE_ENFILE),
    RPC_EMFILE = TE_RC(TE_RPC, TE_EMFILE),
    RPC_ENOTTY = TE_RC(TE_RPC, TE_ENOTTY),
    RPC_ETXTBSY = TE_RC(TE_RPC, TE_ETXTBSY),
    RPC_EFBIG = TE_RC(TE_RPC, TE_EFBIG),
    RPC_ENOSPC = TE_RC(TE_RPC, TE_ENOSPC),
    RPC_ESPIPE = TE_RC(TE_RPC, TE_ESPIPE),
    RPC_EROFS = TE_RC(TE_RPC, TE_EROFS),
    RPC_EMLINK = TE_RC(TE_RPC, TE_EMLINK),
    RPC_EPIPE = TE_RC(TE_RPC, TE_EPIPE),
    RPC_EDOM = TE_RC(TE_RPC, TE_EDOM),
    RPC_ERANGE = TE_RC(TE_RPC, TE_ERANGE),
    RPC_EDEADLK = TE_RC(TE_RPC, TE_EDEADLK),
    RPC_ENAMETOOLONG = TE_RC(TE_RPC, TE_ENAMETOOLONG),
    RPC_ENOLCK = TE_RC(TE_RPC, TE_ENOLCK),
    RPC_ENOSYS = TE_RC(TE_RPC, TE_ENOSYS),
    RPC_ENOTEMPTY = TE_RC(TE_RPC, TE_ENOTEMPTY),
    RPC_ELOOP = TE_RC(TE_RPC, TE_ELOOP),
    RPC_EWOULDBLOCK = TE_RC(TE_RPC, TE_EWOULDBLOCK),
    RPC_ENOMSG = TE_RC(TE_RPC, TE_ENOMSG),
    RPC_EIDRM = TE_RC(TE_RPC, TE_EIDRM),
    RPC_ECHRNG = TE_RC(TE_RPC, TE_ECHRNG),
    RPC_EL2NSYNC = TE_RC(TE_RPC, TE_EL2NSYNC),
    RPC_EL3HLT = TE_RC(TE_RPC, TE_EL3HLT),
    RPC_EL3RST = TE_RC(TE_RPC, TE_EL3RST),
    RPC_ELNRNG = TE_RC(TE_RPC, TE_ELNRNG),
    RPC_EUNATCH = TE_RC(TE_RPC, TE_EUNATCH),
    RPC_ENOCSI = TE_RC(TE_RPC, TE_ENOCSI),
    RPC_EL2HLT = TE_RC(TE_RPC, TE_EL2HLT),
    RPC_EBADE = TE_RC(TE_RPC, TE_EBADE),
    RPC_EBADR = TE_RC(TE_RPC, TE_EBADR),
    RPC_EXFULL = TE_RC(TE_RPC, TE_EXFULL),
    RPC_ENOANO = TE_RC(TE_RPC, TE_ENOANO),
    RPC_EBADRQC = TE_RC(TE_RPC, TE_EBADRQC),
    RPC_EBADSLT = TE_RC(TE_RPC, TE_EBADSLT),
    RPC_EDEADLOCK = TE_RC(TE_RPC, TE_EDEADLOCK),
    RPC_EBFONT = TE_RC(TE_RPC, TE_EBFONT),
    RPC_ENOSTR = TE_RC(TE_RPC, TE_ENOSTR),
    RPC_ENODATA = TE_RC(TE_RPC, TE_ENODATA),
    RPC_ETIME = TE_RC(TE_RPC, TE_ETIME),
    RPC_ENOSR = TE_RC(TE_RPC, TE_ENOSR),
    RPC_ENONET = TE_RC(TE_RPC, TE_ENONET),
    RPC_ENOPKG = TE_RC(TE_RPC, TE_ENOPKG),
    RPC_EREMOTE = TE_RC(TE_RPC, TE_EREMOTE),
    RPC_ENOLINK = TE_RC(TE_RPC, TE_ENOLINK),
    RPC_EADV = TE_RC(TE_RPC, TE_EADV),
    RPC_ESRMNT = TE_RC(TE_RPC, TE_ESRMNT),
    RPC_ECOMM = TE_RC(TE_RPC, TE_ECOMM),
    RPC_EPROTO = TE_RC(TE_RPC, TE_EPROTO),
    RPC_EMULTIHOP = TE_RC(TE_RPC, TE_EMULTIHOP),
    RPC_EDOTDOT = TE_RC(TE_RPC, TE_EDOTDOT),
    RPC_EBADMSG = TE_RC(TE_RPC, TE_EBADMSG),
    RPC_EOVERFLOW = TE_RC(TE_RPC, TE_EOVERFLOW),
    RPC_ENOTUNIQ = TE_RC(TE_RPC, TE_ENOTUNIQ),
    RPC_EBADFD = TE_RC(TE_RPC, TE_EBADFD),
    RPC_EREMCHG = TE_RC(TE_RPC, TE_EREMCHG),
    RPC_ELIBACC = TE_RC(TE_RPC, TE_ELIBACC),
    RPC_ELIBBAD = TE_RC(TE_RPC, TE_ELIBBAD),
    RPC_ELIBSCN = TE_RC(TE_RPC, TE_ELIBSCN),
    RPC_ELIBMAX = TE_RC(TE_RPC, TE_ELIBMAX),
    RPC_ELIBEXEC = TE_RC(TE_RPC, TE_ELIBEXEC),
    RPC_EILSEQ = TE_RC(TE_RPC, TE_EILSEQ),
    RPC_ERESTART = TE_RC(TE_RPC, TE_ERESTART),
    RPC_ESTRPIPE = TE_RC(TE_RPC, TE_ESTRPIPE),
    RPC_EUSERS = TE_RC(TE_RPC, TE_EUSERS),
    RPC_ENOTSOCK = TE_RC(TE_RPC, TE_ENOTSOCK),
    RPC_EDESTADDRREQ = TE_RC(TE_RPC, TE_EDESTADDRREQ),
    RPC_EMSGSIZE = TE_RC(TE_RPC, TE_EMSGSIZE),
    RPC_EPROTOTYPE = TE_RC(TE_RPC, TE_EPROTOTYPE),
    RPC_ENOPROTOOPT = TE_RC(TE_RPC, TE_ENOPROTOOPT),
    RPC_EPROTONOSUPPORT = TE_RC(TE_RPC, TE_EPROTONOSUPPORT),
    RPC_ESOCKTNOSUPPORT = TE_RC(TE_RPC, TE_ESOCKTNOSUPPORT),
    RPC_EOPNOTSUPP = TE_RC(TE_RPC, TE_EOPNOTSUPP),
    RPC_EPFNOSUPPORT = TE_RC(TE_RPC, TE_EPFNOSUPPORT),
    RPC_EAFNOSUPPORT = TE_RC(TE_RPC, TE_EAFNOSUPPORT),
    RPC_EADDRINUSE = TE_RC(TE_RPC, TE_EADDRINUSE),
    RPC_EADDRNOTAVAIL = TE_RC(TE_RPC, TE_EADDRNOTAVAIL),
    RPC_ENETDOWN = TE_RC(TE_RPC, TE_ENETDOWN),
    RPC_ENETUNREACH = TE_RC(TE_RPC, TE_ENETUNREACH),
    RPC_ENETRESET = TE_RC(TE_RPC, TE_ENETRESET),
    RPC_ECONNABORTED = TE_RC(TE_RPC, TE_ECONNABORTED),
    RPC_ECONNRESET = TE_RC(TE_RPC, TE_ECONNRESET),
    RPC_ENOBUFS = TE_RC(TE_RPC, TE_ENOBUFS),
    RPC_EISCONN = TE_RC(TE_RPC, TE_EISCONN),
    RPC_ENOTCONN = TE_RC(TE_RPC, TE_ENOTCONN),
    RPC_ESHUTDOWN = TE_RC(TE_RPC, TE_ESHUTDOWN),
    RPC_ETOOMANYREFS = TE_RC(TE_RPC, TE_ETOOMANYREFS),
    RPC_ETIMEDOUT = TE_RC(TE_RPC, TE_ETIMEDOUT),
    RPC_ECONNREFUSED = TE_RC(TE_RPC, TE_ECONNREFUSED),
    RPC_EHOSTDOWN = TE_RC(TE_RPC, TE_EHOSTDOWN),
    RPC_EHOSTUNREACH = TE_RC(TE_RPC, TE_EHOSTUNREACH),
    RPC_EALREADY = TE_RC(TE_RPC, TE_EALREADY),
    RPC_EINPROGRESS = TE_RC(TE_RPC, TE_EINPROGRESS),
    RPC_ESTALE = TE_RC(TE_RPC, TE_ESTALE),
    RPC_EUCLEAN = TE_RC(TE_RPC, TE_EUCLEAN),
    RPC_ENOTNAM = TE_RC(TE_RPC, TE_ENOTNAM),
    RPC_ENAVAIL = TE_RC(TE_RPC, TE_ENAVAIL),
    RPC_EISNAM = TE_RC(TE_RPC, TE_EISNAM),
    RPC_EREMOTEIO = TE_RC(TE_RPC, TE_EREMOTEIO),
    RPC_EDQUOT = TE_RC(TE_RPC, TE_EDQUOT),
    RPC_ENOMEDIUM = TE_RC(TE_RPC, TE_ENOMEDIUM),
    RPC_EMEDIUMTYPE = TE_RC(TE_RPC, TE_EMEDIUMTYPE),
    RPC_ECANCELED = TE_RC(TE_RPC, TE_ECANCELED),
    RPC_ERPCNOTSUPP = TE_RC(TE_RPC, TE_ERPCNOTSUPP),
    
    RPC_E_UNEXP_NET_ERR = TE_RC(TE_RPC, TE_E_UNEXP_NET_ERR),
    RPC_E_WAIT_TIMEOUT = TE_RC(TE_RPC, TE_E_WAIT_TIMEOUT),
    RPC_E_IO_INCOMPLETE = TE_RC(TE_RPC, TE_E_IO_INCOMPLETE),
    RPC_E_IO_PENDING = TE_RC(TE_RPC, TE_E_IO_PENDING),
    
    RPC_EUNKNOWN = TE_RC(TE_RPC, TE_EUNKNOWN),
} rpc_errno;

/** Check, if errno is RPC errno, not other TE errno */
#define RPC_IS_ERRNO_RPC(_errno) \
    (((_errno) == 0) || (TE_RC_GET_MODULE(_errno) == TE_RPC))

/**
 * Coverts system native constant to its mirror in RPC namespace
 */
#define H2RPC(name_) \
    case name_: return RPC_ ## name_

#define RPC2H(name_) \
    case RPC_ ## name_: return name_

#define RPC2STR(name_) \
    case RPC_ ## name_: return #name_

/** Convert system native errno to RPC errno */
static inline rpc_errno 
errno_h2rpc(int host_errno_val)
{
    return TE_RC(TE_RPC, te_rc_os2te(host_errno_val));
}

/**
 * Convert RPC errno to string.
 *
 * @note If confiversion fails, the function is not reenterable.
 */
static inline const char *
errno_rpc2str(rpc_errno rpc_errno_val)
{
    if (rpc_errno_val == 0 || TE_RC_GET_MODULE(rpc_errno_val) == TE_RPC)
    {
        return te_rc_err2str(rpc_errno_val);
    }
    else
    {
        static char buf[64];

        strcpy(buf, "Non-RPC ");
        strcat(buf, te_rc_mod2str(rpc_errno_val));
        strcat(buf, "-");
        strcat(buf, te_rc_err2str(rpc_errno_val));
        return buf;
    }
}


/** Operations for RPC */
typedef enum {
    RCF_RPC_CALL,       /**< Call non-blocking RPC (if supported) */
    RCF_RPC_IS_DONE,    /**< Check whether non-blocking RPC is done
                             (to be used from rcf_rpc_is_op_done() only) */
    RCF_RPC_WAIT,       /**< Wait until non-blocking RPC is finished */
    RCF_RPC_CALL_WAIT   /**< Call blocking RPC */
} rcf_rpc_op;

#define RCF_RPC_NAME_LEN    32

#define RCF_RPC_MAX_BUF     1048576
#define RCF_RPC_MAX_IOVEC   32
#define RCF_RPC_MAX_CMSGHDR 8

#endif /* !__TE_RCF_RPC_DEFS_H__ */
