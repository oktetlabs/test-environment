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

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#include "te_stdint.h"

#define TE_NEW_ERRNO    (1 << 30)

/** Type to store TE error numbers */
typedef enum {
    /* OS error codes */
    TE_EPERM = TE_NEW_ERRNO + 1,     
                      /**< Operation not permitted */
    TE_ENOENT,        /**< No such file or directory */
    TE_ESRCH,         /**< No such process */
    TE_EINTR,         /**< Interrupted system call */
    TE_EIO,           /**< I/O error */
    TE_ENXIO,         /**< No such device or address */
    TE_E2BIG,         /**< Arg list too long */
    TE_ENOEXEC,       /**< Exec format error */
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
    TE_EBFONT,        /**< Bad font file format */
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
    
    /* TE-specific error codes */
    
/** @name Common errno's */
    TE_EOK = TE_NEW_ERRNO + 500, 
                  /**< Success when 0 can't be used */
    TE_EFAIL,     /**< Generic failure */
    TE_ESMALLBUF, /**< Too small buffer is provided */
    TE_EPENDING,  /**< Pending data retain on connection */
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
    TE_ERCFIO = TE_NEW_ERRNO + 600, 
                     /**< Could not interact with RCF */
    TE_ENORCF,       /**< RCF initialization failed */
    TE_ETALOCAL,     /**< TA runs on the same station with TEN and
                          cannot be rebooted */
    TE_ETADEAD,      /**< Test Agent is dead */
    TE_ETAREBOOTED,  /**< Test Agent is rebooted */
    TE_ETARTN,       /**< Routine on the TA failed */
    TE_ESUNRPC,      /**< SUN RPC failed */
    TE_ECORRUPTED,   /**< Data are corrupted by the software under test */
    TE_ERPCTIMEOUT,  /**< Timeout ocurred during RPC call */
    TE_ERPCDEAD,     /**< RPC server is dead */
/*@}*/

/** @name ASN.1 text parse errors */
    TE_EASNGENERAL = TE_NEW_ERRNO + 700, 
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
    TE_ETADCSAPNOTEX = TE_NEW_ERRNO + 800,  
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
    TE_EBACKUP = TE_NEW_ERRNO + 900,  
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
    TE_ESTEMPTY = TE_NEW_ERRNO + 1000, 
                   /**< Test session/pkg is empty */
    TE_ESTSKIP,    /**< Test skipped */
    TE_ESTFAKE,    /**< Test not really run */
    TE_ESTPASS,    /**< Test passed */
    TE_ESTCONF,    /**< Test changed configuration */
    TE_ESTKILL,    /**< Test killed by signal */
    TE_ESTCORE,    /**< Test dumped core */
    TE_ESTPROLOG,  /**< Session prologue failed */
    TE_ESTEPILOG,  /**< Session epilogue failed */
    TE_ESTALIVE,   /**< Session keep-alive failed */
    TE_ESTFAIL,    /**< Test failed */
    TE_ESTUNEXP,   /**< Test unexpected results */

    TE_ESTRESULTMIN = ETESTEMPTY  /**< Minimum test result errno */
    TE_ESTRESULTMAX = ETESTUNEXP  /**< Maximum test result errno */
/*@}*/

/** @name TARPC errno's */
    TE_ERPC2H = TE_NEW_ERRNO + 1100,  /**< RPC to host conv failed */
    TE_EH2RPC,         /**< Host to RPC conv failed */
/*@}*/

/** @name IPC errno's */
    TE_ESYNCFAILED = TE_NEW_ERRNO + 1200 
                       /**< IPC synchronisation is broken */
/*@}*/
} te_errno;


/**
 * @name Identifiers of TE modules used in errors
 *
 * @todo Make it 'enum' when %r specified will be supported.
 */
