/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Proteos, TAPI DHCP.
 *
 * Implementation of TAPI DHCP library.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAPI DHCP"

#include "te_config.h"

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif

#include "te_errno.h"
#include "logger_api.h"
#include "rcf_api.h"
#include "ndn.h"
#include "ndn_dhcp.h"
#include "tapi_tad.h"
#include "tapi_dhcp.h"


#define DHCP_MAGIC_SIZE        4

#define CHECK_RC(expr_) \
    do {                                                \
        int rc_ = (expr_);                              \
                                                        \
        if (rc_ != 0)                                   \
        {                                               \
            ERROR("%s():%u: " #expr_ " FAILED rc=%r", \
                  __FUNCTION__, __LINE__, rc_);         \
            return rc_;                                 \
        }                                               \
    } while (0)

struct dhcp_rcv_pkt_info {
    struct dhcp_message *dhcp_msg;
};

/*
 * Now TAPI DHCP allows issuing only one request on receiving
 * DHCP message simultaneously.
 */

/** */
static struct dhcp_rcv_pkt_info rcv_pkt;
/** Indicates if TAPI DHCP can issue request on receiving DHCP packet */
static te_bool rcv_op_busy = FALSE;

/* If pthread mutexes are supported - OK; otherwise hope for the best... */
#ifdef HAVE_PTHREAD_H
static pthread_mutex_t tapi_dhcp_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

static int ndn_dhcpv4_option_to_plain(const asn_value *dhcp_opt,
                                      struct dhcp_option **opt_p);
static int ndn_dhcpv4_add_opts(asn_value *container,
                               struct dhcp_option *opt);
static void dhcp_options_destroy(struct dhcp_option *opt);


/**
 * Convert DHCPv4 ASN value to plain C structrue
 *
 * @param[in]   pkt         ASN value of type DHCPv4 message or
 *                          Generic-PDU with choice "dhcp"
 * @param[out]  dhcp_msg    Converted structure
 *
 * @return Zero on success or error code
 *
 * @se Function allocates memory under dhcp_message data structure,
 *     which should be freed with dhcpv4_message_destroy
 */
int
ndn_dhcpv4_packet_to_plain(const asn_value *pkt,
                           struct dhcp_message **dhcp_msg)
{
    struct dhcp_option **opt_p;
    const asn_value     *dhcp_opts;
    const asn_value     *opt;

    int         rc = 0;
    size_t      len;
    int         i;
    int         n_opts;

    *dhcp_msg = (struct dhcp_message *)malloc(sizeof(**dhcp_msg));
    if (*dhcp_msg == NULL)
        return TE_ENOMEM;

    memset(*dhcp_msg, 0, sizeof(**dhcp_msg));

/** Stores the value of 'field_' entry in user-specified buffer 'ptr_' */
#define PKT_GET_VALUE(ptr_, field_) \
    do {                                                           \
        len = sizeof((*dhcp_msg)->field_);                         \
        if (rc == 0)                                               \
        {                                                          \
            rc = asn_read_value_field(pkt, (ptr_),                 \
                                      &len, #field_ ".#plain");    \
            if (rc == 0)                                           \
                (*dhcp_msg)->is_ ## field_ ## _set = TRUE;         \
            else if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)     \
            {                                                      \
                /*                                                 \
                 * Field name is valid but the value               \
                 * is not specified                                \
                 */                                                \
                (*dhcp_msg)->is_ ## field_ ## _set = FALSE;        \
                rc = 0;                                            \
            }                                                      \
            else                                                   \
                WARN("%s() at line %d: error %r for fld %s",     \
                     __FUNCTION__, __LINE__, rc, #field_);         \
        }                                                          \
    } while (0)

#define PKT_GET_SIMPLE_VALUE(field_) \
    PKT_GET_VALUE((&((*dhcp_msg)->field_)), field_)

#define PKT_GET_ARRAY_VALUE(field_) \
    PKT_GET_VALUE(((*dhcp_msg)->field_), field_)

    PKT_GET_SIMPLE_VALUE(op);
    PKT_GET_SIMPLE_VALUE(htype);
    PKT_GET_SIMPLE_VALUE(hlen);
    PKT_GET_SIMPLE_VALUE(hops);
    PKT_GET_SIMPLE_VALUE(xid);
    PKT_GET_SIMPLE_VALUE(secs);
    PKT_GET_SIMPLE_VALUE(flags);
    PKT_GET_SIMPLE_VALUE(ciaddr);
    PKT_GET_SIMPLE_VALUE(yiaddr);
    PKT_GET_SIMPLE_VALUE(siaddr);
    PKT_GET_SIMPLE_VALUE(giaddr);
    PKT_GET_ARRAY_VALUE(chaddr);
    PKT_GET_ARRAY_VALUE(sname);
    PKT_GET_ARRAY_VALUE(file);

#undef PKT_GET_ARRAY_VALUE
#undef PKT_GET_SIMPLE_VALUE
#undef PKT_GET_VALUE

    if (rc != 0 ||
        (rc = asn_get_descendent(pkt, (asn_value **)&dhcp_opts,
                                 "options")) != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
        {
            /* No options specified */
            return 0;
        }

        free(*dhcp_msg);
        return rc;
    }

    n_opts = asn_get_length(dhcp_opts, "");
    opt_p = &((*dhcp_msg)->opts);
    for (i = 0; i < n_opts; i++)
    {
        if ((rc = asn_get_indexed(dhcp_opts, (asn_value **)&opt, i,
                                  NULL)) != 0 ||
            (rc = ndn_dhcpv4_option_to_plain(opt, opt_p)) != 0)
        {
            dhcpv4_message_destroy(*dhcp_msg);
            return rc;
        }
        opt_p = &((*opt_p)->next);
    }

    return 0;
}

/**
 * Convert DHCPv4 Option ASN value to plain C structrue
 *
 * @param[in]   dhcp_opt    ASN value of type DHCPv4 option
 * @param[out]  opt_p       Converted structure
 *
 * @return Zero on success or error code
 */
static int
ndn_dhcpv4_option_to_plain(const asn_value *dhcp_opt,
                           struct dhcp_option **opt_p)
{
    int rc = 0;
    uint8_t len_buf;
    size_t len = sizeof(len_buf);
    int i;
    int n_subopts;

