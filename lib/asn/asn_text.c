/** @file
 * @brief ASN.1 library
 *
 * Implementation of method to convert to/from textual ASN value notation.
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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */ 


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "te_errno.h" 
#include "te_stdint.h" 
#include "te_defs.h" 

#include "asn_impl.h" 

#include "logger_api.h"

/*
 * Declaraions of local functions. 
 */ 
size_t number_of_digits(int value);

size_t asn_count_len_array_fields(const asn_value *value, 
                                  unsigned int indent);

te_errno asn_impl_pt_label(const char*text, char *label, int *parsed_syms);


te_errno asn_impl_pt_charstring(const char *text, const asn_type *type, 
                           asn_value **parsed, int *parsed_syms);

te_errno asn_impl_pt_bool(const char *text, const asn_type *type, 
                     asn_value **parsed, int *parsed_syms);

te_errno asn_impl_pt_integer(const char *text, const asn_type *type, 
                        asn_value **parsed, int *parsed_syms);

te_errno asn_impl_pt_null(const char*text, const asn_type *type, 
                     asn_value **parsed, int *parsed_syms);

te_errno asn_impl_pt_objid(const char*text, const asn_type *type,
                      asn_value **parsed, int *parsed_syms);


te_errno asn_impl_pt_octstring(const char *text, const asn_type *type, 
                          asn_value **parsed, int *parsed_syms);

te_errno asn_impl_pt_enum(const char*text, const asn_type *type, 
                     asn_value **parsed, int *parsed_syms);

te_errno asn_impl_pt_named_array(const char*text, const asn_type *type, 
                            asn_value **parsed, int *parsed_syms);

te_errno asn_impl_pt_indexed_array(const char*text, const asn_type *type, 
                              asn_value **parsed, int *parsed_syms);

te_errno asn_impl_pt_choice(const char *txt, const asn_type *type, 
                       asn_value **parsed, int *parsed_syms);

te_errno asn_parse_value_text(const char*text, const asn_type *type, 
                         asn_value **parsed, int *parsed_syms);


#define PARSE_BUF 0x1000

/**
 * Parse label in ASN.1 text, that is "valuereference" according to ASN.1 
 * specification terminology. 
 *
 * @param text          text to be parsed;
 * @param label         location for parsed label;
 * @param syms          size of 'label' buffer (IN), 
 *                      number of parsed symbols (OUT);
 *
 * @return zero on success, otherwise error code.
 */ 
te_errno 
asn_impl_pt_label(const char *text, char *label, int *syms)
{ 
    int         l = 0;
    const char *l_begin;
    const char *pt = text;

    ENTRY("text='%s' label=%p syms=%p", text, label, syms);

    while (isspace(*pt)) 
        pt++;

    l_begin = pt;
        
    /* first letter in 'valuereference' should be lower case character */
    if (!islower(*pt)) 
    {
        *syms = pt - text;
        EXIT("EASNTXTVALNAME because of '%d'", *pt);
        return TE_EASNTXTVALNAME; 
    }
    pt++, l++;

    while (isdigit(*pt) || isalpha(*pt) || (*pt == '-'))
        pt++, l++;

    if ((l + 1) > *syms) /* '+ 1' for tailing zero. */
    {
        EXIT("ESMALLBUF since %d > %d", l + 1, *syms);
        return TE_ESMALLBUF;
    }

    memcpy (label, l_begin, l);
    label[l] = 0;
    *syms = pt - text;

    EXIT("label=%s *syms=%d", label, *syms);

    return 0; 
}


#define TEXT_BLOCK 0x400
/**
 * Parse textual presentation of single ASN.1 value of UniversalString type,
 * create new instance of asn_value type with its internal presentation.
 *
 * @param text          text to be parsed;
 * @param type          ASN type of value to be parsed;
 * @param parsed        parsed ASN value (OUT);
 * @param syms_parsed   quantity of parsed symbols in 'text' (OUT);
 *
 * @return zero on success, otherwise error code.
 */ 
te_errno 
asn_impl_pt_charstring(const char *text, const asn_type *type, 
                       asn_value **parsed, int *syms_parsed)
{
    const char *pt = text; 

    size_t num_blocks = 1;
    char  *buffer;
    char  *pb;       
    te_errno  rc = 0;
    
    size_t l;
    size_t total = 0;

    if (!text || !parsed || !syms_parsed)
        return TE_EWRONGPTR; 

    pb = buffer = malloc(TEXT_BLOCK);       
    while (isspace(*pt))
        pt++;

    if (*pt != '"')
    {
        /* ERROR! there are no char string */
        return TE_EASNTXTNOTCHSTR;
    } 
    pt++; 

    while (*pt != '"' && *pt != '\0')
    { 
        l = strcspn(pt, "\\\""); /* find first \ or " */ 
        while (total + l > num_blocks * TEXT_BLOCK)
        {
            num_blocks++;
            buffer = realloc(buffer, num_blocks * TEXT_BLOCK);
            pb = buffer + total;
        }
        memcpy(pb, pt, l); 
        pt+= l, pb += l, total += l;

        if (*pt == '\\')
        {
            pt++;
            *pb = *pt;
            pb++, pt++, total++;
        }
    }
    
    if (*pt == '\0')
    {
        /* We reached the end of the string, but haven't found quote mark */
        return TE_EASNTXTPARSE;
    }

    *pb = '\0';

    *parsed = asn_init_value(type); 
    *syms_parsed = pt - text + 1; 

    rc = asn_write_value_field(*parsed, buffer, total, ""); 
    free(buffer);

    return rc;
}



/**
 * Parse textual presentation of single ASN.1 value of OCTET STRING type,
 * create new instance of asn_value type with its internal presentation.
 *
 * @param text          text to be parsed;
 * @param type          ASN type of value to be parsed;
 * @param parsed        parsed ASN value (OUT);
 * @param syms_parsed   quantity of parsed symbols in 'text' (OUT);
 *
 * @return zero on success, otherwise error code.
 */ 
