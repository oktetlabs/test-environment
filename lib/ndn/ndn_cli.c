/** @file
 * @brief Proteos, TAD CLI protocol, NDN.
 *
 * Definitions of ASN.1 types for NDN for CLI protocol.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#include "te_config.h"

#include "asn_impl.h"
#include "tad_common.h"
#include "ndn_internal.h"
#include "ndn_cli.h"

/* CLI-Message definitions */
static asn_named_entry_t _ndn_cli_message_ne_array [] = {
    { "message",        &ndn_data_unit_char_string_s,
        {PRIVATE, NDN_CLI_MESSAGE} },
    { "command-prompt", &ndn_data_unit_char_string_s,
        {PRIVATE, NDN_CLI_COMMAND_PROMPT} },
    { "password-prompt",&ndn_data_unit_char_string_s,
        {PRIVATE, NDN_CLI_PASSWORD_PROMPT} },
    { "password",       &ndn_data_unit_char_string_s,
        {PRIVATE, NDN_CLI_PASSWORD} },
};

asn_type ndn_cli_message_s = {
    "CLI-Message", {PRIVATE, TE_PROTO_CLI}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_cli_message_ne_array),
    {_ndn_cli_message_ne_array}
};

asn_type * ndn_cli_message = &ndn_cli_message_s;


/* CLI-Telnet-Params definitions */
static asn_named_entry_t _ndn_cli_telnet_params_ne_array [] = {
    { "host", &ndn_data_unit_char_string_s, {PRIVATE, NDN_CLI_HOST} },
    { "port", &ndn_data_unit_int16_s, {PRIVATE, NDN_CLI_PORT} }
};

asn_type ndn_cli_telnet_params_s = {
    "CLI-Telnet-Params", {PRIVATE, NDN_CLI_TELNET}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_cli_telnet_params_ne_array),
    {_ndn_cli_telnet_params_ne_array}
};

asn_type * ndn_cli_telnet_params = &ndn_cli_telnet_params_s;


/* CLI-Serial-Params definitions */
static asn_named_entry_t _ndn_cli_serial_params_ne_array [] = {
    { "device", &ndn_data_unit_char_string_s, {PRIVATE, 1} }
};

asn_type ndn_cli_serial_params_s = {
    "CLI-Serial-Params", {PRIVATE, NDN_CLI_SERIAL}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_cli_serial_params_ne_array),
    {_ndn_cli_serial_params_ne_array}
};

asn_type * ndn_cli_serial_params = &ndn_cli_serial_params_s;


/* CLI-Shell-Params definitions */
static asn_named_entry_t _ndn_cli_shell_params_ne_array [] = {
    { "args", &ndn_data_unit_char_string_s, {PRIVATE, NDN_CLI_ARGS} }
};

asn_type ndn_cli_shell_params_s = {
    "CLI-Shell-Params", {PRIVATE, NDN_CLI_SHELL}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_cli_shell_params_ne_array),
    {_ndn_cli_shell_params_ne_array}
};

asn_type * ndn_cli_shell_params = &ndn_cli_shell_params_s;


/* CLI-Params definitions */
static asn_named_entry_t _ndn_cli_params_ne_array [] = {
    { "telnet", &ndn_cli_telnet_params_s, {PRIVATE, NDN_CLI_TELNET} },
    { "serial", &ndn_cli_serial_params_s, {PRIVATE, NDN_CLI_SERIAL} },
    { "shell",  &ndn_cli_shell_params_s,  {PRIVATE, NDN_CLI_SHELL} }
};

asn_type ndn_cli_params_s = {
    "CLI-Params", {PRIVATE, NDN_CLI_CONN_PARAMS}, CHOICE,
    TE_ARRAY_LEN(_ndn_cli_params_ne_array),
    {_ndn_cli_params_ne_array}
};

asn_type * ndn_cli_params = &ndn_cli_params_s;


/* CLI-CSAP definitions */
static asn_named_entry_t _ndn_cli_csap_ne_array [] = {
    { "conn-type",      &asn_base_integer_s,
        {PRIVATE, NDN_CLI_CONN_TYPE} },
    { "conn-params",    &ndn_cli_params_s,
        {PRIVATE, NDN_CLI_CONN_PARAMS} },
    { "command-prompt", &ndn_data_unit_char_string_s,
        {PRIVATE, NDN_CLI_COMMAND_PROMPT} },
    { "login-prompt",   &ndn_data_unit_char_string_s,
        {PRIVATE, NDN_CLI_LOGIN_PROMPT} },
    { "password-prompt",&ndn_data_unit_char_string_s,
        {PRIVATE, NDN_CLI_PASSWORD_PROMPT} },
    { "user",           &ndn_data_unit_char_string_s,
        {PRIVATE, NDN_CLI_USER} },
    { "password",       &ndn_data_unit_char_string_s,
        {PRIVATE, NDN_CLI_PASSWORD} },
};

asn_type ndn_cli_csap_s = {
    "CLI-CSAP", {PRIVATE, TE_PROTO_CLI}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_cli_csap_ne_array),
    {_ndn_cli_csap_ne_array}
};

asn_type * ndn_cli_csap = &ndn_cli_csap_s;
