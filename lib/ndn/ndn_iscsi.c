/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Proteos, TAD ISCSI protocol, NDN.
 *
 * Definitions of ASN.1 types for NDN for ISCSI protocol.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */


#define TE_LGR_USER "NDN iSCSI"

#include "te_config.h"

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

/* for ntohl */
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "asn_impl.h"
#include "tad_common.h"
#include "ndn_internal.h"
#include "ndn_iscsi.h"
#include "te_errno.h"
#include "logger_api.h"


/** Length of iSCSI digests indexed by iscsi_digest_type */
static size_t iscsi_digest_length[] = { 0, 1 };


static asn_enum_entry_t _ndn_iscsi_digest_enum_entries[] = {
    { "none",   ISCSI_DIGEST_NONE },
    { "crc32c", ISCSI_DIGEST_CRC32C },
};

static asn_type ndn_iscsi_digest_s = {
    "iSCSI-digest",
    { PRIVATE, 1 /* FIXME */ },
    ENUMERATED,
    TE_ARRAY_LEN(_ndn_iscsi_digest_enum_entries),
    { enum_entries: _ndn_iscsi_digest_enum_entries }
};

/* ISCSI-CSAP definitions */
static asn_named_entry_t _ndn_iscsi_csap_ne_array[] = {
    { "socket",        &asn_base_int16_s,
      { PRIVATE, NDN_TAG_ISCSI_SOCKET } },
    { "header-digest", &ndn_iscsi_digest_s,
      { PRIVATE, NDN_TAG_ISCSI_HEADER_DIGEST } },
    { "data-digest",   &ndn_iscsi_digest_s,
      { PRIVATE, NDN_TAG_ISCSI_DATA_DIGEST } },
};

asn_type ndn_iscsi_csap_s = {
    "iSCSI-CSAP", { PRIVATE, TE_PROTO_ISCSI }, SEQUENCE,
    TE_ARRAY_LEN(_ndn_iscsi_csap_ne_array),
    { _ndn_iscsi_csap_ne_array }
};

const asn_type *ndn_iscsi_csap = &ndn_iscsi_csap_s;

/* ISSI Segment Data definitions */

/*
Key-Values ::= SEQUENCE OF 'charstring'
*/

asn_type ndn_iscsi_key_values_s = {
    "Key-Values", {PRIVATE, NDN_TAG_ISCSI_SD_KEY_VALUES}, SEQUENCE_OF,
    0, {subtype: &asn_base_charstring_s}
};

const asn_type * const ndn_iscsi_key_values = &ndn_iscsi_key_values_s;

/*
Key-Pair ::= SEQUENCE {
    key UniversalString,
    values Key-Values,
}
*/
static asn_named_entry_t _ndn_iscsi_segment_data_ne_array [] = {
    { "key", &asn_base_charstring_s,     {PRIVATE, NDN_TAG_ISCSI_SD_KEY} },
    { "values", &ndn_iscsi_key_values_s,
        {PRIVATE, NDN_TAG_ISCSI_SD_VALUES} },
};

asn_type ndn_iscsi_key_pair_s = {
    "Key-Pair", {PRIVATE, NDN_TAG_ISCSI_SD_KEY_PAIR}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_iscsi_segment_data_ne_array),
    {_ndn_iscsi_segment_data_ne_array}
};

const asn_type * const ndn_iscsi_key_pair = &ndn_iscsi_key_pair_s;

/*
ISCSI-Segment-Data ::= SEQUENCE OF Key-Pair;
*/

asn_type ndn_iscsi_segment_data_s = {
    "ISCSI-Segment-Data",
    {PRIVATE, NDN_TAG_ISCSI_SD_SEGMENT_DATA}, SEQUENCE_OF,
    0,
    {subtype: &ndn_iscsi_key_pair_s}
};

const asn_type * const ndn_iscsi_segment_data = &ndn_iscsi_segment_data_s;

