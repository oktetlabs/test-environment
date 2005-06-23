/** @file
 * @brief Test API for RADIUS Server Configuration and RADIUS CSAP 
 *
 * Implementation.
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
 *
 * $Id$
 */

#include "tapi_radius.h"
#include <string.h>
#include <net/ethernet.h>
#include "tapi_udp.h"
#include "logger_api.h"
#include "te_errno.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "asn_usr.h"
#include "ndn.h"
#include "rcf_api.h"

/* Attribute dictionary length */
#define TAPI_RADIUS_DICT_LEN     256

/* Index for undefined entries */
#define TAPI_RADIUS_ATTR_UNKNOWN 255

static uint8_t tapi_radius_dict_index[TAPI_RADIUS_DICT_LEN];

/* RADIUS attributes dictionary */
static const tapi_radius_attr_info_t tapi_radius_dict[] =
{
    { 1, "User-Name",     TAPI_RADIUS_TYPE_STRING },
    { 2, "User-Password", TAPI_RADIUS_TYPE_STRING },
};

void
tapi_radius_dict_init()
{
    size_t i;

    for (i = 0; i < TAPI_RADIUS_DICT_LEN; i++)
    {
        tapi_radius_dict_index[i] = TAPI_RADIUS_ATTR_UNKNOWN;
    }
    for (i = 0; i < sizeof(tapi_radius_dict) / sizeof(tapi_radius_dict[0]);
         i++)
    {
        uint8_t id = tapi_radius_dict[i].id;

        if (tapi_radius_dict_index[id] != TAPI_RADIUS_ATTR_UNKNOWN)
        {
            WARN("%s: duplicate entry %u in RADIUS attribute dictionary",
                 __FUNCTION__, id);
        }
        tapi_radius_dict_index[id] = i;
    }
}

/**
 * Lookup RADIUS attribute dictionary entry by the attribute type
 */
const tapi_radius_attr_info_t *
tapi_radius_dict_lookup(uint8_t type)
{
    uint8_t index = tapi_radius_dict_index[type];

    if (index != TAPI_RADIUS_ATTR_UNKNOWN)
        return &tapi_radius_dict[index];

    return NULL;
}

const tapi_radius_attr_info_t *
tapi_radius_dict_lookup_by_name(const char *name)
{
    size_t i;

    for (i = 0; i < sizeof(tapi_radius_dict) / sizeof(tapi_radius_dict[0]);
         i++)
    {
        if (strcmp(name, tapi_radius_dict[i].name) == 0)
            return &tapi_radius_dict[i];
    }
    return NULL;
}

int
tapi_radius_attr_list_push(tapi_radius_attr_list_t *list,
                           const tapi_radius_attr_t *attr)
{
    tapi_radius_attr_t *p;

    p = realloc(list->attr, list->len + 1);
    if (p == NULL)
    {
        ERROR("%s: failed to allocate memory for attribute", __FUNCTION__);
        return ENOMEM;
    }
    memcpy(&p[list->len], attr, sizeof(p[list->len]));
    list->attr = p;
    list->len++;

    return 0;
}

const tapi_radius_attr_t *
tapi_radius_attr_list_find(const tapi_radius_attr_list_t *list,
                           uint8_t type)
{
    size_t i;

    for (i = 0; i < list->len; i++)
        if (list->attr[i].type == type)
            return &list->attr[i];

    return NULL;
}

void
tapi_radius_attr_list_init(tapi_radius_attr_list_t *list)
{
    list->len = 0;
    list->attr = NULL;
}

void
tapi_radius_attr_list_free(tapi_radius_attr_list_t *list)
{
    size_t  i;

    for (i = 0; i < list->len; i++)
    {
        const tapi_radius_attr_info_t *attr_info;

        attr_info = tapi_radius_dict_lookup(list->attr[i].type);
        if (attr_info != NULL &&
            (attr_info->type == TAPI_RADIUS_TYPE_TEXT ||
             attr_info->type == TAPI_RADIUS_TYPE_STRING))
        {
                free(list->attr[i].string);
        }
    }
    free(list->attr);
    list->attr = NULL;
}

