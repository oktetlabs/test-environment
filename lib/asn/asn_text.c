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

/*
 * Declaraions of local functions. 
 */ 
size_t number_of_digits      (int value);

size_t asn_count_len_array_fields(const asn_value *value, unsigned int indent);

int asn_impl_pt_label      (const char*text, char *label, int *parsed_syms);


int asn_impl_pt_charstring (const char*text, const asn_type *, asn_value_p *parsed, int *parsed_syms);

int asn_impl_pt_integer    (const char*text, const asn_type *, asn_value_p *parsed, int *parsed_syms);

int asn_impl_pt_null       (const char*text, const asn_type *, asn_value_p *parsed, int *parsed_syms);

int asn_impl_pt_objid      (const char*text, const asn_type *, asn_value_p *parsed, int *parsed_syms);


int asn_impl_pt_octstring    (const char  *text,   const asn_type *type, 
                              asn_value_p *parsed, int            *parsed_syms);

int asn_impl_pt_enum         (const char*text, const asn_type *, asn_value_p *parsed, int *parsed_syms);

int asn_impl_pt_named_array  (const char*text, const asn_type *, asn_value_p *parsed, int *parsed_syms);

int asn_impl_pt_indexed_array(const char*text, const asn_type *, asn_value_p *parsed, int *parsed_syms);

int asn_impl_pt_choice       (const char*text, const asn_type *, asn_value_p *parsed, int *parsed_syms);

int asn_parse_value_text     (const char*text, const asn_type *, asn_value_p *parsed, int *parsed_syms);


static char spaces[] = " \t\n\r";

#define PARSE_BUF 0x400

/**
 * Parse label in ASN.1 text, that is "valuereference" according to ASN.1 
 * specification terminology. 
 *
 * @param text          text to be parsed;
 * @param label         found value;
 * @param syms          size of 'label' buffer, size of parsed label (IN/OUT);
 *
 * @return zero on success, otherwise error code.
 */ 
int 
asn_impl_pt_label(const char*text, char *label, int *syms)
{ 
    int    l = 0;
    const char *l_begin;
    const char *pt = text;

    while (isspace(*pt) ) pt++;

    l_begin = pt;
        
    /* first letter in 'valuereference' should be lower case character */
    if (!islower(*pt)) 
    {
        *syms = pt - text;
        return EASNTXTVALNAME; 
    }
    pt++, l++;

    while (isdigit(*pt) || isalpha(*pt) || (*pt == '-'))
        pt++, l++;

    if ((l + 1) > *syms) /* '+ 1' for tailing zero. */
        return ETESMALLBUF;

    memcpy (label, l_begin, l);
    label[l] = 0;
    *syms = pt - text;
    return 0; 
}

/**
 * Parse textual presentation of single ASN.1 value of UniversalString type,
 * create new instance of asn_value type with its internal presentation.
 *
 * @param text          text to be parsed;
 * @param parsed        parsed ASN value (OUT);
 * @param syms_parsed   quantity of parsed symbols in 'text' (OUT);
 *
 * @return zero on success, otherwise error code.
 */ 
int 
asn_impl_pt_charstring(const char*text, const asn_type *type, asn_value_p *parsed, int *syms_parsed)
{
    const char * pt = text; 
    char   buffer[PARSE_BUF];
    char * pb = buffer;       
    
    size_t l;
    size_t total = 0;

    if (!text || !parsed || !syms_parsed)
        return ETEWRONGPTR; 

    l = strspn (pt, spaces);
    pt += l;

    if (*pt != '"')
    {
        /* ERROR! there are no char string */
        return EASNTXTNOTCHSTR;
    } 
    pt++; 

    while (*pt != '"') 
    { 
        l = strcspn (pt, "\\\""); /* find first \ or " */ 
        if (l + total > PARSE_BUF) 
        { 
            /* ERROR! string is too long.. 
               really, allocation of more memory should be here.  */ 
            return EASNGENERAL; 
        }
        memcpy (pb, pt, l); 
        pt+= l; pb += l; total += l;

        if (*pt == '\\')
        {
            pt++;
            *pb = *pt;
            pb ++, pt ++, total++;
        }
    } 
    *pb = 0;

    *parsed = asn_init_value(type); 
    *syms_parsed = pt - text + 1; 

    return asn_write_value_field(*parsed, buffer, total, ""); 
}