te_errno 
asn_impl_pt_octstring(const char *text, const asn_type *type, 
                      asn_value **parsed, int *syms_parsed)
{
    const char *pt = text; 
    uint8_t    *buffer;
    
    char txt_buf[3]; 
    int  octstr_len = 0;
    int  b_num = 0;
    te_errno  rc;

    txt_buf[2] = 0;

    if (!text || !parsed || !syms_parsed)
        return TE_EWRONGPTR; 

    while (isspace(*pt)) 
        pt++;

    if (*pt != '\'')
    {
        /* ERROR! there are no OCTET string */
        *syms_parsed = pt - text;
        return TE_EASNTXTNOTOCTSTR;
    } 
    pt++; 


    if (type) 
        octstr_len = type->len;

    if (octstr_len == 0)
        octstr_len = (strchr(text + 1, '\'') - text + 1)/2; 

    buffer = malloc(octstr_len); 


    while (*pt != '\'') 
    {         
        int   byte;
        char *end_ptr;

        if (b_num == octstr_len)
        {
            *syms_parsed = pt - text;
            return TE_EASNGENERAL;
        }

        while (isspace(*pt)) 
            pt++;
        txt_buf[0] = *pt; pt++;
        while (isspace(*pt)) 
            pt++;
        txt_buf[1] = *pt; pt++;
        while (isspace(*pt)) 
            pt++;

        byte = strtol(txt_buf, &end_ptr, 16);
        if (*end_ptr) /* There are not two hexadecimal digits. */
        {
            *syms_parsed = pt - text;
            return TE_EASNTXTNOTOCTSTR;
        }

        buffer[b_num] = byte; b_num++;
    } 
    pt++;

    if (*pt != 'H')
    {
        /* ERROR! there are no OCTET string */
        *syms_parsed = pt - text;
        return TE_EASNTXTNOTOCTSTR;
    } 

    *parsed = asn_init_value(type); 
    *syms_parsed = pt - text + 1; /* '+1' for tailing 'H' */ 

    if (type && (type->len))
    {
        int rest_len = type->len - b_num;

        if (rest_len)
            memset(buffer + b_num, 0, rest_len); 
        b_num += rest_len;
    }

    rc = asn_write_value_field(*parsed, buffer, b_num, ""); 
    free(buffer);

    return rc;
}

/**
 * Parse textual presentation of single ASN.1 value of INTEGER type,
 * create new instance of asn_value type with its internal presentation.
 *
 * @param text          text to be parsed;
 * @param type          ASN type of value to be parsed;
 * @param parsed        parsed ASN value (OUT);
 * @param syms_parsed   quantity of parsed symbols in 'text' (OUT);
 *
 * @return zero on success, otherwise error code.
 */ 
te_errno 
asn_impl_pt_integer(const char *text, const asn_type *type, 
                    asn_value **parsed, int *syms_parsed)
{
    int   p_value;
    char *endptr; 

    ENTRY("text='%s' type=%p parsed=%p syms_parsed=%p",
          text, type, parsed, syms_parsed);

    if (!text || !parsed || !syms_parsed)
        return TE_EWRONGPTR; 

    p_value = strtol(text, &endptr, 10);
    if ((*syms_parsed = endptr - text) == 0)
    {
        /* ERROR! there are no integer. */
        return TE_EASNTXTNOTINT;
    }

    *parsed = asn_init_value(type); 
    (*parsed)->data.integer = p_value; 
    (*parsed)->txt_len = number_of_digits(p_value);

    EXIT("text+(*syms_parsed)='%s' *syms_parsed=%d",
         text + *syms_parsed, *syms_parsed);

    return 0;
}


/**
 * Parse textual presentation of single ASN.1 value of BOOL type,
 * create new instance of asn_value type with its internal presentation.
 *
 * @param text          text to be parsed;
 * @param type          ASN type of value to be parsed;
 * @param parsed        parsed ASN value (OUT);
 * @param syms_parsed   quantity of parsed symbols in 'text' (OUT);
 *
 * @return zero on success, otherwise error code.
 */ 
te_errno 
asn_impl_pt_bool(const char*text, const asn_type *type, 
                 asn_value **parsed, int *syms_parsed)
{
    if (!text || !parsed || !syms_parsed)
        return TE_EWRONGPTR; 


    *parsed = asn_init_value(type); 
    if (strncmp(text, "TRUE", strlen("TRUE")) == 0)
    {
        (*parsed)->data.integer = 0xff; 
        *syms_parsed = (*parsed)->txt_len = strlen("TRUE");
    }
    else if (strncmp(text, "FALSE", strlen("FALSE")) == 0)
    {
        (*parsed)->data.integer = 0; 
        *syms_parsed = (*parsed)->txt_len = strlen("FALSE");
    }
    else
        return TE_EASNTXTPARSE;

    return 0;
}


/**
 * Parse textual presentation of single ASN.1 value of NULL type,
 * create new instance of asn_value type with its internal presentation.
 *
 * @param text          text to be parsed;
 * @param type          ASN type of value to be parsed;
 * @param parsed        parsed ASN value (OUT);
 * @param syms_parsed   quantity of parsed symbols in 'text' (OUT);
 *
 * @return zero on success, otherwise error code.
 */ 
te_errno 
asn_impl_pt_null(const char *text, const asn_type *type, 
                 asn_value **parsed, int *syms_parsed)
{
    static const char *null_string = "NULL";

    if (!text || !parsed || !syms_parsed)
        return TE_EWRONGPTR; 

    if (memcmp(text, null_string, 4) != 0)
    {
        /* ERROR! there are no NULL. */
        return TE_EASNTXTPARSE;
    }

    *parsed = asn_init_value(type); 
    (*parsed)->data.integer = 0; 
    (*parsed)->txt_len = *syms_parsed = 4;

    return 0;
}


/**
 * Parse textual presentation of single ASN.1 value of ENUMERATED type,
 * create new instance of asn_value type with its internal presentation.
 *
 * @param text          text to be parsed;
 * @param type          ASN enum type specification; 
 * @param parsed        parsed ASN value (OUT);
 * @param syms_parsed   quantity of parsed symbols in 'text' (OUT);
 *
 * @return zero on success, otherwise error code.
 *
 */ 
