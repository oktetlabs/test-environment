/** @file 
 * @brief ACSE 
 * 
 * ACSE user interface declarations. 
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */
#ifndef __TE_LIB_ACSE_INTERNAL_H__
#define __TE_LIB_ACSE_INTERNAL_H__
#include "te_config.h"

/**
 * Main ACSE loop: waiting events from all channels, processing them.
 */
extern void acse_loop(void);

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
 */
extern te_errno acse_send_set_parameter_values(int *request_id,
                                               ...);

#endif /* __TE_LIB_ACSE_INTERNAL_H__ */
