/** @file
 * @brief Proteos, TAD CLI protocol, NDN.
 *
 * Definitions of ASN.1 types for NDN for CLI protocol. 
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

#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_cli.h"

/* CLI-Message definitions */
static asn_named_entry_t _ndn_cli_message_ne_array [] = 
{
    { "message", &ndn_data_unit_char_string_s, {PRIVATE, NDN_CLI_MESSAGE} }
};

asn_type ndn_cli_message_s =
{
    "CLI-Message", {PRIVATE, NDN_TAD_CLI}, SEQUENCE,
    sizeof(_ndn_cli_message_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_cli_message_ne_array}
};

asn_type_p ndn_cli_message = &ndn_cli_message_s;


/* CLI-Telnet-Params definitions */
static asn_named_entry_t _ndn_cli_telnet_params_ne_array [] = 
{
    { "host", &ndn_data_unit_char_string_s, {PRIVATE, NDN_CLI_HOST} },
    { "port", &ndn_data_unit_int16_s, {PRIVATE, NDN_CLI_PORT} }
};

asn_type ndn_cli_telnet_params_s =
{
    "CLI-Telnet-Params", {PRIVATE, NDN_CLI_TELNET}, SEQUENCE,
    sizeof(_ndn_cli_telnet_params_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_cli_telnet_params_ne_array}
};

asn_type_p ndn_cli_telnet_params = &ndn_cli_telnet_params_s;


/* CLI-Serial-Params definitions */
static asn_named_entry_t _ndn_cli_serial_params_ne_array [] = 
{
    { "device", &ndn_data_unit_char_string_s, {PRIVATE, 1} }
};

asn_type ndn_cli_serial_params_s =
{
    "CLI-Serial-Params", {PRIVATE, NDN_CLI_SERIAL}, SEQUENCE,
    sizeof(_ndn_cli_serial_params_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_cli_serial_params_ne_array}
};

asn_type_p ndn_cli_serial_params = &ndn_cli_serial_params_s;


/* CLI-Shell-Params definitions */
static asn_named_entry_t _ndn_cli_shell_params_ne_array [] = 
{
    { "args", &ndn_data_unit_char_string_s, {PRIVATE, NDN_CLI_ARGS} }
};

asn_type ndn_cli_shell_params_s =
{
    "CLI-Shell-Params", {PRIVATE, NDN_CLI_SHELL}, SEQUENCE,
    sizeof(_ndn_cli_shell_params_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_cli_shell_params_ne_array}
};

asn_type_p ndn_cli_shell_params = &ndn_cli_shell_params_s;


/* CLI-Params definitions */
static asn_named_entry_t _ndn_cli_params_ne_array [] = 
{
    { "telnet", &ndn_cli_telnet_params_s, {PRIVATE, NDN_CLI_TELNET} },
    { "serial", &ndn_cli_serial_params_s, {PRIVATE, NDN_CLI_SERIAL} },
    { "shell",  &ndn_cli_shell_params_s,  {PRIVATE, NDN_CLI_SHELL} }
};

asn_type ndn_cli_params_s =
{
    "CLI-Params", {PRIVATE, NDN_CLI_CONN_PARAMS}, CHOICE,
    sizeof(_ndn_cli_params_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_cli_params_ne_array}
};

asn_type_p ndn_cli_params = &ndn_cli_params_s;


/* CLI-CSAP definitions */
static asn_named_entry_t _ndn_cli_csap_ne_array [] = 
{
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

asn_type ndn_cli_csap_s =
{
    "CLI-CSAP", {PRIVATE, NDN_TAD_CLI}, SEQUENCE, 
    sizeof(_ndn_cli_csap_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_cli_csap_ne_array}
};

asn_type_p ndn_cli_csap = &ndn_cli_csap_s;


