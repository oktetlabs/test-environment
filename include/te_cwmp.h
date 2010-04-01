/** @file
 * @brief Test Environment CWMP support
 *
 * Definitions for CPE WAN Management Protocol (TR-069).
 * 
 * 
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_CWMP_H__
#define __TE_CWMP_H__

#include "te_stdint.h"

/**
 * This file contains definitions of structures and enumerated
 * types according with CWMP standard 
 * "TR-069: CPE WAN Management Protocol v1.1, Amendment 2".
 */



/**
 * This enumeration contains types of RPC calls, which 
 * could be called on CPE via CWMP, according with 
 * Table 5 in [TR-069].
 */
typedef enum {
    CWMP_RPC_NONE = 0,
    CWMP_RPC_get_rpc_methods,
    CWMP_RPC_set_parameter_values,
    CWMP_RPC_get_parameter_values,
    CWMP_RPC_get_parameter_names,
    CWMP_RPC_set_parameter_attributes,
    CWMP_RPC_get_parameter_attributes,
    CWMP_RPC_add_object,
    CWMP_RPC_delete_object,
    CWMP_RPC_reboot,
    CWMP_RPC_download,
    CWMP_RPC_upload,
    CWMP_RPC_factory_reset,
    CWMP_RPC_get_queued_transfers,
    CWMP_RPC_get_all_queued_transfers,
    CWMP_RPC_schedule_inform,
    CWMP_RPC_set_vouchers,
    CWMP_RPC_get_options,
} te_cwmp_rpc_cpe_t;

/**
 * This enumeration contains types of RPC calls, which 
 * could be called on ACS via CWMP, according with 
 * Table 5 in [TR-069].
 */
typedef enum {
    CWMP_RPC_ACS_get_rpc_methods,
    CWMP_RPC_inform,
    CWMP_RPC_transfer_complete,
    CWMP_RPC_autonomous_transfer_complete,
    CWMP_RPC_request_download,
    CWMP_RPC_kicked,
} te_cwmp_rpc_acs_t;

/**
 * This enumeration contains types of Events, which 
 * may occur in Inform from CPE to ACS, according with
 * Table 7 in [TR-069].
 */
typedef enum {
    CWMP_EV_0_BOOTSTRAP,
    CWMP_EV_1_BOOT,
    CWMP_EV_2_PERIODIC,
    CWMP_EV_3_SCHEDULED,
    CWMP_EV_4_VALUE_CHANGED,
    CWMP_EV_5_KICKED,
    CWMP_EV_6_CONNECTION_REQUEST,
    CWMP_EV_7_TRANSFER_COMPLETE,
    CWMP_EV_8_DIAGNOSTICS_COMPLETE,
    CWMP_EV_9_REQUEST_DOWNLOAD,
    CWMP_EV_10_AUTONOMOUS_TRANSFER_COMPLETE,
    CWMP_EV_M_Reboot,
    CWMP_EV_M_ScheduleInform,
    CWMP_EV_M_Download,
    CWMP_EV_M_Upload
} te_cwmp_event_code_t;




/** State of Connection Request to CPE. */
typedef enum acse_cr_state_t {
    CR_NONE = 0,        /**< No Conn.Req operation was inited */
    CR_WAIT_AUTH,       /**< Connection Request started, but waiting 
                            for successful authenticate. */
    CR_DONE,            /**< Connection Request was sent and gets 
                            successful HTTP response. 
                            Swith back to CR_NONE after receive Inform
                            with EventCode  = @c CONNECTION REQUEST*/
    CR_ERROR,            /**< Connection Request was sent and gets 
                            HTTP error. 
                            Swith back to CR_NONE after read 
                            Conn.Req. status by EPC */
} acse_cr_state_t;
    
/** CWMP Session states */
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
    CWMP_PENDING,       /**< CWMP session established, waiting for
                             RPC to be sent on CPE, from EPC.           */
} cwmp_sess_state_t;




#endif /*__TE_CWMP_H__ */
