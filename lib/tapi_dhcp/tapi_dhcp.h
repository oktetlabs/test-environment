/** @file
 * @brief Proteos, TAPI DHCP.
 *
 * Declarations of API for TAPI DHCP.
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS in the
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
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */ 

#ifndef __TE_TAPI_DHCP_H__
#define __TE_TAPI_DHCP_H__

#ifdef HAVE_SEMAPHORE_H
#include <semaphore.h>
#else
#error "We have no semaphores"
#endif
#include <assert.h>
#include <netinet/in.h>

#include "te_stdint.h"
#include "tad_common.h"
#include "asn_usr.h"

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
#ifndef bool
#define bool unsigned int
#endif

#define MACADDR_LEN 6

#define CONST_STRING_LENGTH(str_) (sizeof(str_) - 1)

/** Types of DHCP Options */

/** Name of the host */
#define DHCP_OPT_HOSTNAME     12
/** DHCP Message Type */
#define DHCP_OPT_MESSAGE_TYPE 53
/** Server Identifier */
#define DHCP_OPT_SERVER_ID    54

/**
 * Value used in DHCP 'htype' field for Ethernet (10Mb) hardware type
 * [RFC 1700]
 */
#define DHCP_HW_TYPE_ETHERNET_10MB 1

/** Type of the DHCP message (It is used as a value of Option 53) */
#define DHCPDISCOVER 1
#define DHCPOFFER    2
#define DHCPREQUEST  3
#define DHCPDECLINE  4
#define DHCPACK      5
#define DHCPNAK      6
#define DHCPRELEASE  7

/** Values of message op code */
#define DHCP_OP_CODE_BOOTREQUEST  1
#define DHCP_OP_CODE_BOOTREPLY    2

/** BROADCAST flag */
#define FLAG_BROADCAST 0x8000

/** Size 'chaddr' field of DHCP message */
#define DHCPV4_HDR_CHADDR_SIZE 16
/** Size 'sname' field of DHCP message */
#define DHCPV4_HDR_SNAME_SIZE 64
/** Size 'file' field of DHCP message */
#define DHCPV4_HDR_FILE_SIZE 128

/** DHCP option internal representation */
struct dhcp_option {
    uint8_t  type; /**< Option type */
    uint8_t  len;  /**< Value of the "Length" field of the option */
    uint8_t  val_len; /**< Number of bytes in 'val' array */
    uint8_t *val;  /**< Pointer to the value of the option */

    struct dhcp_option *next; /**< Next option */
    struct dhcp_option *subopts; /**< List of sub-options */
};

/** Structure of DHCP message */
struct dhcp_message {
    uint8_t  op; /**< Message op code */
    uint8_t  htype; /**< Hardware address type */ 
    uint8_t  hlen; /**< Hardware address length */
    uint8_t  hops; /**< Client sets to zero, optionally used by relay agents
                        when booting via a relay agent */ 
    uint32_t xid; /**< Transaction ID */
    uint16_t secs; /**< Seconds elapsed since client began address 
                        acquisition or renewal process */
    uint16_t flags; /**< Flags */
    uint32_t ciaddr; /**< Client IP address */
    uint32_t yiaddr; /**< 'your' (client) IP address */
    uint32_t siaddr; /**< IP address of next server to use in bootstrap */
    uint32_t giaddr; /**< Relay agent IP address, used in booting via 
                          a relay agent */

    uint8_t chaddr[DHCPV4_HDR_CHADDR_SIZE]; /**< Client hardware address */
    uint8_t sname[DHCPV4_HDR_SNAME_SIZE]; /**< Optional server host name */
    uint8_t file[DHCPV4_HDR_FILE_SIZE]; /**< Boot file name */

    bool     is_op_set; /**< Is message op code has been set */
    bool     is_htype_set; /**< Is message htype has been set */
    bool     is_hlen_set; /**< Is message hlen has been set */
    bool     is_hops_set; /**< Is message hops has been set */
    bool     is_xid_set; /**< Is message xid has been set */
    bool     is_secs_set; /**< Is message secs has been set */
    bool     is_flags_set; /**< Is message flags has been set */
    bool     is_ciaddr_set; /**< Is message ciaddr has been set */
    bool     is_yiaddr_set; /**< Is message yiaddr has been set */
    bool     is_siaddr_set; /**< Is message siaddr has been set */
    bool     is_giaddr_set; /**< Is message giaddr has been set */
    bool     is_chaddr_set; /**< Is message chaddr has been set */
    bool     is_sname_set; /**< Is message sname has been set */
    bool     is_file_set; /**< Is message file has been set */

