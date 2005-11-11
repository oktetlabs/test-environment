/** @file
 * @brief Test Environment errno codes
 *
 * Definitions of errno codes
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

#ifndef __TE_ERRNO_H__
#define __TE_ERRNO_H__

#include "te_config.h"

#include <stdio.h>
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#include "te_stdint.h"
#include "te_printf.h"


#define TE_MIN_ERRNO    (1 << 22)


/** Type for TE error numbers */
typedef int32_t te_errno;


/** Type to store TE error numbers */
typedef enum {
    /* OS error codes */
    TE_EPERM = TE_MIN_ERRNO + 1,     
                      /**< Operation not permitted */
    TE_ENOENT,        /**< No such file or directory */
    TE_ESRCH,         /**< No such process */
    TE_EINTR,         /**< Interrupted system call */
    TE_EIO,           /**< I/O error */
    TE_ENXIO,         /**< No such device or address */
    TE_E2BIG,         /**< Arg list too long */
    TE_ENOEXEC,       /**< Exec  error */
    TE_EBADF,         /**< Bad file number */
    TE_ECHILD,        /**< No child processes */
    TE_EAGAIN,        /**< Try again */
    TE_ENOMEM,        /**< Out of memory */
    TE_EACCES,        /**< Permission denied */
    TE_EFAULT,        /**< Bad address */
    TE_ENOTBLK,       /**< Block device required */
    TE_EBUSY,         /**< Device or resource busy */
    TE_EEXIST,        /**< File exists */
    TE_EXDEV,         /**< Cross-device link */
    TE_ENODEV,        /**< No such device */
    TE_ENOTDIR,       /**< Not a directory */
    TE_EISDIR,        /**< Is a directory */
    TE_EINVAL,        /**< Invalid argument */
    TE_ENFILE,        /**< File table overflow */
    TE_EMFILE,        /**< Too many open files */
    TE_ENOTTY,        /**< Not a typewriter */
    TE_ETXTBSY,       /**< Text file busy */
    TE_EFBIG,         /**< File too large */
    TE_ENOSPC,        /**< No space left on device */
    TE_ESPIPE,        /**< Illegal seek */
    TE_EROFS,         /**< Read-only file system */
    TE_EMLINK,        /**< Too many links */
    TE_EPIPE,         /**< Broken pipe */
    TE_EDOM,          /**< Math argument out of domain of func */
    TE_ERANGE,        /**< Math result not representable */
    TE_EDEADLK,       /**< Resource deadlock would occur */
    TE_ENAMETOOLONG,  /**< File name too long */
    TE_ENOLCK,        /**< No record locks available */
    TE_ENOSYS,        /**< Function not implemented */
    TE_ENOTEMPTY,     /**< Directory not empty */
    TE_ELOOP,         /**< Too many symbolic links encountered */
    TE_EWOULDBLOCK,   /**< Synonym of EAGAIN */
    TE_ENOMSG,        /**< No message of desired type */
    TE_EIDRM,         /**< Identifier removed */
    TE_ECHRNG,        /**< Channel number out of range */
    TE_EL2NSYNC,      /**< Level 2 not synchronized */
    TE_EL3HLT,        /**< Level 3 halted */
    TE_EL3RST,        /**< Level 3 reset */
    TE_ELNRNG,        /**< Link number out of range */
    TE_EUNATCH,       /**< Protocol driver not attached */
    TE_ENOCSI,        /**< No CSI structure available */
    TE_EL2HLT,        /**< Level 2 halted */
    TE_EBADE,         /**< Invalid exchange */
    TE_EBADR,         /**< Invalid request descriptor */
    TE_EXFULL,        /**< Exchange full */
    TE_ENOANO,        /**< No anode */
    TE_EBADRQC,       /**< Invalid request code */
    TE_EBADSLT,       /**< Invalid slot */
    TE_EDEADLOCK,     /**< Synomym of EDEADLK */
    TE_EBFONT,        /**< Bad font file  */
    TE_ENOSTR,        /**< Device not a stream */
    TE_ENODATA,       /**< No data available */
    TE_ETIME,         /**< Timer expired */
    TE_ENOSR,         /**< Out of streams resources */
    TE_ENONET,        /**< Machine is not on the network */
    TE_ENOPKG,        /**< Package not installed */
    TE_EREMOTE,       /**< Object is remote */
    TE_ENOLINK,       /**< Link has been severed */
    TE_EADV,          /**< Advertise error */
    TE_ESRMNT,        /**< Srmount error */
    TE_ECOMM,         /**< Communication error on send */
    TE_EPROTO,        /**< Protocol error */
    TE_EMULTIHOP,     /**< Multihop attempted */
    TE_EDOTDOT,       /**< RFS specific error */
    TE_EBADMSG,       /**< Not a data message */
    TE_EOVERFLOW,     /**< Value too large for defined data type */
    TE_ENOTUNIQ,      /**< Name not unique on network */
    TE_EBADFD,        /**< File descriptor in bad state */
    TE_EREMCHG,       /**< Remote address changed */
    TE_ELIBACC,       /**< Can not access a needed shared library */
    TE_ELIBBAD,       /**< Accessing a corrupted shared library */
    TE_ELIBSCN,       /**< .lib section in a.out corrupted */
    TE_ELIBMAX,       /**< Attempting to link in too many shared 
                           libraries */
    TE_ELIBEXEC,      /**< Cannot exec a shared library directly */
    TE_EILSEQ,        /**< Illegal byte sequence */
    TE_ERESTART,      /**< Interrupted system call should be restarted */
    TE_ESTRPIPE,      /**< Streams pipe error */
    TE_EUSERS,        /**< Too many users */
    TE_ENOTSOCK,      /**< Socket operation on non-socket */
    TE_EDESTADDRREQ,  /**< Destination address required */
    TE_EMSGSIZE,      /**< Message too long */
    TE_EPROTOTYPE,    /**< Protocol wrong type for socket */
    TE_ENOPROTOOPT,   /**< Protocol not available */
    TE_EPROTONOSUPPORT, /**< Protocol not supported */
    TE_ESOCKTNOSUPPORT, /**< Socket type not supported */
    TE_EOPNOTSUPP,    /**< Operation not supported on transport endpoint */
    TE_EPFNOSUPPORT,  /**< Protocol family not supported */
    TE_EAFNOSUPPORT,  /**< Address family not supported by protocol */
    TE_EADDRINUSE,    /**< Address already in use */
    TE_EADDRNOTAVAIL, /**< Cannot assign requested address */
    TE_ENETDOWN,      /**< Network is down */
    TE_ENETUNREACH,   /**< Network is unreachable */
    TE_ENETRESET,     /**< Network dropped connection because of reset */
    TE_ECONNABORTED,  /**< Software caused connection abort */
    TE_ECONNRESET,    /**< Connection reset by peer */
    TE_ENOBUFS,       /**< No buffer space available */
    TE_EISCONN,       /**< Transport endpoint is already connected */
    TE_ENOTCONN,      /**< Transport endpoint is not connected */
    TE_ESHUTDOWN,     /**< Cannot send after transport endpoint shutdown */
    TE_ETOOMANYREFS,  /**< Too many references: cannot splice */
    TE_ETIMEDOUT,     /**< Connection timed out */
    TE_ECONNREFUSED,  /**< Connection refused */
    TE_EHOSTDOWN,     /**< Host is down */
    TE_EHOSTUNREACH,  /**< No route to host */
    TE_EALREADY,      /**< Operation already in progress */
    TE_EINPROGRESS,   /**< Operation now in progress */
    TE_ESTALE,        /**< Stale NFS file handle */
    TE_EUCLEAN,       /**< Structure needs cleaning */
    TE_ENOTNAM,       /**< Not a XENIX named type file */
    TE_ENAVAIL,       /**< No XENIX semaphores available */
    TE_EISNAM,        /**< Is a named type file */
    TE_EREMOTEIO,     /**< Remote I/O error */
    TE_EDQUOT,        /**< Quota exceeded */
    TE_ENOMEDIUM,     /**< No medium found */
    TE_EMEDIUMTYPE,   /**< Wrong medium type */
    TE_ECANCELED,     /**< Operation is cancelled */
    
    /* TE-specific error codes */
    TE_EUNKNOWN,      /**< Unknown OS errno */
    
/** @name Common errno's */
    TE_EOK = TE_MIN_ERRNO + 500, 
                  /**< Success when 0 can't be used */
    TE_EFAIL,     /**< Generic failure */
    TE_ESMALLBUF, /**< Too small buffer is provided */
    TE_EPENDING,  /**< Pending data retain on connection */
    TE_EIPC,      /**< Could not interact with RCF */
    TE_ESHCMD,    /**< Shell command returned non-zero exit status */
    TE_EWRONGPTR, /**< Wrong pointer was passed to function */
    TE_ETOOMANY,  /**< Too many objects have been already allocated, 
                       so that the resource is not available */
    TE_EFMT,      /**< Invalid format */
    TE_EENV,      /**< Inappropriate environment */
    TE_EWIN,      /**< Windows API function failed, 
                       see log for the description */
/*@}*/

/** @name Remote Control Facility errno's */
    TE_ENORCF = TE_MIN_ERRNO + 600,       
                     /**< RCF initialization failed */
    TE_EACK,         /**< The request is accepted for processing */
    TE_ETALOCAL,     /**< TA runs on the same station with TEN and
                          cannot be rebooted */
    TE_ETADEAD,      /**< Test Agent is dead */
    TE_ETAREBOOTED,  /**< Test Agent is rebooted */
    TE_ESUNRPC,      /**< SUN RPC failed */
    TE_ECORRUPTED,   /**< Data are corrupted by the software under test */
    TE_ERPCTIMEOUT,  /**< Timeout ocurred during RPC call */
    TE_ERPCDEAD,     /**< RPC server is dead */
/*@}*/

/** @name ASN.1 text parse errors */
    TE_EASNGENERAL = TE_MIN_ERRNO + 700, 
                          /**< Generic error */
    TE_EASNWRONGLABEL,    /**< Wrong ASN label */
    TE_EASNTXTPARSE,      /**< General ASN.1 text parse error */
    TE_EASNDERPARSE,      /**< DER decode error */
    TE_EASNINCOMPLVAL,    /**< Imcomplete ASN.1 value */
    TE_EASNOTHERCHOICE,   /**< CHOICE in type is differ then asked */
    TE_EASNWRONGTYPE,     /**< Passed value has wrong type */
    TE_EASNNOTLEAF,       /**< Passed labels of subvalue does not
                               respond to plain-syntax leaf */

    TE_EASNTXTNOTINT,     /**< int expected but not found */
    TE_EASNTXTNOTCHSTR,   /**< Character string expected */
    TE_EASNTXTNOTOCTSTR,  /**< Octet string expected */
    TE_EASNTXTVALNAME,    /**< Wrong subvalue name in constraint value
                               with named fields */
    TE_EASNTXTSEPAR,      /**< Wrong separator between elements
                               in const value */
/*@}*/

/** @name Traffic Application Domain errno's */
    TE_ETADCSAPNOTEX = TE_MIN_ERRNO + 800,  
                        /**< CSAP not exist. */
    TE_ETADLOWER,       /**< Lower layer error, usually from some
                             external library or OS resources, which
                             is used for implementation of CSAP */
    TE_ETADCSAPSTATE,   /**< Command is not appropriate to CSAP state */
    TE_ETADNOTMATCH,    /**< Data do not match to the specified pattern */
    TE_ETADLESSDATA,    /**< Read data matches to the begin of pattern, 
                             but it is not enough for all one,
                             or not enough data for generate */
    TE_ETADMISSNDS,     /**< Missing NDS */
    TE_ETADWRONGNDS,    /**< Wrong NDS passed */
    TE_ETADCSAPDB,      /**< CSAP DB internal error */
    TE_ETADENDOFDATA,   /**< End of incoming data in CSAP */
    TE_ETADEXPRPARSE,   /**< Expression parse error */

/*@}*/

/** @name Configurator errno's */
    TE_EBACKUP = TE_MIN_ERRNO + 900,  
                 /**< Backup verification failed */
    TE_EISROOT,  /**< Attempt to delete the root */
    TE_EHASSON,  /**< Attempt to delete the node with children */
    TE_ENOCONF,  /**< Configurator initialization failed */
    TE_EBADTYPE, /**< Type specified by the user is incorrect */
/*@}*/

/** @name Tester errno's */
/* 
 * Errno codes below are strictly ordered and
 * have ETESTRESULTMIN and ETESTRESULTMAX.
 */
    TE_ETESTEMPTY = TE_MIN_ERRNO + 1000, 
                    /**< Test session/pkg is empty */
    TE_ETESTSKIP,   /**< Test skipped */
    TE_ETESTFAKE,   /**< Test not really run */
    TE_ETESTPASS,   /**< Test passed */
    TE_ETESTCONF,   /**< Test changed configuration */
    TE_ETESTKILL,   /**< Test killed by signal */
    TE_ETESTCORE,   /**< Test dumped core */
    TE_ETESTPROLOG, /**< Session prologue failed */
    TE_ETESTEPILOG, /**< Session epilogue failed */
    TE_ETESTALIVE,  /**< Session keep-alive failed */
    TE_ETESTFAIL,   /**< Test failed */
    TE_ETESTUNEXP,  /**< Test unexpected results */

    TE_ETESTRESULTMIN = TE_ETESTEMPTY,  /**< Minimum test result errno */
    TE_ETESTRESULTMAX = TE_ETESTUNEXP,  /**< Maximum test result errno */
/*@}*/

/** @name TARPC errno's */
    TE_ERPC2H = TE_MIN_ERRNO + 1100,  /**< RPC to host conv failed */
    TE_EH2RPC,      /**< Host to RPC conv failed */
    TE_ERPCNOTSUPP, /**< RPC is not supported
                         (it does not have host analogue) */
/*@}*/

/** @name IPC errno's */
    TE_ESYNCFAILED = TE_MIN_ERRNO + 1200 
                       /**< IPC synchronisation is broken */
/*@}*/
} te_errno_codes;


