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
#include "te_errno.h"

/** RPC errno values */
typedef enum {
    RPC_EOK = 0,
    RPC_EPERM =        /**< Operation not permitted */
        (TE_RCF_RPC << TE_RC_MODULE_SHIFT) + 1,
    RPC_ENOENT,        /**< No such file or directory */
    RPC_ESRCH,         /**< No such process */
    RPC_EINTR,         /**< Interrupted system call */
    RPC_EIO,           /**< I/O error */
    RPC_ENXIO,         /**< No such device or address */
    RPC_E2BIG,         /**< Arg list too long */
    RPC_ENOEXEC,       /**< Exec format error */
    RPC_EBADF,         /**< Bad file number */
    RPC_ECHILD,        /**< No child processes */
    RPC_EAGAIN,        /**< Try again */
    RPC_ENOMEM,        /**< Out of memory */
    RPC_EACCES,        /**< Permission denied */
    RPC_EFAULT,        /**< Bad address */
    RPC_ENOTBLK,       /**< Block device required */
    RPC_EBUSY,         /**< Device or resource busy */
    RPC_EEXIST,        /**< File exists */
    RPC_EXDEV,         /**< Cross-device link */
    RPC_ENODEV,        /**< No such device */
    RPC_ENOTDIR,       /**< Not a directory */
    RPC_EISDIR,        /**< Is a directory */
    RPC_EINVAL,        /**< Invalid argument */
    RPC_ENFILE,        /**< File table overflow */
    RPC_EMFILE,        /**< Too many open files */
    RPC_ENOTTY,        /**< Not a typewriter */
    RPC_ETXTBSY,       /**< Text file busy */
    RPC_EFBIG,         /**< File too large */
    RPC_ENOSPC,        /**< No space left on device */
    RPC_ESPIPE,        /**< Illegal seek */
    RPC_EROFS,         /**< Read-only file system */
    RPC_EMLINK,        /**< Too many links */
    RPC_EPIPE,         /**< Broken pipe */
    RPC_EDOM,          /**< Math argument out of domain of func */
    RPC_ERANGE,        /**< Math result not representable */
    RPC_EDEADLK,       /**< Resource deadlock would occur */
    RPC_ENAMETOOLONG,  /**< File name too long */
    RPC_ENOLCK,        /**< No record locks available */
    RPC_ENOSYS,        /**< Function not implemented */
    RPC_ENOTEMPTY,     /**< Directory not empty */
    RPC_ELOOP,         /**< Too many symbolic links encountered */
    RPC_EWOULDBLOCK,   /**< Synonym of EAGAIN */
    RPC_ENOMSG,        /**< No message of desired type */
    RPC_EIDRM,         /**< Identifier removed */
    RPC_ECHRNG,        /**< Channel number out of range */
    RPC_EL2NSYNC,      /**< Level 2 not synchronized */
    RPC_EL3HLT,        /**< Level 3 halted */
    RPC_EL3RST,        /**< Level 3 reset */
    RPC_ELNRNG,        /**< Link number out of range */
    RPC_EUNATCH,       /**< Protocol driver not attached */
    RPC_ENOCSI,        /**< No CSI structure available */
    RPC_EL2HLT,        /**< Level 2 halted */
    RPC_EBADE,         /**< Invalid exchange */
    RPC_EBADR,         /**< Invalid request descriptor */
    RPC_EXFULL,        /**< Exchange full */
    RPC_ENOANO,        /**< No anode */
    RPC_EBADRQC,       /**< Invalid request code */
    RPC_EBADSLT,       /**< Invalid slot */
    RPC_EDEADLOCK,     /**< Synomym of EDEADLK */
    RPC_EBFONT,        /**< Bad font file format */
    RPC_ENOSTR,        /**< Device not a stream */
    RPC_ENODATA,       /**< No data available */
    RPC_ETIME,         /**< Timer expired */
    RPC_ENOSR,         /**< Out of streams resources */
    RPC_ENONET,        /**< Machine is not on the network */
    RPC_ENOPKG,        /**< Package not installed */
    RPC_EREMOTE,       /**< Object is remote */
    RPC_ENOLINK,       /**< Link has been severed */
    RPC_EADV,          /**< Advertise error */
    RPC_ESRMNT,        /**< Srmount error */
    RPC_ECOMM,         /**< Communication error on send */
    RPC_EPROTO,        /**< Protocol error */
    RPC_EMULTIHOP,     /**< Multihop attempted */
    RPC_EDOTDOT,       /**< RFS specific error */
    RPC_EBADMSG,       /**< Not a data message */
    RPC_EOVERFLOW,     /**< Value too large for defined data type */
    RPC_ENOTUNIQ,      /**< Name not unique on network */
    RPC_EBADFD,        /**< File descriptor in bad state */
    RPC_EREMCHG,       /**< Remote address changed */
    RPC_ELIBACC,       /**< Can not access a needed shared library */
    RPC_ELIBBAD,       /**< Accessing a corrupted shared library */
    RPC_ELIBSCN,       /**< .lib section in a.out corrupted */
    RPC_ELIBMAX,       /**< Attempting to link in too many shared libraries */
    RPC_ELIBEXEC,      /**< Cannot exec a shared library directly */
    RPC_EILSEQ,        /**< Illegal byte sequence */
    RPC_ERESTART,      /**< Interrupted system call should be restarted */
    RPC_ESTRPIPE,      /**< Streams pipe error */
    RPC_EUSERS,        /**< Too many users */
    RPC_ENOTSOCK,      /**< Socket operation on non-socket */
    RPC_EDESTADDRREQ,  /**< Destination address required */
    RPC_EMSGSIZE,      /**< Message too long */
    RPC_EPROTOTYPE,    /**< Protocol wrong type for socket */
    RPC_ENOPROTOOPT,   /**< Protocol not available */
    RPC_EPROTONOSUPPORT, /**< Protocol not supported */
    RPC_ESOCKTNOSUPPORT, /**< Socket type not supported */
    RPC_EOPNOTSUPP,    /**< Operation not supported on transport endpoint */
    RPC_EPFNOSUPPORT,  /**< Protocol family not supported */
    RPC_EAFNOSUPPORT,  /**< Address family not supported by protocol */
    RPC_EADDRINUSE,    /**< Address already in use */
    RPC_EADDRNOTAVAIL, /**< Cannot assign requested address */
    RPC_ENETDOWN,      /**< Network is down */
    RPC_ENETUNREACH,   /**< Network is unreachable */
    RPC_ENETRESET,     /**< Network dropped connection because of reset */
    RPC_ECONNABORTED,  /**< Software caused connection abort */
    RPC_ECONNRESET,    /**< Connection reset by peer */
    RPC_ENOBUFS,       /**< No buffer space available */
    RPC_EISCONN,       /**< Transport endpoint is already connected */
    RPC_ENOTCONN,      /**< Transport endpoint is not connected */
    RPC_ESHUTDOWN,     /**< Cannot send after transport endpoint shutdown */
    RPC_ETOOMANYREFS,  /**< Too many references: cannot splice */
    RPC_ETIMEDOUT,     /**< Connection timed out */
    RPC_ECONNREFUSED,  /**< Connection refused */
    RPC_EHOSTDOWN,     /**< Host is down */
    RPC_EHOSTUNREACH,  /**< No route to host */
    RPC_EALREADY,      /**< Operation already in progress */
    RPC_EINPROGRESS,   /**< Operation now in progress */
    RPC_ESTALE,        /**< Stale NFS file handle */
    RPC_EUCLEAN,       /**< Structure needs cleaning */
    RPC_ENOTNAM,       /**< Not a XENIX named type file */
    RPC_ENAVAIL,       /**< No XENIX semaphores available */
    RPC_EISNAM,        /**< Is a named type file */
    RPC_EREMOTEIO,     /**< Remote I/O error */
    RPC_EDQUOT,        /**< Quota exceeded */
    RPC_ENOMEDIUM,     /**< No medium found */
    RPC_EMEDIUMTYPE,   /**< Wrong medium type */
    
    RPC_ERPCNOTSUPP,   /**< RPC is not supported
                            (it does not have host analogue) */
    RPC_EUNKNOWN,      /**< all unknown errno values on the agent are 
                            mapped to this value */
} rpc_errno;

