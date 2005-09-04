/** @file
 * @brief Proteos, TAD ISCSI protocol, NDN.
 *
 * Definitions of ASN.1 types for NDN for ISCSI protocol. 
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */


#define TE_LGR_USER "NDN iSCSI"
#include <ctype.h>
#include "te_config.h" 

#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_iscsi.h"
#include "te_errno.h"
#include "logger_ta.h"


/* ISCSI-CSAP definitions */
static asn_named_entry_t _ndn_iscsi_csap_ne_array [] = 
{
    { "socket", &asn_base_int16_s, {PRIVATE, NDN_TAG_ISCSI_SOCKET} },
};

asn_type ndn_iscsi_csap_s =
{
    "ISCSI-CSAP", {PRIVATE, NDN_TAD_ISCSI}, SEQUENCE, 
    sizeof(_ndn_iscsi_csap_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_iscsi_csap_ne_array}
};

const asn_type *ndn_iscsi_csap = &ndn_iscsi_csap_s;

/* ISSI Segment Data definitions */

/*
Key-Value ::= CHOICE {
    int  INTEGER,
    hex  INTEGER, 
    str  UniversalString,
};
*/

static asn_named_entry_t _ndn_iscsi_key_value_ne_array [] =
{
    { "int", &asn_base_integer_s, {PRIVATE, NDN_TAG_ISCSI_SD_INT_VALUE} },
    { "hex", &asn_base_integer_s, {PRIVATE, NDN_TAG_ISCSI_SD_HEX_VALUE} },
    { "str", &asn_base_charstring_s, 
        {PRIVATE, NDN_TAG_ISCSI_SD_STR_VALUE} },
};

asn_type ndn_iscsi_key_value_s =
{
    "Key-Value", {PRIVATE, NDN_TAG_ISCSI_SD_KEY_VALUE}, CHOICE,
    sizeof(_ndn_iscsi_key_value_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_iscsi_key_value_ne_array}
};

const asn_type * const ndn_iscsi_key_value = &ndn_iscsi_key_value_s;
/*
Key-Values ::= SEQUENCE OF Key-Value;
*/

asn_type ndn_iscsi_key_values_s =
{
    "Key-Values", {PRIVATE, NDN_TAG_ISCSI_SD_KEY_VALUES}, SEQUENCE_OF,
    0, {subtype: &ndn_iscsi_key_value_s}
};

const asn_type * const ndn_iscsi_key_values = &ndn_iscsi_key_values_s;

/*
Key-Pair ::= SEQUENCE {
    key UniversalString, 
    values Key-Values,
}
*/
static asn_named_entry_t _ndn_iscsi_segment_data_ne_array [] =
{
    { "key", &asn_base_charstring_s,     {PRIVATE, NDN_TAG_ISCSI_SD_KEY} },
    { "values", &ndn_iscsi_key_values_s, 
        {PRIVATE, NDN_TAG_ISCSI_SD_VALUES} },    
};

asn_type ndn_iscsi_key_pair_s =
{
    "Key-Pair", {PRIVATE, NDN_TAG_ISCSI_SD_KEY_PAIR}, SEQUENCE,
    sizeof(_ndn_iscsi_segment_data_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_iscsi_segment_data_ne_array}
};

const asn_type * const ndn_iscsi_key_pair = &ndn_iscsi_key_pair_s;

/*
ISCSI-Segment-Data ::= SEQUENCE OF Key-Pair;
*/

asn_type ndn_iscsi_segment_data_s =
{
    "ISCSI-Segment-Data", 
    {PRIVATE, NDN_TAG_ISCSI_SD_SEGMENT_DATA}, SEQUENCE_OF,
    0,
    {subtype: &ndn_iscsi_key_pair_s}
};

const asn_type * const ndn_iscsi_segment_data = &ndn_iscsi_segment_data_s;