/**
 * @name Identifiers of TE modules (error sources) used in errors
 *
 * @todo Make it 'enum' when %r specified will be supported.
 */
typedef enum {
    TE_IPC = 1,         /**< TE IPC */
    TE_COMM,            /**< RCF<->TA communication libraries */
    TE_RCF,             /**< RCF application */
    TE_RCF_UNIX,        /**< UNIX-like agents management */
    TE_RCF_API,         /**< RCF library */
    TE_RCF_RPC,         /**< RCF RPC support */
    TE_RCF_PCH,         /**< RCF Portable Command Handler */
    TE_RCF_CH,          /**< RCF Command Handler */
    TE_TAD_CH,          /**< TAD Command Handler */
    TE_TAD_CSAP,        /**< TAD CSAP support */
    TE_TARPC,           /**< RPC support in Test Agent */
    TE_LOGGER,          /**< Logger application */
    TE_CS,              /**< Configurator application */
    TE_CONF_API,        /**< Configurator API */
    TE_TESTER,          /**< Tester application */
    TE_TAPI,            /**< Test API libraries */
    TE_TA,              /**< Test Agent libraries */
    TE_TA_UNIX,         /**< Unix Test Agent */
    TE_TA_WIN32,        /**< Windows Test Agent */
    TE_TA_SWITCH_CTL,   /**< Switch Control Test Agent */
    TE_NET_SNMP,        /**< Errors from net-snmp library */
    TE_TA_EXT,          /**< Error generated by external entity */
    TE_RPC,             /**< System error returned by function called
                             via RPC */
    TE_ISCSI_TARGET,    /**< Error related to iSCSI target */   
} te_module;                             
/*@}*/