te_errno 
asn_impl_pt_enum(const char*text, const asn_type *type, 
                 asn_value **parsed, int *syms_parsed)
{
    te_errno   rc;
    int   p_value;
    char *endptr; 
    char  label_buf[100];
    int   p_s = sizeof(label_buf);

    ENTRY("text='%s' type=%p parsed=%p syms_parsed=%p",
          text, type, parsed, syms_parsed);

    if (!text || !parsed || !syms_parsed)
        return TE_EWRONGPTR; 

    p_value = strtol(text, &endptr, 10);
    if ((*syms_parsed = endptr - text) == 0)
    {
        const char  *pt = text;
        unsigned int i;

        while (isspace(*pt)) 
            pt++;
        rc = asn_impl_pt_label(pt, label_buf, &p_s);
        if (rc)
            return rc;
        VERB("%s(): Label to find is '%s'", __FUNCTION__, label_buf);

        pt += p_s; 
        *syms_parsed = pt - text;

        for (i = 0; i < type->len; i++)
        {
            VERB("%s(): Compare label with '%s'", __FUNCTION__,
                 type->sp.enum_entries[i].name);
            if (strcmp(label_buf, type->sp.enum_entries[i].name) == 0)
            {
                p_value = type->sp.enum_entries[i].value;
                break;
            }
        }
        if (i == type->len) 
            return TE_EASNTXTNOTINT;
    }

    *parsed = asn_init_value(type); 
    (*parsed)->data.integer = p_value; 

    return 0;
}


/**
 * Parse textual presentation of single ASN.1 value OID type.
 *
 * @param text          text to be parsed;
 * @param type          ASN type of value to be parsed;
 * @param parsed        parsed ASN value (OUT);
 * @param syms_parsed   quantity of parsed symbols in 'text' (OUT);
 *
 * @return zero on success, otherwise error code.
 *
 * @todo parse of symbolic labels
 */
te_errno 
asn_impl_pt_objid(const char *text, const asn_type *type,
                  asn_value **parsed, int *parsed_syms)
{
    const char *pt = text; 
    int         cur_index = 0;
    int        *parsed_ints = NULL;
    int         parsed_ints_len = 0;
    char       *endptr;
    te_errno    rc = 0;
    int         p_s;

    if (!text || !parsed || !parsed_syms)
        return TE_EWRONGPTR; 

    *parsed_syms = 0;

    /* Skip all the spaces before '{' */
    while (isspace(*pt))
        pt++;

    if (*pt != '{')
        return TE_EASNTXTPARSE; 

    pt++;

    while (isspace(*pt))
        pt++;

    while (*pt != '}')
    {
        /* Allocate memory under the set of sub IDs when required */
        if (parsed_ints_len <= cur_index)
        {
            int *mem_ptr;
            
#define OID_LEN_BLOCK 40
            mem_ptr = (int *)realloc(parsed_ints,
                                     (parsed_ints_len += OID_LEN_BLOCK) *
                                     sizeof(int));
#undef OID_LEN_BLOCK

            if (mem_ptr == NULL)
            {
                free(parsed_ints);
                return TE_ENOMEM;
            }
            parsed_ints = mem_ptr;
        }

        parsed_ints[cur_index] = strtol(pt, &endptr, 10);
        *parsed_syms += (p_s = endptr - pt);

        if (p_s == 0)
        {
            /* ERROR! there are no integer. */
            ERROR("The format of Object ID is incorrect");
            free(parsed_ints);
            return TE_EASNTXTNOTINT;
        }
        pt += p_s;
        while (isspace(*pt))
            pt++;

        cur_index++;
    }
    pt++;

    *parsed = asn_init_value(type);
    *parsed_syms = pt - text;

    if (cur_index)
        rc = asn_write_value_field(*parsed, parsed_ints, cur_index, "");

    if (rc)
        asn_free_value(*parsed);

    free(parsed_ints);

    return rc;
}


/**
 * Parse textual presentation of single ASN.1 value of specified type and
 * create new instance of asn_value type with its internal presentation.
 * Type should be constraint with named components, i.e. SEQUENCE or SET.
 * Note: text should correspoind to the "Value" production label in ASN.1 
 * specification.
 * internal implementation.
 *
 * @param text          text to be parsed;
 * @param type          expected type of value;
 * @param parsed        parsed ASN value (OUT);
 * @param syms_parsed   quantity of parsed symbols in 'text' (OUT);
 *
 * @return zero on success, otherwise error code.
 *
 * @todo        check for the order of elements in SEQUENCE and presence 
 *              of non-OPTIONAL fields should be done. 
 */
te_errno 
asn_impl_pt_named_array(const char *text, const asn_type *type, 
                        asn_value **parsed, int *parsed_syms)
{
    const char *pt = text; 
    te_errno   rc;

    char label_buf[100];

    ENTRY("text='%s' type=%p parsed=%p parsed_syms=%p",
          text, type, parsed, parsed_syms);

    if (!text || !type || !parsed || !parsed_syms)
        return TE_EWRONGPTR; 

    while (isspace(*pt))
        pt++; 
    if (*pt != '{')
    {
        *parsed_syms = pt - text;
        return TE_EASNTXTPARSE; 
    }

    pt++;

    *parsed = asn_init_value(type);

    while (1)
    {
        int p_s = sizeof(label_buf);
        const asn_type *subtype;
        asn_value *subval;

        while (isspace(*pt))
            pt++;
        rc = asn_impl_pt_label(pt, label_buf, &p_s);
        pt += p_s;
        if (rc)
        {
            if (*pt == '}') 
            { pt++; break; }
            *parsed_syms = pt - text;
            return rc;
        }
        rc = asn_impl_find_subtype(type, label_buf, &subtype);
        if (rc) 
        {
            *parsed_syms = pt - text;
            WARN("%s(): subtype for label '%s' not found, %r",
                 __FUNCTION__, label_buf, rc);
            return TE_EASNTXTVALNAME;
        }

        while (isspace(*pt))
            pt++;

        rc = asn_parse_value_text(pt, subtype, &subval, &p_s);
        pt += p_s; 
        *parsed_syms = pt - text;
        if (rc) 
            return rc; 

        rc = asn_put_child_value_by_label(*parsed, subval, label_buf); 
        if (rc) 
            return rc; 

        while (isspace(*pt)) 
            pt++;

        if (*pt == ',') 
        { pt++; continue; } 

        if (*pt == '}') 
        { pt++; break; } 

        *parsed_syms = pt - text; 
        return TE_EASNTXTSEPAR;
    }
    *parsed_syms = pt - text; 
    return 0;
}


