/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent serial console support.
 *
 * @defgroup te_tools_te_serial Serial console
 * @ingroup te_tools
 * @{
 *
 * Definition of unix TA serial console configuring support.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_SERIAL_H__
#define __TE_SERIAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "te_serial_common.h"
#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_sockaddr.h"

/** List of the serial console parsers settings */
typedef struct serial_console_t {
    char    inst_name[TE_SERIAL_MAX_NAME + 1]; /**< Instance name */
    char    name[TE_SERIAL_MAX_NAME + 1];   /**< Name of the console */
    char    user[TE_SERIAL_MAX_NAME + 1];   /**< User name */

    int     port;                           /**< The serial console port */
    struct sockaddr_storage address;        /**< Serial console address */

    SLIST_ENTRY(serial_console_t)  next;    /**< Elements connector */
} serial_console_t;

/**
 * Initialize serial console configuration
 *
 * @return Status code
 */
extern te_errno ta_unix_serial_console_init(void);

/**
 * Cleanup serial console
 *
 * @return Status code
 */
extern te_errno ta_unix_serial_console_cleanup(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ndef __TE_SERIAL_H__ */
/**@} <!-- END te_tools_te_serial --> */