/** Check, if errno ir RPC errno, not other TE errno */
#define RPC_ERRNO_RPC(_errno) \
    ((_errno >= RPC_EPERM) && (_errno <= RPC_EUNKNOWN))

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
    switch (host_errno_val)
    {
        case 0: return RPC_EOK;
        
        H2RPC(EPERM);
        H2RPC(ENOENT);
        H2RPC(ESRCH);
        H2RPC(EINTR);
        H2RPC(EIO);
        H2RPC(ENXIO);
        H2RPC(E2BIG);
        H2RPC(ENOEXEC);
        H2RPC(EBADF);
        H2RPC(ECHILD);
        H2RPC(EAGAIN);
        H2RPC(ENOMEM);
        H2RPC(EACCES);
        H2RPC(EFAULT);
        H2RPC(ENOTBLK);
        H2RPC(EBUSY);
        H2RPC(EEXIST);
        H2RPC(EXDEV);
        H2RPC(ENODEV);
        H2RPC(ENOTDIR);
        H2RPC(EISDIR);
        H2RPC(EINVAL);
        H2RPC(ENFILE);
        H2RPC(EMFILE);
        H2RPC(ENOTTY);
        H2RPC(ETXTBSY);
        H2RPC(EFBIG);
        H2RPC(ENOSPC);
        H2RPC(ESPIPE);
        H2RPC(EROFS);
        H2RPC(EMLINK);
        H2RPC(EPIPE);
        H2RPC(EDOM);
        H2RPC(ERANGE);
        H2RPC(ENAMETOOLONG);
        H2RPC(ENOLCK);
        H2RPC(ENOSYS);
        H2RPC(ENOTEMPTY);
        H2RPC(ELOOP);
        H2RPC(ENOMSG);
        H2RPC(EIDRM);
#ifdef ECHRNG
        H2RPC(ECHRNG);
#endif
#ifdef EL2NSYNC
        H2RPC(EL2NSYNC);
#endif
#ifdef EL3HLT
        H2RPC(EL3HLT);
#endif
#ifdef EL3RST
        H2RPC(EL3RST);
#endif
#ifdef ELNRNG
        H2RPC(ELNRNG);
#endif
#ifdef EUNATCH
        H2RPC(EUNATCH);
#endif
#ifdef ENOCSI
        H2RPC(ENOCSI);
#endif
#ifdef EL2HLT
        H2RPC(EL2HLT);
#endif
#ifdef EBADE
        H2RPC(EBADE);
#endif
#ifdef EBADR
        H2RPC(EBADR);
#endif
#ifdef EXFULL
        H2RPC(EXFULL);
#endif
#ifdef ENOANO
        H2RPC(ENOANO);
#endif
#ifdef EBADRQC
        H2RPC(EBADRQC);
#endif
#ifdef EBADSLT
        H2RPC(EBADSLT);
#endif
#ifdef EDEADLOCK
        H2RPC(EDEADLOCK);
#endif
#ifdef EBFONT
        H2RPC(EBFONT);
#endif
#ifdef ENOSTR
        H2RPC(ENOSTR);
#endif
#ifdef ENODATA
        H2RPC(ENODATA);
#endif
#ifdef ETIME
        H2RPC(ETIME);
#endif
#ifdef ENOSR
        H2RPC(ENOSR);
#endif
#ifdef ENONET
        H2RPC(ENONET);
#endif
#ifdef ENOPKG
        H2RPC(ENOPKG);
#endif
        H2RPC(EREMOTE);
#ifdef ENOLINK
        H2RPC(ENOLINK);
#endif
#ifdef EADV
        H2RPC(EADV);
#endif
#ifdef ESRMNT
        H2RPC(ESRMNT);
#endif
#ifdef ECOMM
        H2RPC(ECOMM);
#endif
#ifdef EPROTO
        H2RPC(EPROTO);
#endif
#ifdef EMULTIHOP
        H2RPC(EMULTIHOP);
#endif
#ifdef EDOTDOT
        H2RPC(EDOTDOT);
#endif
#ifdef EBADMSG
        H2RPC(EBADMSG);
#endif
        H2RPC(EOVERFLOW);
#ifdef ENOTUNIQ
        H2RPC(ENOTUNIQ);
#endif
#ifdef EBADFD
        H2RPC(EBADFD);
#endif
#ifdef EREMCHG
        H2RPC(EREMCHG);
#endif
#ifdef ELIBACC
        H2RPC(ELIBACC);
#endif
#ifdef ELIBBAD
        H2RPC(ELIBBAD);
#endif
#ifdef ELIBSCN
        H2RPC(ELIBSCN);
#endif
#ifdef ELIBMAX
        H2RPC(ELIBMAX);
#endif
#ifdef ELIBEXEC
        H2RPC(ELIBEXEC);
#endif
        H2RPC(EILSEQ);
#ifdef ERESTART
        H2RPC(ERESTART);
#endif
#ifdef ESTRPIPE
        H2RPC(ESTRPIPE);
#endif
        H2RPC(EUSERS);
        H2RPC(ENOTSOCK);
        H2RPC(EDESTADDRREQ);
        H2RPC(EMSGSIZE);
        H2RPC(EPROTOTYPE);
        H2RPC(ENOPROTOOPT);
        H2RPC(EPROTONOSUPPORT);
        H2RPC(ESOCKTNOSUPPORT);
        H2RPC(EOPNOTSUPP);
        H2RPC(EPFNOSUPPORT);
        H2RPC(EAFNOSUPPORT);
        H2RPC(EADDRINUSE);
        H2RPC(EADDRNOTAVAIL);
        H2RPC(ENETDOWN);
        H2RPC(ENETUNREACH);
        H2RPC(ENETRESET);
        H2RPC(ECONNABORTED);
        H2RPC(ECONNRESET);
        H2RPC(ENOBUFS);
        H2RPC(EISCONN);
        H2RPC(ENOTCONN);
        H2RPC(ESHUTDOWN);
        H2RPC(ETOOMANYREFS);
        H2RPC(ETIMEDOUT);
        H2RPC(ECONNREFUSED);
        H2RPC(EHOSTDOWN);
        H2RPC(EHOSTUNREACH);
        H2RPC(EALREADY);
        H2RPC(EINPROGRESS);
        H2RPC(ESTALE);
#ifdef EUCLEAN
        H2RPC(EUCLEAN);
#endif
#ifdef ENOTNAM
        H2RPC(ENOTNAM);
#endif
#ifdef ENAVAIL
        H2RPC(ENAVAIL);
#endif
#ifdef EISNAM
        H2RPC(EISNAM);
#endif
#ifdef EREMOTEIO
        H2RPC(EREMOTEIO);
#endif
        H2RPC(EDQUOT);
#ifdef ENOMEDIUM
        H2RPC(ENOMEDIUM);
#endif
#ifdef EMEDIUMTYPE
        H2RPC(EMEDIUMTYPE);
#endif

        default: return RPC_EUNKNOWN;
    }
}

