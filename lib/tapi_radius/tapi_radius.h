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

#include <stdlib.h>
#include "te_stdint.h"
#include "tad_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Type of RADIUS packet */
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

/** Type of RADIUS attribute */
typedef enum {
    TAPI_RADIUS_TYPE_TEXT,
    TAPI_RADIUS_TYPE_STRING,
    TAPI_RADIUS_TYPE_ADDRESS,
    TAPI_RADIUS_TYPE_INTEGER,
    TAPI_RADIUS_TYPE_TIME
} tapi_radius_type_t;

/* Minimal length of attribute in packet */
#define TAPI_RADIUS_ATTR_MIN_LEN   2

/* RADIUS attribute */
typedef struct {
    uint8_t   type;
    uint8_t   len;
    union {
        uint32_t    integer;
        char       *string;
    };
} tapi_radius_attr_t;

/* RADIUS attributes list */
typedef struct {
    size_t              len;
    tapi_radius_attr_t *attr;
} tapi_radius_attr_list_t;

/* Length of packet */
#define TAPI_RADIUS_PACKET_MIN_LEN  20
#define TAPI_RADIUS_PACKET_MAX_LEN  4096

/* Length of authenticator */
#define TAPI_RADIUS_AUTH_LEN  16

/* RADIUS packet */
typedef struct {
    tapi_radius_code_t       code;
    uint8_t                  identifier;
    char                     authenticator[TAPI_RADIUS_AUTH_LEN];
    tapi_radius_attr_list_t  attrs;
} tapi_radius_packet_t;

/* RADIUS attributes dictionary entry */
typedef struct {
    uint8_t             id;
    char               *name;
    tapi_radius_type_t  type;
} tapi_radius_attr_info_t;

#define TAPI_RADIUS_AUTH_PORT   1812
#define TAPI_RADIUS_ACCT_PORT   1813

typedef void (*radius_callback)(const tapi_radius_packet_t *pkt,
                                void *userdata);

extern void tapi_radius_dict_init();
extern const tapi_radius_attr_info_t *tapi_radius_dict_lookup(uint8_t type);
extern const tapi_radius_attr_info_t
            *tapi_radius_dict_lookup_by_name(const char *name);

extern void tapi_radius_attr_list_init(tapi_radius_attr_list_t *list);
extern int tapi_radius_attr_list_push(tapi_radius_attr_list_t *list,
                                      const tapi_radius_attr_t *attr);
extern void tapi_radius_attr_list_free(tapi_radius_attr_list_t *list);
extern const tapi_radius_attr_t *tapi_radius_attr_list_find(
                           const tapi_radius_attr_list_t *list,
                           uint8_t type);

extern int tapi_radius_parse_packet(const uint8_t *data, size_t data_len,
                                    tapi_radius_packet_t *packet);
extern int tapi_radius_recv_start(const char *ta, int sid, csap_handle_t csap,
                                  radius_callback user_callback,
                                  void *user_data, unsigned int timeout);
extern int tapi_radius_csap_create(const char *ta, int sid,
                                   const char *device,
                                   const uint8_t *net_addr, uint16_t port,
                                   csap_handle_t *csap);
                                   
/*
 * Interface to configure RADIUS Server:
 * This API simplify managing of RADIUS Server configuration, which is
 * done via Configurator DB. The configuration model can be found at 
 * doc/cm/cm_radius.xml
 */

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
