/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API for RADIUS Server Configuration and RADIUS CSAP
 *
 * @defgroup tapi_radius RADIUS Server Configuration and RADIUS CSAP
 * @ingroup te_ts_tapi
 * @{
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_RADIUS_H__
#define __TE_TAPI_RADIUS_H__

#include "te_config.h"

#include <stdlib.h>
#include <stdio.h>
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "te_stdint.h"
#include "te_defs.h"
#include "tad_common.h"
#include "tapi_tad.h"
#include "conf_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Type of RADIUS packet, see RFC 2865 */
typedef enum {
    TAPI_RADIUS_CODE_ACCESS_REQUEST      = 1,   /**< Access-Request */
    TAPI_RADIUS_CODE_ACCESS_ACCEPT       = 2,   /**< Access-Accept */
    TAPI_RADIUS_CODE_ACCESS_REJECT       = 3,   /**< Access-Reject */
    TAPI_RADIUS_CODE_ACCOUNTING_REQUEST  = 4,   /**< Accounting-Request */
    TAPI_RADIUS_CODE_ACCOUNTING_RESPONSE = 5,   /**< Accounting-Response */
    TAPI_RADIUS_CODE_ACCESS_CHALLENGE    = 11,  /**< Access-Challenge */
    TAPI_RADIUS_CODE_STATUS_SERVER       = 12,  /**< Status-Server */
    TAPI_RADIUS_CODE_STATUS_CLIENT       = 13,  /**< Status-Client */
} tapi_radius_code_t;

/**
 * Convert the code of RADIUS packet from integer to readable string.
 *
 * @param code  the code value of RADIUS packet
 *
 * @return string literal pointer
 *
 * @note non-reenterable in the case of unknown value
 */
static inline const char *
tapi_radius_code2str(tapi_radius_code_t code)
{
#define CODE2STR(code_) case TAPI_RADIUS_CODE_ ## code_: return #code_

    switch (code)
    {
        CODE2STR(ACCESS_REQUEST);
        CODE2STR(ACCESS_ACCEPT);
        CODE2STR(ACCESS_REJECT);
        CODE2STR(ACCOUNTING_REQUEST);
        CODE2STR(ACCOUNTING_RESPONSE);
        CODE2STR(ACCESS_CHALLENGE);
        CODE2STR(STATUS_SERVER);
        CODE2STR(STATUS_CLIENT);

        default:
        {
            static char unknown_code[32];

            snprintf(unknown_code, sizeof(unknown_code),
                     "Unknown(%u)", code);
            return unknown_code;
        }
    }
#undef CODE2STR

    /* We will never reach this code */
    return "";
}

/** Type of RADIUS attribute data, see RFC 2865 */
typedef enum {
    TAPI_RADIUS_TYPE_TEXT,      /**< UTF-8 encoded text string,
                                     1-253 octets */
    TAPI_RADIUS_TYPE_STRING,    /**< Binary data, 1-253 octets */
    TAPI_RADIUS_TYPE_ADDRESS,   /**< IPv4 address, 32 bit value */
    TAPI_RADIUS_TYPE_INTEGER,   /**< 32 bit unsigned value */
    TAPI_RADIUS_TYPE_TIME,      /**< 32 bit unsigned value,
                                     seconds since 19700101T000000Z */
    TAPI_RADIUS_TYPE_UNKNOWN    /**< Attribute is not from
                                     TAPI RADIUS dictionary */
} tapi_radius_type_t;

/**
 * Convert the type of RADIUS atribute from integer to readable string.
 *
 * @param type  RADIUS attribute type
 *
 * @return string literal pointer
 *
 * @note non-reenterable in the case of unknown value
 */
static inline const char *
tapi_radius_attr_type2str(tapi_radius_type_t type)
{
#define ATTR_TYPE2STR(type_) case TAPI_RADIUS_TYPE_ ## type_: return #type_

    switch (type)
    {
        ATTR_TYPE2STR(TEXT);
        ATTR_TYPE2STR(STRING);
        ATTR_TYPE2STR(ADDRESS);
        ATTR_TYPE2STR(INTEGER);
        ATTR_TYPE2STR(TIME);

        default:
        {
            static char unknown_type[32];

            snprintf(unknown_type, sizeof(unknown_type),
                     "Unknown(%u)", type);
            return unknown_type;
        }
    }
#undef ATTR_TYPE2STR

    /* We will never reach this code */
    return "";
}

/** Value of Acct-Status-Type attribute, see RFC 2866 */
typedef enum {
    TAPI_RADIUS_ACCT_STATUS_START   = 1,    /**< Start */
    TAPI_RADIUS_ACCT_STATUS_STOP    = 2,    /**< Stop */
    TAPI_RADIUS_ACCT_STATUS_INTERIM = 3,    /**< Interim-Update */
    TAPI_RADIUS_ACCT_STATUS_ON      = 7,    /**< Accounting-On */
    TAPI_RADIUS_ACCT_STATUS_OFF     = 8,    /**< Accounting-Off */
} tapi_radius_acct_status_t;

/**
 * Convert Accounting Status from integer to readable string.
 *
 * @param status  Accounting status value
 *
 * @return string literal pointer
 *
 * @note non-reenterable in the case of unknown value
 */
static inline const char *
tapi_radius_acct_status2str(tapi_radius_acct_status_t status)
{
#define ACCT_STATUS2STR(status_) \
    case TAPI_RADIUS_ACCT_STATUS_ ## status_: return #status_

    switch (status)
    {
        ACCT_STATUS2STR(START);
        ACCT_STATUS2STR(STOP);
        ACCT_STATUS2STR(INTERIM);
        ACCT_STATUS2STR(ON);
        ACCT_STATUS2STR(OFF);

        default:
        {
            static char unknown_status[32];

            snprintf(unknown_status, sizeof(unknown_status),
                     "Unknown(%u)", status);
            return unknown_status;
        }
    }
#undef ACCT_STATUS2STR

    /* We will never reach this code */
    return "";
}


/** Value of Acct-Terminate-Cause attribute, see RFC 2866, and RFC 3580 */
typedef enum {
    TAPI_RADIUS_TERM_USER_REQUEST    = 1,    /**< User Request */
    TAPI_RADIUS_TERM_LOST_CARRIER    = 2,    /**< Lost Carrier */
    TAPI_RADIUS_TERM_LOST_SERVICE    = 3,    /**< Lost Service */
    TAPI_RADIUS_TERM_IDLE_TIMEOUT    = 4,    /**< Idle Timeout */
    TAPI_RADIUS_TERM_SESSION_TIMEOUT = 5,    /**< Session Timeout */
    TAPI_RADIUS_TERM_ADMIN_RESET     = 6,    /**< Admin Reset */
    TAPI_RADIUS_TERM_ADMIN_REBOOT    = 7,    /**< Admin Reboot */
    TAPI_RADIUS_TERM_PORT_ERROR      = 8,    /**< Port Error */
    TAPI_RADIUS_TERM_NAS_ERROR       = 9,    /**< NAS Error */
    TAPI_RADIUS_TERM_NAS_REQUEST     = 10,   /**< NAS Request */
    TAPI_RADIUS_TERM_NAS_REBOOT      = 11,   /**< NAS Reboot */
    TAPI_RADIUS_TERM_PORT_UNNEEDED   = 12,   /**< Port Unneeded */
    TAPI_RADIUS_TERM_PORT_PREEMPTED  = 13,   /**< Port Preempted */
    TAPI_RADIUS_TERM_PORT_SUSPENDED  = 14,   /**< Port Suspended */
    TAPI_RADIUS_TERM_SERVICE_UNAVAIL = 15,   /**< Service Unavailable */
    TAPI_RADIUS_TERM_CALLBACK        = 16,   /**< Callback */
    TAPI_RADIUS_TERM_USER_ERROR      = 17,   /**< User Error */
    TAPI_RADIUS_TERM_HOST_REQUEST    = 18,   /**< Host Request */
    TAPI_RADIUS_TERM_SUPP_RESTART    = 19,   /**< Supplicant Restart */
    TAPI_RADIUS_TERM_REAUTH_FAILURE  = 20,   /**< Reauthentication Failure */
} tapi_radius_term_cause_t;

/**
 * Convert the value of Acct-Terminate-Cause RADIUS attribute
 * from integer to readable string.
 *
 * @param cause  the value of Acct-Terminate-Cause attribute
 *
 * @return string literal pointer
 *
 * @note non-reenterable in the case of unknown value
 */
static inline const char *
tapi_radius_term_cause2str(tapi_radius_term_cause_t cause)
{
#define CAUSE2STR(name_) case TAPI_RADIUS_TERM_ ## name_: return #name_

    switch (cause)
    {
        CAUSE2STR(USER_REQUEST);
        CAUSE2STR(LOST_CARRIER);
        CAUSE2STR(LOST_SERVICE);
        CAUSE2STR(IDLE_TIMEOUT);
        CAUSE2STR(SESSION_TIMEOUT);
        CAUSE2STR(ADMIN_RESET);
        CAUSE2STR(ADMIN_REBOOT);
        CAUSE2STR(PORT_ERROR);
        CAUSE2STR(NAS_ERROR);
        CAUSE2STR(NAS_REQUEST);
        CAUSE2STR(NAS_REBOOT);
        CAUSE2STR(PORT_UNNEEDED);
        CAUSE2STR(PORT_PREEMPTED);
        CAUSE2STR(PORT_SUSPENDED);
        CAUSE2STR(SERVICE_UNAVAIL);
        CAUSE2STR(CALLBACK);
        CAUSE2STR(USER_ERROR);
        CAUSE2STR(HOST_REQUEST);
        CAUSE2STR(SUPP_RESTART);
        CAUSE2STR(REAUTH_FAILURE);

        default:
        {
            static char unknown_cause[32];

            snprintf(unknown_cause, sizeof(unknown_cause),
                     "Unknown(%u)", cause);
            return unknown_cause;
        }
    }
#undef CAUSE2STR

    /* We will never reach this code */
    return "";
}

/** Value of Termination-Action attribute, see RFC 2865 */
typedef enum {
    TAPI_RADIUS_TERM_ACTION_DEFAULT = 0,    /**< Default */
    TAPI_RADIUS_TERM_ACTION_REQUEST = 1,    /**< RADIUS-Request */
} tapi_radius_term_action_t;

/** Value of NAS-Port-Type attribute, see RFC 2865 */
typedef enum {
    TAPI_RADIUS_NAS_PORT_TYPE_ETHERNET = 15, /**< Ethernet */
    TAPI_RADIUS_NAS_PORT_TYPE_IEEE_802_11 = 19, /**< Wireless - IEEE 802.11 */
} tapi_radius_nas_port_type_t;

/** Type of RADIUS attribute */
typedef uint8_t tapi_radius_attr_type_t;

/** Minimal length of attribute in packet */
#define TAPI_RADIUS_ATTR_MIN_LEN   2

/** RADIUS attribute */
typedef struct {
    tapi_radius_attr_type_t type;       /**< Attribute type (identifier
                                             from the dictionary) */
    tapi_radius_type_t      datatype;   /**< Datatype of attribute */
    uint8_t                 len;        /**< Length of value (in bytes)
                                             (not including trailing null
                                             for STRING and TEXT,
                                             4 for INTEGER, ADDRESS, TIME) */
    union {
        uint32_t    integer;            /**< Value for INTEGER, ADDDRESS,
                                             TIME */
        char       *string;             /**< Pointer to value for STRING
                                             and TEXT */
    };
} tapi_radius_attr_t;

/** RADIUS attributes list */
typedef struct {
    size_t              len;
    tapi_radius_attr_t *attr;
} tapi_radius_attr_list_t;

/** Minimal length of packet */
#define TAPI_RADIUS_PACKET_MIN_LEN  20

/** Maximal length of packet */
#define TAPI_RADIUS_PACKET_MAX_LEN  4096

/** Length of authenticator */
#define TAPI_RADIUS_AUTH_LEN  16

/** RADIUS packet */
typedef struct {
    struct timeval           ts;
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
#define TAPI_RADIUS_AUTH_PORT   (htons(1812))

/** Default UDP port for RADIUS accounting service */
#define TAPI_RADIUS_ACCT_PORT   (htons(1813))

typedef void (*radius_callback)(const tapi_radius_packet_t *pkt,
                                void *userdata);

typedef struct tapi_radius_pkt_handler_data {
    radius_callback  callback;
    void            *user_data;
} tapi_radius_pkt_handler_data;


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

extern te_errno tapi_radius_attr_list_push(tapi_radius_attr_list_t *list,
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
extern te_errno tapi_radius_attr_list_push_value(tapi_radius_attr_list_t *list,
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
extern te_errno tapi_radius_attr_list_copy(tapi_radius_attr_list_t *dst,
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
extern te_errno
       tapi_radius_attr_list_to_string(const tapi_radius_attr_list_t *list,
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
extern te_errno tapi_radius_parse_packet(const uint8_t *data, size_t data_len,
                                         tapi_radius_packet_t *packet);


/**
 * Prepare callback data to be passed in tapi_tad_trrecv_{wait,stop,get}
 * to process received RADIUS packets.
 *
 * @param callback        Callback for RADIUS packets handling
 * @param user_data       User-supplied data to be passed to @a callback
 *
 * @return Pointer to allocated callback data or NULL.
 */
extern tapi_tad_trrecv_cb_data *tapi_radius_trrecv_cb_data(
                                    radius_callback  callback,
                                    void            *user_data);


/**
 * Create 'udp.ip4.eth' CSAP for capturing RADIUS packets
 *
 * @param ta               Test Agent name
 * @param sid              RCF session identifier
 * @param device           Ethernet device name on agent to attach
 * @param net_addr         Local IP address on Test Agent
 * @param port             UDP port (network byte order) on Test Agent
 *                         (TAPI_RADIUS_AUTH_PORT, TAPI_RADIUS_ACCT_PORT,
 *                         or -1 to keep unspecified)
 * @param csap             Handle of new CSAP (OUT)
 *
 * @return Zero on success or error code.
 */
extern te_errno tapi_radius_csap_create(const char *ta, int sid,
                                        const char *device,
                                        const in_addr_t net_addr,
                                        int port,
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
extern te_errno tapi_radius_serv_enable(const char *ta_name);

/**
 * Disables RADIUS Server on the particular Agent.
 *
 * @param ta_name  Test Agent name
 *
 * @return Zero on success or error code.
 */
extern te_errno tapi_radius_serv_disable(const char *ta_name);

/**
 * Update RADIUS Server Configuration.
 *
 * @param ta_name  Test Agent name
 * @param cfg      RADIUS Server configuration information
 *
 * @return Zero on success or error code.
 */
extern te_errno tapi_radius_serv_set(const char *ta_name,
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
extern te_errno tapi_radius_serv_add_client(const char *ta_name,
                                            const tapi_radius_clnt_t *cfg);

/**
 * Delete RADIUS Client record from RADIUS Server.
 *
 * @param ta_name   Test Agent name
 * @param net_addr  RADIUS Client's network address
 *
 * @return Zero on success or error code.
 */
extern te_errno tapi_radius_serv_del_client(const char *ta_name,
                                            const struct in_addr *net_addr);


/** Enumeration for user attribute list */
typedef enum {
    TAPI_RADIUS_USR_CHECK_LST, /**< Check attributes list */
    TAPI_RADIUS_USR_ACPT_LST, /**< Accept attributes list */
    TAPI_RADIUS_USR_CHLG_LST, /**< Challenge attributes list */
} tapi_radius_usr_list_t;

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
extern te_errno tapi_radius_serv_add_user(
                            const char *ta_name,
                            const char *user_name,
                            te_bool acpt_user,
                            const tapi_radius_attr_list_t *check_attrs,
                            const tapi_radius_attr_list_t *acpt_attrs,
                            const tapi_radius_attr_list_t *chlg_attrs);

/**
 * Updates the particular user list on RADIUS Server
 *
 * @param ta_name    Test Agent name
 * @param user_name  User name
 * @param list_type  Which list to modify
 * @param attrs      A list of RADIUS attributes that should be used
 *                   for specified user (new list)
 *
 * @return Zero on success or error code.
 */
extern te_errno tapi_radius_serv_set_user_attr(
                            const char *ta_name,
                            const char *user_name,
                            tapi_radius_usr_list_t list_type,
                            const tapi_radius_attr_list_t *attrs);

/**
 * Delete user configuration from RADIUS Server.
 *
 * @param ta_name     Test Agent name
 * @param user_name   User name
 *
 * @return Zero on success or error code.
 */
extern te_errno tapi_radius_serv_del_user(const char *ta_name,
                                          const char *user_name);

/** Key management types supported by supplicant */
typedef enum {
    TAPI_AUTH_KEY_NONE,     /**< No key authentication */
    TAPI_AUTH_KEY_PSK,      /**< Pre-shared key authentication */
    TAPI_AUTH_KEY_8021X     /**< IEEE802.1x/EAP authentication */
} tapi_auth_key_mgmt_t;

/** EAP key management types */
typedef enum {
    TAPI_AUTH_EAP_MD5,      /**< EAP-MD5 authentication */
    TAPI_AUTH_EAP_TLS,      /**< EAP-TLS authentication */
} tapi_auth_eap_t;

/** Wireless authentication protocol */
typedef enum {
    TAPI_AUTH_PROTO_PLAIN,  /**< No WPA/WPA2 */
    TAPI_AUTH_PROTO_WPA,    /**< Wi-Fi Protected Access (WPA) */
    TAPI_AUTH_PROTO_RSN     /**< Robust Security Network (RSN), IEEE 802.11i */
} tapi_auth_proto_t;

/** Wireless cipher algorithms */
#define TAPI_AUTH_CIPHER_NONE       0x00
#define TAPI_AUTH_CIPHER_WEP40      0x01
#define TAPI_AUTH_CIPHER_WEP104     0x02
#define TAPI_AUTH_CIPHER_TKIP       0x04
#define TAPI_AUTH_CIPHER_CCMP       0x08
#define TAPI_AUTH_CIPHER_WEP \
        (TAPI_AUTH_CIPHER_WEP40 | TAPI_AUTH_CIPHER_WEP104)

/** TLS private key and certificate info */
typedef struct tapi_auth_tls_s {
    char *cert_fname;                   /**< Certificate file name */
    char *key_fname;                    /**< Private key file name */
    char *key_passwd;                   /**< Password to private key file */
} tapi_auth_tls_t;

/** Configuration parameters for EAP authentication */
typedef struct tapi_auth_info_s {
    char                    *identity;  /**< EAP identity */
    tapi_auth_eap_t          eap_type;  /**< EAP type */
    te_bool                  valid;     /**< Whether the user allowed */
    union {
        struct {
            char *username;             /**< User name */
            char *passwd;               /**< User's password */
        } md5;                          /**< EAP-MD5 parameters */
        struct {
            tapi_auth_tls_t server;     /**< Server TLS info */
            tapi_auth_tls_t client;     /**< Client TLS info */
            char *root_cert_fname;      /**< CA certificate file name */
        } tls;                          /**< EAP-TLS parameters */
    };
} tapi_auth_info_t;

/** Wireless-specific authentication parameters */
typedef struct tapi_auth_wifi_s {
    tapi_auth_proto_t    proto;             /**< WPA protocol version */
    uint16_t             cipher_pairwise;   /**< Pairwise ciphers set */
    uint16_t             cipher_group;      /**< Group ciphers set */
    tapi_auth_key_mgmt_t key_mgmt;          /**< Key management type */
} tapi_auth_wifi_t;

extern te_errno tapi_radius_add_auth(const char *ta_name,
                                     const tapi_auth_info_t *auth,
                                     const tapi_radius_attr_list_t *acpt_attrs,
                                     const tapi_radius_attr_list_t *chlg_attrs);

static inline te_errno
tapi_radius_del_auth(const char *ta_name, const tapi_auth_info_t *auth)
{
    if (auth == NULL)
        return 0;

    return tapi_radius_serv_del_user(ta_name, auth->identity);
}

static inline te_errno
tapi_radius_disable_auth(const char *ta_name,
                         tapi_auth_info_t *auth)
{
    assert(auth != NULL);
    auth->valid = FALSE;
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, 0),
                                "/agent:%s/radiusserver:/user:%s",
                                ta_name, auth->identity);
}

/**
 * Enable/disable supplicant at specified interface
 *
 * @param ta_name   Name of TA where supplicant resides
 * @param if_name   Name of interface which is controlled by supplicant
 * @param value     Required supplicant state (0 to disable, 1 to enable)
 *
 * @return Status of the operation.
 */
static inline te_errno
tapi_supp_set(const char *ta_name, const char *if_name, int value)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, value),
                                "/agent:%s/interface:%s/supplicant:",
                                ta_name, if_name);
}

extern te_errno tapi_supp_set_wifi_auth(const char *ta_name,
                                        const char *if_name,
                                        const tapi_auth_wifi_t *wifi);
/**
 * Configure supplicant to use EAP authentication and set
 * method-specific parameters on the Agent.
 *
 * @param ta_name  Test Agent name where supplicant reside
 * @param if_name  Interface name
 * @param info     EAP method-specific information
 *
 * @return Status of the operation
 */
extern te_errno tapi_supp_set_auth(const char *ta_name,
                                   const char *if_name,
                                   const tapi_auth_info_t *info);

/**
 * Reset supplicant parameters to default values
 *
 * @param ta_name  Test Agent name where supplicant reside
 * @param if_name  Interface name
 *
 * @return Status of the operation.
 */
extern te_errno tapi_supp_reset(const char *ta_name, const char *if_name);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_RADIUS_H__ */

/**@} <!-- END tapi_radius --> */
