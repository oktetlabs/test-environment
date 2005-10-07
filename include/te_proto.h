/** @file
 * @brief Test Protocol definitions
 *
 * Test Protocol commands definition
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_PROTO_H__
#define __TE_PROTO_H__

#define TE_PROTO_SHUTDOWN       "shutdown"
#define TE_PROTO_REBOOT         "reboot"
#define TE_PROTO_CONFGET        "configure get"
#define TE_PROTO_CONFSET        "configure set"
#define TE_PROTO_CONFADD        "configure add"
#define TE_PROTO_CONFDEL        "configure del"
#define TE_PROTO_CONFGRP_START  "configure group start"
#define TE_PROTO_CONFGRP_END    "configure group end"
#define TE_PROTO_GET_LOG        "get_log"
#define TE_PROTO_VREAD          "vread"
#define TE_PROTO_VWRITE         "vwrite"
#define TE_PROTO_FPUT           "fput"
#define TE_PROTO_FGET           "fget"
#define TE_PROTO_FDEL           "fdel"
#define TE_PROTO_CSAP_CREATE    "csap_create"
#define TE_PROTO_CSAP_DESTROY   "csap_destroy"
#define TE_PROTO_CSAP_PARAM     "csap_param"
#define TE_PROTO_TRSEND_START   "trsend_start"
#define TE_PROTO_TRSEND_STOP    "trsend_stop"
#define TE_PROTO_TRSEND_RECV    "trsend_recv"
#define TE_PROTO_TRRECV_START   "trrecv_start"
#define TE_PROTO_TRRECV_STOP    "trrecv_stop"
#define TE_PROTO_TRRECV_WAIT    "trrecv_wait"
#define TE_PROTO_TRRECV_GET     "trrecv_get"
#define TE_PROTO_EXECUTE        "execute"
#define TE_PROTO_RPC            "rpc"
#define TE_PROTO_KILL           "kill"

#ifdef RCF_NEED_TYPES
/**
 * Types recoding table.
 *
 * Members of rcf_var_type_t enumeration are used as indexes for this
 * array.
 */
static char *rcf_types[] = 
    { "int8", "uint8", "int16", "uint16", "int32", "uint32", 
      "int64", "uint64", "string" };
#endif

#ifdef RCF_NEED_TYPE_LEN
/**
 * Lengthes of simple types supported in TE protocol.
 *
 * Members of rcf_var_type_t enumeration are used as indexes for this
 * array.
 */
static int rcf_type_len[] = 
    { sizeof(int8_t), sizeof(uint8_t),
      sizeof(int16_t), sizeof(uint16_t),
      sizeof(int32_t), sizeof(uint32_t),
      sizeof(int64_t), sizeof(uint64_t), 0};
      
#endif

#define RCF_MAX_TYPE_NAME       6

/**
 * Maximum number of symbols necessary for Test Protocol overhead:
 * session identifier, command and attachment information.
 */
#define TE_PROTO_OVERHEAD       48

#endif /* !__TE_PROTO_H__ */