/**
 * Parse textual presentation of single ASN.1 value of specified type and
 * create new instance of asn_value type with its internal presentation.
 * Type should be constraint with same not-named components, 
 * i.e. SEQUENCE_OF or SET_OF.
 * Note: text should correspoind to the "Value" production label in ASN.1 
 * specification.
 *
 * @param text          text to be parsed;
 * @param type          expected type of value;
 * @param parsed        parsed ASN value (OUT);
 * @param syms_parsed   quantity of parsed symbols in 'text' (OUT);
 *
 * @return zero on success, otherwise error code.
 */
te_errno 
asn_impl_pt_indexed_array(const char*text, const asn_type * type, 
                          asn_value **parsed, int *parsed_syms)
{
    const char *pt = text; 

    te_errno rc = 0;

    if (!text || !type || !parsed || !parsed_syms)
        return TE_EWRONGPTR; 

    *parsed_syms = 0;
    while (isspace(*pt)) 
        pt++, (*parsed_syms)++; 

    if (*pt != '{')
        return TE_EASNTXTPARSE; 

    pt++; 

    *parsed = asn_init_value(type);

    while (isspace(*pt)) 
        pt++; 

    while (*pt != '}')
    {
        int        p_s;
        asn_value *subval;

        while (isspace(*pt)) 
            pt++; 

        rc = asn_parse_value_text(pt, type->sp.subtype, &subval, &p_s);
        pt += p_s; 
        *parsed_syms = pt - text;
        if (rc) 
            return rc; 

        asn_insert_indexed(*parsed, subval, -1, "");

        while (isspace(*pt)) 
            pt++;

        if (*pt == ',') 
        { pt++; continue; } 

        if (*pt != '}') 
        {
            rc = TE_EASNTXTSEPAR;
            break;
        }
    }
    pt++;
    *parsed_syms = pt - text; 

    return 0;
}

/**
 * Parse textual presentation of single ASN.1 value of specified type and
 * create new instance of asn_value type with its internal presentation.
 * Type should be CHOICE.
 * Note: text should correspoind to the "Value" production label in ASN.1 
 * specification.
 *
 * @param text          text to be parsed;
 * @param type          expected type of value;
 * @param parsed        parsed ASN value (OUT);
 * @param syms_parsed   quantity of parsed symbols in 'text' (OUT);
 *
 * @return zero on success, otherwise error code.
 */
te_errno 
asn_impl_pt_choice(const char *txt, const asn_type *type, 
                   asn_value **parsed, int *parsed_syms)
{
    const char *pt = txt; 
    te_errno    rc;

    char  l_b[100];
    char *label_buf;

    int p_s = sizeof(l_b) - 1; 

    const asn_type *subtype;
    asn_value      *subval;

    label_buf = l_b;

    if (!txt || !type || !parsed || !parsed_syms)
        return TE_EWRONGPTR; 

    *parsed = asn_init_value(type); 

    while (isspace(*pt)) 
        pt++;

    rc = asn_impl_pt_label(pt, label_buf, &p_s);
    if (rc) return rc;

    pt += p_s;
    rc = asn_impl_find_subtype(type, label_buf, &subtype);
    if (rc)
    {
        WARN("%s(): subtype for label '%s' not found\n",
             __FUNCTION__, label_buf);
        *parsed_syms = pt - txt;
        return TE_EASNTXTVALNAME;
    }

    while (isspace(*pt)) pt++;
    if (*pt != ':')
    {
        asn_free_value(*parsed);
        *parsed = NULL;
        *parsed_syms = pt - txt;
        return TE_EASNTXTSEPAR;
    }
    pt++;
    while (isspace(*pt)) 
        pt++;

    rc = asn_parse_value_text(pt, subtype, &subval, &p_s);

    *parsed_syms = pt - txt + p_s;

    if (rc) 
    {
        asn_free_value(*parsed);
        *parsed = NULL;
        return rc; 
    }

    rc = asn_put_child_value_by_label(*parsed, subval, l_b);

    if (rc)
    {
        asn_free_value(*parsed);
        *parsed = NULL;
        return rc; 
    } 
    return 0;
}


/**
 * Parse textual presentation of single ASN.1 value of specified type and
 * create new instance of asn_value type with its internal presentation.
 * Note: text should correspoind to the "Value" production label in ASN.1 
 * specification.
 * internal implementation.
 *
 * @param text          text to be parsed;
 * @param type          expected type of value;
 * @param parsed        parsed ASN value (OUT);
 * @param syms_parsed   quantity of parsed symbols in 'text' (OUT);
 *
 * @return zero on success, otherwise error code.
 */ 
te_errno 
asn_parse_value_text(const char *text, const asn_type *type, 
                     asn_value **parsed, int *syms_parsed)
{
    if (!text || !type || !parsed || !syms_parsed)
        return TE_EWRONGPTR;

    ENTRY("text='%s' type->syntax=%u parsed=%p syms_parsed=%p",
          text, type->syntax, parsed, syms_parsed);
    switch (type->syntax)
    {
        case BOOL:
            return asn_impl_pt_bool      (text, type, parsed, syms_parsed);

        case INTEGER:
            return asn_impl_pt_integer   (text, type, parsed, syms_parsed);
            
        case ENUMERATED: 
            return asn_impl_pt_enum      (text, type, parsed, syms_parsed);

        case CHAR_STRING:
            return asn_impl_pt_charstring(text, type, parsed, syms_parsed);

        case OCT_STRING:
            return asn_impl_pt_octstring (text, type, parsed, syms_parsed);

        case PR_ASN_NULL:
            return asn_impl_pt_null      (text, type, parsed, syms_parsed);

        case OID:
            return asn_impl_pt_objid     (text, type, parsed, syms_parsed);
 
        case SEQUENCE:
        case SET:
            return asn_impl_pt_named_array(text, type, parsed, 
                                           syms_parsed);

        case SEQUENCE_OF:
        case SET_OF:
            return asn_impl_pt_indexed_array(text, type, parsed, 
                                             syms_parsed);

        case CHOICE:
            return asn_impl_pt_choice(text, type, parsed, syms_parsed);

        default:
            return TE_EOPNOTSUPP;
    } 
}

