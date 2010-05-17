/** @file 
 * @brief ACSE 
 * 
 * ACSE internal declarations. 
 *
 * @ref acse-main_loop
 * 
 * @ref acse-cwmp_dispatcher
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

#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"
#include "acse_epc.h"
#include "cwmp_soapStub.h"
#include "acse_mem.h"


/**
 * @page acse-cwmp_dispatcher CWMP dispatcher
 * @{
 *
 * @section cwmp-st_m State machine diargam for CWMP session entity:

@htmlonly
<pre>

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
</pre>   
@endhtmlonly
 * @}
 */


/* forward declaration */
struct acs_t;
struct cpe_t;
struct cwmp_session_t;
struct channel_t;
struct conn_data_t;


/** HTTP Authentication mode in ACS */
typedef enum {
    ACSE_AUTH_NONE,  /**< No authentication */
    ACSE_AUTH_BASIC, /**< Basic HTTP authentication */
    ACSE_AUTH_DIGEST,/**< Digest HTTP authentication */
} auth_mode_t;

/**
 * Authentication data collection.
 */
typedef struct auth_t {
    const char *login;  /**< Login name */
    const char *passwd; /**< Password */
} auth_t;

/** CPE RPC queue */
typedef struct cpe_rpc_item_t {
    TAILQ_ENTRY(cpe_rpc_item_t)  links;/**< List links */
    /* TODO: move hear some fields from #acse_epc_cwmp_data_t 
       instead of this pointer to EPC request. */
    acse_epc_cwmp_data_t   *params;    /**< CWMP parameters for RPC */
    uint32_t                index;     /**< index of RPC in queue */
    mheap_t                 heap;      /**< Memory heapwhich contains 
                                        response data, deserialized by
                                        gSOAP engine. Should be freed 
                                        when response is removed from
                                        queue. */
                                        
} cpe_rpc_item_t;


/* TODO: think, maybe not all Inform should be stored, only some Events? */
/** CPE Inform list, in order they were received. */
typedef struct cpe_inform_t {
    LIST_ENTRY(cpe_inform_t) links;/**< List links */

    _cwmp__Inform *inform;      /**< Deserialed inform */
    uint32_t       index;       /**< index of Inform in list. */
} cpe_inform_t;

/** CPE record */
typedef struct cpe_t{
    /** Fields for internal data integrity. */
    LIST_ENTRY(cpe_t)  links;   /**< List links */
    struct acs_t      *acs;     /**< ACS, managing this CPE  */

    /** Fields corresponding to CM leafs in @p cpe node; some may change. */

    const char      *name;      /**< CPE record name.           */
    const char      *url;       /**< CPE URL for Conn.Req.      */
    const char      *cert;      /**< CPE SSL certificate.       */
    auth_t           cr_auth;   /**< Authenticate fields for 
                                     Connection Request.        */
    auth_t           acs_auth;  /**< Authenticate fields for 
                                     CPE->ACS Sessions.         */
    te_bool          enabled;   /**< Flag denotes whether CWMP sessions
                                     are enabled from this CPE.
                                     Set to FALSE during active 
                                     CWMP session leads to stop it.*/
    te_bool          hold_requests; /**< Whether to put "hold requests"
                                         in SOAP msg                   */

    cwmp__DeviceIdStruct device_id; /**< Device Identifier       */


    TAILQ_HEAD(cpe_rpc_queue_t, cpe_rpc_item_t)
                 rpc_queue;  /**< List of RPC to be sent to CPE */
    unsigned int last_queue_index; /**< Last used index in RPC queue.
                                        It increased every time when new
                                        RPC is added. */

    TAILQ_HEAD(cpe_rpc_results_t, cpe_rpc_item_t)
                 rpc_results;  /**< List of RPC responses from CPE */

    LIST_HEAD(cpe_inform_list_t, cpe_inform_t)
                 inform_list; /**< List of Informs, received from CPE;
                                   last Inform stored first in this list.*/

    /** Fields for internal procedure data during CWMP session. */

    struct cwmp_session_t *session;  /**< CWMP session processing     */
    struct sockaddr       *addr;     /**< CPE TCP/IP address for C.R. */
    socklen_t              addr_len; /**< address length              */ 

    acse_cr_state_t        cr_state; /**< State of ConnectionRequest  */
} cpe_t;


