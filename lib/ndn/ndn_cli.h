/** @file
 * @brief Proteos, TAD CLI protocol, NDN.
 *
 * Declarations of ASN.1 types for NDN for CLI protocol.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Alexander Kukuta <kam@oktetlabs.ru>
 *
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

extern asn_type *ndn_cli_message;
extern asn_type *ndn_cli_serial_params;
extern asn_type *ndn_cli_telnet_params;
extern asn_type *ndn_cli_shell_params;
extern asn_type *ndn_cli_params;
extern asn_type *ndn_cli_csap;


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_NDN_CLI_H__ */
