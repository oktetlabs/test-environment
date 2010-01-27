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

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <stdsoap2.h>

#include "te_errno.h"
#include "te_queue.h"
#include "acse.h"

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
 * State machine for CWMP session entity:

   ( NOP )----->( LISTEN )------>[ Reply ]----->( WAIT_AUTH )<-\
      ^                                              |         |
      |                                              V         |
  [ Empty resp, close ]                         < Auth OK? > --/
      ^                                              |{Y}    {N}
      |{N}                                           V
  < Was HoldRequest? >--------------   ----[ Process Inform, reply ]
      ^               {Y}          |   |            
      |                            V   V  {POST}
      |     /--------------------( SERVE )----->[ Process SOAP RPC ]
      |     |       {Empty POST}       ^           |
      |{N}  V                           \----------/ 
 < Is pend.Req? >                 
      |{Y}  ^
      |     \-----------------------------------------\
      V                                               |
[ Send Request to CPE ]--->( WAIT_RESPONSE )---->[ Process Response ] 
   
 */

/** Session */
typedef struct {
    session_state_t state;         /**< Session state                  */
    int             hold_requests; /**< Whether to put "hold requests"
                                        in SOAP msg                    */
} session_t;

/** Device ID */
typedef struct {
    char const *manufacturer;  /**< Manufacturer                     */
    char const *oui;           /**< Organizational Unique Identifier */
    char const *product_class; /**< Product Class                    */
    char const *serial_number; /**< Serial Number                    */
} device_id_t;

/** CPE */
typedef struct {
    char const  *name;          /**< CPE name                      */
    char const  *ip_addr;       /**< CPE IP address                */
    char const  *url;           /**< CPE URL                       */
    char const  *cert;          /**< CPE certificate               */
    char const  *user;          /**< CPE user name                 */
    char const  *pass;          /**< CPE user password             */
    session_t    session;       /**< Session                       */
    device_id_t  device_id;     /**< Device Identifier             */
    struct soap *soap;          /**< Connected socket SOAP struct  */
} cpe_t;

/** CPE list */
typedef struct cpe_item_t
{
    STAILQ_ENTRY(cpe_item_t) link;
    cpe_t                    cpe;
} cpe_item_t;

/** ACS */
typedef struct {
    char const  *name;          /**< ACS name                       */
    char const  *url;           /**< ACS URL                        */
    char const  *cert;          /**< ACS certificate                */
    char const  *user;          /**< ACS user name                  */
    char const  *pass;          /**< ACS user password              */
    int          enabled;       /**< ACS enabled flag               */
    int          ssl;           /**< ACS ssl flag                   */
    int          port;          /**< ACS port value                 */
    STAILQ_HEAD(cpe_list_t, cpe_item_t)
                cpe_list;       /**< The list of CPEs being handled */
    struct soap *soap;          /**< Listenning socket SOAP struct  */
} acs_t;

/** ACS list */
typedef struct acs_item_t
{
    STAILQ_ENTRY(acs_item_t) link;
    acs_t                acs;
} acs_item_t;

typedef STAILQ_HEAD(acs_list_t, acs_item_t) acs_list_t;

/** The list af acs instances */
extern acs_list_t acs_list;

/** Abstraction structure for the 'channel' object */
typedef struct channel_t {
    void       *data;           /**< Channel-specific private data      */
    te_errno  (*before_select)( /**< Called before 'select' syscall     */
        void   *data,           /**< Channel-specific private data      */
        fd_set *rd_set,         /**< Descriptor set checked for reading */
        fd_set *wr_set,         /**< Descriptor set checked for writing */
        int    *fd_max);        /**< Storage for maximal of descriptors */
    te_errno  (*after_select)(  /**< Called after 'select' syscall      */
        void   *data,           /**< Channel-specific private data      */
        fd_set *rd_set,         /**< Descriptor set checked for reading */
        fd_set *wr_set);        /**< Descriptor set checked for writing */
    te_errno  (*recover_fds)(   /**< Called on error during select call */
        void   *data);          /**< Channel-specific private data      */
    te_errno  (*destroy)(       /**< Called on destroy                  */
        void   *data);          /**< Channel-specific private data      */
} channel_t;

typedef struct channel_item_t
{
    STAILQ_ENTRY(channel_item_t) link;
    channel_t                    channel;
} channel_item_t;

extern te_errno acse_lrpc_create(channel_t *channel,
                                 params_t *params, int sock);

extern te_errno acse_conn_create(channel_t *channel);

extern te_errno acse_sreq_create(channel_t *channel);

extern te_errno acse_cwmp_create(channel_t *channel);


extern int cwmp_SendConnectionRequest(const char *endpoint,
                                      const char *username, 
                                      const char *password);

#ifdef __cplusplus
}
#endif

#endif /* __TE_LIB_ACSE_INTERNAL_H__ */
