/** @file
 * @brief Test API for RADIUS Server Configuration and RADIUS CSAP 
 *
 * Definition of API.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Boris Misenov <Boris.Misenov@oktetlabs.ru>
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_RADIUS_H__
#define __TE_TAPI_RADIUS_H__

#include "te_config.h"
#include "te_stdint.h"
#include "te_defs.h"
#include "tad_common.h"

#include <stdlib.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Type of RADIUS packet, see RFC 2865 */
typedef enum {
    TAPI_RADIUS_CODE_ACCESS_REQUEST      = 1,
    TAPI_RADIUS_CODE_ACCESS_ACCEPT       = 2,
    TAPI_RADIUS_CODE_ACCESS_REJECT       = 3,
    TAPI_RADIUS_CODE_ACCOUNTING_REQUEST  = 4,
    TAPI_RADIUS_CODE_ACCOUNTING_RESPONSE = 5,
    TAPI_RADIUS_CODE_ACCESS_CHALLENGE    = 11,
    TAPI_RADIUS_CODE_STATUS_SERVER       = 12,
    TAPI_RADIUS_CODE_STATUS_CLIENT       = 13,
} tapi_radius_code_t;

/** Type of RADIUS attribute data, see RFC 2865 */
typedef enum {
    TAPI_RADIUS_TYPE_TEXT,      /* UTF-8 encoded text string, 1-253 octets */
    TAPI_RADIUS_TYPE_STRING,    /* Binary data, 1-253 octets */
    TAPI_RADIUS_TYPE_ADDRESS,   /* IPv4 address, 32 bit value */
    TAPI_RADIUS_TYPE_INTEGER,   /* 32 bit unsigned value */
    TAPI_RADIUS_TYPE_TIME,      /* 32 bit unsigned value,
                                   seconds since 19700101T000000Z */
    TAPI_RADIUS_TYPE_UNKNOWN
} tapi_radius_type_t;

/** Type of RADIUS attribute */
typedef uint8_t tapi_radius_attr_type_t;

/** Minimal length of attribute in packet */
#define TAPI_RADIUS_ATTR_MIN_LEN   2

/** RADIUS attribute */
typedef struct {
    tapi_radius_attr_type_t type;
    tapi_radius_type_t      datatype;
    uint8_t                 len;
    union {
        uint32_t    integer;
        char       *string;
    };
} tapi_radius_attr_t;

/** RADIUS attributes list */
typedef struct {
    size_t              len;
    tapi_radius_attr_t *attr;
} tapi_radius_attr_list_t;

/** Length of packet */
#define TAPI_RADIUS_PACKET_MIN_LEN  20
#define TAPI_RADIUS_PACKET_MAX_LEN  4096

/** Length of authenticator */
#define TAPI_RADIUS_AUTH_LEN  16

/** RADIUS packet */
typedef struct {
    tapi_radius_code_t       code;
    uint8_t                  identifier;
    char                     authenticator[TAPI_RADIUS_AUTH_LEN];
    tapi_radius_attr_list_t  attrs;
} tapi_radius_packet_t;

/** RADIUS attributes dictionary entry */
typedef struct {
    tapi_radius_attr_type_t  id;
    const char              *name;
    tapi_radius_type_t       type;
} tapi_radius_attr_info_t;

/** Default UDP port for RADIUS authentication service */
#define TAPI_RADIUS_AUTH_PORT   1812

/** Default UDP port for RADIUS accounting service */
#define TAPI_RADIUS_ACCT_PORT   1813

typedef void (*radius_callback)(const tapi_radius_packet_t *pkt,
                                void *userdata);

/** 
 * Initialize RADIUS attribute dictionary
 * (this function should be called before any other TAPI RADIUS calls)
 */
extern void tapi_radius_dict_init();

/**
 * Lookup specified attribute in RADIUS dictionary by its numeric type
 *
 * @param  type         Attribute type to lookup
 *
 * @return Pointer to dictionary entry or NULL if not found
 */
extern const tapi_radius_attr_info_t
             *tapi_radius_dict_lookup(tapi_radius_attr_type_t type);

/**
 * Lookup specified attribute in RADIUS dictionary by its name
 *
 * @param name          Attribute name to lookup
 *
 * @return Pointer to dictionary entry or NULL if not found
 */