/* See description in asn_usr.h */
te_errno
asn_parse_value_assign_text(const char *string, asn_value **value)
{
    UNUSED(string);
    UNUSED(value);
    return TE_EOPNOTSUPP;
}

/**
 * Read ASN.1 text file, parse value assignments in it while they refer to 
 * ASN types which are known to the module, and add all parsed values 
 * to internal Value hash. Names of 
 *
 * @param filename      name of file to be parsed;
 * @param found_names   names of parsed ASN values (OUT);
 * @param found_len     length of array found_names[] (IN/OUT);
 *
 * @return zero on success, otherwise error code.
 */ 
te_errno 
asn_parse_file(const char *filename, char **found_names, int *found_len)
{
    UNUSED(filename);
    UNUSED(found_names);
    UNUSED(found_len);

    return TE_EOPNOTSUPP;
}


/**
 * Count number of symbols required to deciamal notation of integer/
 *
 * @param value         integer number
 *
 * @return number of symbols.
 */
size_t
number_of_digits(int value)
{
    int n = 0;

    if (value < 0) 
        n++, value = -value;
    while (value >= 10)
        value = value / 10, n++; 

    return n + 1;
}

/**
 * Count required length of string for textual presentation 
 * of specified value. 
 *
 * @param value         ASN value.   
 *
 * @return length of requiered string. 
 */ 
size_t
asn_count_len_enum(const asn_value *value)
{
    unsigned int i;

    int *txt_len_p;

    if (value == NULL) 
        return 0;

    /* Explicit discards of 'const' qualifier, whereas field 'txt_len' 
     * should have 'mutable' semantic */
    txt_len_p = (int *)&(value->txt_len);

    if (value->txt_len < 0) 
    { 
        unsigned int len = 0;

        if (value->syntax != ENUMERATED)
            return -1; 

        for (i = 0; i < value->asn_type->len; i++)
        {
            if (value->data.integer == 
                value->asn_type->sp.enum_entries[i].value)
            {
                len = strlen(value->asn_type->sp.enum_entries[i].name);
                break;
            }
        }
        if (len == 0) 
            len = number_of_digits(value->data.integer);

        *txt_len_p = len;
    }
    return value->txt_len;
}



static char t_class[4][30] = {"UNIVERSAL ", "APPLICATION ", "", "PRIVATE "};

/**
 * Count required length of string for textual presentation 
 * of specified value. 
 *
 * @param value         ASN value.   
 * @param indent        current indent
 *
 * @return length of requiered string. 
 */ 
size_t
asn_count_len_tagged(const asn_value *value, unsigned int indent)
{
    int  all_used = 0; 
    int *txt_len_p;

    if (value == NULL)
        return 0; 

    /* Explicit discards of 'const' qualifier, whereas field 'txt_len' 
     * should have 'mutable' semantic */
    txt_len_p = (int *)&(value->txt_len);

    if (value->syntax != TAGGED) 
        return -1; 

    if (value->txt_len < 0)
    {
        asn_value *v_el = value->data.array[0];
        if (v_el)
        { 
            all_used += strlen(t_class[(int)value->tag.cl]); 
            all_used += number_of_digits(value->tag.val);
            all_used += 3; /* square braces and space after tag */ 
            all_used += asn_count_txt_len(v_el, indent);
        } 
        *txt_len_p = all_used;
    }

    return value->txt_len;
}

/**
 * Count required length of string for textual presentation of specified 
 * value. 
 *
 * @param value         ASN value.   
 * @param indent        current indent
 *
 * @return length of requiered string. 
 */ 
size_t
asn_count_len_choice(const asn_value *value, unsigned int indent)
{
    int  all_used = 0; 
    int *txt_len_p;

    asn_value *v_el;

    if (value == NULL)
        return 0; 

    /* Explicit discards of 'const' qualifier, whereas field 'txt_len' 
     * should have 'mutable' semantic */
    txt_len_p = (int *)&(value->txt_len);

    if (value->syntax != CHOICE) 
        return -1; 
    
    if (value->txt_len < 0)
    {
        v_el = value->data.array[0];
        if (v_el)
        { 
            all_used += strlen(v_el->name) + 1; /* ₁ symbol for ':'*/
            all_used += asn_count_txt_len(v_el, indent);
        } 
        *txt_len_p = all_used;
    }

    return value->txt_len;
}


/**
 * Count required length of string for textual presentation 
 * of specified value. 
 *
 * @param value         ASN value.   
 *
 * @return length of requiered string. 
 */ 
size_t
asn_count_len_objid(const asn_value *value)
{ 
    int *txt_len_p;

    if (value == NULL) 
        return 0; 

    /* Explicit discards of 'const' qualifier, whereas field 'txt_len' 
     * should have 'mutable' semantic */
    txt_len_p = (int *)&(value->txt_len);

    if (value->syntax != OID) 
        return -1; 
    
    if (value->txt_len < 0)
    {
        int  all_used = 2; /* braces */
        int *subid;
        
        unsigned int i;

        subid = (int*) value->data.other; 

        for (i = 0; i < value->len; i++)
        { 
            /* 1 for separating space */
            all_used += number_of_digits(subid[i]) + 1; 
        } 
        *txt_len_p = all_used;
    }

    return value->txt_len;
}





/**
 * Prepare textual ASN.1 presentation of passed value ENUMERATED 
 * and put it into specified buffer. 
 *
 * @param value         ASN value to print, should have ENUMERATED type.
 * @param buffer        buffer for ASN text.  
 * @param buf_len       length of buffer. 
 *
 * @return number characters written to buffer or -1 if error occured. 
 */ 
