/** @file
 * @brief Test Environment errno codes
 *
 * Definitions of errno codes
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_ERRNO_H__
#define __TE_ERRNO_H__

#include <errno.h>

/*
 * Errno may be either system (< TE_BASE) or TE-specific.
 * TE-specific errno consists of two parts: module errno base (2 bytes)
 * and module-specific error code defined in corresponding header.
 */

/** @name Common Test Environment errno's */
#define TE_BASE         (1 << 16)
#define ETESMALLBUF     (TE_BASE | 1) /* Too small buffer is provided to user */
#define ETEPENDING      (TE_BASE | 2) /* Pending data retain on connection */
#define ETESHCMD        (TE_BASE | 3) /* Shell command returned non-zero exit
                                         status */
#define ETEWRONGPTR     (TE_BASE | 4) /* Wrong pointer was passed to function */
#define ETENOSUPP       (TE_BASE | 5) /* Asked functionality is not supported */
#define ETOOMANY        (TE_BASE | 6) /* Too many objects have been already
                                         allocated, so that the resource is
                                         not available */
#define ETEFMT          (TE_BASE | 7) /* Invalid format */
#define ETEENV          (TE_BASE | 8) /* Inappropriate environment */
/*@}*/

/** @name Remote Control Facility errno's */
#define RCF_ERRNO_BASE  (2 << 16)
#define ETAREBOOTED     (RCF_ERRNO_BASE | 1) /* Test Agent is rebooted */
#define ETEBADFORMAT    (RCF_ERRNO_BASE | 2) /* Data of bad format are
                                                returned from the Test Agent */
#define ETENOSUCHNAME   (RCF_ERRNO_BASE | 3) /* Name of variable or routine
                                                or object identifier is not
                                                known */
#define ETEIO           (RCF_ERRNO_BASE | 4) /* Could not interact with RCF */
#define ETALOCAL        (RCF_ERRNO_BASE | 5) /* TA runs on the same
                                                station with TEN and
                                                cannot be rebooted */
#define ETENORCF        (RCF_ERRNO_BASE | 6) /* RCF initialization failed */
#define ETETARTN        (RCF_ERRNO_BASE | 7) /* Routine on the TA failed */
#define ETESUNRPC       (RCF_ERRNO_BASE | 8) /* SUN RPC failed */
#define ETECORRUPTED    (RCF_ERRNO_BASE | 9) /* Data are corrupted by the
                                                software under test */
#define ETERPCTIMEOUT   (RCF_ERRNO_BASE | 10) /* Timeout ocurred during
                                                 RPC call */                                                
#define ETERPCDEAD      (RCF_ERRNO_BASE | 11) /* RPC server is dead */ 
/*@}*/


/** @name Logger errno's */
#define LOGGER_ERRNO_BASE   (3 << 16)
/*@}*/


#define ASN_ERRNO_BASE    (4 << 16)

#define EASNGENERAL       (ASN_ERRNO_BASE | 1) 
#define EASNWRONGLABEL    (ASN_ERRNO_BASE | 2)  /**< Wrong ASN label */
#define EASNTXTPARSE      (ASN_ERRNO_BASE | 3)  /**< General ASN.1 text parse
                                                     error */
#define EASNDERPARSE      (ASN_ERRNO_BASE | 4)  /**< DER decode error */
#define EASNINCOMPLVAL    (ASN_ERRNO_BASE | 5)  /**< Imcomplete ASN.1 value */
#define EASNOTHERCHOICE   (ASN_ERRNO_BASE | 6)  /**< CHOICE in type is differ
                                                     then asked */
#define EASNWRONGTYPE     (ASN_ERRNO_BASE | 7)  /**< Passed value has wrong
                                                     type */
#define EASNNOTLEAF       (ASN_ERRNO_BASE | 8)  /**< Passed labels of subvalue 
                                                     does not respond to 
                                                     plain-syntax leaf  */

/** @name ASN.1 text parse errors */
#define EASNTXTNOTINT     (ASN_ERRNO_BASE | 9)   /**< int expected but not
                                                      found */
#define EASNTXTNOTCHSTR   (ASN_ERRNO_BASE | 0xA)   /**< character string
                                                      expected */
#define EASNTXTNOTOCTSTR  (ASN_ERRNO_BASE | 0xB) /**< octet string expected */
#define EASNTXTVALNAME    (ASN_ERRNO_BASE | 0xC) /**< wrong subvalue name in 
                                                      constraint value with 
                                                      named fields */
#define EASNTXTSEPAR      (ASN_ERRNO_BASE | 0xD) /**< wrong separator between 
                                                      elements in const.
                                                      value */
/*@}*/

/** @name Traffic Application Domain errno's */
#define TAD_ERRNO_BASE  (5 << 16)
#define ETADCSAPNOTEX   (TAD_ERRNO_BASE |1) /**< CSAP not exist. */
#define ETADLOWER       (TAD_ERRNO_BASE |2) /**< Lower layer error, usually
                                                 from some external library or 
                                                 OS resources, which is used 
                                                 for implementation of CSAP */