extern const tapi_radius_attr_info_t
             *tapi_radius_dict_lookup_by_name(const char *name);

/**
 * Initialize a list of RADIUS attributes
 *
 * @param list          List to initialize
 */
extern void tapi_radius_attr_list_init(tapi_radius_attr_list_t *list);

/**
 * Push an attribute to the end of RADIUS attribute list
 *
 * @param list          Attribute list
 * @param attr          Attribute to push
 *
 * @return Zero on success or error code.
 */

extern int tapi_radius_attr_list_push(tapi_radius_attr_list_t *list,
                                      const tapi_radius_attr_t *attr);

/**
 * Create RADIUS attribute by name and value and push it to the end
 * of attribute list. Type of value is determined from the dictionary.
 * Values are:
 *   for TAPI_RADIUS_TYPE_INTEGER       (int value)
 *   for TAPI_RADIUS_TYPE_TEXT          (char *value)
 *   for TAPI_RADIUS_TYPE_STRING        (uint8_t *value, int length)
 * E.g.: tapi_radius_attr_list_push_value(&list, "NAS-Port", 20);
 *
 * @param list          Attribute list to push attribute to
 * @param name          Attribute name
 *
 * @return Zero on success or error code.
 */
extern int tapi_radius_attr_list_push_value(tapi_radius_attr_list_t *list,
                                            const char *name, ...);

/**
 * Free memory allocated for attribute list
 *
 * @param  list     List to free
 */
extern void tapi_radius_attr_list_free(tapi_radius_attr_list_t *list);

/**
 * Copy RADIUS attribute list
 *
 * @param dst       Location for the new copy of list
 * @param src       Original attribute list
 *
 * @return Zero on success or error code.
 */
extern int tapi_radius_attr_list_copy(tapi_radius_attr_list_t *dst,
                                      const tapi_radius_attr_list_t *src);

/**
 * Find specified attribute in the attribute list
 *
 * @param   list        Attribute list
 * @param   type        Identifier of attribute to find
 *
 * @return Pointer to attribute in the list or NULL if not found.
 */
extern const tapi_radius_attr_t
             *tapi_radius_attr_list_find(const tapi_radius_attr_list_t *list,
                                         tapi_radius_attr_type_t type);

/**
 * Convert attribute list into a string of comma-separated pairs
 * 'Attribute=Value'
 *
 * @param   list        Attribute list
 * @param   str         Location for a pointer to result string (will be
 *                      allocated by this function)
 *
 * @return Zero on success or result code.
 */
extern int tapi_radius_attr_list_to_string(const tapi_radius_attr_list_t *list,
                                           char **str);

/**
 * Parse binary RADIUS packet payload to C structure
 *
 * @param data      RADIUS packet data
 * @param data_len  Length of packet data
 * @param packet    Packet structure to be filled
 *
 * @return Zero on success or error code.
 */
extern int tapi_radius_parse_packet(const uint8_t *data, size_t data_len,
                                    tapi_radius_packet_t *packet);

/**
 * Start receiving RADIUS packets using 'udp.ip4.eth' CSAP on the specified
 * Test Agent
 *
 * @param ta              Test Agent name
 * @param sid             RCF session identifier
 * @param csap            Handle of 'udp.ip4.eth' CSAP on the Test Agent
 * @param user_callback   Callback for RADIUS packets handling
 * @param user_data       User-supplied data to be passed to callback
 * @param timeout         
 *
 * @return Zero on success or error code.
 */
extern int tapi_radius_recv_start(const char *ta, int sid, csap_handle_t csap,
                                  radius_callback user_callback,
                                  void *user_data, unsigned int timeout);

/**
 * Create 'udp.ip4.eth' CSAP for capturing RADIUS packets
 *
 * @param ta               Test Agent name
 * @param sid              RCF session identifier
 * @param device           Ethernet device name on agent to attach
 * @param net_addr         Local IP address on Test Agent
 * @param port             UDP port on Test Agent (TAPI_RADIUS_AUTH_PORT
 *                          or TAPI_RADIUS_ACCT_PORT)
 * @param csap             Handle of new CSAP (OUT)
 *
 * @return Zero on success or error code.
 */
extern int tapi_radius_csap_create(const char *ta, int sid,
                                   const char *device,
                                   const in_addr_t net_addr, uint16_t port,
                                   csap_handle_t *csap);