int
asn_sprint_enum(const asn_value *value, char *buffer, size_t buf_len)
{
    int          used;
    unsigned int i, need;
    const char  *val_label = NULL;

    if ((value == NULL) || (buffer == NULL) || (buf_len == 0) )
        return 0;

    if (value->syntax != ENUMERATED)
        return -1;


    for (i = 0; i < value->asn_type->len; i++)
    {
        if (value->data.integer == 
            value->asn_type->sp.enum_entries[i].value)
        {
            val_label = value->asn_type->sp.enum_entries[i].name;
            break;
        }
    }
    if (i < value->asn_type->len) 
        used = snprintf(buffer, buf_len, "%s", val_label);
    else
        used = snprintf(buffer, buf_len, "%d", value->data.integer);

    if (buf_len <= (need = asn_count_len_enum(value)))
        return need;


    return used;
}





/**
 * Prepare textual ASN.1 presentation of passed value Character String 
 * and put it into specified buffer. 
 * NOTE: see description for asn_sprint_value(), this function is very 
 * similar, just syntax-specific. 
 *
 * @param value         ASN value to be printed, should have INTEGER type.
 * @param buffer        buffer for ASN text.  
 * @param buf_len       length of buffer. 
 *
 * @return number characters written to buffer or -1 if error occured. 
 */ 
int
asn_sprint_charstring(const asn_value *value, char *buffer, size_t buf_len)
{
    char *string;
    char *quote_place;
    char *buf_place;
    size_t total_syms = 0, interval = 0;

    if ((value == NULL) || (buffer == NULL) || (buf_len == 0) )
        return 0;

    if (value->syntax != CHAR_STRING)
        return -1;

    if (buf_len == 1)
        goto finish;

    buf_place = buffer;
    strcpy(buf_place, "\""); 
    total_syms++;
    buf_place++;

#define PUT_PIECE(_src, _len) \
    do {                                                \
        size_t loc_len = (_len);                        \
        interval = (buf_len - total_syms);              \
        strncpy(buf_place, (_src),                      \
                (interval > loc_len) ?                  \
                            loc_len : interval);        \
        if (interval <= loc_len)                        \
            goto finish;                                \
        total_syms += loc_len;                          \
        buf_place += loc_len;                           \
    } while (0)

    string = value->data.other;
    while (string != NULL && string[0] != '\0')
    {
        char quote[] = "\\\"";
        quote_place = index(string, '"');
        if (quote_place == NULL)
            break;

        PUT_PIECE(string, quote_place - string);

        PUT_PIECE(quote, strlen(quote));

        string = quote_place + 1;
    }

    if (string != NULL && string[0] != '\0')
        PUT_PIECE(string, strlen(string));
    
    if (total_syms + 1 >= buf_len)
        goto finish;

    strcpy(buf_place, "\""); 
    total_syms++;

#undef PUT_PIECE

finish:
    buffer[buf_len - 1] = 0; 
    return value->txt_len; /* assume, that for character string 'txt_len'
                              is allways correct - it is updated when
                              value is changed */
}


/**
 * Prepare textual ASN.1 presentation of passed value OCTET STRING
 * and put it into specified buffer. 
 *
 * @param value         ASN value to be printed, should have INTEGER type.  
 * @param buffer        buffer for ASN text.  
 * @param buf_len       length of buffer. 
 *
 * @return number characters written to buffer or -1 if error occured. 
 */ 
int
asn_sprint_octstring(const asn_value *value, char *buffer, size_t buf_len)
{
    char        *pb = buffer;
    char        *last_b = buffer + buf_len - 1;
    unsigned int i;
    static char  hex_digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', 
                                 '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', };
    uint8_t     *cur_byte;

    if ((value == NULL) || (buffer == NULL) || (buf_len == 0) )
        return 0;

    if (value->syntax != OCT_STRING)
        return -1;

#define PUT_OCT_SYM(_byte) \
    do {                        \
        if (pb == last_b)       \
            goto finish;        \
        *pb++ = (_byte);        \
    } while (0)

    PUT_OCT_SYM('\'');

    for (i = 0, cur_byte = (uint8_t*)value->data.other; i < value->len; 
         i++, cur_byte++)
    {
        PUT_OCT_SYM(hex_digits[(*cur_byte) >> 4  ]);
        PUT_OCT_SYM(hex_digits[(*cur_byte) & 0x0f]);
        PUT_OCT_SYM(' ');
    }
    PUT_OCT_SYM('\'');
    PUT_OCT_SYM('H');

#undef PUT_OCT_SYM
finish: 
    *pb = '\0';
    return value->txt_len;
}

/**
 * Prepare textual ASN.1 presentation of passed value of complex type with 
 * TAGGED syntax and put it into specified buffer. 
 *
 * @param value         ASN value to be printed, should have INTEGER type.  
 * @param buffer        buffer for ASN text.  
 * @param buf_len       length of buffer. 
 * @param indent        current indent
 *
 * @return number characters written to buffer or -1 if error occured. 
 */ 
int
asn_sprint_tagged(const asn_value *value, char *buffer, size_t buf_len, 
        unsigned int indent)
{
    unsigned int all_used = 0, used;

    asn_value *v_el;

    if ((value == NULL) || (buffer == NULL) || (buf_len == 0) )
        return 0; 

    /* codes of syntaxes which processed in this method 
     * may have arbitrary last two bits*/ 
    if (value->syntax != TAGGED) 
        return -1; 
    
    v_el = value->data.array[0];
    if (v_el)
    { 
        char t_class[4][30] = {"UNIVERSAL ","APPLICATION ","","PRIVATE "};

        used = snprintf(buffer, buf_len, "[%s%d]", 
                        t_class[(int)value->tag.cl], value->tag.val); 
        if (used >= buf_len)
            return used;
        all_used += used; buffer += used; 

        used = asn_sprint_value(v_el, buffer, buf_len - all_used, indent);
        all_used += used; 
    } 

    return all_used;
}