    rc = asn_read_value_field(dhcp_opt, &len_buf, &len, "length.#plain");
    if (rc == 0)
    {
        len = asn_get_length(dhcp_opt, "value.#plain");
    }
    else if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        /*
         * Option doesn't have length and value fields
         * (END and PAD options)
         */
        len = 0;
    }
    else
        return rc;

    if ((*opt_p = (struct dhcp_option *)malloc(sizeof(**opt_p))) == NULL ||
        (len != 0 && ((*opt_p)->val = (uint8_t *)malloc(len)) == NULL))
    {
        free(*opt_p);
        *opt_p = NULL;
        return TE_ENOMEM;
    }
    (*opt_p)->val_len = len;
    (*opt_p)->next = NULL;
    (*opt_p)->subopts = NULL;

    if (len == 0)
    {
        /* Option without payload */
        (*opt_p)->len = 0;
        (*opt_p)->val = NULL;
        len = sizeof((*opt_p)->type);
        rc = asn_read_value_field(dhcp_opt, &((*opt_p)->type),
                                  &len, "type.#plain");
        return rc;
    }

    if ((rc = asn_read_value_field(dhcp_opt, (*opt_p)->val,
                                   &len, "value.#plain")) != 0 ||
        (len = sizeof((*opt_p)->len),
         (rc = asn_read_value_field(dhcp_opt, &((*opt_p)->len),
                                    &len, "length.#plain")) != 0) ||
        (len = sizeof((*opt_p)->type),
         (rc = asn_read_value_field(dhcp_opt, &((*opt_p)->type),
                                    &len, "type.#plain")) != 0))
    {
        free((*opt_p)->val);
        free(*opt_p);
        *opt_p = NULL;
        return rc;
    }

    if ((n_subopts = asn_get_length(dhcp_opt, "options")) > 0)
    {
        struct dhcp_option **sub_opt_p;
        const asn_value     *sub_opt;
        const asn_value     *sub_opts;

        sub_opt_p = &((*opt_p)->subopts);
        rc = asn_get_descendent(dhcp_opt, (asn_value **)&sub_opts,
                                "options");
        if (rc != 0)
        {
            free((*opt_p)->val);
            free(*opt_p);
            *opt_p = NULL;
            return rc;
        }

        for (i = 0; i < n_subopts; i++)
        {
            if ((rc = asn_get_indexed(sub_opts, (asn_value **)&sub_opt,
                                      i, NULL)) != 0 ||
                (rc = ndn_dhcpv4_option_to_plain(sub_opt, sub_opt_p)) != 0)
            {
                free((*opt_p)->val);
                free(*opt_p);
                *opt_p = NULL;
                return rc;
            }
            sub_opt_p = &((*sub_opt_p)->next);
        }
    }

    return rc;
}

/**
 * Convert C structure to DHCPv4 ASN value
 *
 * @param[in]   dhcp_msg    DHCP message in plain C data structure
 * @param[out]  pkt         Placeholder for the new ASN.1 DHCP packet
 *
 * @return Zero on success or error code
 */
