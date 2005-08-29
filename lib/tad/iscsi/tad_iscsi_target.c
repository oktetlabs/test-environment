/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * iSCSI CSAP support description structures. 
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * Author: Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * @(#) $Id$
 */

#include "config.h"

#include <string.h>

#include "tad_iscsi_impl.h"

#define TE_LGR_USER     "TAD iSCSI Target"
#include "logger_ta.h"

/* target thread function */
void *
iscsi_server_rx_thread(void *arg)
{
    int rc;
    uint8_t buffer[200];
    iscsi_target_thread_params_t *params = arg;

    if (arg == NULL)
    {
        ERROR("%s(): NULL arg");
        return NULL;
    }

    RING("%s(): called with send_recv handle %d", __FUNCTION__, 
         params->send_recv_csap);

    rc = iscsi_tad_recv(params->send_recv_csap, buffer, 200);
    RING("tad recv return %d", rc);

    if (rc > 0)
    {
        rc = iscsi_tad_send(params->send_recv_csap, buffer, 200);
        RING("tad send return %d", rc);
    }

    return NULL;
}