/** Shift of the module ID in 'int' (at least 32 bit) error code */
#define TE_RC_MODULE_SHIFT      24

/** Get identifier of the module generated an error */
#define TE_RC_GET_MODULE(_rc) \
    ((te_module)((_rc) >> TE_RC_MODULE_SHIFT))

/** Get error number without module identifier */
#define TE_RC_GET_ERROR(_rc) \
    ((_rc) & \
     ((~(unsigned int)0) >> ((sizeof(unsigned int) * 8) - \
                                 TE_RC_MODULE_SHIFT)))

/** Compose base error code and module identifier */
#define TE_RC(_mod_id, _error) \
    ((_error && (TE_RC_GET_MODULE(_error) == 0)) ? \
     ((int)(_mod_id) << TE_RC_MODULE_SHIFT) | (_error) : (_error))

/** Create error code from OS errno and module identifier */
#define TE_OS_RC(_mod_id, _error) TE_RC(_mod_id, te_rc_os2te(_error))


/**
 * Update \i main return code, if it's OK, otherwise keep it.
 *
 * @param _rc         main return code to be updated
 * @param _rc_new     new return code to be assigned to main, if
 *                    main is OK
 *
 * @return Updated return code.
 */
#define TE_RC_UPDATE(_rc, _rc_new) \
    ((_rc) = (((_rc) == 0) ? (_rc_new) : (_rc)))

