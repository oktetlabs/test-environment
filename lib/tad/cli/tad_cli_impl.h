/** @file
 * @brief TAD CLI
 *
 * Traffic Application Domain Command Handler.
 * CLI CSAP implementaion internal declarations.
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAD_CLI_IMPL_H__
#define __TE_TAD_CLI_IMPL_H__ 

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_EXPECT_H
#include <expect.h>
#elif defined(HAVE_TCL8_4_EXPECT_H)
#include <tcl8.4/expect.h>
#else
#error There is no expect headers in the system.
#endif

#include "te_errno.h"
#include "asn_usr.h" 
#include "ndn_cli.h"
#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_utils.h"


#include "logger_api.h"

typedef enum cli_conn_type {
    CLI_CONN_TYPE_SERIAL,   /**< Serial CLI connection (millicom) */
    CLI_CONN_TYPE_TELNET,   /**< Telnet CLI connection */
    CLI_CONN_TYPE_SSH,      /**< Ssh CLI connection */
    CLI_CONN_TYPE_SHELL,    /**< Shell (/bin/sh) CLI connection */
} cli_conn_type_t;

#define CLI_MAX_PROMPTS             4 /**< Maximum number of allowed prompts */

/* Define timeout used for waiting for prompt in CSAP creation procedure */
#ifndef CLI_CSAP_DEFAULT_TIMEOUT
#define CLI_CSAP_DEFAULT_TIMEOUT    25 /**< Seconds to wait for prompt */
#endif

#define CLI_PROMPT_STATUS_COMMAND   0x1 /**< command-prompt present */
#define CLI_PROMPT_STATUS_LOGIN     0x2 /**< login-prompt present */
#define CLI_PROMPT_STATUS_PASSWORD  0x4 /**< password-prompt present */

/** We still haven't got a reply for the previous command */
#define CLI_CSAP_STATUS_REPLY_WAITING 0x01

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * CLI CSAP specific data
 */
struct cli_csap_specific_data;
typedef struct cli_csap_specific_data *cli_csap_specific_data_p;
typedef struct cli_csap_specific_data
{
    int    io;          /**< file descriptor of CLI session stdin and stdout  */
    FILE  *fp;          /**< file descriptor of CLI session stdin and stdout  */
    pid_t  expect_pid;  /**< Expect process ID                                */
    pid_t  session_pid; /**< CLI session process ID                           */

    te_bool kernel_like_2_4; /**< Wheter we are working with 2.4 kernel or not */

    int data_sock; /**< Endpoint for communication with peer:
                        - on CSAP Engine: Used for sending commands and
                          for reading command results,
                        - on Expect side: For reading commands and
                          for sending command results */
    int sync_pipe; /**< Used for sync mesages sent from Expect side to
                        CSAP Engine. */
    size_t last_cmd_len; /**< The length of the last command run */

    cli_conn_type_t conn_type; /**< CLI protocol type                         */
    char *program;     /**< Default program to start (millicom, telnet,
                             ssh or sh) */

    char  *device;      /**< Default device (NULL if not defined)             */
    char  *host;        /**< Default remote host (NULL if not defined)        */
    unsigned short port;    /**< remote host port                             */

    char  *shell_args;  /**< Shell CLI session arguments (NULL if not defined)*/

    char  *user;        /**< Default user account (NULL if not defined)       */
    char  *password;    /**< Default user password (NULL if not defined)      */


    uint32_t status; /**< Status bits of the CSAP */
    uint32_t prompts_status;   /**< Available prompts found on init             */

    struct exp_case prompts[CLI_MAX_PROMPTS]; /**< expect cases structure that
                                                   contains known prompts     */

    int   read_timeout; /**< Number of second to wait for data                */
    
} cli_csap_specific_data_t;


/**
 * Callback for read data from media of CLI CSAP. 
 *
 * The function complies with csap_read_cb_t prototype.
 */ 
extern int tad_cli_read_cb(csap_p csap, int timeout, char *buf,
                           size_t buf_len);

/**
 * Callback for write data to media of CLI CSAP. 
 *
 * The function complies with csap_write_cb_t prototype.
 */ 
extern te_errno tad_cli_write_cb(csap_p csap, const tad_pkt *pkt);

/**
 * Callback for write data to media of CLI CSAP and read
 * data from media just after write, to get answer to sent request. 
 *
 * The function complies with csap_write_read_cb_t prototype.
 */ 
extern int tad_cli_write_read_cb(csap_p csap, int timeout,
                                 const tad_pkt *w_pkt,
                                 char *r_buf, size_t r_buf_len);


/**
 * Callback for init 'file' CSAP layer.
 *
 * The function complies with csap_rw_init_cb_t prototype.
 */ 
extern te_errno tad_cli_rw_init_cb(csap_p csap, const asn_value *csap_nds);

/**
 * Callback for destroy 'file' CSAP layer.
 *
 * The function complies with csap_rw_destroy_cb_t prototype.
 */ 
extern te_errno tad_cli_rw_destroy_cb(csap_p csap);

/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */ 
extern te_errno tad_cli_gen_bin_cb(csap_p                csap,
                                   unsigned int          layer,
                                   const asn_value      *tmpl_pdu,
                                   void                 *opaque,
                                   const tad_tmpl_arg_t *args,
                                   size_t                arg_num,
                                   tad_pkts             *sdus,
                                   tad_pkts             *pdus);

/**
 * Callback for parse received packet and match it with pattern. 
 *
 * The function complies with csap_layer_match_bin_cb_t prototype.
 */
extern te_errno tad_cli_match_bin_cb(csap_p          csap,
                                     unsigned int    layer, 
                                     const asn_value *pattern_pdu,
                                     const csap_pkts *pkt,
                                     csap_pkts       *payload, 
                                     asn_value       *parsed_packet);

/**
 * Callback for generating pattern to filter 
 * just one response to the packet which will be sent by this CSAP 
 * according to this template. 
 *
 * The function complies with csap_layer_gen_pattern_cb_t prototype.
 */
extern te_errno tad_cli_gen_pattern_cb(csap_p            csap,
                                       unsigned int      layer,
                                       const asn_value  *tmpl_pdu, 
                                       asn_value       **pattern_pdu);


/**
 * Create and bind raw socket to listen specified interface
 *
 * @param cli_type  CLI protocol type 
 * @param pkt_type  Type of packet socket (PACKET_HOST, PACKET_OTHERHOST,
 *                  PACKET_OUTGOING
 * @param if_index  interface index
 * @param sock      pointer to place where socket handler will be saved
 *
 * @param 0 on succees, -1 on fail
 */
extern int open_raw_socket(int cli_type, int pkt_type, int if_index, int *sock);


/**
 * Free all memory allocated by cli csap specific data
 *
 * @param cli_csap_specific_data_p poiner to structure
 *
 */ 
extern void free_cli_csap_data(cli_csap_specific_data_p spec_data);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_CLI_IMPL_H__ */