/** ACS */
typedef struct acs_t {
    /** Fields for internal data integrity. */
    LIST_ENTRY(acs_t) links;    /**< List links */
    LIST_HEAD(cpe_list_t, cpe_t)
                cpe_list;       /**< The list of CPEs being handled */

    /** Fields corresponding to CM leafs in @p acs node. */
    const char  *name;          /**< ACS name                       */
    const char  *url;           /**< ACS URL                        */
    const char  *cert;          /**< ACS certificate                */
    te_bool      ssl;           /**< SSL usage boolean flag         */
    uint16_t     port;          /**< TCP port value in host byte order */
    auth_mode_t  auth_mode;     /**< Authentication mode            */

    /** Fields for internal procedure data. */
    struct sockaddr *addr_listen;/**< TCP/IP address to listen      */
    socklen_t        addr_len;   /**< address length */
    struct conn_data_t    *conn_listen;  /**< Listen TCP connection 
                                      descriptor, 
                                      or NULL if ACS is disabled. */

    struct cwmp_session_t *session; /**< CWMP session, while it is not
                                       associated with particular CPE */
} acs_t;

/** Type for list of ACS objects */
typedef LIST_HEAD(acs_list_t, acs_t) acs_list_t;

/** The list af ACS instances */
extern acs_list_t acs_list;

/**
 * @page acse-main_loop Main loop
 * @{
 * ACSE main event loop process set of input abstract channels. 
 * Each channel described by instance of #channel_t structure.
 * Channels can be registered by acse_add_channel() procedure.
 * 
 * User, who register new channel, is responsible for initiate
 * all internal context before register, and for release it 
 * when channel is closed and channel_t::destroy callback is called.
 * 
 */

/** 
 * Abstraction structure for the 'channel' object 
 * @ingroup acse-main_loop
 */
typedef struct channel_t {
    LIST_ENTRY(channel_t) links;/**< List links */
    void       *data;           /**< Channel-specific private data      */

    te_errno  (*before_poll)(   
        void   *data,           /**< Channel-specific private data      */
        struct pollfd *pfd      /**< Poll descriptor for events         */
        );    /**< Called before poll(), should prepare @p pfd.  */
    te_errno  (*after_poll)(    
        void   *data,           /**< Channel-specific private data      */
        struct pollfd *pfd      /**< Poll descriptor for events         */
        );    /**< Called after poll(), should process detected event. 
                  Return code TE_ENOTCONN denote that underlying
                  connection is closed and channel should be finished. */
    te_errno  (*destroy)(       
        void   *data);          /**< Called on channel destroy    */
} channel_t;


/**
 * Add I/O channel to ACSE internal main loop.
 *
 * @param ch_item       Channel item, should have all callbacks 
 *                      set to function addresses.
 *
 * @return none
 */
extern void acse_add_channel(channel_t *ch_item);

/**
 * Remove I/O channel from ACSE internal main loop.
 * Call 'destroy' callback for channel.
 *
 * @param ch_item       Channel. 
 *
 * @return none
 */
extern void acse_remove_channel(channel_t *ch_item);

/**
 * @}
 */

/** Descriptor of active CWMP session.
 *  Used as user-info in gSOAP internal struct. 
 *  Fields cwmp_session_t::acs_owner and 
 */
typedef struct cwmp_session_t {
    cwmp_sess_state_t    state;     /**< CWMP session state.        */
    acs_t               *acs_owner; /**< NULL or master ACS.        */
    cpe_t               *cpe_owner; /**< NULL or master CPE record. */
    cpe_rpc_item_t      *rpc_item;  /**< NULL or last sent RPC in 
                                          @c WAIT_RESPONSE state.   */
    channel_t           *channel;   /**< I/O ACSE channel.          */
    struct soap          m_soap;     /**< SOAP struct               */
    mheap_t              def_heap;   /**< default memory heap,
                                           when @p rpc_item is NULL */

    int (*orig_fparse)(struct soap*);/**< Original fparse in gSOAP  */
} cwmp_session_t;