/* ISCSI-Message definitions */
static asn_named_entry_t _ndn_iscsi_message_ne_array [] = 
{
    { "param",   &asn_base_integer_s, {PRIVATE, NDN_TAG_ISCSI_PARAM} },
    { "segment-data", &ndn_iscsi_segment_data_s, 
        {PRIVATE, NDN_TAG_ISCSI_SD} },
};

asn_type ndn_iscsi_message_s =
{
    "ISCSI-Message", {PRIVATE, NDN_TAD_ISCSI}, SEQUENCE,
    sizeof(_ndn_iscsi_message_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_iscsi_message_ne_array}
};

const asn_type *ndn_iscsi_message = &ndn_iscsi_message_s;

#if 1

#define MAX_INT_VALUE_LEN   10

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
    uint32_t   int_value;

    uint32_t   write_data_len = 0;
   
    asn_value *key_pair;
    asn_value *key_values;
    asn_value *key_value;

    asn_value    *value;
    asn_tag_class tag_class;
    uint16_t      tag;
    
    memset(data, 0, *data_len);    
    segment_data_len = asn_get_length(segment_data, "");
    segment_data_index = 0;
#define tail_len (*data_len - write_data_len)    
    while (segment_data_index < segment_data_len)
    {
        if ((rc = asn_get_indexed(segment_data, 
                                  (const asn_value **)&key_pair, 
                                  segment_data_index++)) != 0)
        {
            ERROR("%s, %d: cannot get segment data length", 
                  __FUNCTION__, __LINE__);
            return rc;
        }
        if ((rc =asn_read_string(key_pair, &key, "key")) != 0)
        {
            ERROR("%s, %d: cannot read string", 
                  __FUNCTION__, __LINE__);
            return rc;
        }
        
        if (tail_len < strlen(key))
        {
            ERROR("%s, %d: unsufficient buffer length",
                  __FUNCTION__, __LINE__);
            return TE_ENOBUFS;
        }
        strncpy(current, key, strlen(key));
        current += strlen(key);
        write_data_len += strlen(key);
        
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
            ERROR("%s, %d: cannot get child value",
                  __FUNCTION__, __LINE__);
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
            if ((rc = asn_get_indexed(key_values, 
                                      (const asn_value **)&key_value, 
                                      key_values_index++)) != 0)
            {
                ERROR("%s, %d: cannot get key_value",
                      __FUNCTION__, __LINE__);
                return rc;
            }
            if ((rc = asn_get_choice_value(key_value, 
                                           (const asn_value **)&value, 
                                           &tag_class, &tag)) != 0)
            {
                ERROR("%s, %d: cannot get choice value",
                      __FUNCTION__, __LINE__);
                return rc;
            }
            switch (tag)
            {
                case NDN_TAG_ISCSI_SD_INT_VALUE:
                {
                    if ((rc = asn_read_int32(value, 
                                             &int_value, 
                                             "")) != 0)
                    {
                        ERROR("%s, %d: cannot read int value",
                              __FUNCTION__, __LINE__);
                        return rc;
                    }
                    if (tail_len < MAX_INT_VALUE_LEN)
                    {
                        ERROR("%s, %d: unsufficient buffer length",
                              __FUNCTION__, __LINE__);
                        return TE_ENOBUFS;
                    }
                    sprintf(current, "%d", int_value);
                    write_data_len += strlen(current);
                    current += strlen(current);
                    break;
                }
                case NDN_TAG_ISCSI_SD_HEX_VALUE:
                {
                    if ((rc = asn_read_int32(value, 
                                             &int_value, 
                                             "")) != 0)
                    {
                        ERROR("%s, %d: cannot read int value",
                              __FUNCTION__, __LINE__);
                        return rc;
                    }
                    sprintf(current, "%x", int_value);
                    write_data_len += strlen(current);
                    current += strlen(current);
                    break;
                }
                case NDN_TAG_ISCSI_SD_STR_VALUE:
                {
                    if ((rc = asn_read_string(value, 
                                              &str_value, 
                                              "")) != 0)
                    {
                        ERROR("%s, %d: cannot read string value, %r",
                              __FUNCTION__, __LINE__, rc);
                        return rc;
                    }

                    ERROR("Here we are %s", str_value);
                    ERROR("key_value_index %d, key_values_len %d",
                          key_values_index, key_values_len);
                    sprintf(current, "%s", str_value);
#if 0                    
                    strncpy(current, str_value, strlen(str_value));
#endif                    
                    write_data_len += strlen(str_value);
                    current += strlen(str_value);
                    break;
                }
            }    
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
#undef tail_len    
    *data_len = write_data_len; 
    return 0;
}