/*
 * Interface to configure RADIUS Server:
 * This API simplify managing of RADIUS Server configuration, which is
 * done via Configurator DB. The configuration model can be found at 
 * doc/cm/cm_radius.xml
 */

/** 
 * Structure that keeps configuration of RADIUS Server.
 * This structure was created to make tapi_radius_serv_set()
 * function backward compatible when someone adds a new configuration 
 * value for RADIUS Server.
 */
typedef struct tapi_radius_serv_s {
    uint16_t       auth_port; /**< RADIUS Authentication Server port,
                                   zero value means that we want use
                                   default value. */
    uint16_t       acct_port; /**< RADIUS Accounting Server port,
                                   zero value means that we want use
                                   default value. */
    struct in_addr net_addr; /**< Network address on which RADIUS Server
                                  listens incoming Requests,
                                  INADDR_ANY means that we want RADIUS 
                                  Server listen on all interfaces. */
} tapi_radius_serv_t;

/** 
 * Structure that keeps configuration of RADIUS Client.
 * This structure was created to make tapi_radius_serv_add_client()
 * function backward compatible when someone adds a new configuration 
 * value for RADIUS Client.
 */
typedef struct tapi_radius_clnt_s {
    const char     *secret; /**< Secret string that should be shared 
                                 between RADIUS Server and Client */
    struct in_addr  net_addr; /**< Network address of RADIUS Client */
} tapi_radius_clnt_t;


/**
 * Enables RADIUS Server on the particular Agent.
 *
 * @param ta_name Test Agent name
 *
 * @return Zero on success or error code.
 */
extern int tapi_radius_serv_enable(const char *ta_name);

/**
 * Disables RADIUS Server on the particular Agent.
 *
 * @param ta_name  Test Agent name
 *
 * @return Zero on success or error code.
 */
extern int tapi_radius_serv_disable(const char *ta_name);

/**
 * Update RADIUS Server Configuration.
 *
 * @param ta_name  Test Agent name
 * @param cfg      RADIUS Server configuration information
 *
 * @return Zero on success or error code.
 */
extern int tapi_radius_serv_set(const char *ta_name,
                                const tapi_radius_serv_t *cfg);

/**
 * Add a new RADIUS Client record on RADIUS Server.
 * Clients differ in network address, which is specified as a field of
 * tapi_radius_clnt_t data structure.
 *
 * @param ta_name  Test Agent name
 * @param cfg      RADIUS Client configuration information
 *
 * @return Zero on success or error code.
 */
extern int tapi_radius_serv_add_client(const char *ta_name,
                                       const tapi_radius_clnt_t *cfg);

/**
 * Delete RADIUS Client record from RADIUS Server.
 *
 * @param ta_name   Test Agent name
 * @param net_addr  RADIUS Client's network address
 *
 * @return Zero on success or error code.
 */
extern int tapi_radius_serv_del_client(const char *ta_name,
                                       const struct in_addr *net_addr);

/**
 * Add user configuration on RADIUS Server.
 *
 * @param ta_name     Test Agent name
 * @param user_name   User name
 * @param acpt_user   Wheter this user should be accepted on successful
 *                    authentication
 * @param check_attrs A list of RADIUS attributes that should be checked
 *                    additionally for this user
 * @param acpt_attrs  A list of RADIUS attributes that should be sent
 *                    to this user in Access-Accept RADIUS message.
 *                    May be NULL if no special attributes desired.
 * @param chlg_attrs  A list of RADIUS attributes that should be sent
 *                    to this user in Access-Challenge RADIUS message
 *                    May be NULL if no special attributes desired.
 *
 * @return Zero on success or error code.
 */
extern int tapi_radius_serv_add_user(
                            const char *ta_name,
                            const char *user_name,
                            te_bool acpt_user,
                            const tapi_radius_attr_list_t *check_attrs,
                            const tapi_radius_attr_list_t *acpt_attrs,
                            const tapi_radius_attr_list_t *chlg_attrs);

/**
 * Delete user configuration from RADIUS Server.
 *
 * @param ta_name     Test Agent name
 * @param user_name   User name
 *
 * @return Zero on success or error code.
 */
extern int tapi_radius_serv_del_user(const char *ta_name,
                                     const char *user_name);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RADIUS_H__ */