/**
 * Prepare textual ASN.1 presentation of passed value of complex type with 
 * CHOICE syntax and put it into specified buffer. 
 *
 * @param value         ASN value to be printed, should have INTEGER type.  
 * @param buffer        buffer for ASN text.  
 * @param buf_len       length of buffer. 
 * @param indent        current indent
 *
 * @return number characters written to buffer or -1 if error occured. 
 */ 
int
asn_sprint_choice(const asn_value *value, char *buffer, size_t buf_len, 
                  unsigned int indent)
{
    int   used;
    char *p = buffer, 
         *last = buffer + buf_len - 1,
         *name_p;

    asn_value *v_el;

    if ((value == NULL) || (buffer == NULL) || (buf_len == 0) )
        return 0; 

    if (value->syntax != CHOICE) 
        return -1; 
    
    if ((v_el = value->data.array[0]) == NULL)
        return -1;

    
    for (name_p = v_el->name; 
         (*name_p != '\0') && (p < last);
         p++, name_p++)
        *p = *name_p;

    if (p < last)
        *p++ = ':';

    if (p == last) 
    {
        *p = '\0';
        return asn_count_len_choice(value, indent);
    }

    used = p - buffer;
    used += asn_sprint_value(v_el, p, buf_len - used, indent);

    return used;
}


/**
 * Prepare textual ASN.1 presentation of passed value of OID type and 
 * put it into specified buffer. 
 *
 * @param value         ASN value to be printed, should have INTEGER type.  
 * @param buffer        buffer for ASN text.  
 * @param buf_len       length of buffer. 
 *
 * @return number characters written to buffer or -1 if error occured. 
 */ 
int
asn_sprint_objid(const asn_value *value, char *buffer, size_t buf_len)
{
    unsigned int i;
    char *last = buffer + buf_len - 1;

    unsigned int all_used = 0, used;
    int *subid;

    if ((value == NULL) || (buffer == NULL) || (buf_len == 0) )
        return 0; 

    if (value->syntax != OID) 
        return -1; 
    

    if (buf_len == 1)
        goto error;
    strcpy(buffer, "{");

    all_used++; buffer++; buf_len--;

    subid = (int *)value->data.other; 

    for (i = 0; i < value->len; i++)
    { 
        used = snprintf(buffer, buf_len, "%d ", subid[i]);
        if (used >= buf_len)
            goto error;
        all_used += used, buffer += used, buf_len -= used;
    } 
    if (buffer == last)
        goto error; 
    strcpy(buffer, "}"); 
    all_used++;

    return all_used;

error:
    *last = '\0';
    return asn_count_len_objid(value);
}


/**
 * Prepare textual ASN.1 presentation of passed value of complex 
 * type with many subvalues (i.e. 'SEQUENCE[_OF]' and 'SET[_OF]') 
 * and put it into specified buffer. 
 *
 * @param value         ASN value to be printed, should have INTEGER type.
 * @param buffer        buffer for ASN text.  
 * @param buf_len       length of buffer. 
 * @param indent        current indent
 *
 * @return number characters written to buffer or -1 if error occured. 
 */ 
int
asn_sprint_array_fields(const asn_value *value, char *buffer, 
                        size_t buf_len, unsigned int indent)
{
    unsigned int i, j; 
    unsigned int all_used = 0, used;
    int was_element = 0;
    
    char *last = buffer + buf_len - 1;

    if ((value == NULL) || (buffer == NULL) || (buf_len == 0) )
        return 0; 

    /* codes of syntaxes which processed in this method 
     * may have arbitrary last two bits*/
    if ((value->syntax & ((-1)<<2) )!= SEQUENCE) 
        return -1; 

#define PUT_OCT_SYM(_byte) \
    do {                        \
        if (buffer == last)     \
            goto error;         \
        *buffer++ = (_byte);    \
        all_used++;             \
    } while (0)
    
    PUT_OCT_SYM('{');

    indent += 2;

    for (i = 0; i < value->len; i++)
    { 
        asn_value *v_el = value->data.array[i];


        if (v_el)
        {
            if (was_element) 
                PUT_OCT_SYM(',');

            PUT_OCT_SYM('\n');

            for (j = 0; j < indent; j++)
                PUT_OCT_SYM(' ');

            /* Check if we have structure with named components. */
            if (!(value->syntax & 1))
            {
                used = snprintf(buffer, buf_len - all_used, "%s ",
                                v_el->name);
                if (used >= buf_len - all_used)
                    goto error;
                all_used += used; buffer += used; 
            } 

            used = asn_sprint_value(v_el, buffer, buf_len - all_used, 
                                    indent);
            if (used >= buf_len - all_used)
                goto error;
            all_used += used; buffer += used;

            was_element = 1;
        }
    } 
    PUT_OCT_SYM('\n');

    indent -= 2;
    for (j = 0; j < indent; j++)
        PUT_OCT_SYM(' ');

    PUT_OCT_SYM('}'); 
    *buffer = '\0';

#undef PUT_OCT_SYM

    return all_used;
error:
    *last = '\0';
    return asn_count_txt_len(value, indent);
}

/**
 * Prepare textual ASN.1 presentation of passed value and put it into 
 * specified buffer. 
 *
 * @param value         ASN value to be printed.  
 * @param buffer        buffer for ASN text.  
 * @param buf_len       length of buffer. 
 * @param indent        current indent
 *
 * @return number characters written to buffer or -1 if error occured. 
 */ 
int
asn_sprint_value(const asn_value *value, char *buffer, size_t buf_len, 
                 unsigned int indent)
{
    if ((value == NULL) || (buffer == NULL) || (buf_len == 0) )
        return 0;

    switch (value->syntax)
    {
        case BOOL:
            if (value->data.integer) 
                return snprintf(buffer, buf_len, "TRUE");
            else
                return snprintf(buffer, buf_len, "FALSE");

        case INTEGER:
            return snprintf(buffer, buf_len, "%d", value->data.integer);

        case ENUMERATED: 
            return asn_sprint_enum(value, buffer, buf_len);

        case CHAR_STRING:
            return asn_sprint_charstring(value, buffer, buf_len);

        case OCT_STRING:
            return asn_sprint_octstring(value, buffer, buf_len);

        case PR_ASN_NULL:
            return snprintf(buffer, buf_len, "NULL");

        case LONG_INT:
        case BIT_STRING:
        case REAL:
            return 0; /* not implemented yet.*/
        case OID:
            return asn_sprint_objid(value, buffer, buf_len);
        case CHOICE:
            return asn_sprint_choice(value, buffer, buf_len, indent);

        case TAGGED:
            return asn_sprint_tagged(value, buffer, buf_len, indent);

        case SEQUENCE:
        case SEQUENCE_OF:
        case SET:
        case SET_OF:
            return asn_sprint_array_fields(value, buffer, buf_len, indent);

        default: 
            return 0; /* nothing to do. */
    }
}