static int
parse_key_value(char *str, asn_value *value)
{
    int rc;

    uint32_t int_value;

    ERROR("Parsing key value");
    
    /* String value */
    if (!isdigit(*str))
    {    
        ERROR("Key value is string %s", str);
        if ((rc = asn_write_string(value, 
                                   str, 
                                   "#str")) != 0)
        {
            ERROR("#s, %d: cannot write string",
                  __FUNCTION__, __LINE__);
            return rc;
        }
    }    
            
    else
    {
        ERROR("Key value is digit %s", str);
        if ((strlen(str)) > 3 && (strcmp(str, "0x") == 0))
        {    
            int_value = strtol(str, NULL, 16);
#if 0            
            if (int_value == LONG_MIN || int_value == LONG_MAX)
            {
                ERROR("%s, %d: strange integer in segment data",
                      __FUNCTION, __LINE__);
                return TE_EFMT;
            }
#endif            
            if ((rc = asn_write_int32(value, 
                                      int_value, "#hex")) != 0)
            {
                ERROR("%s, %d: cannot write integer",
                      __FUNCTION__, __LINE__);
                return rc;
            }
        }    
        else
        {    
            int_value = strtol(str, NULL, 0);
#if 0            
            if (int_value == LONG_MIN || int_value == LONG_MAX)
            {
                ERROR("%s, %d: strange integer in segment data",
                      __FUNCTION, __LINE__);
                return TE_EFMT;
            }
#endif            
            if ((rc = asn_write_int32(value, int_value, "#int")) != 0)
            {
                ERROR("%s, %d: cannot write integer",
                      __FUNCTION__, __LINE__);
                return rc;
            }
        }    
    }
    return 0;    
}

#define SEGMENT_DATA_MAX_LEN 2048

int
bin_data2asn(uint8_t *data, uint32_t data_len, asn_value_p *value)
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

    char packet[SEGMENT_DATA_MAX_LEN];

    uint32_t   parsed_len = 0;

    asn_value_p     key_pair;

    if (data_len > SEGMENT_DATA_MAX_LEN)
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
            ERROR("%s, %d: cannot write string",
                  __FUNCTION__, __LINE__);
            ERROR("key is %s", current);
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
        if ((key_value = asn_init_value(ndn_iscsi_key_value)) == NULL)
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

            if ((rc = parse_key_value(current, key_value)) != 0)
            {
                ERROR("%s, %d: cannot parse key value",
                      __FUNCTION__, __LINE__);
                return rc;
            }
            if ((rc = asn_insert_indexed(key_values, 
                                         key_value, 
                                         key_values_index++, "")) != 0)
            {
                ERROR("%s, %d: cannot insert indexed",
                      __FUNCTION__, __LINE__);
                return rc;
            }
            current = comma_delimiter + 1;
        }

        if ((rc = asn_put_child_value_by_label(key_pair, 
                                               key_values, 
                                               "values")) != 0)
        {
            ERROR("%s, %d: cannot put child value",
                  __FUNCTION__, __LINE__);
            return rc;
        }
        if ((rc = asn_insert_indexed(segment_data, 
                                     key_pair, 
                                     segment_data_index++, "")) != 0)
        {
            ERROR("%s, %d: cannot insert indexed",
                  __FUNCTION__, __LINE__);
            return rc;
        }
        asn_free_value(key_pair);
    }
    
padding:
    while (parsed_len < data_len)
    {
       if (*current != '\0')
       {
           ERROR("%s, %d: padding is not zeroed");
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
    return 0;
}
#endif

