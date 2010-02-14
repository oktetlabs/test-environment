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

#define LOG_LEVEL 0xff
#define TE_LOG_LEVEL 0xff

#include "te_config.h"

#include <poll.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif


#include <stdsoap2.h>

#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"
#include "acse_epc.h"
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


/* forward declaration */
struct acs_t;
struct cpe_t;
struct cwmp_session_t;
struct channel_t;

/** CPE RPC queue */
typedef struct cpe_rpc_t {
    TAILQ_ENTRY(cpe_rpc_t) links;
    te_cwmp_rpc_cpe_t      rpc_code;

    union {
        _cwmp__GetRPCMethods            *get_rpc_methods;
        _cwmp__SetParameterValues       *set_parameter_values;
        _cwmp__GetParameterValues       *get_parameter_values;
        _cwmp__GetParameterNames        *get_parameter_names;
        _cwmp__SetParameterAttributes   *set_parameter_attributes;
        _cwmp__GetParameterAttributes   *get_parameter_attributes;
        _cwmp__AddObject                *add_object;
        _cwmp__DeleteObject             *delete_object;
        _cwmp__Reboot                   *reboot;
        _cwmp__Download                 *download;
        _cwmp__Upload                   *upload;
        _cwmp__FactoryReset             *factory_reset;
        _cwmp__GetQueuedTransfers       *get_queued_transfers;
        _cwmp__GetAllQueuedTransfers    *get_all_queued_transfers;
        _cwmp__ScheduleInform           *schedule_inform;
        _cwmp__SetVouchers              *set_vouchers;
        _cwmp__GetOptions               *get_options;
    } arg;
} cpe_rpc_t;

/** CPE */
typedef struct cpe_t{
    /** Fields for internal data integrity. */

    LIST_ENTRY(cpe_t) links;
    struct acs_t    *acs;       /**< ACS, managing this CPE  */

    /** Fields corresponding to CM leafs in @p cpe node; some may change. */

    char const      *name;      /**< CPE record name         */
    char const      *url;       /**< CPE URL for Conn.Req.   */
    char const      *cert;      /**< CPE certificate         */
    char const      *username;  /**< CPE user name           */
    char const      *password;  /**< CPE user password       */
    int              enabled;   /**< Enabled CWMP func. flag */
    te_bool          hold_requests;  /**< Whether to put "hold requests"
                                        in SOAP msg                    */
    cwmp__DeviceIdStruct device_id; /**< Device Identifier       */

    TAILQ_HEAD(cpe_rpc_list_t, cpe_rpc_t)
                     rpc_list;  /**< List of RPC to be sent to CPE */

    /** Fields for internal procedure data during CWMP session. */

    struct cwmp_session_t *session; /**< CWMP session processing */
    struct sockaddr     *addr;      /**< CPE TCP/IP address for C.R.*/
    socklen_t            addr_len;  /**< address length          */ 
} cpe_t;

/** ACS */
typedef struct acs_t {
    /** Fields for internal data integrity. */
    LIST_ENTRY(acs_t) links;
    LIST_HEAD(cpe_list_t, cpe_t)
                cpe_list;       /**< The list of CPEs being handled */

    /** Fields corresponding to CM leafs in @p acs node. */
    const char  *name;          /**< ACS name                       */
    const char  *url;           /**< ACS URL                        */
    const char  *cert;          /**< ACS certificate                */
    const char  *username;      /**< ACS user name                  */
    const char  *password;      /**< ACS user password              */
    int          enabled;       /**< Enabled flag, true if listening
                                    new CWMP connections            */
    int          ssl;           /**< SSL usage flag                 */
    int          port;          /**< TCP port value in host byte order */

    /** Fields for internal procedure data. */
    struct sockaddr *addr_listen;/**< TCP/IP address to listen      */
    socklen_t        addr_len;   /**< address length */
    struct cwmp_session_t *session; /**< CWMP session, while it is not
                                       associated with particular CPE */
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



typedef struct cwmp_session_t {
    session_state_t      state;     /**< CWMP session state      */
    acs_t               *acs_owner;
    cpe_t               *cpe_owner;
    channel_t           *channel; 
    struct soap          m_soap;      /**< SOAP struct             */

    int (*orig_fparse)(struct soap*);
} cwmp_session_t;


/**
 * Allocate struct for new CWMP session, init session,
 * init gSOAP sturct, allocate ACSE IO channel for this session.
 * User of this function is reponsible for insert IO channel
 * into main loop.
 * 
 * @param socket        TCP connection socket, just accepted.
 * 
 * @return pointer to new struct or NULL if fails.
 */
extern cwmp_session_t *cwmp_new_session(int socket);


/**
 * Close CWMP session, release all data, finish related gSOAP
 * activity and release gSOAP internal data.
 * 
 * Note: passed pointer to session struct is not valid after
 * return from this function!
 * 
 * @return   status code
 */
extern te_errno cwmp_close_session(cwmp_session_t *sess);

extern int cwmp_SendConnectionRequest(const char *endpoint,
                                      const char *username, 
                                      const char *password);

/**
 * Check wheather accepted TCP connection is related to 
 * particular ACS; if it is, start processing of CWMP session.
 *
 * @return      0 if connection accepted by this ACS;
 *              TE_ECONNREFUSED if connection NOT accepted by this ACS;
 *              other error status on some error.
 */
extern te_errno cwmp_accept_cpe_connection(acs_t *acs, int socket);

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
 * @return              ACS instance address or NULL if not found
 */
extern acs_t * db_find_acs(const char *acs_name);

/**
 * Find a cpe instance from the cpe list of an acs instance
 *
 * @param acs_item      ACS object ptr, if already found, or NULL;
 * @param acs_name      Name of the acs instance;
 * @param cpe_name      Name of the cpe instance.
 *
 * @return              CPE instance address or NULL if not found
 */
extern cpe_t * db_find_cpe(acs_t *acs_item,
                           const char *acs_name,
                           const char *cpe_name);


/**
 * Remove a ACS object
 *
 * @param acs_item      ACS record to be removed
 *
 * @return              status code
 */
extern te_errno db_remove_acs(acs_t *acs_item);

/**
 * Remove a cpe instance from the cpe list of an acs instance
 *
 * @param cpe_item      CPE record to be removed
 *
 * @return              status code
 */
extern te_errno db_remove_cpe(cpe_t *cpe_item);


/**
 * Init EPC dispatcher.
 */
extern te_errno acse_epc_create(channel_t *channel, 
                                acse_params_t *params, int sock);


/**
 * Init TCP Listener dispatcher (named 'conn' - by old style. to be fixed).
 */
extern te_errno acse_conn_create(void);


extern void acse_add_channel(channel_t *ch_item);

extern void acse_remove_channel(channel_t *ch_item);

extern te_errno conn_register_acs(acs_t *acs);

extern te_errno acse_enable_acs(acs_t *acs);
#ifdef __cplusplus
}
#endif

#endif /* __TE_LIB_ACSE_INTERNAL_H__ */
