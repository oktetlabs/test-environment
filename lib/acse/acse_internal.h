/** @file 
 * @brief ACSE 
 * 
 * ACSE internal declarations. 
 *
 *
 * Copyright (C) 2009-2010 Test Environment authors (see file AUTHORS
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
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
*/

#ifndef __TE_LIB_ACSE_INTERNAL_H__
#define __TE_LIB_ACSE_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "te_config.h"

#include <poll.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <stdsoap2.h>

#include "te_errno.h"
#include "te_queue.h"
#include "acse.h"
#include "acse_soapStub.h"

/** Session states */
typedef enum { 
    CWMP_NOP,           /**< No any TCP activity: neither active
                          connection, nor listening for incoming ones.  */
    CWMP_LISTEN,        /**< Listening for incoming HTTP connection.    */
    CWMP_WAIT_AUTH,     /**< TCP connection established, first HTTP
                            request received, but not authenicated,
                            response with our WWW-Authenticate is sent. */
    CWMP_SERVE,         /**< CWMP session established, waiting for
                            incoming SOAP RPC requests from CPE.        */
    CWMP_WAIT_RESPONSE, /**< CWMP session established, SOAP RPC is sent
                            to the CPE, waiting for response.           */
} session_state_t;

/*
 * State machine diargam for CWMP session entity:

   ( NOP )----->( LISTEN )------>[ Reply ]----->( WAIT_AUTH )<-\
      ^                                              |         |
      |                                              V         |
  [ Empty resp, close ]                         < Auth OK? > --/
      ^                                              |{Y}    {N}
      |{N}                                           V
  < Was HoldRequest? >-------------\   /---[ Process Inform, reply ]
      ^               {Y}          |   |            
      |                            V   V  {POST}
      |          /---------------( SERVE )----->[ Process SOAP RPC ]
      |          |  {Empty POST}       ^           |
      |{N}       V                     \-----------/ 
 < Have pending Req to CPE? >                 
      |{Y}       ^
      |          \------------------------------------\
      V                                               |
[ Send Request to CPE ]--->( WAIT_RESPONSE )---->[ Process Response ] 
   
 */


/** Session */
typedef struct {
    session_state_t state;         /**< Session state                  */
    int             hold_requests; /**< Whether to put "hold requests"
                                        in SOAP msg                    */
} session_t;

/* forward declaration */
struct acs_struct;

/** CPE */
typedef struct cpe_t{
    LIST_ENTRY(cpe_t) links;

    char const          *name;      /**< CPE record name         */
    char const          *url;       /**< CPE URL for Conn.Req.   */
    struct sockaddr     *addr;      /**< CPE TCP/IP address for C.R.*/
    socklen_t            addr_len;  /**< address length          */
    char const          *cert;      /**< CPE certificate         */
    char const          *username;  /**< CPE user name           */
    char const          *password;  /**< CPE user password       */
    session_t            session;   /**< Session                 */
    cwmp__DeviceIdStruct device_id; /**< Device Identifier       */
    int                  enabled;   /**< Enabled CWMP func. flag */
    struct soap         *soap;      /**< SOAP struct             */
    struct acs_t        *acs;       /**< ACS, managing this CPE  */
} cpe_t;

/** ACS */
typedef struct acs_t {
    LIST_ENTRY(acs_t) links;

    const char  *name;          /**< ACS name                       */
    const char  *url;           /**< ACS URL                        */
    const char  *cert;          /**< ACS certificate                */
    const char  *username;      /**< ACS user name                  */
    const char  *password;      /**< ACS user password              */

    struct sockaddr *addr_listen;/**< TCP/IP address to listen      */
    socklen_t        addr_len;   /**< address length */

    int          enabled;       /**< ACS enabled flag               */
    int          ssl;           /**< ACS ssl flag                   */
    int          port;          /**< ACS port value                 */
    LIST_HEAD(cpe_list_t, cpe_t)
                cpe_list;       /**< The list of CPEs being handled */
} acs_t;


typedef LIST_HEAD(acs_list_t, acs_t) acs_list_t;

/** The list af acs instances */
extern acs_list_t acs_list;

/** Abstraction structure for the 'channel' object */
typedef struct channel_t {
    LIST_ENTRY(channel_t) links;
    void       *data;           /**< Channel-specific private data      */
    te_errno  (*before_poll)(   /**< Called before 'select' syscall     */
        void   *data,           /**< Channel-specific private data      */
        struct pollfd *pfd);    /**< Poll descriptor for events         */
    te_errno  (*after_poll)(    /**< Called after 'select' syscall      */
        void   *data,           /**< Channel-specific private data      */
        struct pollfd *pfd);    /**< Poll descriptor for events         */
    te_errno  (*destroy)(       /**< Called on destroy                  */
        void   *data);          /**< Channel-specific private data      */
} channel_t;


extern int cwmp_SendConnectionRequest(const char *endpoint,
                                      const char *username, 
                                      const char *password);

/**
 * Check wheather accepted TCP connection is related to 
 * particular ACS.
 *
 * @return      0 if connection accepted by this ACS;
 *              TE_ECONNREFUSED if connection NOT accepted by this ACS;
 *              other error status on some error.
 */
extern te_errno cwmp_check_cpe_connection(acs_t *acs, int socket);

/**
 * Add an ACS object to internal DB
 *
 * @param acs_name      Name of the ACS
 *
 * @return              status code
 *
 */
extern te_errno db_add_acs(const char *acs_name);

/**
 * Add a CPE record for particular ACS object to internal DB
 *
 * @param acs_name      Name of the ACS
 * @param cpe_name      Name of the CPE 
 *
 * @return              status code
 *
 */
extern te_errno db_add_cpe(const char *acs_name, const char *cpe_name);

/**
 * Find an acs instance from the acs list
 *
 * @param acs_name      Name of the acs instance
 *
 * @return              Acs instance address or NULL if not found
 */
extern acs_t * db_find_acs(const char *acs_name);

/**
 * Find a cpe instance from the cpe list of an acs instance
 *
 * @param acs_item      ACS object ptr, if already found, or NULL;
 * @param acs_name      Name of the acs instance;
 * @param cpe_name      Name of the cpe instance.
 *
 * @return              Cpe instance address or NULL if not found
 */
extern cpe_t * db_find_cpe(acs_t *acs_item,
                           const char *acs_name,
                           const char *cpe_name);




/**
 * Init EPC dispatcher.
 */
extern te_errno acse_epc_create(channel_t *channel, params_t *params,
                               int sock);


/**
 * Init TCP Listener dispatcher (named 'conn' - by old style. to be fixed).
 */
extern te_errno acse_conn_create(void);


extern void acse_add_channel(channel_t *ch_item);

extern void acse_remove_channel(channel_t *ch_item);

extern te_errno conn_register_acs(acs_t *acs);
#ifdef __cplusplus
}
#endif

#endif /* __TE_LIB_ACSE_INTERNAL_H__ */