/**
 * Allocate struct for new CWMP session, init session,
 * init gSOAP struct, init Digest plugin or SSL if need, 
 * allocate ACSE I/O channel for this session,
 * insert I/O channel into main loop.
 * 
 * @param socket        TCP connection socket, just accepted.
 * @param acs           ACS object which responsible for this session.
 * 
 * @return status code
 */
extern te_errno cwmp_new_session(int socket, acs_t *acs);


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


/**
 * Check wheather accepted TCP connection is related to 
 * particular ACS; if it is, start processing of CWMP session.
 * This procedure does not wait any data in TCP connection and 
 * does not perform regular read from it. 
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
extern acs_t *db_find_acs(const char *acs_name);

/**
 * Find a cpe instance from the cpe list of an acs instance
 *
 * @param acs_item      ACS object ptr, if already found, or NULL;
 * @param acs_name      Name of the acs instance;
 * @param cpe_name      Name of the cpe instance.
 *
 * @return              CPE instance address or NULL if not found
 */
extern cpe_t *db_find_cpe(acs_t *acs_item,
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
 * 
 * @param msg_sock_name    name of PF_UNIX socket for EPC messages, 
 *                              or NULL for internal EPC default;
 * @param shmem_name       name of shared memory space for EPC data
 *                            exchange or NULL for internal default.
 * 
 * @return              status code
 */
extern te_errno acse_epc_disp_init(const char *msg_sock_name,
                                   const char *shmem_name);


/**
 * Init TCP Listener dispatcher (named 'conn' - by old style. to be fixed).
 * 
 * @return              status code
 */
extern te_errno acse_conn_create(void);



/**
 * Register ACS object in TCP Connection Dispatcher.
 * This action require to set on remote_addr internal field in ACS.
 * 
 * @param acs           ACS object
 *
 * @return status code
 */
extern te_errno conn_register_acs(acs_t *acs);

/**
 * Register ACS object in TCP Connection Dispatcher.
 * This action require to set on remote_addr internal field in ACS.
 * 
 * @param acs           ACS object
 *
 * @return status code
 */
extern te_errno conn_deregister_acs(acs_t *acs);

/**
 * Enable ACS object to receipt CWMP sessions.
 * This action require to set on @p port field in ACS. 
 * After success of this action ACS object will be registered 
 * in TCP Connection Dispatcher.
 * 
 * @param acs           ACS object
 *
 * @return status code
 */
extern te_errno acse_enable_acs(acs_t *acs);

/**
 * Disable ACS object to receipt CWMP sessions, deregister
 * ACS from TCP Connection Dispatcher.
 * 
 * @param acs           ACS object
 *
 * @return status code
 */
extern te_errno acse_disable_acs(acs_t *acs);

/**
 * Initiate CWMP Connection Request to specified CPE. 
 *
 * @param cpe_item      CPE
 *
 * @return status code.
 */
extern te_errno acse_init_connection_request(cpe_t *cpe_item);


/**
 * Main ACSE loop: waiting events from all channels, processing them.
 */
extern void acse_loop(void);


/**
 * Clear ACSE internal main loop channels; that is call destructor 
 * for every registered channel.
 * Usually means finish of ACSE operation.
 */
extern void acse_clear_channels(void);


/**
 * Memory allocation wrapper for SOAP engine. 
 * Designed for usage of deserialized SOAP data after free of soap 
 * context, because RPC replies and Informs may wait their report 
 * after CWMP session close.
 *
 * @param soap          gSOAP context.
 * @param n             size of block.
 *
 * @return pointer to allocated memory block, which first @p n bytes 
 *         are available for user. 
 */
extern void *acse_cwmp_malloc(struct soap *soap, size_t n);

/**
 * Read from gSOAP HTTP request which is RPC response to previous call.
 *
 * @param cwmp_sess     CWMP session descriptor.
 */
extern void acse_soap_serve_response(cwmp_session_t *cwmp_sess);


/**
 * Free allocated memory, related to RPC item in #rpc_queue or
 * #rpc_results. Should be called after remove item from respecitve
 * queue.
 *
 * @param rpc_item      pointer to item.
 *
 * @return status
 */ 
extern te_errno acse_rpc_item_free(cpe_rpc_item_t *rpc_item);

#ifdef __cplusplus
}
#endif

#endif /* __TE_LIB_ACSE_INTERNAL_H__ */