int
tapi_radius_parse_packet(const uint8_t *data, size_t data_len,
                         tapi_radius_packet_t *packet)
{
    const uint8_t                 *p = data;
    uint16_t                       radius_len;
    tapi_radius_attr_t             attr;
    const tapi_radius_attr_info_t *attr_info;

    if (data == NULL)
        return EINVAL;

    if (data_len < TAPI_RADIUS_PACKET_MIN_LEN)
    {
        ERROR("%s: data length is too small, %u bytes",
              __FUNCTION__, data_len);
        return EINVAL;
    }

    packet->code = *p++;
    packet->identifier = *p++;
    memcpy(&radius_len, p, sizeof(radius_len));
    radius_len = ntohs(radius_len);
    if (radius_len > data_len)
    {
        ERROR("%s: buffer size (%u) is smaller than RADIUS packet length (%u)",
              __FUNCTION__, data_len, radius_len);
        return EINVAL;
    }
    if (radius_len < TAPI_RADIUS_PACKET_MIN_LEN ||
        radius_len > TAPI_RADIUS_PACKET_MAX_LEN)
    {
        ERROR("%s: RADIUS packet with invalid length %d",
              __FUNCTION__, radius_len);
        return EINVAL;
    }
    memcpy(packet->authenticator, p, sizeof(packet->authenticator));
    p += sizeof(packet->authenticator);

    /* Attributes */
    tapi_radius_attr_list_init(&packet->attrs);
    while ((p - data) + TAPI_RADIUS_ATTR_MIN_LEN <= radius_len)
    {
        uint32_t value;

        attr.type = *p++;
        attr.len = (*p++) - 2;
        if ((p - data) + attr.len > radius_len)
        {
            ERROR("%s: invalid RADIUS packet - attribute %u value is out "
                  "of packet data", __FUNCTION__, attr.type);
            return EINVAL;
        }

        attr_info = tapi_radius_dict_lookup(attr.type);
        if (attr_info == NULL)
        {
            ERROR("%s: unknown attribute %u", __FUNCTION__, attr.type);
            return EINVAL;
        }
        switch (attr_info->type)
        {
            case TAPI_RADIUS_TYPE_INTEGER:
            case TAPI_RADIUS_TYPE_ADDRESS:
            case TAPI_RADIUS_TYPE_TIME:
                if (attr.len != sizeof(value))
                {
                    ERROR("%s: invalid length of attribute %u",
                          __FUNCTION__, attr.type);
                    break;
                }
                memcpy(&value, p, sizeof(value));
                attr.integer = ntohl(value);
                break;

            case TAPI_RADIUS_TYPE_TEXT:
            case TAPI_RADIUS_TYPE_STRING:
                attr.string = calloc(attr.len + 1, sizeof(char));
                if (attr.string == NULL)
                {
                    ERROR("%s: failed to allocate memory for attribute %u",
                          __FUNCTION__, attr.type);
                    return ENOMEM;
                }
                memcpy(attr.string, p, attr.len);
                break;

            default:
                ERROR("%s: unknown type %u for attribute %u", __FUNCTION__,
                      attr_info->type, attr.type);
                return EINVAL;
        }
        p += attr.len;
        tapi_radius_attr_list_push(&packet->attrs, &attr);
    }
    return 0;
}


int
tapi_radius_csap_create(const char *ta, int sid, const char *device,
                        const uint8_t *net_addr, uint16_t port,
                        csap_handle_t *csap)
{
    int        rc;
    int        num = 0;
    char       csap_fname[] = "/tmp/te_radius_csap.XXXXXX";
    asn_value *csap_spec;

    mkstemp(csap_fname);
    rc = asn_parse_value_text("{ udp:{}, ip4:{}, eth:{} }",
                              ndn_csap_spec, &csap_spec, &num);
    if (rc != 0)
        return rc;

    do {
        if (device != NULL)
        {
            rc = asn_write_value_field(csap_spec, device, strlen(device),
                                       "2.#eth.device-id.#plain");
            if (rc != 0) break;
        }
        if (net_addr != NULL)
        {
            rc = asn_write_value_field(csap_spec, net_addr, 4,
                                       "1.#ip4.local-addr.#plain");
            if (rc != 0) break;
        }
        rc = asn_write_value_field(csap_spec, &port, sizeof(port),
                                   "0.#udp.local-port.#plain");
        if (rc != 0) break;

        rc = asn_save_to_file(csap_spec, csap_fname);
        if (rc != 0) break;

        rc = rcf_ta_csap_create(ta, sid, "udp.ip4.eth", csap_fname, csap);
        unlink(csap_fname);
    } while (0);

    asn_free_value(csap_spec);

    return rc;
}

typedef struct {
    radius_callback  callback;
    void            *userdata;
} radius_cb_data_t;


void
tapi_radius_callback(const udp4_datagram *pkt, void *userdata)
{
    tapi_radius_packet_t  packet;
    radius_cb_data_t     *cb_data = (radius_cb_data_t *)userdata;
    int                   rc;

    rc = tapi_radius_parse_packet(pkt->payload, pkt->payload_len, &packet);
    if (rc != 0)
    {
        ERROR("%s: failed to parse UDP packet to %s:%u",
              __FUNCTION__, inet_ntoa(pkt->dst_addr), pkt->dst_port);
        return;
    }
    cb_data->callback(&packet, cb_data->userdata);
    tapi_radius_attr_list_free(&packet.attrs);
}

int
tapi_radius_recv_start(const char *ta, int sid, csap_handle_t csap,
                       radius_callback user_callback, void *user_data,
                       unsigned int timeout)
{
    radius_cb_data_t  *cb_data;

    cb_data = (radius_cb_data_t *)malloc(sizeof(radius_cb_data_t));
    if (cb_data == NULL)
    {
        ERROR("%s: failed to allocate memory", __FUNCTION__);
        return ENOMEM;
    }
    cb_data->callback = user_callback;
    cb_data->userdata = user_data;
/*    rc = tapi_tad_trrecv_start(ta, sid, csap, NULL, tapi_radius_callback,
                               cb_data, timeout, 0);*/
    return tapi_udp4_dgram_start_recv(ta, sid, csap, NULL,
                                    tapi_radius_callback, cb_data);
}