/* ISCSI-Message definitions */
static asn_named_entry_t _ndn_iscsi_message_ne_array [] = {
    { "i-bit",   &ndn_data_unit_int1_s, {PRIVATE, NDN_TAG_ISCSI_I_BIT} },
    { "opcode",  &ndn_data_unit_int6_s, {PRIVATE, NDN_TAG_ISCSI_OPCODE} },
    { "f-bit",   &ndn_data_unit_int1_s, {PRIVATE, NDN_TAG_ISCSI_F_BIT} },
    { "op-specific",
        &ndn_data_unit_int24_s, {PRIVATE, NDN_TAG_ISCSI_OP_SPECIFIC} },
    { "ahs-len", &ndn_data_unit_int8_s, {PRIVATE, NDN_TAG_ISCSI_AHS_LEN} },
    { "ds-len",  &ndn_data_unit_int24_s,{PRIVATE, NDN_TAG_ISCSI_DS_LEN} },
    { "length",  &asn_base_integer_s,   {PRIVATE, NDN_TAG_ISCSI_LEN} },
#if 0
    { "have-hdig",&asn_base_null_s,     {PRIVATE, NDN_TAG_ISCSI_HAVE_HDIG}},
    { "have-ddig",&asn_base_null_s,     {PRIVATE, NDN_TAG_ISCSI_HAVE_DDIG}},
#endif
    { "segment-data", &ndn_iscsi_segment_data_s,
        {PRIVATE, NDN_TAG_ISCSI_SD} },
    { "last-data",  &asn_base_null_s, {PRIVATE, NDN_TAG_ISCSI_LAST} },
};

asn_type ndn_iscsi_message_s = {
    "ISCSI-Message", {PRIVATE, TE_PROTO_ISCSI}, SEQUENCE,
    TE_ARRAY_LEN(_ndn_iscsi_message_ne_array),
    {_ndn_iscsi_message_ne_array}
};

const asn_type *ndn_iscsi_message = &ndn_iscsi_message_s;

/* Buffer for logging asn value */
#define ASN_VAL_BUF_LEN  2048
static char asn_val_buf[ASN_VAL_BUF_LEN];

/* See description in ndn_iscsi.h */
int
asn2bin_data(asn_value *segment_data, uint8_t *data, uint32_t *data_len)
{
    int rc;

    char *current = (char *)data;

    int segment_data_index;
    int segment_data_len;

    int key_values_index;
    int key_values_len;

    char      *key;
    char      *str_value;

    uint32_t   write_data_len = 0;

    asn_value *key_pair;
    asn_value *key_values;
    asn_value *key_value;

    memset(asn_val_buf, 0, ASN_VAL_BUF_LEN);
    if (asn_sprint_value(segment_data, asn_val_buf,
                               ASN_VAL_BUF_LEN, 0) < 0)
    {
        ERROR("%s, %d: Cannot printf asn value",
              __FUNCTION__, __LINE__);
    }

    INFO("asn2bin_data result %s", asn_val_buf);

    memset(data, 0, *data_len);

    segment_data_len = asn_get_length(segment_data, "");
    segment_data_index = 0;
#define tail_len (*data_len - write_data_len)
#define MAX_INT_VALUE_LEN           10
    while (segment_data_index < segment_data_len)
    {
        size_t key_len;

        if ((rc = asn_get_indexed(segment_data, &key_pair,
                                  segment_data_index++, NULL)) != 0)
        {
            ERROR("%s, %d: cannot get segment data length, %r",
                  __FUNCTION__, __LINE__, rc);
            return rc;
        }
        if ((rc =asn_read_string(key_pair, &key, "key")) != 0)
        {
            ERROR("%s, %d: cannot read string, %r",
                  __FUNCTION__, __LINE__, rc);
            return rc;
        }

        key_len = strlen(key);
        if (tail_len <= key_len)
        {
            ERROR("%s, %d: unsufficient buffer length",
                  __FUNCTION__, __LINE__);
            return TE_ENOBUFS;
        }
        /*
         * We checked above that we have sufficient space and use plain
         * memcpy() here to copy together with terminating NUL.
         */
        memcpy(current, key, key_len + 1);
        current += key_len;
        write_data_len += key_len;

        if (tail_len < 1)
        {
            ERROR("%s, %d: unsufficient buffer length",
                  __FUNCTION__, __LINE__);
            return TE_ENOBUFS;
        }
        *current = '=';
        current += 1;
        write_data_len += 1;

        if ((rc = asn_get_child_value(key_pair,
                                      (const asn_value **)&key_values,
                                      PRIVATE,
                                      NDN_TAG_ISCSI_SD_VALUES)) != 0)
        {
            ERROR("%s, %d: cannot get child value, %r",
                  __FUNCTION__, __LINE__, rc);
            return rc;
        }
        if ((key_values_len = asn_get_length(key_values, "")) == -1)
        {
            ERROR("%s, %d: cannot get key values length",
                  __FUNCTION__, __LINE__);
            return TE_EASNTXTPARSE;
        }

        key_values_index = 0;

        while (key_values_index < key_values_len)
        {
            if ((rc = asn_get_indexed(key_values, &key_value,
                                      key_values_index++, NULL)) != 0)
            {
                ERROR("%s, %d: cannot get key_value, %r",
                      __FUNCTION__, __LINE__, rc);
                return rc;
            }

            if ((rc = asn_read_string(key_value,
                                      &str_value,
                                      "")) != 0)
            {
                ERROR("%s, %d: cannot read string value, %r",
                      __FUNCTION__, __LINE__, rc);
                return rc;
            }

            if (tail_len < strlen(str_value))
            {
                ERROR("%s, %d: unsufficient "
                      "buffer length",
                      __FUNCTION__, __LINE__);
                return TE_ENOBUFS;
            }
            sprintf(current, "%s", str_value);
            write_data_len += strlen(str_value);
            current += strlen(str_value);

            if (tail_len < 1)
            {
                ERROR("%s, %d: unsufficient buffer length",
                      __FUNCTION__, __LINE__);
                return TE_ENOBUFS;
            }
            *current = ',';
            current += 1;
            write_data_len += 1;
        }
        current -= 1;
        *current = '\0';
        current += 1;
    }

    if ((write_data_len & 0x3) != 0)
    {
        unsigned int pad_size = 4 - (write_data_len & 0x3);

        if (pad_size > tail_len)
            pad_size = tail_len;

        memset(current, 0, pad_size);
    }

    *data_len = write_data_len;
#undef MAX_INT_VALUE_LEN
#undef tail_len

    return 0;
}