/**
 * Parse textual presentation of single ASN.1 value of OCTET STRING type,
 * create new instance of asn_value type with its internal presentation.
 *
 * @param text          text to be parsed;
 * @param parsed        parsed ASN value (OUT);
 * @param syms_parsed   quantity of parsed symbols in 'text' (OUT);
 *
 * @return zero on success, otherwise error code.
 */ 
int 
asn_impl_pt_octstring(const char*text, const asn_type *type, 
                      asn_value_p *parsed, int *syms_parsed)
{
    const char * pt = text; 
    uint8_t   buffer[PARSE_BUF];
    
    char txt_buf[3]; 
    int  octstr_len = 0;
    int  b_num = 0;

    txt_buf[2] = 0;

    if (!text || !parsed || !syms_parsed)
        return ETEWRONGPTR; 

    if (type) 
        octstr_len = type->len;

    while (isspace(*pt)) pt++;

    if (*pt != '\'')
    {
        /* ERROR! there are no OCTET string */
        return EASNTXTNOTOCTSTR;
    } 
    pt++; 


    while (*pt != '\'') 
    {         
        int byte;
        char *end_ptr;

        if ((octstr_len && (b_num == octstr_len) ) || 
            (b_num == PARSE_BUF))
        {
            return EASNTXTPARSE;
        }

        while (isspace(*pt)) pt++;
        txt_buf[0] = *pt; pt++;
        while (isspace(*pt)) pt++;
        txt_buf[1] = *pt; pt++;
        while (isspace(*pt)) pt++;

        byte = strtol(txt_buf, &end_ptr, 16);
        if (*end_ptr) /* There are not two hexadecimal digits. */
            return EASNTXTNOTOCTSTR;

        buffer[b_num] = byte; b_num++;
    } 
    pt ++;

    if (*pt != 'H')
    {
        /* ERROR! there are no OCTET string */
        return EASNTXTNOTOCTSTR;
    } 

    *parsed = asn_init_value(type); 
    *syms_parsed = pt - text + 1; /* '+1' for tailing 'H' */ 

    if (type && (type->len))
    {
        int rest_len = type->len - b_num;
        if (rest_len)
            memset (buffer + b_num, 0, rest_len); 
        b_num += rest_len;
    }

    return asn_write_value_field(*parsed, buffer, b_num, ""); 
}

/**
 * Parse textual presentation of single ASN.1 value of INTEGER type,
 * create new instance of asn_value type with its internal presentation.
 *
 * @param text          text to be parsed;
 * @param parsed        parsed ASN value (OUT);
 * @param syms_parsed   quantity of parsed symbols in 'text' (OUT);
 *
 * @return zero on success, otherwise error code.
 */ 
int 
asn_impl_pt_integer(const char*text, const asn_type *type, asn_value_p *parsed, int *syms_parsed)
{
    int   p_value;
    char *endptr; 

    if (!text || !parsed || !syms_parsed)
        return ETEWRONGPTR; 

    p_value = strtol (text, &endptr, 10);
    if ((*syms_parsed = endptr - text) == 0)
    {
        /* ERROR! there are no integer. */
        return EASNTXTNOTINT;
    }

    *parsed = asn_init_value(type); 
    (*parsed)->data.integer = p_value; 
    (*parsed)->txt_len = number_of_digits(p_value);

    return 0;
}


/**
 * Parse textual presentation of single ASN.1 value of NULL type,
 * create new instance of asn_value type with its internal presentation.
 *
 * @param text          text to be parsed;
 * @param parsed        parsed ASN value (OUT);
 * @param syms_parsed   quantity of parsed symbols in 'text' (OUT);
 *
 * @return zero on success, otherwise error code.
 */ 