/**
 * Convert error source from integer to readable string.
 *
 * @param err   integer source or errno (source / error code composition)
 *
 * @return string literal pointer
 *
 * @note non-reenterable in the case of unknown module
 */
static inline const char * 
te_rc_mod2str(te_errno err)
{
    static char unknown_module[32];

#define MOD2STR(name_) case TE_ ## name_: return #name_

    switch (TE_RC_GET_MODULE(err))
    {
        MOD2STR(IPC);        
        MOD2STR(COMM);        
        MOD2STR(RCF);   
        MOD2STR(RCF_UNIX);
        MOD2STR(RCF_API);    
        MOD2STR(RCF_RPC);    
        MOD2STR(RCF_PCH);     
        MOD2STR(RCF_CH);     
        MOD2STR(TAD_CH);   
        MOD2STR(TAD_CSAP);
        MOD2STR(TARPC);
        MOD2STR(LOGGER);
        MOD2STR(CS);
        MOD2STR(CONF_API);
        MOD2STR(TESTER);
        MOD2STR(TAPI);   
        MOD2STR(TA);   
        MOD2STR(TA_UNIX);   
        MOD2STR(TA_WIN32);
        MOD2STR(TA_SWITCH_CTL);
        MOD2STR(NET_SNMP);     
        MOD2STR(TA_EXT);
        MOD2STR(RPC);
        MOD2STR(ISCSI_TARGET);
        case 0: return "";
        default:
            snprintf(unknown_module, sizeof(unknown_module),
                     "Unknown(%u)", TE_RC_GET_MODULE(err));
            return unknown_module;
    }
#undef MOD2STR    
}

/**
 * Convert code from integer to readable string.
 *
 * @param src   integer source
 *
 * @return string literal pointer
 *
 * @note non-reenterable
 */