typedef enum {
    TE_IPC              /**< TE IPC */
    TE_RCF              /**< RCF application */
    TE_RCF_UNIX         /**< UNIX-like agents management */
    TE_RCF_API          /**< RCF library */
    TE_RCF_RPC          /**< RCF RPC support */
    TE_RCF_PCH          /**< RCF Portable Command Handler */
    TE_RCF_CH           /**< RCF Command Handler */
    TE_TAD_CH           /**< TAD Command Handler */
    TE_TAD_CSAP         /**< TAD CSAP support */
    TE_TARPC            /**< RPC support in Test Agent */
    TE_CS               /**< Configuratorapplication */
    TE_CONF_API         /**< Configurator API */
    TE_TAPI             /**< Test API libraries */
    TE_TA_LINUX         /**< Linux Test Agent */
    TE_TA_WIN32         /**< Windows Test Agent */
    TE_TA_SWITCH_CTL    /**< Switch Control Test Agent */
    TE_NET_SNMP         /**< Errors from net-snmp library */
    TE_TA_EXT           /**< Error generated by external entity */
    TE_RPC              /**< System error returned by function called
                             via RPC */
} te_error_source;                             
/*@}*/


/** Shift of the module ID in 'int' (at least 32 bit) error code */
#define TE_RC_MODULE_SHIFT      24

/** Get identifier of the module generated an error */
#define TE_RC_GET_MODULE(_rc) \
    ((te_error_source)((_rc) >> TE_RC_MODULE_SHIFT))

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
#define TE_OS_RC(_mod_id, _error) \

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
 * @param src   integer source or errno (source / error code composition)
 *
 * @return string literal pointer
 */
