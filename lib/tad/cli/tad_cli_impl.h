/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * CLI CSAP implementaion internal declarations.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 * Author: Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * @(#) $Id$
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

#define CLI_CONN_TYPE_SERIAL        0 /**< serial CLI connection (millicom) */
#define CLI_CONN_TYPE_TELNET        1 /**< telnet CLI connection */
#define CLI_CONN_TYPE_SSH           2 /**< ssh CLI connection */
#define CLI_CONN_TYPE_SHELL         3 /**< shell (/bin/sh) CLI connection */

#define CLI_MAX_PROMPTS             4 /**< Maximum number of allowed prompts */

#ifndef CLI_CSAP_DEFAULT_TIMEOUT
#define CLI_CSAP_DEFAULT_TIMEOUT    5 /**< Seconds to wait for incoming data */
#endif

#define CLI_PROMPT_STATUS           0x7 /**< prompt status mask */
#define CLI_PROMPT_STATUS_COMMAND   0x1 /**< command-prompt is present */
#define CLI_PROMPT_STATUS_LOGIN     0x2 /**< login-prompt is present */
#define CLI_PROMPT_STATUS_PASSWORD  0x4 /**< password-prompt is present */

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
    FILE  *dbg_file;    /**< file to gather debug data from expect session    */
    int    io;          /**< file descriptor of CLI session stdin and stdout  */
    FILE  *fp;          /**< file descriptor of CLI session stdin and stdout  */
    pid_t  expect_pid;  /**< Expect process id                                */
    pid_t  session_pid; /**< CLI session process id                           */

    int    sync_p2c[2]; /**< CSAP to Expect process pipe filedescriptors      */
    int    sync_c2p[2]; /**< Expect process to CSAP pipe filedescriptors      */

    int    conn_type;   /**< CLI protocol type                                */
    char  *program;     /**< Default program to start (millicom, telnet,
                             ssh or sh) */

    char  *device;      /**< Default device (NULL if not defined)             */
    char  *host;        /**< Default remote host (NULL if not defined)        */
    unsigned short port;    /**< remote host port                             */

    char  *shell_args;  /**< Shell CLI session arguments (NULL if not defined)*/

    char  *user;        /**< Default user account (NULL if not defined)       */
    char  *password;    /**< Default user password (NULL if not defined)      */


    int    prompts_status;   /**< available prompts found on init             */

    struct exp_case prompts[CLI_MAX_PROMPTS]; /**< expect cases structure that
                                                   contains known prompts     */

    int   read_timeout; /**< Number of second to wait for data                */
    
} cli_csap_specific_data_t;


/**
 * Callback for read parameter value of CLI CSAP.
 *
 * @param csap_id       identifier of CSAP.
 * @param level         Index of level in CSAP stack, which param is wanted.
 * @param param         Protocol-specific name of parameter.
 *
 * @return 
 *     String with textual presentation of parameter value, or NULL 
 *     if error occured. User have to free memory at returned pointer.
 */ 
extern char *cli_get_param_cb(int csap_id, int level, const char *param);

/**
 * Callback for read data from media of CLI CSAP. 
 *
 * @param csap_descr    identifier of CSAP.
 * @param timeout       timeout of waiting for data.
 * @param buf           buffer for read data.
 * @param buf_len       length of available buffer.
 *
 * @return 
 *      quantity of read octets, or -1 if error occured, 0 if timeout expired. 
 */ 
extern int cli_read_cb(csap_p csap_descr, int timeout, char *buf,
                       size_t buf_len);

/**
 * Callback for write data to media of CLI CSAP. 
 *
 * @param csap_descr    identifier of CSAP.
 * @param buf           buffer with data to be written.
 * @param buf_len       length of data in buffer.
 *
 * @return 
 *      quantity of written octets, or -1 if error occured. 
 */ 
extern int cli_write_cb(csap_p csap_descr, char *buf, 
                        size_t buf_len);