    struct dhcp_option *opts; /**< List of DHCP options */
};

/**
 * Gets the value of the field in dhcp_message structure
 *
 * @param msg_    Pointer to DHCP message
 * @param field_  Field name whose value to be got
 */
#define dhcpv4_message_get_field(msg_, field_) \
            ((((msg_)->is_ ## field_ ## _set != true) ? assert(0) : 0), \
             ((msg_)->field_))

/** Get field macros */
#define dhcpv4_message_get_op(msg_) \
            dhcpv4_message_get_field(msg_, op)
#define dhcpv4_message_get_htype(msg_) \
            dhcpv4_message_get_field(msg_, htype)
#define dhcpv4_message_get_hlen(msg_) \
            dhcpv4_message_get_field(msg_, hlen)
#define dhcpv4_message_get_xid(msg_) \
            dhcpv4_message_get_field(msg_, xid)
#define dhcpv4_message_get_flags(msg_) \
            dhcpv4_message_get_field(msg_, flags)
#define dhcpv4_message_get_ciaddr(msg_) \
            dhcpv4_message_get_field(msg_, ciaddr)
#define dhcpv4_message_get_yiaddr(msg_) \
            dhcpv4_message_get_field(msg_, yiaddr)
#define dhcpv4_message_get_siaddr(msg_) \
            dhcpv4_message_get_field(msg_, siaddr)
#define dhcpv4_message_get_giaddr(msg_) \
            dhcpv4_message_get_field(msg_, giaddr)
#define dhcpv4_message_get_chaddr(msg_) \
            dhcpv4_message_get_field(msg_, chaddr)

/**
 * Sets a new value in dhcp_message structure
 * (It is used to update simple type fields - not arrays)
 *
 * @param msg_    Pointer to DHCP message
 * @param field_  Field name to be updated
 * @param value_  Pointer to the new value
 */
#define dhcpv4_message_set_simple_field(msg_, field_, value_) \
            ((msg_)->is_ ## field_ ## _set = true,                       \
             memcpy(&((msg_)->field_), value_, sizeof((msg_)->field_)))

/**
 * Sets a new value in dhcp_message structure
 * (It is used to update array fields of the structure)
 *
 * @param msg_    Pointer to DHCP message
 * @param field_  Field name to be updated
 * @param value_  Pointer to the new value
 */
#define dhcpv4_message_set_array_field(msg_, field_, value_) \
            ((msg_)->is_ ## field_ ## _set = true,                       \
             memcpy(((msg_)->field_), value_, sizeof((msg_)->field_)))

/** Set field macros */
#define dhcpv4_message_set_op(msg_, value_) \
            dhcpv4_message_set_simple_field(msg_, op, value_)
#define dhcpv4_message_set_htype(msg_, value_) \
            dhcpv4_message_set_simple_field(msg_, htype, value_)
#define dhcpv4_message_set_hlen(msg_, value_) \
            dhcpv4_message_set_simple_field(msg_, hlen, value_)
#define dhcpv4_message_set_hops(msg_, value_) \
            dhcpv4_message_set_simple_field(msg_, hops, value_)
#define dhcpv4_message_set_xid(msg_, value_) \
            dhcpv4_message_set_simple_field(msg_, xid, value_)
#define dhcpv4_message_set_secs(msg_, value_) \
            dhcpv4_message_set_simple_field(msg_, secs, value_)
#define dhcpv4_message_set_flags(msg_, value_) \
            dhcpv4_message_set_simple_field(msg_, flags, value_)
#define dhcpv4_message_set_ciaddr(msg_, value_) \
            dhcpv4_message_set_simple_field(msg_, ciaddr, value_)
#define dhcpv4_message_set_yiaddr(msg_, value_) \
            dhcpv4_message_set_simple_field(msg_, yiaddr, value_)
#define dhcpv4_message_set_siaddr(msg_, value_) \
            dhcpv4_message_set_simple_field(msg_, siaddr, value_)
#define dhcpv4_message_set_giaddr(msg_, value_) \
            dhcpv4_message_set_simple_field(msg_, giaddr, value_)

/**
 * Note that 'value_' MUST be a pointer to a buffer with at 
 * least DHCPV4_HDR_CHADDR_SIZE valid bytes
 */
#define dhcpv4_message_set_chaddr(msg_, value_) \
            dhcpv4_message_set_array_field(msg_, chaddr, value_)

/**
 * Note that 'value_' MUST be a pointer to a buffer with at 
 * least DHCPV4_HDR_SNAME_SIZE valid bytes
 */
#define dhcpv4_message_set_sname(msg_, value_) \
            dhcpv4_message_set_array_field(msg_, sname, value_)
/**
 * Note that 'value_' MUST be a pointer to a buffer with at 
 * least DHCPV4_HDR_FILE_SIZE valid bytes
 */
#define dhcpv4_message_set_file(msg_, value_) \
            do {                                           \
                size_t str_len = strlen(value_);           \
                                                           \
                assert(str_len < sizeof((msg_)->file));    \
                (msg_)->is_file_set = true;                \
                memcpy((msg_)->file, value_, str_len + 1); \
            } while (0)

extern int ndn_dhcpv4_packet_to_plain(asn_value_p pkt,
                                      struct dhcp_message **dhcp_msg);
extern int ndn_dhcpv4_plain_to_packet(const struct dhcp_message *dhcp_msg,
                                      asn_value_p *pkt);

extern struct dhcp_message *dhcpv4_message_create(uint8_t msg_type);
extern void dhcpv4_message_destroy(struct dhcp_message *);

extern int tapi_dhcpv4_plain_csap_create(const char *ta_name,
                                         const char *iface,
                                         dhcp_csap_mode mode,
                                         csap_handle_t *dhcp_csap);

extern int tapi_dhcpv4_message_send(const char *ta_name,
                                    csap_handle_t dhcp_csap,
                                    const struct dhcp_message *dhcp_msg);
extern int dhcpv4_message_start_recv(const char *ta_name,
                                     csap_handle_t dhcp_csap,
                                     uint8_t msg_type);
extern struct dhcp_message *dhcpv4_message_capture(const char *ta_name,
                                                   csap_handle_t dhcp_csap,
                                                   unsigned int *timeout);
extern struct dhcp_message *tapi_dhcpv4_send_recv(const char *ta_name,
                                                  csap_handle_t dhcp_csap,
                      const struct dhcp_message *dhcp_msg, int *tv,
                      const char **err_msg);
extern int tapi_dhcpv4_csap_get_ipaddr(const char *ta_name,
                                       csap_handle_t dhcp_csap, void *addr);

extern const struct dhcp_option *dhcpv4_message_get_option(
                                    const struct dhcp_message *dhcp_msg,
                                    uint8_t type);
extern const struct dhcp_option *dhcpv4_message_get_sub_option(
                                    const struct dhcp_option *opt,
                                    uint8_t type);
extern void dhcpv4_message_fill_reply_from_req(
                struct dhcp_message *dhcp_rep,
                const struct dhcp_message *dhcp_req);

extern struct dhcp_option *dhcpv4_option_create(uint8_t type,
                                                uint8_t len,
                                                uint8_t val_len,
                                                uint8_t *val);
extern int dhcpv4_option_add_subopt(struct dhcp_option *opt, uint8_t type,
                                    uint8_t len, uint8_t *val);
extern int dhcpv4_option_insert_subopt(struct dhcp_option *opt,
                                       struct dhcp_option *subopt);
extern int dhcpv4_message_add_option(struct dhcp_message *dhcp_msg,
                                     uint8_t type, uint8_t len,
                                     const void *val);
extern int dhcpv4_message_insert_option(struct dhcp_message *dhcp_msg,
                                        struct dhcp_option *opt);
extern bool dhcpv4_option55_has_code(const struct dhcp_option *opt,
                                     uint8_t type);

#endif /* !__TE_TAPI_DHCP_H__ */