int 
asn_impl_pt_null(const char *text, const asn_type *type, 
                 asn_value_p *parsed, int *syms_parsed)
{
    static const char *null_string = "NULL";

    if (!text || !parsed || !syms_parsed)
        return ETEWRONGPTR; 

    if (memcmp (text, null_string, 4) != 0)
    {
        /* ERROR! there are no NULL. */
        return EASNTXTPARSE;
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
int 
asn_impl_pt_enum(const char*text, const asn_type * type, asn_value_p *parsed, int *syms_parsed)
{
    int    rc;
    int    p_value;
    char * endptr; 
    char   label_buf[100];
    int    p_s = sizeof (label_buf);

    if (!text || !parsed || !syms_parsed)
        return ETEWRONGPTR; 

    p_value = strtol (text, &endptr, 10);
    if ((*syms_parsed = endptr - text) == 0)
    {
        const char *pt = text;
        unsigned int i;

        while (isspace(*pt)) pt++;
        rc = asn_impl_pt_label(pt, label_buf, &p_s);
        if (rc) return rc;

        pt += p_s; 
        *syms_parsed = pt - text;

        for (i = 0; i < type->len; i++)
        {
            if (strncmp (label_buf, type->sp.enum_entries[i].name, p_s) == 0)
            {
                p_value = type->sp.enum_entries[i].value;
                break;
            }
        }
        if (i == type->len) 
            return EASNTXTNOTINT;
    }

    *parsed = asn_init_value(type); 
    (*parsed)->data.integer = p_value; 

    return 0;
}


/**
 * Parse textual presentation of single ASN.1 value OID type.
 *
 * @param text          text to be parsed;
 * @param parsed        parsed ASN value (OUT);
 * @param syms_parsed   quantity of parsed symbols in 'text' (OUT);
 *
 * @return zero on success, otherwise error code.
 *
 * @todo parse of symbolic labels
 */
int 
asn_impl_pt_objid(const char*text, const asn_type *type, asn_value_p *parsed, int *parsed_syms)
{
#define OID_MAX 40
    int parsed_ints [OID_MAX];
    int cur_index = 0;
    const char * pt = text; 
    char *endptr;

    int rc;
    int p_s;

    if (!text || !parsed || !parsed_syms)
        return ETEWRONGPTR; 

    *parsed_syms = 0;

    while (isspace(*pt)) pt++; 
    if (*pt != '{')
    {
        return EASNTXTPARSE; 
    }
    pt++;

    while (isspace(*pt)) pt++;

    while(1)
    { 
        parsed_ints[cur_index] = strtol (pt, &endptr, 10);
        *parsed_syms += (p_s = endptr - pt);

        if (p_s == 0)
        {
            /* ERROR! there are no integer. */
            return EASNTXTNOTINT;
        } 
        pt += p_s;
        while (isspace(*pt)) pt++;
        cur_index++;

        if (*pt == '}') break; 
    }
    pt ++;
    *parsed = asn_init_value(type);
    *parsed_syms = pt - text;

    rc = asn_write_value_field(*parsed, parsed_ints, cur_index, ""); 
    if (rc) asn_free_value(*parsed); 

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
int 
asn_impl_pt_named_array(const char*text, const asn_type * type, 
                        asn_value_p *parsed, int *parsed_syms)
{
    const char * pt = text; 

    char label_buf[100];
    int rc;

    if (!text || !type || !parsed || !parsed_syms)
        return ETEWRONGPTR; 

    while (isspace(*pt)) pt++; 
    if (*pt != '{')
    {
        return EASNTXTPARSE; 
    }
    pt++;

    *parsed = asn_init_value(type);

    while(1)
    {
        int p_s = sizeof(label_buf);
        const asn_type *subtype;
        asn_value *subval;

        while (isspace(*pt)) pt++;
        rc = asn_impl_pt_label(pt, label_buf, &p_s);
        pt += p_s;
        if (rc)
        {
            if (*pt == '}') break;
            return rc;
        }
        rc = asn_impl_find_subtype(type, label_buf, &subtype);
        if (rc) 
        {
            *parsed_syms = pt - text;
            return EASNTXTVALNAME;
        }

        while (isspace(*pt)) pt++;

        rc = asn_parse_value_text(pt, subtype, &subval, &p_s);
        pt += p_s; 
        *parsed_syms = pt - text;
        if (rc) return rc; 

        rc = asn_impl_insert_subvalue(*parsed, label_buf, subval); 
        if (rc) return rc; 

        while (isspace(*pt)) pt++;

        if (*pt == ',') 
        { pt++; continue; } 

        if (*pt == '}') 
        { pt++; break; } 

        *parsed_syms = pt - text; 
        return EASNTXTSEPAR;
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
int 
asn_impl_pt_indexed_array(const char*text, const asn_type * type, 
                        asn_value_p *parsed, int *parsed_syms)
{
    const char * pt = text; 

    int rc = 0;

    if (!text || !type || !parsed || !parsed_syms)
        return ETEWRONGPTR; 

    *parsed_syms = 0;
    while (isspace(*pt)) pt++, (*parsed_syms)++; 

    if (*pt != '{')
    {
        return EASNTXTPARSE; 
    }
    pt++;


    *parsed = asn_init_value(type);

    while (isspace(*pt)) pt++; 

    while(*pt != '}')
    {
        int p_s;
        asn_value_p subval;

        while (isspace(*pt)) pt++; 

        rc = asn_parse_value_text(pt, type->sp.subtype, &subval, &p_s);
        pt += p_s; 
        *parsed_syms = pt - text;
        if (rc) return rc; 

        asn_insert_indexed (*parsed, subval, -1, "");
        asn_free_value(subval);

        while (isspace(*pt)) pt++;

        if (*pt == ',') 
        { pt++; continue; } 

        if (*pt != '}') 
        {
            rc = EASNTXTSEPAR;
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
int 
asn_impl_pt_choice(const char*text, const asn_type * type, 
                        asn_value_p *parsed, int *parsed_syms)
{
    const char * pt = text; 
    char l_b[100];
    char *label_buf;

    int rc;
    int p_s = sizeof(l_b) - 1; 

    const asn_type *subtype;
    asn_value *subval;

#if 0
    label_buf = l_b + 1;
    l_b[0] = '#';
#else
    label_buf = l_b;
#endif

    if (!text || !type || !parsed || !parsed_syms)
        return ETEWRONGPTR; 

    *parsed = asn_init_value(type); 

    while (isspace(*pt)) pt++;

    rc = asn_impl_pt_label(pt, label_buf, &p_s);
    if (rc) return rc;

    pt += p_s;
    rc = asn_impl_find_subtype(type, label_buf, &subtype);
    if (rc)
    {
        *parsed_syms = pt - text;
        return EASNTXTVALNAME;
    }

    while (isspace(*pt)) pt++;
    if (*pt != ':')
    {
        asn_free_value(*parsed);
        *parsed = NULL;
        *parsed_syms = pt - text;
        return EASNTXTSEPAR;
    }
    pt++;
    while (isspace(*pt)) pt++;

    rc = asn_parse_value_text(pt, subtype, &subval, &p_s);

    *parsed_syms = pt - text + p_s;

    if (rc) 
    {
        asn_free_value(*parsed);
        *parsed = NULL;
        return rc; 
    }

    rc = asn_impl_insert_subvalue (*parsed, l_b, subval);

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
int 
asn_parse_value_text(const char *text, const asn_type * type, 
                    asn_value_p *parsed, int *syms_parsed)
{
    if (!text || !type || !parsed || !syms_parsed)
        return ETEWRONGPTR;

    switch (type->syntax)
    {
        case INTEGER:
            return asn_impl_pt_integer       (text, type, parsed, syms_parsed);
        case ENUMERATED: 
            return asn_impl_pt_enum          (text, type, parsed, syms_parsed);

        case CHAR_STRING:
            return asn_impl_pt_charstring    (text, type, parsed, syms_parsed);

        case OCT_STRING:
            return asn_impl_pt_octstring     (text, type, parsed, syms_parsed);

        case PR_ASN_NULL:
            return asn_impl_pt_null          (text, type, parsed, syms_parsed);

        case OID:
            return asn_impl_pt_objid         (text, type, parsed, syms_parsed);
 
        case SEQUENCE:
        case SET:
            return asn_impl_pt_named_array   (text, type, parsed, syms_parsed);

        case SEQUENCE_OF:
        case SET_OF:
            return asn_impl_pt_indexed_array (text, type, parsed, syms_parsed);

        case CHOICE:
            return asn_impl_pt_choice        (text, type, parsed, syms_parsed);

        default:
            return ETENOSUPP;
    } 
}

/**
 * Parse ASN.1 text with "Value assignment" (see ASN.1 specification) 
 * and create new ASN value instance with internal presentation of this value.
 * If type of ASN value in 'string' is not known for module, string will not 
 * parsed and NULL will be returned. 
 * Name of value is stored in field 'name' of asn_value structure. 
 *
 * @param string        text to be parsed;
 *
 * @return pointer to new ASN_value instance or NULL if error occurred. 
 */ 
asn_value_p 
asn_parse_value_assign_text(const char *string)
{
    UNUSED(string);
    return NULL;
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
int 
asn_parse_file(const char *filename, char **found_names, int *found_len)
{
    UNUSED(filename);
    UNUSED(found_names);
    UNUSED(found_len);

    return ETENOSUPP;
}


/**
 * Count number of symbols required to deciamal notation of integer/
 * @param value         integer number
 * @return number of digits.
 */
size_t
number_of_digits(int value)
{
    int n = 0;
    if (value < 0) 
        n++, value = -value;
    while (value >= 10)
        value = value / 10, n ++; 
    return n+1;
}

/**
 * Count required length of string for textual presentation of specified value. 
 *
 * @param value         ASN value.   
 *
 * @return length of requiered string. 
 */ 
size_t
asn_count_len_enum(const asn_value *value)
{
    unsigned int i;

    if (value == NULL) 
        return 0;
    /* Explicit discards of 'const' qualifier, whereas field 'txt_len' should 
     * have 'mutable' semantic */
    int *txt_len_p = (int *)&(value->txt_len);

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


/**
 * Count required length of string for textual presentation of specified value. 
 *
 * @param value         ASN value.   
 *
 * @return length of requiered string. 
 */ 
size_t
asn_count_len_octstring(const asn_value *value)
{ 
    int *txt_len_p;
    if (value == NULL)
        return 0;

    /* Explicit discards of 'const' qualifier, whereas field 'txt_len' should 
     * have 'mutable' semantic */
    txt_len_p = (int *)&(value->txt_len);

    if (value->syntax != OCT_STRING)
        return -1; 

    if (value->txt_len < 0)
        *txt_len_p = value->len * 3 + 3;

    return value->txt_len;
}

static char t_class[4][30] = {"UNIVERSAL ", "APPLICATION ", "", "PRIVATE "};

/**
 * Count required length of string for textual presentation of specified value. 
 *
 * @param value         ASN value.   
 * @param indent        current indent
 *
 * @return length of requiered string. 
 */ 
size_t
asn_count_len_tagged(const asn_value *value, unsigned int indent)
{
    int all_used = 0; 

    if (value == NULL)
        return 0; 
    /* Explicit discards of 'const' qualifier, whereas field 'txt_len' should 
     * have 'mutable' semantic */
    int *txt_len_p = (int *)&(value->txt_len);

    /* syntaxes processed in this method may have arbitrary last two bits*/
    if (value->syntax != TAGGED) 
        return -1; 

    if (value->txt_len < 0)
    {
        asn_value_p v_el = value->data.array[0];
        if (v_el)
        { 
            all_used += strlen (t_class[(int)value->tag.cl]); 
            all_used += number_of_digits(value->tag.val);
            all_used += 3; /* square braces and space after tag */ 
            all_used += asn_count_txt_len(v_el, indent);
        } 
        *txt_len_p = all_used;
    }

    return value->txt_len;
}

/**
 * Count required length of string for textual presentation of specified value. 
 *
 * @param value         ASN value.   
 * @param indent        current indent
 *
 * @return length of requiered string. 
 */ 
size_t
asn_count_len_choice(const asn_value *value, unsigned int indent)
{
    int all_used = 0; 
    asn_value_p v_el;
    if (value == NULL)
        return 0; 

    /* Explicit discards of 'const' qualifier, whereas field 'txt_len' should 
     * have 'mutable' semantic */
    int *txt_len_p = (int *)&(value->txt_len);

    /* syntaxes processed in this method may have arbitrary last two bits*/
    if (value->syntax != CHOICE) 
        return -1; 
    
    if (value->txt_len < 0)
    {
        v_el = value->data.array[0];
        if (v_el)
        { 
            all_used += strlen(v_el->name) + 1; 
            all_used += asn_count_txt_len(v_el, indent);
        } 
        *txt_len_p = all_used;
    }

    return value->txt_len;
}


/**
 * Count required length of string for textual presentation of specified value. 
 *
 * @param value         ASN value.   
 *
 * @return length of requiered string. 
 */ 
size_t
asn_count_len_objid(const asn_value *value)
{ 
    if (value == NULL) 
        return 0; 
    /* Explicit discards of 'const' qualifier, whereas field 'txt_len' should 
     * have 'mutable' semantic */
    int *txt_len_p = (int *)&(value->txt_len);

    if (value->syntax != OID) 
        return -1; 
    
    if (value->txt_len < 0)
    {
        int all_used = 2; /* braces */
        int *subid;
        unsigned int i;

        subid = (int*) value->data.other; 

        for (i = 0; i < value->len; i++)
        { 
            all_used += number_of_digits (subid[i]) + 1;
        } 
        *txt_len_p = all_used;
    }

    return value->txt_len;
}





/**
 * Prepare textual ASN.1 presentation of passed value ENUMERATED 
 * and put it into specified buffer. 
 *
 * @param value         ASN value to be printed, should have ENUMERATED type.  
 * @param buffer        buffer for ASN text.  
 * @param buf_len       length of buffer. 
 *
 * @return number characters written to buffer or -1 if error occured. 
 */ 
int
asn_sprint_enum(const asn_value *value, char *buffer, size_t buf_len)
{
    int used;
    unsigned int i;
    const char * val_label;

    if ((value == NULL) || (buffer == NULL) || (buf_len == 0) )
        return 0;

    if (value->syntax != ENUMERATED)
        return -1;

    if (buf_len <= asn_count_len_enum(value))
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
        used = sprintf (buffer, "%s", val_label);
    else
        used = sprintf (buffer, "%d", value->data.integer);

    return used;
}




/**
 * Prepare textual ASN.1 presentation of passed value INTEGER 
 * and put it into specified buffer. 
 *
 * @param value         ASN value to be printed, should have INTEGER type.  
 * @param buffer        buffer for ASN text.  
 * @param buf_len       length of buffer. 
 *
 * @return number characters written to buffer or -1 if error occured. 
 */ 
int
asn_sprint_integer(const asn_value *value, char *buffer, size_t buf_len)
{
    char loc_buf[16];
    unsigned int used;

    if ((value == NULL) || (buffer == NULL) || (buf_len == 0) )
        return 0;

    if (value->syntax != INTEGER)
        return -1;

    if ((int)buf_len <= value->txt_len)
	return -1;

    used = sprintf (loc_buf, "%d", value->data.integer);

    if (used > buf_len) 
        return -1;

    strncpy (buffer, loc_buf, buf_len);

    return used;
}

/**
 * Prepare textual ASN.1 presentation of passed value Character String 
 * and put it into specified buffer. 
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
    char * string;

    if ((value == NULL) || (buffer == NULL) || (buf_len == 0) )
        return 0;

    if (value->syntax != CHAR_STRING)
        return -1;

    if ((int)buf_len <= value->txt_len)
	return -1;

    string = value->data.other;

    strcpy  (buffer, "\""); 
    if (string)
        strncat (buffer, string, buf_len-1); 
    strcat  (buffer, "\""); 

    return strlen(buffer);
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
    char *pb = buffer;
    unsigned int i;

    if ((value == NULL) || (buffer == NULL) || (buf_len == 0) )
        return 0;

    if (value->syntax != OCT_STRING)
        return -1;

    if (buf_len <= asn_count_len_octstring(value))
	return -1;

    strcpy  (buffer, "\'"); 
    pb++;
    for (i = 0; i < value->len; i++)
        pb += sprintf (pb, "%02X ", ((uint8_t*)value->data.other)[i]);

    strcat  (pb, "\'H"); 

    return strlen(buffer);
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
    int all_used = 0, used;

    asn_value_p v_el;

    if ((value == NULL) || (buffer == NULL) || (buf_len == 0) )
        return 0; 

    /* syntaxes processed in this method may have arbitrary last two bits*/
    if (value->syntax != TAGGED) 
        return -1; 
    
    if (buf_len <= asn_count_len_tagged(value, indent))
	return -1;

    v_el = value->data.array[0];
    if (v_el)
    { 
        char t_class[4][30] = {"UNIVERSAL ", "APPLICATION ", "", "PRIVATE "};

        used = sprintf (buffer, "[%s%d]", 
                        t_class[(int)value->tag.cl], value->tag.val); 
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
    int all_used = 0, used;

    asn_value_p v_el;

    if ((value == NULL) || (buffer == NULL) || (buf_len == 0) )
        return 0; 

    /* syntaxes processed in this method may have arbitrary last two bits*/
    if (value->syntax != CHOICE) 
        return -1; 
    
    if (buf_len <= asn_count_len_choice(value, indent))
	return -1;

    v_el = value->data.array[0];
    if (v_el)
    { 
        strcpy (buffer, v_el->name);
        strcat (buffer, ":");

        used = strlen(buffer);
        all_used += used; buffer += used; 

        used = asn_sprint_value(v_el, buffer, buf_len - all_used, indent);
        all_used += used; 
    } 

    return all_used;
}


/**
 * Prepare textual ASN.1 presentation of passed value of OID type and put it 
 * into specified buffer. 
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
    int all_used = 0, used;
    unsigned int i;
    int *subid;

    if ((value == NULL) || (buffer == NULL) || (buf_len == 0) )
        return 0; 

    if (value->syntax != OID) 
        return -1; 
    
    if (buf_len <= asn_count_len_objid(value))
	return -1;

    strcpy (buffer, "{");

    all_used ++ ; buffer ++;

    subid = (int*) value->data.other; 

    for (i = 0; i < value->len; i++)
    { 
        used = sprintf (buffer, "%d", subid[i]);
        all_used += used; buffer += used;
        strcat (buffer, " ");
        all_used ++ ; buffer ++;
    } 
    strcat (buffer, "}"); 
    all_used++;

    return all_used;
}


/**
 * Prepare textual ASN.1 presentation of passed value of complex type with many
 * subvalues (i.e. 'SEQUENCE[_OF]' and 'SET[_OF]') and put it into specified buffer. 
 *
 * @param value         ASN value to be printed, should have INTEGER type.  
 * @param buffer        buffer for ASN text.  
 * @param buf_len       length of buffer. 
 * @param indent        current indent
 *
 * @return number characters written to buffer or -1 if error occured. 
 */ 
int
asn_sprint_array_fields(const asn_value *value, char *buffer, size_t buf_len, 
                        unsigned int indent)
{
    int all_used = 0, used;
    unsigned int i;
    int was_element = 0;
    
    char *str_indent = malloc (indent + 3);

    if ((value == NULL) || (buffer == NULL) || (buf_len == 0) )
        return 0; 

    /* syntaxes processed in this method may have arbitrary last two bits*/
    if ((value->syntax & ((-1)<<2) )!= SEQUENCE) 
        return -1; 
    
    if (buf_len <= asn_count_txt_len(value, indent))
	return -1;

    /* strcpy (buffer, asn_print_current_indent); */
    strcpy (buffer, "{");

    used = strlen(buffer);
    all_used += used; buffer += used;

    /* set indent string */
    indent += 2;
    for (i = 0; i < indent; i++)  
        str_indent[i] = ' ';
    str_indent[i] = '\0';

    for (i = 0; i < value->len; i++)
    { 
        asn_value_p v_el = value->data.array[i];
        if (v_el)
        {
            if (was_element) 
                strcat(buffer, ",\n"); 
            else 
                strcat(buffer, "\n"); 

            strcat (buffer, str_indent); 
            if ( ! (value->syntax & 1) )
            { /* We have structure with named components. */
                strcat (buffer, v_el->name);
                strcat (buffer, " ");
            }

            used = strlen(buffer);
            all_used += used; buffer += used; 

            used = asn_sprint_value(v_el, buffer, buf_len - all_used, indent);
            all_used += used; buffer += used;

            was_element = 1;
        }
    } 
    strcat(buffer, "\n"); 

    /* decrease indent */
    str_indent[indent - 2] = '\0';

    strcat (buffer, str_indent);
    strcat (buffer, "}");

    used = strlen(buffer);
    all_used += used;

    free (str_indent);
    return all_used;
}

/**
 * Prepare textual ASN.1 presentation of passed value and put it into specified
 * buffer. 
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

    if (buf_len <= asn_count_txt_len(value, indent))
	return -1;

    switch (value->syntax)
    {
    case BOOL:
        if (value->data.integer) 
            return sprintf (buffer, "TRUE");
        else
            return sprintf (buffer, "FALSE");

    case INTEGER:
        return asn_sprint_integer(value, buffer, buf_len);

    case ENUMERATED: 
        return asn_sprint_enum(value, buffer, buf_len);

    case CHAR_STRING:
        return asn_sprint_charstring(value, buffer, buf_len);

    case OCT_STRING:
        return asn_sprint_octstring(value, buffer, buf_len);

    case PR_ASN_NULL:
        return sprintf (buffer, "NULL");

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
 * Count required length of string for textual presentation of specified value. 
 *
 * @param value         ASN value.   
 * @param indent        current indent
 *
 * @return length of requiered string. 
 */ 
size_t 
asn_count_txt_len(const asn_value *value, unsigned int indent) 
{ 
    if (value == NULL) return 0;
    switch (value->syntax)
    {
    case BOOL:
        if (value->data.integer) 
            return 4;
        else
            return 5;

    case INTEGER:
        return value->txt_len;

    case ENUMERATED: 
        return asn_count_len_enum(value);

    case CHAR_STRING:
        return value->txt_len;

    case OCT_STRING:
        return asn_count_len_octstring(value);

    case PR_ASN_NULL:
        return 4;

    case LONG_INT:
    case BIT_STRING:
    case REAL:
        return 0; /* not implemented yet.*/
    case OID:
        return asn_count_len_objid(value);
    case CHOICE:
        return asn_count_len_choice(value, indent) + 10;

    case TAGGED:
        return asn_count_len_tagged(value, indent) + 10;

    case SEQUENCE:
    case SEQUENCE_OF:
    case SET:
    case SET_OF:
        return asn_count_len_array_fields(value, indent) + 10;

    default: 
        return 0; /* nothing to do. */
    }
}


/**
 * Count required length of string for textual presentation of specified value. 
 *
 * @param value         ASN value.   
 * @param indent        current indent
 *
 * @return length of requiered string. 
 */ 
size_t
asn_count_len_array_fields(const asn_value *value, unsigned int indent)
{ 
    if (value == NULL)
        return 0; 
    /* Explicit discards of 'const' qualifier, whereas field 'txt_len' should 
     * have 'mutable' semantic */
    int *txt_len_p = (int *)&(value->txt_len);

    /* syntaxes processed in this method may have arbitrary last two bits*/
    if ((value->syntax & ((-1)<<2) )!= SEQUENCE) 
        return -1; 
    
    if (value->txt_len < 0)
    {
        unsigned int i;
        int all_used = 1; /* for "{" */
        int elems = 0;

        for (i = 0; i < value->len; i++)
        { 
            asn_value_p v_el = value->data.array[i];
            if (v_el)
            { 
                if ( !(value->syntax & 1) )
                { 
                    all_used += strlen (v_el->name) + 1;
                } 
                all_used += asn_count_txt_len(v_el, indent + 2); 
                elems ++;
            }
        } 
        all_used +=   elems * (indent + 2)      /* indents before subvalues */
                    + elems + 1                 /* new line symbols */
                    + (elems ? (elems - 1) : 0) /* commas */    
                    + indent + 1;               /* closing brace wint indent */

        *txt_len_p = all_used;
    }

    return value->txt_len;
}


/**
 * Prepare textual ASN.1 presentation of passed value and save this string to
 * file with specified name. If file already exists, it will be overwritten.
 *
 * @param value         ASN value to be saved. 
 * @param filename      name of file 
 *
 * @return zero on success, otherwise error code.
 */ 
int
asn_save_to_file(const asn_value *value, const char *filename)
{
    FILE *fp;
    int len = asn_count_txt_len(value, 0);
    char * buffer;

    fp = fopen(filename, "w+");
    if (fp == NULL) 
        return errno;

    buffer = malloc(len + 11);
    if (buffer == NULL) 
        return ENOMEM; 
    memset(buffer, 0, len+11);
    
    asn_sprint_value(value, buffer, len + 10, 0);
    fwrite(buffer, strlen(buffer), 1, fp);

    fclose(fp);

    free(buffer);

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
int 
asn_parse_dvalue_in_file(const char *filename, const asn_type * type, 
                                asn_value_p *parsed_value, int *syms_parsed) 
{
    struct stat fst;

    char * buf;
    int    fd;
    int    read_sz;
    int    rc;

    if (stat (filename, &fst) != 0)
        return errno; 
    buf = malloc (fst.st_size + 1); 
    fd = open (filename, O_RDONLY);
    if (fd < 0)
        return errno; 

    read_sz = read(fd, buf, fst.st_size + 1);
    close (fd);

    rc = asn_parse_value_text(buf, type, parsed_value, syms_parsed);

    free(buf);

    return rc;
}