/**
 * Count required length of string for textual presentation of 
 * specified value. 
 *
 * @param value         ASN value.   
 * @param indent        current indent
 *
 * @return length of requiered string. 
 */ 
size_t 
asn_count_txt_len(const asn_value *value, unsigned int indent) 
{ 
    if (value == NULL) 
        return 0;

    switch (value->syntax)
    {
        case BOOL:
            if (value->data.integer) 
                return strlen("TRUE");
            else
                return strlen("FALSE");

        case INTEGER:
            return value->txt_len;

        case ENUMERATED: 
            return asn_count_len_enum(value);

        case CHAR_STRING:
            return value->txt_len;

        case OCT_STRING:
            return value->txt_len;

        case PR_ASN_NULL:
            return strlen("NULL");

        case LONG_INT:
        case BIT_STRING:
        case REAL:
            return 0; /* not implemented yet.*/
            
        case OID:
            return asn_count_len_objid(value);

        /* Due to some (not found) bugs in counting length, obtained value 
         * is a bit less then really need, so the folowing ugly hacks
         * are made. 
         * TODO: found and fix bugs. */
        case CHOICE:
            return asn_count_len_choice(value, indent);

        case TAGGED:
            return asn_count_len_tagged(value, indent);

        case SEQUENCE:
        case SEQUENCE_OF:
        case SET:
        case SET_OF:
            return asn_count_len_array_fields(value, indent);

        default: 
            return 0; /* nothing to do. */
    }
}


/**
 * Count required length of string for textual presentation 
 * of specified value. 
 *
 * @param value         ASN value.   
 * @param indent        current indent
 *
 * @return length of requiered string. 
 */ 
size_t
asn_count_len_array_fields(const asn_value *value, unsigned int indent)
{ 
    int *txt_len_p;

    if (value == NULL)
        return 0; 
    /* Explicit discards of 'const' qualifier, whereas field 'txt_len' 
     * should have 'mutable' semantic */
    txt_len_p = (int *)&(value->txt_len);

    /* codes of syntaxes which processed in this method 
     * may have arbitrary last two bits*/
    if ((value->syntax & ((-1)<<2) )!= SEQUENCE) 
        return -1; 
    
    if (value->txt_len < 0)
    {
        unsigned int i;

        int all_used = 1; /* for "{" */
        int elems = 0;

        for (i = 0; i < value->len; i++)
        { 
            asn_value *v_el = value->data.array[i];

            if (v_el)
            { 
                if ( !(value->syntax & 1) )
                { 
                    all_used += strlen(v_el->name) + 1;
                } 
                all_used += asn_count_txt_len(v_el, indent + 2); 
                elems ++;
            }
        } 
        all_used +=   elems * (indent + 2) /* indents before subvalues */
                    + elems + 1            /* new line symbols */
                    + (elems ? (elems - 1) : 0)/* commas */    
                    + indent + 1;          /* closing brace wint indent */

        *txt_len_p = all_used;
    }

    return value->txt_len;
}


/**
 * Prepare textual ASN.1 presentation of passed value and save this string 
 * to file with specified name. 
 * If file already exists, it will be overwritten.
 *
 * @param value         ASN value to be saved. 
 * @param filename      name of file 
 *
 * @return zero on success, otherwise error code.
 */ 
te_errno
asn_save_to_file(const asn_value *value, const char *filename)
{
    FILE *fp;
    char *buffer;
    int   len;

    if (value == NULL || filename == NULL)
        return TE_EWRONGPTR;

    len = asn_count_txt_len(value, 0);
    fp = fopen(filename, "w+");
    if (fp == NULL) 
        return errno;

    buffer = malloc(len + 11);
    if (buffer == NULL)
    {
        /* We need to close files - this comment is for "konst" */
        fclose(fp);
        return TE_ENOMEM;
    }
    memset(buffer, 0, len + 11);
    
    asn_sprint_value(value, buffer, len + 10, 0);
    fwrite(buffer, strlen(buffer), 1, fp);

    fclose(fp);

    free(buffer);

    return 0;
}


static te_errno
file_len(const char *filename, size_t *len)
{
    struct stat fst;

    if (len == NULL)
        return TE_EWRONGPTR;

    if (stat(filename, &fst) != 0)
        return errno; 

    *len = fst.st_size;
    return 0;
}

/**
 * Read ASN.1 text file, parse DefinedValue of specified ASN type
 *
 * @param filename      name of file to be parsed;
 * @param type          expected type of value
 * @param parsed_value  parsed value (OUT)
 * @param syms_parsed   quantity of parsed symbols in 'text' (OUT); 
 *
 * @return zero on success, otherwise error code.
 */ 
te_errno 
asn_parse_dvalue_in_file(const char *filename, const asn_type *type, 
                         asn_value **parsed_value, int *syms_parsed) 
{ 
    size_t flen;
    char *buf;
    int   fd;
    int   read_sz;

    te_errno rc;

    if (filename == NULL || type == NULL || 
        parsed_value == NULL || syms_parsed == NULL)
        return TE_EWRONGPTR; 

    if ((rc = file_len(filename, &flen)) != 0)
        return rc;

    buf = calloc(flen + 1, 1); 
    fd = open(filename, O_RDONLY);
    if (fd < 0)
        return errno; 

    read_sz = read(fd, buf, flen + 1);
    close(fd);

    rc = asn_parse_value_text(buf, type, parsed_value, syms_parsed);

    free(buf);

    return rc;
}
