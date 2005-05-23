/** @file
 * @brief Proteos, TAD CLI protocol, NDN.
 *
 * Declarations of ASN.1 types for NDN for CLI protocol. 
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
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
 * @author Alexander Kukuta <kam@oktetlabs.ru>
 *
 * $Id$
 */ 
#ifndef __TE_NDN_CLI_H__
#define __TE_NDN_CLI_H__

#include "asn_usr.h"
#include "ndn.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ASN.1 tags for CLI CSAP NDN
 */
typedef enum {
    NDN_CLI_MESSAGE,
    NDN_CLI_HOST,
    NDN_CLI_PORT,
    NDN_CLI_DEVICE,
    NDN_CLI_ARGS,
    NDN_CLI_TELNET,
    NDN_CLI_SHELL,
    NDN_CLI_SERIAL,
    NDN_CLI_CONN_TYPE,
    NDN_CLI_CONN_PARAMS,
    NDN_CLI_COMMAND_PROMPT,
    NDN_CLI_LOGIN_PROMPT,
    NDN_CLI_PASSWORD_PROMPT,
    NDN_CLI_USER,
    NDN_CLI_PASSWORD,
} ndn_cli_tags_t;

extern asn_type_p ndn_cli_message;
extern asn_type_p ndn_cli_serial_params;
extern asn_type_p ndn_cli_telnet_params;
extern asn_type_p ndn_cli_shell_params;
extern asn_type_p ndn_cli_params;
extern asn_type_p ndn_cli_csap;


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_NDN_CLI_H__ */
