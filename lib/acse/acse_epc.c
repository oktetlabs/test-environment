/** @file
 * @brief ACSE 
 *
 * ACSE EPC messaging support library
 *
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"


#include <string.h>
#include <ctype.h>

#include "acse_epc.h"

#include "te_stdint.h"
#include "te_errno.h"
#include "logger_api.h"

#include "cwmp_data.h"

static int epc_socket = -1;
static acse_epc_role_t epc_role = ACSE_EPC_SERVER;
static void *epc_shmem = NULL;

te_errno
acse_epc_open(const char *msg_sock_name, const char *shmem_name,
                              acse_epc_role_t role)
{
    epc_role = role;
    return 0;
}

int
acse_epc_sock(void)
{
    return epc_socket;
}

void*
acse_epc_shmem(void)
{
    return epc_shmem;
}

te_errno
acse_epc_close(void)
{
    return 0;
}

te_errno
acse_epc_send(const acse_epc_msg_t *params)
{
    return 0;
}

te_errno
acse_epc_recv(acse_epc_msg_t **params)
{
    return 0;
}