/** Convert system native errno to RPC errno */
static inline const char *
errno_rpc2str(rpc_errno rpc_errno_val)
{
    switch (rpc_errno_val)
    {
        case RPC_EOK: return "0";
        
        RPC2STR(EPERM);
        RPC2STR(ENOENT);
        RPC2STR(ESRCH);
        RPC2STR(EINTR);
        RPC2STR(EIO);
        RPC2STR(ENXIO);
        RPC2STR(E2BIG);
        RPC2STR(ENOEXEC);
        RPC2STR(EBADF);
        RPC2STR(ECHILD);
        RPC2STR(EAGAIN);
        RPC2STR(ENOMEM);
        RPC2STR(EACCES);
        RPC2STR(EFAULT);
        RPC2STR(ENOTBLK);
        RPC2STR(EBUSY);
        RPC2STR(EEXIST);
        RPC2STR(EXDEV);
        RPC2STR(ENODEV);
        RPC2STR(ENOTDIR);
        RPC2STR(EISDIR);
        RPC2STR(EINVAL);
        RPC2STR(ENFILE);
        RPC2STR(EMFILE);
        RPC2STR(ENOTTY);
        RPC2STR(ETXTBSY);
        RPC2STR(EFBIG);
        RPC2STR(ENOSPC);
        RPC2STR(ESPIPE);
        RPC2STR(EROFS);
        RPC2STR(EMLINK);
        RPC2STR(EPIPE);
        RPC2STR(EDOM);
        RPC2STR(ERANGE);
        RPC2STR(ENAMETOOLONG);
        RPC2STR(ENOLCK);
        RPC2STR(ENOSYS);
        RPC2STR(ENOTEMPTY);
        RPC2STR(ELOOP);
        RPC2STR(ENOMSG);
        RPC2STR(EIDRM);
#ifdef ECHRNG
        RPC2STR(ECHRNG);
#endif
#ifdef EL2NSYNC
        RPC2STR(EL2NSYNC);
#endif
#ifdef EL3HLT
        RPC2STR(EL3HLT);
#endif
#ifdef EL3RST
        RPC2STR(EL3RST);
#endif
#ifdef ELNRNG
        RPC2STR(ELNRNG);
#endif
#ifdef EUNATCH
        RPC2STR(EUNATCH);
#endif
#ifdef ENOCSI
        RPC2STR(ENOCSI);
#endif
#ifdef EL2HLT
        RPC2STR(EL2HLT);
#endif
#ifdef EBADE
        RPC2STR(EBADE);
#endif
#ifdef EBADR
        RPC2STR(EBADR);
#endif
#ifdef EXFULL
        RPC2STR(EXFULL);
#endif
#ifdef ENOANO
        RPC2STR(ENOANO);
#endif
#ifdef EBADRQC
        RPC2STR(EBADRQC);
#endif
#ifdef EBADSLT
        RPC2STR(EBADSLT);
#endif
#ifdef EDEADLOCK
        RPC2STR(EDEADLOCK);
#endif
#ifdef EBFONT
        RPC2STR(EBFONT);
#endif
#ifdef ENOSTR
        RPC2STR(ENOSTR);
#endif
#ifdef ENODATA
        RPC2STR(ENODATA);
#endif
#ifdef ETIME
        RPC2STR(ETIME);
#endif
#ifdef ENOSR
        RPC2STR(ENOSR);
#endif
#ifdef ENONET
        RPC2STR(ENONET);
#endif
#ifdef ENOPKG
        RPC2STR(ENOPKG);
#endif
        RPC2STR(EREMOTE);
#ifdef ENOLINK
        RPC2STR(ENOLINK);
#endif
#ifdef EADV
        RPC2STR(EADV);
#endif
#ifdef ESRMNT
        RPC2STR(ESRMNT);
#endif
#ifdef ECOMM
        RPC2STR(ECOMM);
#endif
#ifdef EPROTO
        RPC2STR(EPROTO);
#endif
#ifdef EMULTIHOP
        RPC2STR(EMULTIHOP);
#endif
#ifdef EDOTDOT
        RPC2STR(EDOTDOT);
#endif
#ifdef EBADMSG
        RPC2STR(EBADMSG);
#endif
        RPC2STR(EOVERFLOW);
#ifdef ENOTUNIQ
        RPC2STR(ENOTUNIQ);
#endif
#ifdef EBADFD
        RPC2STR(EBADFD);
#endif
#ifdef EREMCHG
        RPC2STR(EREMCHG);
#endif
#ifdef ELIBACC
        RPC2STR(ELIBACC);
#endif
#ifdef ELIBBAD
        RPC2STR(ELIBBAD);
#endif
#ifdef ELIBSCN
        RPC2STR(ELIBSCN);
#endif
#ifdef ELIBMAX
        RPC2STR(ELIBMAX);
#endif
#ifdef ELIBEXEC
        RPC2STR(ELIBEXEC);
#endif
        RPC2STR(EILSEQ);
#ifdef ERESTART
        RPC2STR(ERESTART);
#endif
#ifdef ESTRPIPE
        RPC2STR(ESTRPIPE);
#endif
        RPC2STR(EUSERS);
        RPC2STR(ENOTSOCK);
        RPC2STR(EDESTADDRREQ);
        RPC2STR(EMSGSIZE);
        RPC2STR(EPROTOTYPE);
        RPC2STR(ENOPROTOOPT);
        RPC2STR(EPROTONOSUPPORT);
        RPC2STR(ESOCKTNOSUPPORT);
        RPC2STR(EOPNOTSUPP);
        RPC2STR(EPFNOSUPPORT);
        RPC2STR(EAFNOSUPPORT);
        RPC2STR(EADDRINUSE);
        RPC2STR(EADDRNOTAVAIL);
        RPC2STR(ENETDOWN);
        RPC2STR(ENETUNREACH);
        RPC2STR(ENETRESET);
        RPC2STR(ECONNABORTED);
        RPC2STR(ECONNRESET);
        RPC2STR(ENOBUFS);
        RPC2STR(EISCONN);
        RPC2STR(ENOTCONN);
        RPC2STR(ESHUTDOWN);
        RPC2STR(ETOOMANYREFS);
        RPC2STR(ETIMEDOUT);
        RPC2STR(ECONNREFUSED);
        RPC2STR(EHOSTDOWN);
        RPC2STR(EHOSTUNREACH);
        RPC2STR(EALREADY);
        RPC2STR(EINPROGRESS);
        RPC2STR(ESTALE);
#ifdef EUCLEAN
        RPC2STR(EUCLEAN);
#endif
#ifdef ENOTNAM
        RPC2STR(ENOTNAM);
#endif
#ifdef ENAVAIL
        RPC2STR(ENAVAIL);
#endif
#ifdef EISNAM
        RPC2STR(EISNAM);
#endif
#ifdef EREMOTEIO
        RPC2STR(EREMOTEIO);
#endif
        RPC2STR(EDQUOT);
#ifdef ENOMEDIUM
        RPC2STR(ENOMEDIUM);
#endif
#ifdef EMEDIUMTYPE
        RPC2STR(EMEDIUMTYPE);
#endif

        RPC2STR(ERPCNOTSUPP);

        default: 
        {
            static char unknown_err[100];

            snprintf(unknown_err, sizeof(unknown_err),
                     "EUNKNOWN (%X)", rpc_errno_val);
            return unknown_err;
        }
    }
}