int
ndn_dhcpv4_plain_to_packet(const struct dhcp_message *dhcp_msg,
                           asn_value **pkt)
{
    size_t len;
    int rc = 0;

/**
 * Sets the value of 'field_' in ASN.1 DHCPv4 structure
 * It sets the value only if is_'field'_set flag is set
 */

#define PKT_SET_SIMPLE_VALUE(field_) \
    do {                                                          \
        if (rc == 0 && (dhcp_msg)->is_ ## field_ ## _set == TRUE) \
        {                                                         \
            rc = asn_write_int32(*pkt, (dhcp_msg)->field_,        \
                                 #field_ ".#plain");              \
        }                                                         \
    } while (0)

#define PKT_SET_ARRAY_VALUE(field_) \
    do {                                                          \
        len = sizeof((dhcp_msg)->field_);                         \
        if (rc == 0 && (dhcp_msg)->is_ ## field_ ## _set == TRUE) \
        {                                                         \
            rc = asn_write_value_field(*pkt, ((dhcp_msg)->field_),\
                                       len, #field_ ".#plain");   \
        }                                                         \
    } while (0)

    *pkt = asn_init_value(ndn_dhcpv4_message);

    PKT_SET_SIMPLE_VALUE(op);
    PKT_SET_SIMPLE_VALUE(htype);
    PKT_SET_SIMPLE_VALUE(hlen);
    PKT_SET_SIMPLE_VALUE(hops);
    PKT_SET_SIMPLE_VALUE(xid);
    PKT_SET_SIMPLE_VALUE(secs);
    PKT_SET_SIMPLE_VALUE(flags);
    PKT_SET_SIMPLE_VALUE(ciaddr);
    PKT_SET_SIMPLE_VALUE(yiaddr);
    PKT_SET_SIMPLE_VALUE(siaddr);
    PKT_SET_SIMPLE_VALUE(giaddr);
    PKT_SET_ARRAY_VALUE(chaddr);
    PKT_SET_ARRAY_VALUE(sname);
    PKT_SET_ARRAY_VALUE(file);

#undef PKT_SET_ARRAY_VALUE
#undef PKT_SET_SIMPLE_VALUE

    if (rc == 0)
        rc = ndn_dhcpv4_add_opts(*pkt, dhcp_msg->opts);
    return rc;
}

static int
ndn_dhcpv4_add_opts(asn_value *container, struct dhcp_option *opt)
{
    if (opt != NULL)
    {
        asn_value *opts = asn_init_value(ndn_dhcpv4_options);

        CHECK_RC(asn_write_component_value(container, opts, "options"));

        for (; opt != NULL; opt = opt->next)
        {
            asn_value *dhcp_opt =
                asn_init_value((opt->type == 255 || opt->type == 0) ?
                               ndn_dhcpv4_end_pad_option :
                               ndn_dhcpv4_option);

            CHECK_RC(asn_write_int32(dhcp_opt, opt->type, "type.#plain"));

            /* Options 255 and 0 don't have length and value parts */
            if (opt->type != 255 && opt->type != 0)
            {
                CHECK_RC(asn_write_int32(dhcp_opt, opt->len,
                                         "length.#plain"));
                CHECK_RC(asn_write_value_field(dhcp_opt, opt->val,
                                               opt->val_len,
                                               "value.#plain"));
            }

            CHECK_RC(ndn_dhcpv4_add_opts(dhcp_opt, opt->subopts));
            CHECK_RC(asn_insert_indexed(container,
                                        asn_copy_value(dhcp_opt), -1,
                                        "options"));

            if (opt->subopts)
                CHECK_RC(asn_free_subvalue(dhcp_opt, "options"));

            asn_free_value(dhcp_opt);
        }
    }

    return 0;
}


/* See description in tapi_dhcp.h */
struct dhcp_message *
dhcpv4_bootp_message_create(uint8_t op)
{
    struct dhcp_message *dhcp_msg;

    dhcp_msg = (struct dhcp_message *)calloc(1, sizeof(*dhcp_msg));
    if (dhcp_msg == NULL)
        return NULL;

    dhcp_msg->op = op;
    dhcp_msg->is_op_set = TRUE;
    dhcp_msg->htype = DHCP_HW_TYPE_ETHERNET_10MB;
    dhcp_msg->is_htype_set = TRUE;
    dhcp_msg->hlen = ETHER_ADDR_LEN;
    dhcp_msg->is_hlen_set = TRUE;

    return dhcp_msg;
}

/* See description in tapi_dhcp.h */
struct dhcp_message *
dhcpv4_message_create(dhcp_message_type msg_type)
{
    uint8_t              op;
    struct dhcp_message *dhcp_msg;
    struct dhcp_option  *opt;

    if (msg_type == DHCPDISCOVER || msg_type == DHCPREQUEST ||
        msg_type == DHCPDECLINE || msg_type == DHCPRELEASE)
    {
        op = DHCP_OP_CODE_BOOTREQUEST;
    }
    else if (msg_type == DHCPOFFER || msg_type == DHCPACK ||
             msg_type == DHCPNAK)
    {
        op = DHCP_OP_CODE_BOOTREPLY;
    }
    else
        assert(0);

    dhcp_msg = dhcpv4_bootp_message_create(op);
    if (dhcp_msg == NULL)
        return NULL;

    if ((opt = (struct dhcp_option *)malloc(sizeof(*opt))) == NULL ||
        (opt->val = (uint8_t *)malloc(1)) == NULL)
    {
        free(opt);
        free(dhcp_msg);
        return NULL;
    }
    opt->next = NULL;
    opt->subopts = NULL;
    opt->type = DHCP_OPT_MESSAGE_TYPE;
    opt->len = opt->val_len = 1;
    *(opt->val) = msg_type;
    dhcp_msg->opts = opt;

    return dhcp_msg;
}

/**
 * Gets specified DHCP option from the DHCP message
 *
 * @param dhcp_msg  DHCP message handle
 * @param type      Type of the option to return
 *
 * @return  Pointer to the option or NULL if there is no
 *          such option in the message
 */
const struct dhcp_option *
dhcpv4_message_get_option(const struct dhcp_message *dhcp_msg, uint8_t type)
{
    struct dhcp_option *cur_opt;

    assert(dhcp_msg != NULL);

    for (cur_opt = dhcp_msg->opts; cur_opt != NULL; cur_opt = cur_opt->next)
    {
        if (cur_opt->type == type)
            break;
    }
    return cur_opt;
}

/**
 * Gets sub-option of specified type from the option
 *
 * @param opt   Option from which the sub-option to return
 * @param type  Sub-option type
 *
 * @return  Pointer to the sub-option or NULL if there is no
 *          such sub-option in the option
 */
const struct dhcp_option *
dhcpv4_message_get_sub_option(const struct dhcp_option *opt, uint8_t type)
{
    struct dhcp_option *cur_sub_opt;

    assert(opt != NULL);

    for (cur_sub_opt = opt->subopts;
         cur_sub_opt != NULL;
         cur_sub_opt = cur_sub_opt->next)
    {
        if (cur_sub_opt->type == type)
            break;
    }
    return cur_sub_opt;
}

/**
 * Creates a new option
 *
 * @param type      Type of the option
 * @param len       Value of the 'Length' field of the option
 * @param val_len   Length of the buffer to be used as the value
 *                  of the option
 * @param val       Pointer to the buffer of the value of the option
 *
 * @return  Pointer to created option of NULL if there is not enough memory
 */
struct dhcp_option *
dhcpv4_option_create(uint8_t type, uint8_t len,
                     uint8_t val_len, uint8_t *val)
{
    struct dhcp_option *opt;

    if ((opt = (struct dhcp_option *)malloc(sizeof(*opt))) == NULL ||
        (opt->val = (uint8_t *)malloc(val_len)) == NULL)
    {
        free(opt);
        return NULL;
    }

    opt->next = NULL;
    opt->subopts = NULL;
    opt->type = type;
    opt->len = len;
    opt->val_len = val_len;
    memcpy(opt->val, val, val_len);

    return opt;
}

/**
 * Adds a new sub-option to the end of the sub-options list of the option
 *
 * @param opt       Handle of the option
 * @param type      Type of the sub-option
 * @param len       Length of the sub-option value
 * @param val       Pointer to the buffer of the sub-option value
 *
 * @return  Status of the operation
 */
int
dhcpv4_option_add_subopt(struct dhcp_option *opt, uint8_t type, uint8_t len,
                         uint8_t *val)
{
    struct dhcp_option  *subopt;
    struct dhcp_option **cur_opt_p;

    if ((subopt = dhcpv4_option_create(type, len, len, val)) == NULL)
        return TE_ENOMEM;

    cur_opt_p = &(opt->subopts);
    while (*cur_opt_p != NULL)
    {
        cur_opt_p = &((*cur_opt_p)->next);
    }

    *cur_opt_p = subopt;

    return 0;
}

/**
 * Inserts a new sub-option to the end of the sub-options list of the option
 *
 * @param opt       Handle of the option
 * @param subopt    Sub-option to insert
 *
 * @return  Status of the operation
 */
int
dhcpv4_option_insert_subopt(struct dhcp_option *opt,
                            struct dhcp_option *subopt)
{
    struct dhcp_option **cur_opt_p;

    cur_opt_p = &(opt->subopts);
    while (*cur_opt_p != NULL)
    {
        cur_opt_p = &((*cur_opt_p)->next);
    }

    *cur_opt_p = subopt;

    return 0;
}

/**
 * Adds an option to the end of option list in the DHCP message
 *
 * @param dhcp_msg  DHCP message handle
 * @param type      Type of the option to insert
 * @param len       Length of the option value
 * @param val       Pointer to the buffer of the option value
 *
 * @return  Status of the operation
 *
 * @note The function cannot be used for adding options with incorrect
 *       length
 */
int
dhcpv4_message_add_option(struct dhcp_message *dhcp_msg, uint8_t type,
                          uint8_t len, const void *val)
{
    struct dhcp_option **cur_opt_p;

    assert(dhcp_msg != NULL);
    assert((len != 0 && val != NULL) || (len == 0 && val == NULL));

    cur_opt_p = &(dhcp_msg->opts);
    while (*cur_opt_p != NULL)
    {
        cur_opt_p = &((*cur_opt_p)->next);
    }
    if ((*cur_opt_p = (struct dhcp_option *)
                                  malloc(sizeof(**cur_opt_p))) == NULL ||
        (len > 0 && ((*cur_opt_p)->val = (uint8_t *)malloc(len)) == NULL))
    {
        free(*cur_opt_p);
        return TE_ENOMEM;
    }
    (*cur_opt_p)->next = NULL;
    (*cur_opt_p)->subopts = NULL;
    (*cur_opt_p)->type = type;
    (*cur_opt_p)->len = len;
    (*cur_opt_p)->val_len = len;

    if (val != NULL)
        memcpy((*cur_opt_p)->val, val, len);
    else
        (*cur_opt_p)->val = NULL;

    return 0;
}


/**
 * Inserts user-prepared option to the end of the option list
 * in the DHCP message
 *
 * @param dhcp_msg  DHCP message handle
 * @param opt       Option to insert
 *
 * @return  Status of the operation
 *
 * @note User is responsible for correct values 'next' and 'subopts' fields
 * of the inserted option
 */
int
dhcpv4_message_insert_option(struct dhcp_message *dhcp_msg,
                             struct dhcp_option *opt)
{
    struct dhcp_option **cur_opt_p;

    assert(dhcp_msg != NULL);

    cur_opt_p = &(dhcp_msg->opts);
    while (*cur_opt_p != NULL)
    {
        cur_opt_p = &((*cur_opt_p)->next);
    }
    *cur_opt_p = opt;

    return 0;
}



void
dhcpv4_message_destroy(struct dhcp_message *dhcp_msg)
{
    if (dhcp_msg == NULL)
        return;

    dhcp_options_destroy(dhcp_msg->opts);
    free(dhcp_msg);
}

static void
dhcp_options_destroy(struct dhcp_option *opt)
{
    struct dhcp_option *tmp;

    while (opt != NULL)
    {
        free(opt->val);
        dhcp_options_destroy(opt->subopts);
        tmp = opt;
        opt = opt->next;
        free(tmp);
    }
}

/**
 * Fills some fields of reply message based on the values of request
 * message: Copies xid, yiaddr, siaddr, flags, giaddr and chaddr fields
 *
 * @param dhcp_rep  DHCP reply message to be updated
 * @param dhcp_req  DHCP request message whose values are used as a basic
 *
 * @return N/A
 */
void
dhcpv4_message_fill_reply_from_req(struct dhcp_message *dhcp_rep,
                                   const struct dhcp_message *dhcp_req)
{
    uint32_t xid = dhcpv4_message_get_xid(dhcp_req);
    uint16_t flags = dhcpv4_message_get_flags(dhcp_req);
    uint32_t yiaddr = dhcpv4_message_get_yiaddr(dhcp_req);
    uint32_t siaddr = dhcpv4_message_get_siaddr(dhcp_req);
    uint32_t giaddr = dhcpv4_message_get_giaddr(dhcp_req);
    const uint8_t *chaddr = dhcpv4_message_get_chaddr(dhcp_req);

    dhcpv4_message_set_xid(dhcp_rep, &xid);
    dhcpv4_message_set_flags(dhcp_rep, &flags);
    dhcpv4_message_set_yiaddr(dhcp_rep, &yiaddr);
    dhcpv4_message_set_siaddr(dhcp_rep, &siaddr);
    dhcpv4_message_set_giaddr(dhcp_rep, &giaddr);
    dhcpv4_message_set_chaddr(dhcp_rep, chaddr);
}

/**
 * Checks if Option 55 contains specified option code in its list
 *
 * @param opt   Pointer to option with type 55
 * @param type  Option type to check if Option 55 has it in the list
 *
 * @return Result of the checking
 * @retval TRUE   Option 55 contains option code 'type' in its list
 * @retval FALSE  Option 55 does not contain option code 'type' in its list
 */
te_bool
dhcpv4_option55_has_code(const struct dhcp_option *opt, uint8_t type)
{
    unsigned int i;

    for (i = 0; i < opt->val_len; i++)
    {
        if (opt->val[i] == type)
            return TRUE;
    }
    return FALSE;
}

/**
 * Creates DHCPv4 CSAP in server mode
 * (Sends/receives traffic from/on DHCPv4 server port)
 *
 * @param ta_name       Test Agent name
 * @param if_addr       IPv4 address of a Test Agent interface, on which
 *                      the CSAP is attached
 * @param mode          DHCP mode (client or server)
 * @param dhcp_csap     Location for the DHCPv4 CSAP handle (OUT)
 *
 * @return  Status of the operation
 */
int
tapi_dhcpv4_plain_csap_create(const char *ta_name,
                              const char *iface,
                              dhcp_csap_mode mode,
                              csap_handle_t *dhcp_csap)
{
    int         rc;
    asn_value  *asn_dhcp_csap;
    asn_value  *csap_spec;
    asn_value  *csap_layers;
    asn_value  *csap_layer_spec;

    csap_spec       = asn_init_value(ndn_csap_spec);
    csap_layers     = asn_init_value(ndn_csap_layers);
    csap_layer_spec = asn_init_value(ndn_generic_csap_layer);
    asn_dhcp_csap   = asn_init_value(ndn_dhcpv4_csap);

    asn_put_child_value(csap_spec, csap_layers, PRIVATE, NDN_CSAP_LAYERS);
    asn_write_int32(asn_dhcp_csap, mode, "mode");
    asn_write_value_field(asn_dhcp_csap, iface, strlen(iface) + 1, "iface");

    rc = asn_write_component_value(csap_layer_spec, asn_dhcp_csap, "#dhcp");
    if (rc == 0)
    {
        rc = asn_insert_indexed(csap_layers, csap_layer_spec, -1, "");
    }

    if (rc == 0)
    {
        rc = tapi_tad_csap_create(ta_name, 0, "dhcp", csap_spec, dhcp_csap);
    }

    asn_free_value(csap_spec);
    asn_free_value(asn_dhcp_csap);

    return rc;
}

/**
 * Creates ASN.1 text file with traffic template of one DHCPv4 message
 *
 * @param dhcp_msg     DHCPv4 message
 * @param templ_fname  Traffic template file name (OUT)
 *
 * @return status code
 */
int
dhcpv4_prepare_traffic_template(const dhcp_message *dhcp_msg,
                                const char **templ_fname)
{
    asn_value *asn_dhcp_msg;
    asn_value *asn_traffic;
    asn_value *asn_pdus;
    asn_value *asn_pdu;

    /* Create ASN.1 representation of DHCP message */
    CHECK_RC(ndn_dhcpv4_plain_to_packet(dhcp_msg, &asn_dhcp_msg));

    asn_traffic = asn_init_value(ndn_traffic_template);
    asn_pdus = asn_init_value(ndn_generic_pdu_sequence);
    asn_pdu = asn_init_value(ndn_generic_pdu);

    CHECK_RC(asn_write_component_value(asn_pdu, asn_dhcp_msg, "#dhcp"));
    CHECK_RC(asn_insert_indexed(asn_pdus, asn_pdu, -1, ""));
    CHECK_RC(asn_write_component_value(asn_traffic, asn_pdus, "pdus"));

    /* @todo Generate temporary file name */
    *templ_fname = "./tmp_ndn_send.dat";

    CHECK_RC(asn_save_to_file(asn_traffic, *templ_fname));

    return 0;
}

/**
 * Creates ASN.1 text file with traffic pattern of one DHCPv4 message
 *
 * @param dhcp_msg       DHCPv4 message to be used as a pattern
 * @param pattern_fname  Traffic pattern file name (OUT)
 *
 * @return status code
 */
int
dhcpv4_prepare_traffic_pattern(const dhcp_message *dhcp_msg,
                               char **pattern_fname)
{
    asn_value  *asn_dhcp_msg;
    asn_value  *asn_pattern;
    asn_value  *asn_pattern_unit;
    asn_value  *asn_pdus;
    asn_value  *asn_pdu;
    int         rc;

    if (pattern_fname == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    if ((rc = ndn_dhcpv4_plain_to_packet(dhcp_msg, &asn_dhcp_msg)) != 0)
        return TE_RC(TE_TAPI, rc);

    asn_pattern = asn_init_value(ndn_traffic_pattern);
    asn_pattern_unit = asn_init_value(ndn_traffic_pattern_unit);
    asn_pdus = asn_init_value(ndn_generic_pdu_sequence);
    asn_pdu = asn_init_value(ndn_generic_pdu);

    rc = asn_write_component_value(asn_pdu, asn_dhcp_msg, "#dhcp");
    if (rc == 0)
        rc = asn_insert_indexed(asn_pdus, asn_pdu, -1, "");
    if (rc == 0)
        rc = asn_write_component_value(asn_pattern_unit, asn_pdus, "pdus");
    if (rc == 0)
        rc = asn_insert_indexed(asn_pattern, asn_pattern_unit, -1, "");

    if (rc != 0)
        return TE_RC(TE_TAPI, rc);

    /* @todo Generate temporary file name */
    *pattern_fname = calloc(1, 100);

    strcpy(*pattern_fname, "/tmp/te-dhcp-pattern.asn.XXXXXX");

    if ((rc = te_make_tmp_file(*pattern_fname)) != 0)
    {
        free(*pattern_fname);
        return TE_RC(TE_TAPI, rc);
    }

    rc = asn_save_to_file(asn_pattern, *pattern_fname);

    return TE_RC(TE_TAPI, rc);
}

/**
 * Sends one DHCP message from the CSAP
 *
 * @param dhcp_csap  CSAP handle
 * @param dhcp_msg   DHCP message to be sent
 *
 * @return  Status of the operation
 */
int
tapi_dhcpv4_message_send(const char *ta_name, csap_handle_t dhcp_csap,
                         const struct dhcp_message *dhcp_msg)
{
    int         rc;
    const char *templ_fname;
    int         sid;

    if ((rc = dhcpv4_prepare_traffic_template(dhcp_msg, &templ_fname)) != 0)
        return rc;

    rc = rcf_ta_create_session(ta_name, &sid);
    if (rc == 0)
        rc = rcf_ta_trsend_start(ta_name, sid,  dhcp_csap, templ_fname,
                                 RCF_MODE_BLOCKING);
    return rc;
}

/**
 * Handler that is used as a callback routine for processing incoming
 * packets
 *
 * @param pkt_fname     File name with ASN.1 representation of the received
 *                      packet
 * @param user_param    Pointer to the placeholder of the DHCP message
 *                      handler
 *
 * @se It allocates a DHCP message
 */
static void
dhcp_pkt_handler(const char *pkt_fname, void *user_param)
{
    struct dhcp_rcv_pkt_info *rcv_pkt =
        (struct dhcp_rcv_pkt_info *)user_param;
    struct dhcp_message      *dhcp_msg;
    int                       rc;
    int                       s_parsed;
    asn_value                *pkt;
    asn_value                *dhcp_pkt;

    rc = asn_parse_dvalue_in_file(pkt_fname, ndn_raw_packet, &pkt,
                                  &s_parsed);
    if (rc != 0)
    {
        ERROR("Failed to parse ASN.1 text file to ASN.1 value: "
              "rc=%r", rc);
        return;
    }

    rc = asn_get_descendent(pkt, &dhcp_pkt, "pdus.0.#dhcp");
    if (rc != 0)
    {
        ERROR("Failed to get 'pdus' from packet, rc %r", rc);
        asn_free_value(pkt);
        return;
    }

    rc = ndn_dhcpv4_packet_to_plain(dhcp_pkt, &dhcp_msg);
    if (rc != 0)
    {
        ERROR("Failed to convert DHCP packet from ASN.1 value to C "
              "structure: rc=%r", rc);
        return;
    }

    rcv_pkt->dhcp_msg = dhcp_msg;
}



/**
 * Star receive of DHCP message of desired type during timeout.
 *
 * @param ta_name       Name of Test Agent
 * @param dhcp_csap     ID of DHCP CSAP, which should receive message
 * @param timeout       Time while CSAP will receive, measured in
 *                      milliseconds, counted wince start receive,
 *                      may be TAD_TIMEOUT_INF for infinitive wait
 * @param msg_type      Desired type of message
 *
 * @return status code
 */
int
dhcpv4_message_start_recv(const char *ta_name, csap_handle_t dhcp_csap,
                          unsigned int timeout, dhcp_message_type msg_type)
{
    struct dhcp_message *dhcp_msg = NULL;

    char *pattern_fname;
    int   rc;
    int   sid;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&tapi_dhcp_lock);
#endif

    if (rcv_op_busy)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&tapi_dhcp_lock);
#endif
        return TE_EBUSY;
    }
    rcv_op_busy = TRUE;
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&tapi_dhcp_lock);
#endif
    rcv_pkt.dhcp_msg = NULL;

    /*
     * Fill in asn_dhcp_msg data structure: specify 'op' field and
     * Option 53
     */
    dhcp_msg = dhcpv4_message_create(msg_type);
    dhcpv4_prepare_traffic_pattern(dhcp_msg, &pattern_fname);
    dhcpv4_message_destroy(dhcp_msg);

    /* receive exactly one packet */
    if ((rc = rcf_ta_create_session(ta_name, &sid)) != 0 ||
        (rc = rcf_ta_trrecv_start(ta_name, sid, dhcp_csap, pattern_fname,
                                  timeout, 1, RCF_TRRECV_PACKETS)) != 0)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_lock(&tapi_dhcp_lock);
#endif
        rcv_op_busy = FALSE;
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&tapi_dhcp_lock);
#endif
    }

    unlink(pattern_fname);

    return rc;
}

