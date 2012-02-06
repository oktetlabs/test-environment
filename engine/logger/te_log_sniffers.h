/** @file
 * @brief Unix Logger sniffers support.
 *
 * Defenition of unix Logger sniffers logging support.
 *
 * Copyright (C) 2004-2011 Test Environment authors (see file AUTHORS
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
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id: 
 */

#include "te_sniffers.h"
#include "te_queue.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * This is an entry point of sniffers message server.
 * This server should be run as separate thread.
 * All log messages from all sniffers entities will be processed by
 * this routine.
 * 
 * @param agent     Agent name
 */
extern void sniffers_handler(char *agent);

/**
 * This is an entry point of sniffers mark message server.
 * This server should be run as separate thread.
 * Mark messages to all sniffers tranmitted by this routine.
 * 
 * @param mark_data     Data for the marker packet
 */
extern void sniffer_mark_handler(char *mark_data);

/**
 * Make folder for capture logs or cleanup existing folder.
 * 
 * @param agt_fldr  Full path to the folder
 */
extern void sniffers_logs_cleanup(char *agt_fldr);

/**
 * Initialization of components to work of the sniffers.
 */
extern void sniffers_init(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
