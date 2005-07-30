/** @file
 * @brief Test API for TAD. iSCSI CSAP
 *
 * Implementation of Test API
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */


#ifndef __TE_TAPI_ISCSI_H__
#define __TE_TAPI_ISCSI_H__

#include <assert.h>
#include <netinet/in.h>

#include "te_stdint.h"
#include "tad_common.h"
#include "asn_usr.h"



/**
 * Creates 'iscsi' server CSAP
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param srv_csap      location for handle of new CSAP
 *
 * @return  Status code of the operation
 */
extern int tapi_iscsi_csap_create(const char *ta_name, int sid, 
                                  csap_handle_t *srv_csap);


/**
 * Receive one message from internal TA iSCSI target.
 * 
 * @param ta_name       test Agent name
 * @param sid           RCF SID
 * @param csap          identifier of CSAP
 * @param timeout       timeout of operation in milliseconds
 * @param params        location for iSCSI current params (OUT)
 * @param buffer        location for received data (OUT)
 * @param length        length of buffer / received data (IN/OUT)
 * 
 * @return Zero on success or error code.
 */
extern int tapi_iscsi_recv_pkt(const char *ta_name, int sid, 
                               csap_handle_t csap,
                               int timeout,
                               iscsi_target_params_t *params,
                               uint8_t *buffer,
                               size_t  *length);

/**
 * Send one message to internal TA iSCSI target.
 * 
 * @param ta_name       test Agent name
 * @param sid           RCF SID
 * @param csap          identifier of CSAP
 * @param timeout       timeout of operation in milliseconds
 * @param params        iSCSI new params
 * @param buffer        data to be sent
 * @param length        length of buffer
 * 
 * @return Zero on success or error code.
 */
extern int tapi_iscsi_send_pkt(const char *ta_name, int sid, 
                               csap_handle_t csap,
                               int timeout,
                               iscsi_target_params_t *params,
                               uint8_t *buffer,
                               size_t  length);

#endif /* !__TE_TAPI_ISCSI_H__ */