struct dhcp_message *
dhcpv4_message_capture(const char *ta_name, csap_handle_t dhcp_csap,
                       unsigned int *timeout)
{
    struct dhcp_message *msg;
    unsigned int         num = 0;
    te_errno             rc;
    unsigned int         tv = *timeout;

    assert(timeout != NULL);

    while (tv > 0 && num == 0)
    {
        rc = rcf_ta_trrecv_get(ta_name, 0, dhcp_csap,
                               dhcp_pkt_handler, &rcv_pkt, &num);
        if (rc != 0)
            return NULL;

        sleep(1);
        tv--;
    }

    rc = rcf_ta_trrecv_stop(ta_name, 0, dhcp_csap,
                            dhcp_pkt_handler, &rcv_pkt, &num);
    if (rc != 0)
        return NULL;

    msg = rcv_pkt.dhcp_msg;
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&tapi_dhcp_lock);
#endif
    rcv_op_busy = FALSE;
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&tapi_dhcp_lock);
#endif

    return msg;
}

struct dhcp_message *
tapi_dhcpv4_send_recv(const char *ta_name, csap_handle_t dhcp_csap,
                      const struct dhcp_message *dhcp_msg,
                      unsigned int *tv, const char **err_msg)
{
    struct dhcp_message      *msg = NULL;
    const char               *templ_fname;
    unsigned int              timeout = *tv;
    int                       rc = 0;
    int                       sid;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&tapi_dhcp_lock);