/**
 * Callback for write data to media of CLI CSAP and read
 *  data from media just after write, to get answer to sent request. 
 *
 * @param csap_descr    identifier of CSAP.
 * @param timeout       timeout of waiting for data.
 * @param w_buf         buffer with data to be written.
 * @param w_buf_len     length of data in w_buf.
 * @param r_buf         buffer for data to be read.
 * @param r_buf_len     available length r_buf.
 *
 * @return 
 *      quantity of read octets, or -1 if error occured, 0 if timeout expired. 
 */ 
extern int cli_write_read_cb(csap_p csap_descr, int timeout,
                             char *w_buf, size_t w_buf_len,
                             char *r_buf, size_t r_buf_len);


/**
 * Callback for init 'file' CSAP layer if single in stack.
 *
 * @param csap_id       identifier of CSAP.
 * @param csap_nds      asn_value with CSAP init parameters
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
extern int cli_single_init_cb (int csap_id, const asn_value * csap_nds, int layer);

/**
 * Callback for destroy 'file' CSAP layer if single in stack.
 *      This callback should free all undeground media resources used by 
 *      this layer and all memory used for layer-specific data and pointed 
 *      in respective structure in 'layer-data' in CSAP instance struct. 
 *
 * @param csap_id       identifier of CSAP.
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
extern int cli_single_destroy_cb (int csap_id, int layer);

/**
 * Callback for confirm PDU with ehternet CSAP parameters and possibilities.
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU (IN/OUT)
 *
 * @return zero on success or error code.
 */ 
extern int cli_confirm_pdu_cb (int csap_id, int layer, asn_value * tmpl_pdu); 

/**
 * Callback for generate binary data to be sent to media.
 *
 * @param csap_descr    CSAP instance
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU. 
 * @param args          Template iteration parameters array, may be used to 
 *                      prepare binary data.
 * @param arg_num       Length of array above. 
 * @param up_payload    pointer to data which is already generated for upper 
 *                      layers and is payload for this protocol level. 
 *                      May be zero.  Presented as list of packets. 
 *                      Almost always this list will contain only one element, 
 *                      but need in fragmentation sometimes may occur. 
 *                      Of cause, on up level only one PDU is passed, 
 *                      but upper layer (if any present) may perform 
 *                      fragmentation, and current layer may have possibility 
 *                      to de-fragment payload.
 * @param pkts          Callback have to fill this structure with list of 
 *                      generated packets. Almost always this list will 
 *                      contain only one element, but need 
 *                      in fragmentation sometimes may occur. (OUT)
 *
 * @return zero on success or error code.
 */ 
extern int cli_gen_bin_cb(csap_p csap_descr, int layer,
                          const asn_value *tmpl_pdu,
                          const tad_tmpl_arg_t *args, size_t arg_num,
                          csap_pkts_p up_payload, csap_pkts_p pkts);


/**
 * Callback for parse received packet and match it with pattern. 
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param pattern_pdu   pattern NDS 
 * @param pkt           recevied packet
 * @param payload       rest upper layer payload, if any exists. (OUT)
 * @param parsed_packet caller of method should pass here empty asn_value 
 *                      instance of ASN type of expected PDU. Callback 
 *                      have to fill this instance with values from 
 *                      parsed and matched packet
 *
 * @return zero on success or error code.
 */
extern int cli_match_bin_cb (int csap_id, int layer, 
                            const asn_value * pattern_pdu,
                             const csap_pkts *  pkt, csap_pkts * payload, 
                             asn_value *  parsed_packet );

/**
 * Callback for generating pattern to filter 
 * just one response to the packet which will be sent by this CSAP 
 * according to this template. 
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      ASN value with template PDU.
 * @param pattern_pdu   OUT: ASN value with pattern PDU, generated according 
 *                      to passed template PDU and CSAP parameters. 
 *
 * @return zero on success or error code.
 */
extern int cli_gen_pattern_cb (int csap_id, int layer, const asn_value * tmpl_pdu, 
                               asn_value *   *pattern_pdu);


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

#if 0
/**
 * Find number of CLI layer in CSAP stack.
 *
 * @param csap_descr    CSAP description structure.
 * @param layer_name    Name of the layer to find.
 *
 * @return number of layer (start from zero) or -1 if not found. 
 */ 
extern int find_csap_layer(csap_p csap_descr, char *layer_name);
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_CLI_IMPL_H__ */