/** Convert RPC errno to system native errno */
static inline int  
errno_rpc2h(rpc_errno rpc_errno_val)
{
    switch (rpc_errno_val)
    {
        case RPC_EOK: return 0;
        
        RPC2H(EPERM);
        RPC2H(ENOENT);
        RPC2H(ESRCH);
        RPC2H(EINTR);
        RPC2H(EIO);
        RPC2H(ENXIO);
        RPC2H(E2BIG);
        RPC2H(ENOEXEC);
        RPC2H(EBADF);
        RPC2H(ECHILD);
        RPC2H(EAGAIN);
        RPC2H(ENOMEM);
        RPC2H(EACCES);
        RPC2H(EFAULT);
        RPC2H(ENOTBLK);
        RPC2H(EBUSY);
        RPC2H(EEXIST);
        RPC2H(EXDEV);
        RPC2H(ENODEV);
        RPC2H(ENOTDIR);
        RPC2H(EISDIR);
        RPC2H(EINVAL);
        RPC2H(ENFILE);
        RPC2H(EMFILE);
        RPC2H(ENOTTY);
        RPC2H(ETXTBSY);
        RPC2H(EFBIG);
        RPC2H(ENOSPC);
        RPC2H(ESPIPE);
        RPC2H(EROFS);
        RPC2H(EMLINK);
        RPC2H(EPIPE);
        RPC2H(EDOM);
        RPC2H(ERANGE);
        RPC2H(ENAMETOOLONG);
        RPC2H(ENOLCK);
        RPC2H(ENOSYS);
        RPC2H(ENOTEMPTY);
        RPC2H(ELOOP);
        RPC2H(ENOMSG);
        RPC2H(EIDRM);
#ifdef ECHRNG
        RPC2H(ECHRNG);
#endif
#ifdef EL2NSYNC
        RPC2H(EL2NSYNC);
#endif
#ifdef EL3HLT
        RPC2H(EL3HLT);
#endif
#ifdef EL3RST
        RPC2H(EL3RST);
#endif
#ifdef ELNRNG
        RPC2H(ELNRNG);
#endif
#ifdef EUNATCH
        RPC2H(EUNATCH);
#endif
#ifdef ENOCSI
        RPC2H(ENOCSI);
#endif
#ifdef EL2HLT
        RPC2H(EL2HLT);
#endif
#ifdef EBADE
        RPC2H(EBADE);
#endif
#ifdef EBADR
        RPC2H(EBADR);
#endif
#ifdef EXFULL
        RPC2H(EXFULL);
#endif
#ifdef ENOANO
        RPC2H(ENOANO);
#endif
#ifdef EBADRQC
        RPC2H(EBADRQC);
#endif
#ifdef EBADSLT
        RPC2H(EBADSLT);
#endif
#ifdef EDEADLOCK
        RPC2H(EDEADLOCK);
#endif
#ifdef EBFONT
        RPC2H(EBFONT);
#endif
#ifdef ENOSTR
        RPC2H(ENOSTR);
#endif
#ifdef ENODATA
        RPC2H(ENODATA);
#endif
#ifdef ETIME
        RPC2H(ETIME);
#endif
#ifdef ENOSR
        RPC2H(ENOSR);
#endif
#ifdef ENONET
        RPC2H(ENONET);
#endif
#ifdef ENOPKG
        RPC2H(ENOPKG);
#endif
        RPC2H(EREMOTE);
#ifdef ENOLINK
        RPC2H(ENOLINK);
#endif
#ifdef EADV
        RPC2H(EADV);
#endif
#ifdef ESRMNT
        RPC2H(ESRMNT);
#endif
#ifdef ECOMM
        RPC2H(ECOMM);
#endif
#ifdef EPROTO
        RPC2H(EPROTO);
#endif
#ifdef EMULTIHOP
        RPC2H(EMULTIHOP);
#endif
#ifdef EDOTDOT
        RPC2H(EDOTDOT);
#endif
#ifdef EBADMSG
        RPC2H(EBADMSG);
#endif
        RPC2H(EOVERFLOW);
#ifdef ENOTUNIQ
        RPC2H(ENOTUNIQ);
#endif
#ifdef EBADFD
        RPC2H(EBADFD);
#endif
#ifdef EREMCHG
        RPC2H(EREMCHG);
#endif
#ifdef ELIBACC
        RPC2H(ELIBACC);
#endif
#ifdef ELIBBAD
        RPC2H(ELIBBAD);
#endif
#ifdef ELIBSCN
        RPC2H(ELIBSCN);
#endif
#ifdef ELIBMAX
        RPC2H(ELIBMAX);
#endif
#ifdef ELIBEXEC
        RPC2H(ELIBEXEC);
#endif
        RPC2H(EILSEQ);
#ifdef ERESTART
        RPC2H(ERESTART);
#endif
#ifdef ESTRPIPE
        RPC2H(ESTRPIPE);
#endif
        RPC2H(EUSERS);
        RPC2H(ENOTSOCK);
        RPC2H(EDESTADDRREQ);
        RPC2H(EMSGSIZE);
        RPC2H(EPROTOTYPE);
        RPC2H(ENOPROTOOPT);
        RPC2H(EPROTONOSUPPORT);
        RPC2H(ESOCKTNOSUPPORT);
        RPC2H(EOPNOTSUPP);
        RPC2H(EPFNOSUPPORT);
        RPC2H(EAFNOSUPPORT);
        RPC2H(EADDRINUSE);
        RPC2H(EADDRNOTAVAIL);
        RPC2H(ENETDOWN);
        RPC2H(ENETUNREACH);
        RPC2H(ENETRESET);
        RPC2H(ECONNABORTED);
        RPC2H(ECONNRESET);
        RPC2H(ENOBUFS);
        RPC2H(EISCONN);
        RPC2H(ENOTCONN);
        RPC2H(ESHUTDOWN);
        RPC2H(ETOOMANYREFS);
        RPC2H(ETIMEDOUT);
        RPC2H(ECONNREFUSED);
        RPC2H(EHOSTDOWN);
        RPC2H(EHOSTUNREACH);
        RPC2H(EALREADY);
        RPC2H(EINPROGRESS);
        RPC2H(ESTALE);
#ifdef EUCLEAN
        RPC2H(EUCLEAN);
#endif
#ifdef ENOTNAM
        RPC2H(ENOTNAM);
#endif
#ifdef ENAVAIL
        RPC2H(ENAVAIL);
#endif
#ifdef EISNAM
        RPC2H(EISNAM);
#endif
#ifdef EREMOTEIO
        RPC2H(EREMOTEIO);
#endif
        RPC2H(EDQUOT);
#ifdef ENOMEDIUM
        RPC2H(ENOMEDIUM);
#endif
#ifdef EMEDIUMTYPE
        RPC2H(EMEDIUMTYPE);
#endif

        default: 
            /* ERROR("RPC errno not match any available!!!"); */
            return -1;
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

#define RCF_RPC_NAME_LEN        16

#define RCF_RPC_MAX_BUF     1048576
#define RCF_RPC_MAX_IOVEC   32

#define RCF_RPC_EOR_TIMEOUT 1000000

#endif /* !__TE_RCF_RPC_DEFS_H__ */