#endif

    if (rcv_op_busy)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&tapi_dhcp_lock);
#endif
        return NULL;
    }
    rcv_op_busy = TRUE;
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&tapi_dhcp_lock);
#endif
    rcv_pkt.dhcp_msg = NULL;

    if ((rc = dhcpv4_prepare_traffic_template(dhcp_msg, &templ_fname)) != 0)
    {
        *err_msg = "dhcpv4_prepare_traffic_template fails";
        goto free_rcv_op;
    }
    if ((rc = rcf_ta_create_session(ta_name, &sid)) != 0)
    {
        *err_msg = "cannot create rcf session";
        goto free_rcv_op;
    }

    if ((rc = rcf_ta_trsend_recv(ta_name, sid, dhcp_csap, templ_fname,
                                 dhcp_pkt_handler, &rcv_pkt,
                                 timeout, NULL)) != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_ETADNOTMATCH && timeout == 1)
            *err_msg = "DHCP answer doesn't come";
        else
            *err_msg = "rcf_ta_trsend_recv fails";
    }
    else
    {
        msg = rcv_pkt.dhcp_msg;
    }

    if (msg == NULL)
        *err_msg = "DHCP message doesn't come";
free_rcv_op:

#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&tapi_dhcp_lock);
#endif
    rcv_op_busy = FALSE;
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&tapi_dhcp_lock);
#endif

    return msg;
}