static inline const char * 
te_rc_err2str(te_errno err)
{
    static char old_errno[64];
    static char unknown_errno[32];

    if ((err != 0) && ((err & TE_MIN_ERRNO) == 0))
    {
        snprintf(old_errno, sizeof(old_errno), 
                 "Old errno 0x%" TE_PRINTF_32 "X", TE_RC_GET_ERROR(err));
        return old_errno;
    }
    
#define ERR2STR(name_) case TE_ ## name_: return #name_
    switch (TE_RC_GET_ERROR(err))
    {
        ERR2STR(EPERM);        
                     
        ERR2STR(ENOENT);       
        ERR2STR(ESRCH);        
        ERR2STR(EINTR);        
        ERR2STR(EIO);          
        ERR2STR(ENXIO);        
        ERR2STR(E2BIG);        
        ERR2STR(ENOEXEC);      
        ERR2STR(EBADF);        
        ERR2STR(ECHILD);       
        ERR2STR(EAGAIN);       
        ERR2STR(ENOMEM);       
        ERR2STR(EACCES);       
        ERR2STR(EFAULT);       
        ERR2STR(ENOTBLK);      
        ERR2STR(EBUSY);        
        ERR2STR(EEXIST);       
        ERR2STR(EXDEV);        
        ERR2STR(ENODEV);       
        ERR2STR(ENOTDIR);      
        ERR2STR(EISDIR);       
        ERR2STR(EINVAL);       
        ERR2STR(ENFILE);       
        ERR2STR(EMFILE);       
        ERR2STR(ENOTTY);       
        ERR2STR(ETXTBSY);      
        ERR2STR(EFBIG);        
        ERR2STR(ENOSPC);       
        ERR2STR(ESPIPE);       
        ERR2STR(EROFS);        
        ERR2STR(EMLINK);       
        ERR2STR(EPIPE);        
        ERR2STR(EDOM);         
        ERR2STR(ERANGE);       
        ERR2STR(EDEADLK);      
        ERR2STR(ENAMETOOLONG); 
        ERR2STR(ENOLCK);       
        ERR2STR(ENOSYS);       
        ERR2STR(ENOTEMPTY);    
        ERR2STR(ELOOP);        
        ERR2STR(EWOULDBLOCK);  
        ERR2STR(ENOMSG);       
        ERR2STR(EIDRM);        
        ERR2STR(ECHRNG);       
        ERR2STR(EL2NSYNC);     
        ERR2STR(EL3HLT);       
        ERR2STR(EL3RST);       
        ERR2STR(ELNRNG);       
        ERR2STR(EUNATCH);      
        ERR2STR(ENOCSI);       
        ERR2STR(EL2HLT);       
        ERR2STR(EBADE);        
        ERR2STR(EBADR);        
        ERR2STR(EXFULL);       
        ERR2STR(ENOANO);       
        ERR2STR(EBADRQC);      
        ERR2STR(EBADSLT);      
        ERR2STR(EDEADLOCK);    
        ERR2STR(EBFONT);       
        ERR2STR(ENOSTR);       
        ERR2STR(ENODATA);      
        ERR2STR(ETIME);        
        ERR2STR(ENOSR);        
        ERR2STR(ENONET);       
        ERR2STR(ENOPKG);       
        ERR2STR(EREMOTE);      
        ERR2STR(ENOLINK);      
        ERR2STR(EADV);         
        ERR2STR(ESRMNT);       
        ERR2STR(ECOMM);        
        ERR2STR(EPROTO);       
        ERR2STR(EMULTIHOP);    
        ERR2STR(EDOTDOT);      
        ERR2STR(EBADMSG);      
        ERR2STR(EOVERFLOW);    
        ERR2STR(ENOTUNIQ);     
        ERR2STR(EBADFD);       
        ERR2STR(EREMCHG);      
        ERR2STR(ELIBACC);      
        ERR2STR(ELIBBAD);      
        ERR2STR(ELIBSCN);      
        ERR2STR(ELIBMAX);      
        ERR2STR(ELIBEXEC);     
        ERR2STR(EILSEQ);       
        ERR2STR(ERESTART);     
        ERR2STR(ESTRPIPE);     
        ERR2STR(EUSERS);       
        ERR2STR(ENOTSOCK);     
        ERR2STR(EDESTADDRREQ); 
        ERR2STR(EMSGSIZE);     
        ERR2STR(EPROTOTYPE);   
        ERR2STR(ENOPROTOOPT);  
        ERR2STR(EPROTONOSUPPORT); 
        ERR2STR(ESOCKTNOSUPPORT); 
        ERR2STR(EOPNOTSUPP);   
        ERR2STR(EPFNOSUPPORT); 
        ERR2STR(EAFNOSUPPORT); 
        ERR2STR(EADDRINUSE);   
        ERR2STR(EADDRNOTAVAIL);
        ERR2STR(ENETDOWN);     
        ERR2STR(ENETUNREACH);  
        ERR2STR(ENETRESET);    
        ERR2STR(ECONNABORTED); 
        ERR2STR(ECONNRESET);   
        ERR2STR(ENOBUFS);      
        ERR2STR(EISCONN);      
        ERR2STR(ENOTCONN);     
        ERR2STR(ESHUTDOWN);    
        ERR2STR(ETOOMANYREFS); 
        ERR2STR(ETIMEDOUT);    
        ERR2STR(ECONNREFUSED); 
        ERR2STR(EHOSTDOWN);    
        ERR2STR(EHOSTUNREACH); 
        ERR2STR(EALREADY);     
        ERR2STR(EINPROGRESS);  
        ERR2STR(ESTALE);       
        ERR2STR(EUCLEAN);      
        ERR2STR(ENOTNAM);      
        ERR2STR(ENAVAIL);      
        ERR2STR(EISNAM);       
        ERR2STR(EREMOTEIO);    
        ERR2STR(EDQUOT);       
        ERR2STR(ENOMEDIUM);    
        ERR2STR(EMEDIUMTYPE);  
        ERR2STR(ECANCELED);

        ERR2STR(EUNKNOWN);

        ERR2STR(EOK); 
        ERR2STR(EFAIL);     
        ERR2STR(ESMALLBUF); 
        ERR2STR(EPENDING);  
        ERR2STR(EIPC); 
        ERR2STR(ESHCMD);    
        ERR2STR(EWRONGPTR); 
        ERR2STR(ETOOMANY);  

        ERR2STR(EFMT);      
        ERR2STR(EENV);      
        ERR2STR(EWIN);      
        ERR2STR(ENORCF);        
        ERR2STR(EACK);        
        ERR2STR(ETALOCAL);     
        ERR2STR(ETADEAD);      
        ERR2STR(ETAREBOOTED);  
        ERR2STR(ESUNRPC);      
        ERR2STR(ECORRUPTED);   
        ERR2STR(ERPCTIMEOUT);  
        ERR2STR(ERPCDEAD);     

        ERR2STR(EASNGENERAL);
        ERR2STR(EASNWRONGLABEL);    
        ERR2STR(EASNTXTPARSE);      
        ERR2STR(EASNDERPARSE);      
        ERR2STR(EASNINCOMPLVAL);    
        ERR2STR(EASNOTHERCHOICE);   
        ERR2STR(EASNWRONGTYPE);     
        ERR2STR(EASNNOTLEAF);       
                          

        ERR2STR(EASNTXTNOTINT);     
        ERR2STR(EASNTXTNOTCHSTR);   
        ERR2STR(EASNTXTNOTOCTSTR);  
        ERR2STR(EASNTXTVALNAME);    
                          
        ERR2STR(EASNTXTSEPAR);      
                          
        ERR2STR(ETADCSAPNOTEX);  
        ERR2STR(ETADLOWER);       
                        
                        
        ERR2STR(ETADCSAPSTATE);   
        ERR2STR(ETADNOTMATCH);    
        ERR2STR(ETADLESSDATA);    
                        
                        
        ERR2STR(ETADMISSNDS);     
        ERR2STR(ETADWRONGNDS);    
        ERR2STR(ETADCSAPDB);      
        ERR2STR(ETADENDOFDATA);   
        ERR2STR(ETADEXPRPARSE);   

        ERR2STR(EBACKUP);
        ERR2STR(EISROOT);  
        ERR2STR(EHASSON);  
        ERR2STR(ENOCONF);  
        ERR2STR(EBADTYPE); 

        ERR2STR(ETESTEMPTY);
        ERR2STR(ETESTSKIP);    
        ERR2STR(ETESTFAKE);    
        ERR2STR(ETESTPASS);    
        ERR2STR(ETESTCONF);    
        ERR2STR(ETESTKILL);    
        ERR2STR(ETESTCORE);    
        ERR2STR(ETESTPROLOG);  
        ERR2STR(ETESTEPILOG);  
        ERR2STR(ETESTALIVE);   
        ERR2STR(ETESTFAIL);    
        ERR2STR(ETESTUNEXP);   

        ERR2STR(ERPCNOTSUPP);
        ERR2STR(ERPC2H);
        ERR2STR(EH2RPC);

        ERR2STR(ESYNCFAILED);
        
        case 0:  return "OK";
        default:
            snprintf(unknown_errno, sizeof(unknown_errno),
                     "Unknown(%" TE_PRINTF_32 "u)", TE_RC_GET_ERROR(err));
            return unknown_errno;
    }
#undef ERR2STR    
}