/* See description in ndn_iscsi.h */
int
bin_data2asn(uint8_t *data, uint32_t data_len, asn_value **value)
{
    int rc;

    char *current         = NULL;
    char *eq_delimiter    = NULL;
    char *zero_delimiter  = NULL;
    char *comma_delimiter = NULL;

    asn_value *segment_data;
    asn_value *key_values;
    asn_value *key_value;

    int segment_data_index;
    int key_values_index;

    char *packet = NULL;

    uint32_t   parsed_len = 0;

    asn_value *key_pair;

    packet = calloc(1, data_len);
    if (packet == NULL)
    {
        ERROR("%s, %d: unsufficient memory",
              __FUNCTION__, __LINE__);
        return TE_ENOMEM;
    }
    memcpy(packet, (char *)data, data_len);

    if ((segment_data =
         asn_init_value(ndn_iscsi_segment_data)) == NULL)
    {
        ERROR("%s, %d: cannot init asn_value",
              __FUNCTION__, __LINE__);
        return TE_ENOMEM;
    }

    current = packet;
    segment_data_index = 0;

    while (parsed_len < data_len)
    {
        zero_delimiter = strchr(current, '\0');
        if (zero_delimiter == NULL)
        {
            ERROR("%s, %d: cannot find delimiter 0",
                  __FUNCTION__, __LINE__);
            return TE_EFMT;
        }

        parsed_len += zero_delimiter - current + 1;

        eq_delimiter = strchr(current, '=');
        if (eq_delimiter == NULL)
        {
            goto padding;
        }
        *eq_delimiter = '\0';

        if ((key_pair = asn_init_value(ndn_iscsi_key_pair)) == NULL)
        {
            ERROR("%s, %d: cannot init asn_value",
                  __FUNCTION__, __LINE__);
            return TE_ENOMEM;
        }

        if ((rc = asn_write_string(key_pair, current, "key")) != 0)
        {
            ERROR("%s, %d: cannot write string, %r",
                  __FUNCTION__, __LINE__, rc);
            return rc;
        }

        if ((key_values = asn_init_value(ndn_iscsi_key_values)) == NULL)
        {
            ERROR("%s, %d: cannot init asn_value",
                  __FUNCTION__, __LINE__);
            return TE_ENOMEM;
        }
        current = eq_delimiter + 1;

        key_values_index = 0;
        if ((key_value = asn_init_value(asn_base_charstring)) == NULL)
        {
            ERROR("%s, %d: cannot init asn_value",
                  __FUNCTION__, __LINE__);
            return TE_ENOMEM;
        }

        while (comma_delimiter != zero_delimiter)
        {
            comma_delimiter = strchr(current, ',');
            if (comma_delimiter == NULL)
                comma_delimiter = zero_delimiter;
            else
                *comma_delimiter = '\0';

            if ((rc = asn_write_string(key_value,
                                       current,
                                       "")) != 0)
            {
                ERROR("%s, %d: cannot write string, %r",
                      __FUNCTION__, __LINE__, rc);
                return rc;
            }

            if ((rc = asn_insert_indexed(key_values,
                                         asn_copy_value(key_value),
                                         key_values_index++, "")) != 0)
            {
                ERROR("%s, %d: cannot insert indexed, %r",
                      __FUNCTION__, __LINE__, rc);
                return rc;
            }
            current = comma_delimiter + 1;
        }
        asn_free_value(key_value);

        if ((rc = asn_put_child_value_by_label(key_pair,
                                               key_values,
                                               "values")) != 0)
        {
            ERROR("%s, %d: cannot put child value, %r",
                  __FUNCTION__, __LINE__, rc);
            return rc;
        }
        if ((rc = asn_insert_indexed(segment_data, key_pair,
                                     segment_data_index++, "")) != 0)
        {
            ERROR("%s, %d: cannot insert indexed, %r",
                  __FUNCTION__, __LINE__, rc);
            return rc;
        }
    }

padding:
    while (parsed_len < data_len)
    {
       if (*current != '\0')
       {
           ERROR("%s, %d: padding is not zeroed",
                 __FUNCTION__, __LINE__);
           return TE_EFMT;
       }
       current++;
       parsed_len++;
    }
    if (parsed_len > data_len)
    {
        ERROR("%s, %d: Parse error: parsed length is bigger than given",
              __FUNCTION__, __LINE__);
        return TE_EFMT;
    }
    if (parsed_len > data_len)
    {
        ERROR("%s, %d: Parse error: parsed length is bigger than given",
              __FUNCTION__, __LINE__);
        return TE_EFMT;
    }
    *value = segment_data;
    free(packet);
    memset(asn_val_buf, 0, ASN_VAL_BUF_LEN);
    if (asn_sprint_value(segment_data, asn_val_buf,
                               ASN_VAL_BUF_LEN, 0) < 0)
    {
        ERROR("%s, %d: Cannot printf asn value",
              __FUNCTION__, __LINE__);
    }

    INFO("bin_data2asn result %s", asn_val_buf);
    return 0;
}