/**
 * Obtains IPv4 address of DHCPv4 CSAP is bound to
 *
 * @param ta_name    Test Agent name
 * @param dhcp_csap  CSAP handle
 * @param addr       Location for IPv4 address (OUT)
 */
int
tapi_dhcpv4_csap_get_ipaddr(const char *ta_name, csap_handle_t dhcp_csap,
                            void *addr)
{
    char inet_addr_str[INET_ADDRSTRLEN];
    int  rc;

    if ((rc = rcf_ta_csap_param(ta_name, 0, dhcp_csap, "ipaddr",
                                sizeof(inet_addr_str), inet_addr_str)) != 0)
    {
        return rc;
    }

    if (inet_pton(AF_INET, inet_addr_str, addr) <= 0)
        return errno;

    return 0;
}

/**
 * Tries to send DHCP request and waits for ans answer
 *
 * @param ta        TA name
 * @param csap      DHCP CSAP handle
 * @param lladdr    Local hardware address
 * @param type      Message type (DHCP Option 53)
 * @param broadcast Whether DHCP request is marked as broadcast
 * @param myaddr    in: current IP address; out: obtained IP address
 * @param srvaddr   in: current IP address; out: obtained IP address
 * @param xid       DHCP XID field
 * @param xid       DHCP XID field
 *
 * @return boolean success
 */