static inline const char * 
te_rc_src2str(te_error_source src)
{
#define SRC2STR(name_) case TE_ ## name_: return #name_

    switch (TE_RC_GET_MODULE(src))
    {
        SRC2STR(IPC);        
        SRC2STR(RCF);   
        SRC2STR(RCF_UNIX);
        SRC2STR(RCF_API);    
        SRC2STR(RCF_RPC);    
        SRC2STR(RCF_PCH);     
        SRC2STR(RCF_CH);     
        SRC2STR(TAD_CH);   
        SRC2STR(TAD_CSAP);
        SRC2STR(TARPC);
        SRC2STR(CS);
        SRC2STR(CONF_API);
        SRC2STR(TAPI);   
        SRC2STR(TA_LINUX);   
        SRC2STR(TA_WIN32);
        SRC2STR(TA_SWITCH_CTL);
        SRC2STR(NET_SNMP);     
        SRC2STR(TA_EXT);
        SRC2STR(RPC);
        case 0: return "";
        default: return "Unknown";
    }
#undef SRC2STR    
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
    
    if ((err & TE_NEW_ERRNO) == 0)
    {
        TE_SPRINTF(old_errno, "Old errno 0x%X", TE_RC_GET_ERROR(err));
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
        ERR2STR(EOK); 
        ERR2STR(EFAIL);     
        ERR2STR(ESMALLBUF); 
        ERR2STR(EPENDING);  
        ERR2STR(ESHCMD);    
        ERR2STR(EWRONGPTR); 
        ERR2STR(ETOOMANY);  
                  
        ERR2STR(EFMT);      
        ERR2STR(EENV);      
        ERR2STR(EWIN);      
        ERR2STR(ERCFIO); 
        ERR2STR(ENORCF);        
        ERR2STR(ETALOCAL);     
        ERR2STR(ETADEAD);      
        ERR2STR(ETAREBOOTED);  
        ERR2STR(ETARTN);       
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

        ERR2STR(ESTEMPTY);
        ERR2STR(ESTSKIP);    
        ERR2STR(ESTFAKE);    
        ERR2STR(ESTPASS);    
        ERR2STR(ESTCONF);    
        ERR2STR(ESTKILL);    
        ERR2STR(ESTCORE);    
        ERR2STR(ESTPROLOG);  
        ERR2STR(ESTEPILOG);  
        ERR2STR(ESTALIVE);   
        ERR2STR(ESTFAIL);    
        ERR2STR(ESTUNEXP);   

        ERR2STR(ERPC2H);
        ERR2STR(EH2RPC);

        ERR2STR(ESYNCFAILED);
        
        default: return "Unknown";
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
        ERR2ERR(EPERM);        
                     
        ERR2ERR(ENOENT);       
        ERR2ERR(ESRCH);        
        ERR2ERR(EINTR);        
        ERR2ERR(EIO);          
        ERR2ERR(ENXIO);        
        ERR2ERR(E2BIG);        
        ERR2ERR(ENOEXEC);      
        ERR2ERR(EBADF);        
        ERR2ERR(ECHILD);       
        ERR2ERR(EAGAIN);       
        ERR2ERR(ENOMEM);       
        ERR2ERR(EACCES);       
        ERR2ERR(EFAULT);       
        ERR2ERR(ENOTBLK);      
        ERR2ERR(EBUSY);        
        ERR2ERR(EEXIST);       
        ERR2ERR(EXDEV);        
        ERR2ERR(ENODEV);       
        ERR2ERR(ENOTDIR);      
        ERR2ERR(EISDIR);       
        ERR2ERR(EINVAL);       
        ERR2ERR(ENFILE);       
        ERR2ERR(EMFILE);       
        ERR2ERR(ENOTTY);       
        ERR2ERR(ETXTBSY);      
        ERR2ERR(EFBIG);        
        ERR2ERR(ENOSPC);       
        ERR2ERR(ESPIPE);       
        ERR2ERR(EROFS);        
        ERR2ERR(EMLINK);       
        ERR2ERR(EPIPE);        
        ERR2ERR(EDOM);         
        ERR2ERR(ERANGE);       
        ERR2ERR(EDEADLK);      
        ERR2ERR(ENAMETOOLONG); 
        ERR2ERR(ENOLCK);       
        ERR2ERR(ENOSYS);       
        ERR2ERR(ENOTEMPTY);    
        ERR2ERR(ELOOP);        
        ERR2ERR(EWOULDBLOCK);  
        ERR2ERR(ENOMSG);       
        ERR2ERR(EIDRM);        
        ERR2ERR(ECHRNG);       
        ERR2ERR(EL2NSYNC);     
        ERR2ERR(EL3HLT);       
        ERR2ERR(EL3RST);       
        ERR2ERR(ELNRNG);       
        ERR2ERR(EUNATCH);      
        ERR2ERR(ENOCSI);       
        ERR2ERR(EL2HLT);       
        ERR2ERR(EBADE);        
        ERR2ERR(EBADR);        
        ERR2ERR(EXFULL);       
        ERR2ERR(ENOANO);       
        ERR2ERR(EBADRQC);      
        ERR2ERR(EBADSLT);      
        ERR2ERR(EDEADLOCK);    
        ERR2ERR(EBFONT);       
        ERR2ERR(ENOSTR);       
        ERR2ERR(ENODATA);      
        ERR2ERR(ETIME);        
        ERR2ERR(ENOSR);        
        ERR2ERR(ENONET);       
        ERR2ERR(ENOPKG);       
        ERR2ERR(EREMOTE);      
        ERR2ERR(ENOLINK);      
        ERR2ERR(EADV);         
        ERR2ERR(ESRMNT);       
        ERR2ERR(ECOMM);        
        ERR2ERR(EPROTO);       
        ERR2ERR(EMULTIHOP);    
        ERR2ERR(EDOTDOT);      
        ERR2ERR(EBADMSG);      
        ERR2ERR(EOVERFLOW);    
        ERR2ERR(ENOTUNIQ);     
        ERR2ERR(EBADFD);       
        ERR2ERR(EREMCHG);      
        ERR2ERR(ELIBACC);      
        ERR2ERR(ELIBBAD);      
        ERR2ERR(ELIBSCN);      
        ERR2ERR(ELIBMAX);      
        ERR2ERR(ELIBEXEC);     
        ERR2ERR(EILSEQ);       
        ERR2ERR(ERESTART);     
        ERR2ERR(ESTRPIPE);     
        ERR2ERR(EUSERS);       
        ERR2ERR(ENOTSOCK);     
        ERR2ERR(EDESTADDRREQ); 
        ERR2ERR(EMSGSIZE);     
        ERR2ERR(EPROTOTYPE);   
        ERR2ERR(ENOPROTOOPT);  
        ERR2ERR(EPROTONOSUPPORT); 
        ERR2ERR(ESOCKTNOSUPPORT); 
        ERR2ERR(EOPNOTSUPP);   
        ERR2ERR(EPFNOSUPPORT); 
        ERR2ERR(EAFNOSUPPORT); 
        ERR2ERR(EADDRINUSE);   
        ERR2ERR(EADDRNOTAVAIL);
        ERR2ERR(ENETDOWN);     
        ERR2ERR(ENETUNREACH);  
        ERR2ERR(ENETRESET);    
        ERR2ERR(ECONNABORTED); 
        ERR2ERR(ECONNRESET);   
        ERR2ERR(ENOBUFS);      
        ERR2ERR(EISCONN);      
        ERR2ERR(ENOTCONN);     
        ERR2ERR(ESHUTDOWN);    
        ERR2ERR(ETOOMANYREFS); 
        ERR2ERR(ETIMEDOUT);    
        ERR2ERR(ECONNREFUSED); 
        ERR2ERR(EHOSTDOWN);    
        ERR2ERR(EHOSTUNREACH); 
        ERR2ERR(EALREADY);     
        ERR2ERR(EINPROGRESS);  
        ERR2ERR(ESTALE);       
        ERR2ERR(EUCLEAN);      
        ERR2ERR(ENOTNAM);      
        ERR2ERR(ENAVAIL);      
        ERR2ERR(EISNAM);       
        ERR2ERR(EREMOTEIO);    
        ERR2ERR(EDQUOT);       
        ERR2ERR(ENOMEDIUM);    
        ERR2ERR(EMEDIUMTYPE);  
        ERR2ERR(EOK); 
        ERR2ERR(EFAIL);     
        ERR2ERR(ESMALLBUF); 
        ERR2ERR(EPENDING);  
        ERR2ERR(ESHCMD);    
        ERR2ERR(EWRONGPTR); 
        ERR2ERR(ETOOMANY);  
                  
        ERR2ERR(EFMT);      
        ERR2ERR(EENV);      
        ERR2ERR(EWIN);      
        ERR2ERR(ERCFIO); 
        ERR2ERR(ENORCF);        
        ERR2ERR(ETALOCAL);     
        ERR2ERR(ETADEAD);      
        ERR2ERR(ETAREBOOTED);  
        ERR2ERR(ETARTN);       
        ERR2ERR(ESUNRPC);      
        ERR2ERR(ECORRUPTED);   
        ERR2ERR(ERPCTIMEOUT);  
        ERR2ERR(ERPCDEAD);     

        ERR2ERR(EASNGENERAL);
        ERR2ERR(EASNWRONGLABEL);    
        ERR2ERR(EASNTXTPARSE);      
        ERR2ERR(EASNDERPARSE);      
        ERR2ERR(EASNINCOMPLVAL);    
        ERR2ERR(EASNOTHERCHOICE);   
        ERR2ERR(EASNWRONGTYPE);     
        ERR2ERR(EASNNOTLEAF);       
                          

        ERR2ERR(EASNTXTNOTINT);     
        ERR2ERR(EASNTXTNOTCHSTR);   
        ERR2ERR(EASNTXTNOTOCTSTR);  
        ERR2ERR(EASNTXTVALNAME);    
                          
        ERR2ERR(EASNTXTSEPAR);      
                          
        ERR2ERR(ETADCSAPNOTEX);  
        ERR2ERR(ETADLOWER);       
                        
                        
        ERR2ERR(ETADCSAPSTATE);   
        ERR2ERR(ETADNOTMATCH);    
        ERR2ERR(ETADLESSDATA);    
                        
                        
        ERR2ERR(ETADMISSNDS);     
        ERR2ERR(ETADWRONGNDS);    
        ERR2ERR(ETADCSAPDB);      
        ERR2ERR(ETADENDOFDATA);   
        ERR2ERR(ETADEXPRPARSE);   

        ERR2ERR(EBACKUP);
        ERR2ERR(EISROOT);  
        ERR2ERR(EHASSON);  
        ERR2ERR(ENOCONF);  
        ERR2ERR(EBADTYPE); 

        ERR2ERR(ESTEMPTY);
        ERR2ERR(ESTSKIP);    
        ERR2ERR(ESTFAKE);    
        ERR2ERR(ESTPASS);    
        ERR2ERR(ESTCONF);    
        ERR2ERR(ESTKILL);    
        ERR2ERR(ESTCORE);    
        ERR2ERR(ESTPROLOG);  
        ERR2ERR(ESTEPILOG);  
        ERR2ERR(ESTALIVE);   
        ERR2ERR(ESTFAIL);    
        ERR2ERR(ESTUNEXP);   

        ERR2ERR(ERPC2H);
        ERR2ERR(EH2RPC);

        ERR2ERR(ESYNCFAILED);
        
        default: return "Unknown";
    }
}

#endif /* !__TE_ERRNO_H__ */