/* See description in ndn_iscsi.h */
size_t
iscsi_rest_data_len(uint8_t           *bhs,
                    iscsi_digest_type  header_digest,
                    iscsi_digest_type  data_digest)
{
    /* Lengths of header and data digests in 4-byte units */
    size_t      h_dig_len = iscsi_digest_length[header_digest];
    size_t      d_dig_len = iscsi_digest_length[data_digest];

    size_t      total_ahs_len;
    uint32_t    data_segment_len;

    union {
        uint32_t    i;
        uint8_t     b[4];
    } dsl_convert;

    /*
     * It is assumed here that digests do not appear in
     * Login Request/Response commands.
     */
    if ((bhs[0] & 0x1f) == 0x03)
        h_dig_len = d_dig_len = 0;

    total_ahs_len = bhs[4];

    dsl_convert.b[0] = 0;
    dsl_convert.b[1] = bhs[5];
    dsl_convert.b[2] = bhs[6];
    dsl_convert.b[3] = bhs[7];

    data_segment_len = dsl_convert.i;

    /* Zero is the same in host and network byte orders */
    if (data_segment_len == 0)
    {
        /*
         * RFC 3720 10.2.3.
         * A zero-length Data Segment also implies a zero-length
         * data-digest.
         */
        d_dig_len = 0;
    }
    else
    {
        data_segment_len = ntohl(data_segment_len);

        /* DataSegment length in 4-byte units after padding */
        data_segment_len = (data_segment_len + 0x3) >> 2;
    }

    return (total_ahs_len + h_dig_len + data_segment_len + d_dig_len) << 2;
}