static te_bool
dhcp_request_reply(const char *ta, csap_handle_t csap,
                        const struct sockaddr *lladdr,
                        int type, te_bool broadcast,
                        struct in_addr *myaddr,
                        struct in_addr *srvaddr,
                        unsigned long *xid, te_bool set_ciaddr)
{
    struct dhcp_message *request = NULL;
    uint16_t             flags = (broadcast ? FLAG_BROADCAST : 0);
    struct dhcp_option **opt;
    int                  i;
    te_errno             rc = 0;

    if ((request = dhcpv4_message_create(type)) == NULL)
    {
        ERROR("Failed to create DHCP message");
        rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
        goto cleanup0;
    }

    dhcpv4_message_set_flags(request, &flags);
    dhcpv4_message_set_chaddr(request, lladdr->sa_data);
    dhcpv4_message_set_xid(request, xid);

    if (set_ciaddr)
        dhcpv4_message_set_ciaddr(request, myaddr);

    if (type != DHCPDISCOVER)
        dhcpv4_message_add_option(request, DHCP_OPT_SERVER_ID,
                                  sizeof(srvaddr->s_addr),
                                  &srvaddr->s_addr);
    if (type == DHCPREQUEST)
        dhcpv4_message_add_option(request, DHCP_OPT_REQUESTED_IP,
                                  sizeof(myaddr->s_addr),
                                  &myaddr->s_addr);

    /* Add the 'end' option (RFC2131, chapter 4.1, page 22:
     * "The last option must always be the 'end' option")
     */
    dhcpv4_message_add_option(request, 255, 0, NULL);

    /* Calculate the space in octets currently occupied by options */
    for (i = DHCP_MAGIC_SIZE, opt = &(request->opts);
         *opt != NULL;
         opt = &((*opt)->next))
    {
        int opt_type = (*opt)->type;

        i++;                          /** Option type length       */
        if (opt_type != 255 && opt_type != 0)
            i += 1 + (*opt)->val_len; /** Option len + body length */
    }

    /* Align to 32-octet boundary; no such requirement in RFC,
     * Solaris 'dhcp' server does this way, so the test too:
     * alignment is performed over adding appropriate number
     * of 'pad' options
     */
    for (i = 32 - i % 32; i < 32 && i > 0; i--)
        dhcpv4_message_add_option(request, 0, 0, NULL);

    if (type != DHCPRELEASE)
    {
        const char          *err_msg;
        unsigned int         timeout = 10000; /**< 10 sec */
        struct dhcp_message *reply   = tapi_dhcpv4_send_recv(ta, csap,
                                                             request,
                                                             &timeout,
                                                             &err_msg);
        struct dhcp_option const *server_id_option;

        if (reply == NULL)
        {
            ERROR("Failed send/receive DHCP request/reply: %s", err_msg);
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup1;
        }

        myaddr->s_addr = dhcpv4_message_get_yiaddr(reply);

        RING("Got address %d.%d.%d.%d",
             (myaddr->s_addr & 0xFF),
             (myaddr->s_addr >> 8) & 0xFF,
             (myaddr->s_addr >> 16) & 0xFF,
             (myaddr->s_addr >> 24) & 0xFF);

        if (dhcpv4_message_get_xid(reply) != *xid)
        {
            ERROR("Reply XID doesn't match that of request");
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup2;
        }

        if ((server_id_option = dhcpv4_message_get_option(reply,
                                    DHCP_OPT_SERVER_ID)) == NULL)
        {
            ERROR("Cannot get ServerID option");
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup2;
        }

        if (server_id_option->len != 4 || server_id_option->val_len != 4)
        {
            ERROR("Invalid ServerID option value length: %u",
                  server_id_option->val_len);
            rc = TE_RC(TE_TA_UNIX, TE_EFAIL);
            goto cleanup2;
        }

        memcpy(&srvaddr->s_addr, server_id_option->val, 4);

cleanup2:
        free(reply);
    }
    else
    {
        rc = tapi_dhcpv4_message_send(ta, csap, request);
    }

cleanup1:
    free(request);

cleanup0:
    return rc;
}

te_errno
tapi_dhcp_request_ip_addr(char const *ta, char const *if_name,
                          uint8_t const mac[ETHER_ADDR_LEN],
                          struct sockaddr *ip_addr)
{
    int                     rc;
    csap_handle_t           csap = CSAP_INVALID_HANDLE;
    struct in_addr          myaddr = {INADDR_ANY};
    struct in_addr          srvaddr = {INADDR_ANY};
    unsigned long           xid = random();
    struct sockaddr         mac_addr = { .sa_family = AF_LOCAL };

    memcpy(mac_addr.sa_data, mac, ETHER_ADDR_LEN);

    if ((rc = tapi_dhcpv4_plain_csap_create(ta, if_name,
                                            DHCP4_CSAP_MODE_CLIENT,
                                            &csap)) != 0)
    {
        ERROR("Failed to create DHCP client CSAP for interface %s on %s",
              if_name, ta);
        goto cleanup0;
    }