/**
 * Convert OS errno to TE error code.
 *
 * @param err   OS errno
 *
 * @return TE error code
 */
static inline te_errno
te_rc_os2te(int err)
{
    switch (err)
    {
        case 0: return 0;

#ifdef EPERM
        case EPERM: return TE_EPERM;
#endif        
        
#ifdef ENOENT
        case ENOENT: return TE_ENOENT;
#endif       
        
#ifdef ESRCH
        case ESRCH: return TE_ESRCH;
#endif        
        
#ifdef EINTR
        case EINTR: return TE_EINTR;
#endif        
        
#ifdef EIO
        case EIO: return TE_EIO;
#endif          
        
#ifdef ENXIO
        case ENXIO: return TE_ENXIO;
#endif        
        
#ifdef E2BIG
        case E2BIG: return TE_E2BIG;
#endif        
        
#ifdef ENOEXEC
        case ENOEXEC: return TE_ENOEXEC;
#endif      
        
#ifdef EBADF
        case EBADF: return TE_EBADF;
#endif        
        
#ifdef ECHILD
        case ECHILD: return TE_ECHILD;
#endif       
        
#ifdef EAGAIN
        case EAGAIN: return TE_EAGAIN;
#endif       
        
#ifdef ENOMEM
        case ENOMEM: return TE_ENOMEM;
#endif       
        
#ifdef EACCES
        case EACCES: return TE_EACCES;
#endif       
        
#ifdef EFAULT
        case EFAULT: return TE_EFAULT;
#endif       
        
#ifdef ENOTBLK
        case ENOTBLK: return TE_ENOTBLK;
#endif      
        
#ifdef EBUSY
        case EBUSY: return TE_EBUSY;
#endif        
        
#ifdef EEXIST
        case EEXIST: return TE_EEXIST;
#endif       
        
#ifdef EXDEV
        case EXDEV: return TE_EXDEV;
#endif        
        
#ifdef ENODEV
        case ENODEV: return TE_ENODEV;
#endif       
        
#ifdef ENOTDIR
        case ENOTDIR: return TE_ENOTDIR;
#endif      
        
#ifdef EISDIR
        case EISDIR: return TE_EISDIR;
#endif       
        
#ifdef EINVAL
        case EINVAL: return TE_EINVAL;
#endif       
        
#ifdef ENFILE
        case ENFILE: return TE_ENFILE;
#endif       
        
#ifdef EMFILE
        case EMFILE: return TE_EMFILE;
#endif       
        
#ifdef ENOTTY
        case ENOTTY: return TE_ENOTTY;
#endif       
        
#ifdef ETXTBSY
        case ETXTBSY: return TE_ETXTBSY;
#endif      
        
#ifdef EFBIG
        case EFBIG: return TE_EFBIG;
#endif        
        
#ifdef ENOSPC
        case ENOSPC: return TE_ENOSPC;
#endif       
        
#ifdef ESPIPE
        case ESPIPE: return TE_ESPIPE;
#endif       
        
#ifdef EROFS
        case EROFS: return TE_EROFS;
#endif        
        
#ifdef EMLINK
        case EMLINK: return TE_EMLINK;
#endif       
        
#ifdef EPIPE
        case EPIPE: return TE_EPIPE;
#endif        
        
#ifdef EDOM
        case EDOM: return TE_EDOM;
#endif         
        
#ifdef ERANGE
        case ERANGE: return TE_ERANGE;
#endif       
        
#ifdef EDEADLK
        case EDEADLK: return TE_EDEADLK;
#endif      
        
#ifdef ENAMETOOLONG
        case ENAMETOOLONG: return TE_ENAMETOOLONG;
#endif 
        
#ifdef ENOLCK
        case ENOLCK: return TE_ENOLCK;
#endif       
        
#ifdef ENOSYS
        case ENOSYS: return TE_ENOSYS;
#endif       
        
#ifdef ENOTEMPTY
        case ENOTEMPTY: return TE_ENOTEMPTY;
#endif    
        
#ifdef ELOOP
        case ELOOP: return TE_ELOOP;
#endif        
        
#if (defined(EWOULDBLOCK) && !(defined(EAGAIN) && EWOULDBLOCK == EAGAIN))
        case EWOULDBLOCK: return TE_EWOULDBLOCK;
#endif  
        
#ifdef ENOMSG
        case ENOMSG: return TE_ENOMSG;
#endif       
        
#ifdef EIDRM
        case EIDRM: return TE_EIDRM;
#endif        
        
#ifdef ECHRNG
        case ECHRNG: return TE_ECHRNG;
#endif       
        
#ifdef EL2NSYNC
        case EL2NSYNC: return TE_EL2NSYNC;
#endif     
        
#ifdef EL3HLT
        case EL3HLT: return TE_EL3HLT;
#endif       
        
#ifdef EL3RST
        case EL3RST: return TE_EL3RST;
#endif       
        
#ifdef ELNRNG
        case ELNRNG: return TE_ELNRNG;
#endif       
        
#ifdef EUNATCH
        case EUNATCH: return TE_EUNATCH;
#endif      
        
#ifdef ENOCSI
        case ENOCSI: return TE_ENOCSI;
#endif       
        
#ifdef EL2HLT
        case EL2HLT: return TE_EL2HLT;
#endif       
        
#ifdef EBADE
        case EBADE: return TE_EBADE;
#endif        
        
#ifdef EBADR
        case EBADR: return TE_EBADR;
#endif        
        
#ifdef EXFULL
        case EXFULL: return TE_EXFULL;
#endif       
        
#ifdef ENOANO
        case ENOANO: return TE_ENOANO;
#endif       
        
#ifdef EBADRQC
        case EBADRQC: return TE_EBADRQC;
#endif      
        
#ifdef EBADSLT
        case EBADSLT: return TE_EBADSLT;
#endif      
        
#if (defined(EDEADLOCK) && !(defined(EDEADLK) && EDEADLK == EDEADLOCK))
        case EDEADLOCK: return TE_EDEADLOCK;
#endif    
        
#ifdef EBFONT
        case EBFONT: return TE_EBFONT;
#endif       
        
#ifdef ENOSTR
        case ENOSTR: return TE_ENOSTR;
#endif       
        
#ifdef ENODATA
        case ENODATA: return TE_ENODATA;
#endif      
        
#ifdef ETIME
        case ETIME: return TE_ETIME;
#endif        
        
#ifdef ENOSR
        case ENOSR: return TE_ENOSR;
#endif        
        
#ifdef ENONET
        case ENONET: return TE_ENONET;
#endif       
        
#ifdef ENOPKG
        case ENOPKG: return TE_ENOPKG;
#endif       
        
#ifdef EREMOTE
        case EREMOTE: return TE_EREMOTE;
#endif      
        
#ifdef ENOLINK
        case ENOLINK: return TE_ENOLINK;
#endif      
        
#ifdef EADV
        case EADV: return TE_EADV;
#endif         
        
#ifdef ESRMNT
        case ESRMNT: return TE_ESRMNT;
#endif       
        
#ifdef ECOMM
        case ECOMM: return TE_ECOMM;
#endif        
        
#ifdef EPROTO
        case EPROTO: return TE_EPROTO;
#endif       
        
#ifdef EMULTIHOP
        case EMULTIHOP: return TE_EMULTIHOP;
#endif    
        
#ifdef EDOTDOT
        case EDOTDOT: return TE_EDOTDOT;
#endif      
        
#ifdef EBADMSG
        case EBADMSG: return TE_EBADMSG;
#endif      
        
#ifdef EOVERFLOW
        case EOVERFLOW: return TE_EOVERFLOW;
#endif    
        
#ifdef ENOTUNIQ
        case ENOTUNIQ: return TE_ENOTUNIQ;
#endif     
        
#ifdef EBADFD
        case EBADFD: return TE_EBADFD;
#endif       
        
#ifdef EREMCHG
        case EREMCHG: return TE_EREMCHG;
#endif      
        
#ifdef ELIBACC
        case ELIBACC: return TE_ELIBACC;
#endif      
        
#ifdef ELIBBAD
        case ELIBBAD: return TE_ELIBBAD;
#endif      
        
#ifdef ELIBSCN
        case ELIBSCN: return TE_ELIBSCN;
#endif      
        
#ifdef ELIBMAX
        case ELIBMAX: return TE_ELIBMAX;
#endif      
        
#ifdef ELIBEXEC
        case ELIBEXEC: return TE_ELIBEXEC;
#endif     
        
#ifdef EILSEQ
        case EILSEQ: return TE_EILSEQ;
#endif       
        
#ifdef ERESTART
        case ERESTART: return TE_ERESTART;
#endif     
        
#ifdef ESTRPIPE
        case ESTRPIPE: return TE_ESTRPIPE;
#endif     
        
#ifdef EUSERS
        case EUSERS: return TE_EUSERS;
#endif       
        
#ifdef ENOTSOCK
        case ENOTSOCK: return TE_ENOTSOCK;
#endif     
        
#ifdef EDESTADDRREQ
        case EDESTADDRREQ: return TE_EDESTADDRREQ;
#endif 
        
#ifdef EMSGSIZE
        case EMSGSIZE: return TE_EMSGSIZE;
#endif     
        
#ifdef EPROTOTYPE
        case EPROTOTYPE: return TE_EPROTOTYPE;
#endif   
        
#ifdef ENOPROTOOPT
        case ENOPROTOOPT: return TE_ENOPROTOOPT;
#endif  
        
#ifdef EPROTONOSUPPORT
        case EPROTONOSUPPORT: return TE_EPROTONOSUPPORT;
#endif 
        
#ifdef ESOCKTNOSUPPORT
        case ESOCKTNOSUPPORT: return TE_ESOCKTNOSUPPORT;
#endif 
        
#ifdef EOPNOTSUPP
        case EOPNOTSUPP: return TE_EOPNOTSUPP;
#endif   
        
#ifdef EPFNOSUPPORT
        case EPFNOSUPPORT: return TE_EPFNOSUPPORT;
#endif 
        
#ifdef EAFNOSUPPORT
        case EAFNOSUPPORT: return TE_EAFNOSUPPORT;
#endif 
        
#ifdef EADDRINUSE
        case EADDRINUSE: return TE_EADDRINUSE;
#endif   
        
#ifdef EADDRNOTAVAIL
        case EADDRNOTAVAIL: return TE_EADDRNOTAVAIL;
#endif
        
#ifdef ENETDOWN
        case ENETDOWN: return TE_ENETDOWN;
#endif     
        
#ifdef ENETUNREACH
        case ENETUNREACH: return TE_ENETUNREACH;
#endif  
        
#ifdef ENETRESET
        case ENETRESET: return TE_ENETRESET;
#endif    
        
#ifdef ECONNABORTED
        case ECONNABORTED: return TE_ECONNABORTED;
#endif 
        
#ifdef ECONNRESET
        case ECONNRESET: return TE_ECONNRESET;
#endif   
        
#ifdef ENOBUFS
        case ENOBUFS: return TE_ENOBUFS;
#endif      
        
#ifdef EISCONN
        case EISCONN: return TE_EISCONN;
#endif      
        
#ifdef ENOTCONN
        case ENOTCONN: return TE_ENOTCONN;
#endif     
        
#ifdef ESHUTDOWN
        case ESHUTDOWN: return TE_ESHUTDOWN;
#endif    
        
#ifdef ETOOMANYREFS
        case ETOOMANYREFS: return TE_ETOOMANYREFS;
#endif 
        
#ifdef ETIMEDOUT
        case ETIMEDOUT: return TE_ETIMEDOUT;
#endif    
        
#ifdef ECONNREFUSED
        case ECONNREFUSED: return TE_ECONNREFUSED;
#endif 
        
#ifdef EHOSTDOWN
        case EHOSTDOWN: return TE_EHOSTDOWN;
#endif    
        
#ifdef EHOSTUNREACH
        case EHOSTUNREACH: return TE_EHOSTUNREACH;
#endif 
        
#ifdef EALREADY
        case EALREADY: return TE_EALREADY;
#endif     
        
#ifdef EINPROGRESS
        case EINPROGRESS: return TE_EINPROGRESS;
#endif  
        
#ifdef ESTALE
        case ESTALE: return TE_ESTALE;
#endif       
        
#ifdef EUCLEAN
        case EUCLEAN: return TE_EUCLEAN;
#endif      
        
#ifdef ENOTNAM
        case ENOTNAM: return TE_ENOTNAM;
#endif      
        
#ifdef ENAVAIL
        case ENAVAIL: return TE_ENAVAIL;
#endif      
        
#ifdef EISNAM
        case EISNAM: return TE_EISNAM;
#endif       
        
#ifdef EREMOTEIO
        case EREMOTEIO: return TE_EREMOTEIO;
#endif    
        
#ifdef EDQUOT
        case EDQUOT: return TE_EDQUOT;
#endif       
        
#ifdef ENOMEDIUM
        case ENOMEDIUM: return TE_ENOMEDIUM;
#endif    
        
#ifdef EMEDIUMTYPE
        case EMEDIUMTYPE: return TE_EMEDIUMTYPE;
#endif
                          
#ifdef ECANCELED
        case ECANCELED: return TE_ECANCELED;
#endif  
 
        default: return TE_EUNKNOWN;
    }
}

#endif /* !__TE_ERRNO_H__ */
