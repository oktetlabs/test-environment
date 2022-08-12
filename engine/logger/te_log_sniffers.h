/** @file
 * @brief Unix Logger sniffers support.
 *
 * Defenition of unix Logger sniffers logging support.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
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