#define ETADCSAPSTATE   (TAD_ERRNO_BASE |3) /**< command dos not appropriate to 
                                                 CSAP state */
#define ETADNOTMATCH    (TAD_ERRNO_BASE |4) /**< data does not match 
                                                 to the specified pattern. */
#define ETADLESSDATA    (TAD_ERRNO_BASE |5) /**< read data matches to the
                                                 begin of pattern, but it is
                                                 not enough for all one,
                                                 or not enough data for 
                                                 generate.*/
#define ETADMISSNDS     (TAD_ERRNO_BASE |6) /**< Missing NDS, but it must. */
#define ETADWRONGNDS    (TAD_ERRNO_BASE |7) /**< Wrong NDS passed. */
#define ETADCSAPDB      (TAD_ERRNO_BASE |8) /**< CSAP DB internal error. */
#define ETADENDOFDATA   (TAD_ERRNO_BASE |9) /**< End of incoming data in CSAP */
#define ETADEXPRPARSE   (TAD_ERRNO_BASE|10) /**< Expression parse error. */

/*@}*/

/** @name Configurator errno's */
#define CONF_ERRNO_BASE (6 << 16) 
#define ETEBACKUP       (CONF_ERRNO_BASE | 1) /**< Backup verification failed */
#define ETEISROOT       (CONF_ERRNO_BASE | 2) /**< Attempt to delete the root */
#define ETEHASSON       (CONF_ERRNO_BASE | 3) /**< Attempt to delete the node
                                                   with children */
#define ETENOCONF       (CONF_ERRNO_BASE | 4) /**< Configurator
                                                   initialization failed */
#define ETEBADTYPE      (CONF_ERRNO_BASE | 5) /**< Type specified by the user
                                                   is incorrect */
/*@}*/

/** @name Tester errno's */
#define TESTER_ERRNO_BASE   (7 << 16)
#define ETESTPASS   (0)                     /**< Test passed */
#define ETESTFAKE   (TESTER_ERRNO_BASE | 1) /**< Test not really run */
#define ETESTSKIP   (TESTER_ERRNO_BASE | 2) /**< Test skipped */
#define ETESTCONF   (TESTER_ERRNO_BASE | 3) /**< Test changes configuration */
#define ETESTKILL   (TESTER_ERRNO_BASE | 4) /**< Test killed by signal */
#define ETESTCORE   (TESTER_ERRNO_BASE | 5) /**< Test dumped core */
#define ETESTPROLOG (TESTER_ERRNO_BASE | 6) /**< Session prologue failed */
#define ETESTEPILOG (TESTER_ERRNO_BASE | 7) /**< Session epilogue failed */
#define ETESTALIVE  (TESTER_ERRNO_BASE | 8) /**< Session keep-alive failed */
#define ETESTFAIL   (TESTER_ERRNO_BASE | 9) /**< Test failed */
#define ETESTUNEXP  (TESTER_ERRNO_BASE | 10) /**< Test unexpected results */

#define ETESTFAILMIN    ETESTFAKE   /**< Minimum test failure errno */
#define ETESTFAILMAX    ETESTUNEXP  /**< Maximum test failure errno */
/*@}*/


#define ESYNCFAILED 123


/** Shift of the module ID in 'int' (at least 32 bit) error code */
#define TE_RC_MODULE_SHIFT      24

/** Get identifier of the module generated an error */
#define TE_RC_GET_MODULE(_rc)   ((_rc) >> TE_RC_MODULE_SHIFT)

/** Get error number without module identifier */
#define TE_RC_GET_ERROR(_rc) \
    ((_rc) & \
     ((~(unsigned int)0) >> ((sizeof(unsigned int) * 8) - \
                                 TE_RC_MODULE_SHIFT)))

/** Compose base error code and module identifier */
#define TE_RC(_mod_id, _error) \
    ((_error && (TE_RC_GET_MODULE(_error) == 0)) ? \
     ((int)(_mod_id) << TE_RC_MODULE_SHIFT) | (_error) : (_error))


/**
 * @name Identifiers of TE modules used in errors
 *
 * @todo Make it 'enum' when %r specified will be supported.
 */
#define TE_RCF              (1)
#define TE_RCF_API          (2)
#define TE_RCF_CH           (3)
#define TE_TAD_CH           (4)
#define TE_TAD_CSAP         (5)
#define TE_TAPI             (6)
#define TE_CONF_API         (7)
#define TE_TA_LINUX         (8)
#define TE_TA_SWITCH_CTL    (9)
#define TE_RCF_RPC          (10)
#define TE_TA_WIN32         (11)
/*@}*/


/**
 * Update \i main return code, if it's OK, otherwise keep it.
 *
 * @param _rc       - main return code to be updated
 * @param _rc_new   - new return code to be assigned to main, if
 *                    main is OK
 *
 * @return Updated return code.
 */
#define RC_UPDATE(_rc, _rc_new) \
    ((_rc) = (((_rc) == 0) ? (_rc_new) : (_rc)))


#endif /* !__TE_ERRNO_H__ */