    if ((rc = dhcp_request_reply(ta, csap, &mac_addr, DHCPDISCOVER, TRUE,
                                 &myaddr, &srvaddr, &xid, FALSE)) != 0)
    {
        ERROR("DHCP discovery failed");
        goto cleanup1;
    }

    if ((rc = dhcp_request_reply(ta, csap, &mac_addr, DHCPREQUEST, TRUE,
                                 &myaddr, &srvaddr, &xid, FALSE)) != 0)
    {
        ERROR("DHCP lease cannot be obtained");
        goto cleanup1;
    }

    ip_addr->sa_family = AF_INET;
    SIN(ip_addr)->sin_addr = myaddr;

cleanup1:
    tapi_tad_csap_destroy(ta, 0, csap);

cleanup0:
    return rc;
}

te_errno
tapi_dhcp_release_ip_addr(char const *ta, char const *if_name,
                          struct sockaddr const *addr)
{
    csap_handle_t  csap = CSAP_INVALID_HANDLE;
    struct in_addr myaddr = {INADDR_ANY};
    struct in_addr srvaddr = {INADDR_ANY};
    unsigned long  xid = random();
    te_errno       rc;

    memcpy(&myaddr, &SIN(addr)->sin_addr, sizeof(myaddr));

    if ((rc = tapi_dhcpv4_plain_csap_create(ta, if_name,
                                            DHCP4_CSAP_MODE_CLIENT,
                                            &csap)) != 0)
    {
        ERROR("Failed to create DHCP client CSAP on %s:%s", ta, if_name);
        goto cleanup0;
    }

    if ((rc = dhcp_request_reply(ta, csap, addr, DHCPRELEASE, FALSE,
                                 &myaddr, &srvaddr, &xid, TRUE)) != 0)
    {
        ERROR("Error releasing DHCP lease");
        goto cleanup1;
    }

cleanup1:
    tapi_tad_csap_destroy(ta, 0, csap);

cleanup0:
    return rc;
}

/**
 * Creates DHCPv6 CSAP in server mode
 * (Sends/receives traffic from/on DHCPv4 server port)
 *
 * @param ta_name       Test Agent name
 * @param iface         Tester interfase used to send/receive
 *                      DHCPv6 messages
 * @param mode          DHCPv6 mode (client or server)
 * @param dhcp_csap     Pointer to DHCPv6 CSAP handle (OUT)
 *
 * @return  Status of the operation
 */
int
tapi_dhcpv6_plain_csap_create(const char *ta_name,
                              const char *iface,
                              dhcp6_csap_mode mode,
                              csap_handle_t *dhcp_csap)
{
    int         rc;
    asn_value  *asn_dhcp_csap;
    asn_value  *csap_spec;
    asn_value  *csap_layers;
    asn_value  *csap_layer_spec;

    csap_spec       = asn_init_value(ndn_csap_spec);
    csap_layers     = asn_init_value(ndn_csap_layers);
    csap_layer_spec = asn_init_value(ndn_generic_csap_layer);
    asn_dhcp_csap   = asn_init_value(ndn_dhcpv6_csap);

    asn_put_child_value(csap_spec, csap_layers, PRIVATE, NDN_CSAP_LAYERS);
    asn_write_int32(asn_dhcp_csap, mode, "mode");
    asn_write_value_field(asn_dhcp_csap, iface, strlen(iface) + 1, "iface");

    if ((rc = asn_write_component_value(csap_layer_spec, asn_dhcp_csap,
                                        "#dhcp6"))  == 0)
    {
        rc = asn_insert_indexed(csap_layers, csap_layer_spec, -1, "");
    }

    if (rc == 0)
    {
        rc = tapi_tad_csap_create(ta_name, 0, "dhcp6",
                                  csap_spec, dhcp_csap);
    }

    asn_free_value(csap_spec);
    asn_free_value(asn_dhcp_csap);

    return rc;
}

/**
 * Creates traffic template of one DHCPv6 message
 *
 * @param dhcp6_msg DHCPv6 message in ASN representation
 * @param templ_p   Traffic template
 *
 * @return status code
 */
int
dhcpv6_prepare_traffic_template(const asn_value *dhcp6_msg,
                                asn_value **templ_p)
{
    asn_value  *asn_pdus;
    asn_value  *asn_pdu;
    int         rc;

    assert(dhcp6_msg != NULL);

    *templ_p = asn_init_value(ndn_traffic_template);
    asn_pdus = asn_init_value(ndn_generic_pdu_sequence);
    asn_pdu = asn_init_value(ndn_generic_pdu);

    rc = asn_write_component_value(asn_pdu, dhcp6_msg, "#dhcp6");
    if (rc == 0)
        rc = asn_insert_indexed(asn_pdus, asn_pdu, -1, "");
    if (rc == 0)
        rc = asn_write_component_value(*templ_p, asn_pdus, "pdus");

    return TE_RC(TE_TAPI, rc);
}

/**
 * Creates traffic pattern of one DHCPv6 message
 *
 * @param dhcp6_msg      DHCPv6 message in ASN representation
 * @param pattern_p      Traffic pattern (OUT)
 *
 * @return status code
 */
int
dhcpv6_prepare_traffic_pattern(const asn_value *dhcp6_msg,
                               asn_value **pattern_p)
{
    asn_value  *asn_pattern_unit;
    asn_value  *asn_pdus;
    asn_value  *asn_pdu;
    int         rc;

    assert(dhcp6_msg != NULL);

    *pattern_p = asn_init_value(ndn_traffic_pattern);
    asn_pattern_unit = asn_init_value(ndn_traffic_pattern_unit);
    asn_pdus = asn_init_value(ndn_generic_pdu_sequence);
    asn_pdu = asn_init_value(ndn_generic_pdu);

    rc = asn_write_component_value(asn_pdu, dhcp6_msg, "#dhcp6");
    if (rc == 0)
        rc = asn_insert_indexed(asn_pdus, asn_pdu, -1, "");
    if (rc == 0)
        rc = asn_write_component_value(asn_pattern_unit, asn_pdus, "pdus");
    if (rc == 0)
        rc = asn_insert_indexed(*pattern_p, asn_pattern_unit, -1, "");

    return TE_RC(TE_TAPI, rc);
}
