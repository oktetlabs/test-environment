/** @file
 * @brief Test Environment: 
 *
 * Traffic Application Domain Command Handler
 * Implementation of some common useful utilities for TAD.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>

/* for ntohs, etc */
#include <netinet/in.h> 

#define TE_LGR_USER     "TAD CH"
#include "logger_ta.h"

#include "tad.h" 
#include "ndn.h" 


/**
 * Description see in tad.h
 */
tad_payload_type 
tad_payload_asn_label_to_enum(const char *label)
{
    if (strcmp(label, "function") == 0)
        return TAD_PLD_FUNCTION;
    else if (strcmp(label, "bytes") == 0)
        return TAD_PLD_BYTES;
    else if (strcmp(label, "length") == 0)
        return TAD_PLD_LENGTH;

    return TAD_PLD_UNKNOWN;
}

/**
 * Description see in tad.h
 */
int 
tad_confirm_pdus(csap_p csap_descr, asn_value *pdus)
{
    int level;
    int rc = 0;

    for (level = 0; (rc == 0) && (level < csap_descr->depth); level ++)
    { 
        char label[40];
        csap_spt_type_p csap_spt_descr; 
        asn_value * level_pdu;
        csap_spt_descr = find_csap_spt(csap_descr->proto[level]);

        snprintf(label, sizeof(label), "%d.#%s", 
                level, csap_descr->proto[level]);

        /* TODO: rewrite with more fast ASN method, that methods should 
         * be prepared and tested first */
        rc  = asn_get_subvalue(pdus, (const asn_value **)&level_pdu, label);

        if (rc) 
        {
            ERROR("asn_write_ind rc: %x, write for level %d", 
                    rc, level);
            break;
        }

        rc = csap_spt_descr->confirm_cb(csap_descr->id, level, level_pdu);
        VERB("confirm rc: %d", rc);

        if (rc)
        {
            ERROR(
                       "template does not confirm to CSAP; "
                       "rc: 0x%x, csap id: %d, level: %d\n", 
                       rc, csap_descr->id, level);
            break;
        }
    }

    return rc;
}





/**
 * Generic method to match data in incoming packet with DATA_UNIT pattern
 * field. If data matches, it will be written into respective field in pkt_pdu. 
 * Label of field should be provided by user. If pattern has "plain"
 * type, data will be simply compared. If it is integer, data will be
 * converted from "network byte order" to "host byte order" before matching. 
 *
 * @param pattern_pdu   ASN value with pattern PDU. 
 * @param pkt_pdu       ASN value with parsed packet PDU, may be NULL 
 *                      if parsed packet is not need (OUT). 
 * @param data          binary data to be matched.
 * @param d_len         length of data packet to be matched, in bytes. 
 * @param label         textual label of desired field.
 *
 * @return zero on success (that means "data matches to the pattern"),
 *              otherwise error code. 
 */
int 
tad_univ_match_field (const tad_data_unit_t *pattern, asn_value *pkt_pdu, 
                         uint8_t *data, size_t d_len, const char *label)
{ 
    char labels_buffer[200];
    int  rc = 0;
    int  user_int;
 
    strcpy(labels_buffer, label);
    strcat(labels_buffer, ".#plain");

    if (data == NULL)
        return ETEWRONGPTR; 

    VERB("label '%s', du type %d", label, pattern->du_type);

    switch (pattern->du_type)
    {
        case TAD_DU_I32:
        /* TODO: case TAD_DU_I64: */
        case TAD_DU_INT_NM:
        case TAD_DU_INTERVALS:
            switch (d_len)
            {
                case 2: 
                    user_int = ntohs(*((uint16_t *)data));
                    break;
                case 4: 
                    user_int = ntohl(*((uint32_t *)data));
                    break;
                case 8: 
                    return ETENOSUPP;
                default:
                    user_int = *data;
            }
            VERB("user int: %d", user_int);
            break;
        default:
            break;
    } 

    switch (pattern->du_type)
    {
        case TAD_DU_I32:
            VERB("pattern int: %d", pattern->val_i32);
            if (user_int == pattern->val_i32)
            {
                VERB(
                        "univ_match of %s; INT, data IS matched\n", 
                        labels_buffer);
                if (pkt_pdu)
                    rc = asn_write_value_field(pkt_pdu, &user_int,
                                      sizeof(user_int), labels_buffer);
            }
            else
            { 
                rc = ETADNOTMATCH;
            }
            break;

        case TAD_DU_INTERVALS:
            {
                unsigned int i;
                F_VERB("intervals\n");

                for (i = 0; i < pattern->val_intervals.length; i++)
                {
                    if (user_int >= pattern->val_intervals.begin[i] &&
                        user_int <= pattern->val_intervals.end[i] )
                        break;
                }
                F_VERB("after loop: i %d, u_i %d", 
                        i, user_int);
                if (i == pattern->val_intervals.length)
                    rc = ETADNOTMATCH;
                else if (pkt_pdu)
                    rc = asn_write_value_field(pkt_pdu, &user_int,
                                      sizeof(user_int), labels_buffer);
                F_VERB("intervals rc %x", rc);
            }
            break;

        case TAD_DU_STRING:
            if (strncmp(data, pattern->val_string, d_len) == 0)
            { 
                F_VERB("univ_match; data IS matched\n");
                if (pkt_pdu)
                    rc = asn_write_value_field(pkt_pdu, data, d_len, 
                                            labels_buffer);
            }
            else
            { 
                rc = ETADNOTMATCH;
            }
            break;

        case TAD_DU_DATA:
            if (d_len == pattern->val_mask.length && 
                memcmp(data, pattern->val_mask.pattern, d_len) == 0)
            { 
                F_VERB("univ_match; data IS matched\n");
                if (pkt_pdu)
                    rc = asn_write_value_field(pkt_pdu, data, d_len, 
                                            labels_buffer);
            }
            else
            { 
                rc = ETADNOTMATCH;
            }
            break;

        case TAD_DU_MASK:
            if (d_len != pattern->val_mask.length)
                rc = ETADNOTMATCH;
            else
            {
                unsigned n;
                const uint8_t *d = data; 
                const uint8_t *m = pattern->val_mask.mask; 
                const uint8_t *p = pattern->val_mask.pattern;
                for (n = 0; n < d_len; n++, d++, p++, m++)
                    if ((*d & *m) != (*p & *m))
                    {
                        rc = ETADNOTMATCH;
                        break;
                    }

                if (pkt_pdu)
                    rc = asn_write_value_field(pkt_pdu, data, d_len, 
                                            labels_buffer);
            }
            break;
        case TAD_DU_INT_NM:
            if (pkt_pdu)
                rc = asn_write_value_field(pkt_pdu, 
                            &user_int, sizeof(user_int), labels_buffer);
            break;

        case TAD_DU_DATA_NM:
            if (pkt_pdu)
                rc = asn_write_value_field(pkt_pdu, data, d_len, labels_buffer); 
            break;

        default:
            rc = ETENOSUPP;
    }
    return rc;
}





/**
 * Parse textual presentation of expression. 
 * Syntax is very restricted Perl-like, references to template arguments 
 * noted as $1, $2, etc. All (sub)expressions except simple constants  and 
 * references to vaiables should be in parantheses, no priorities of 
 * operations are detected. 
 *
 * @param string        text with expression. 
 * @param expr          place where pointer to new expression will be put (OUT).
 *
 * @return status code.
 */
int 
tad_int_expr_parse(const char *string, tad_int_expr_t **expr, int *syms)
{
    const char *p = string;
    int rc = 0;

    if (string == NULL || expr == NULL || syms == NULL)
        return ETEWRONGPTR;

    if ((*expr = calloc(1, sizeof(tad_int_expr_t))) == NULL)
        return ENOMEM;

    *syms = 0; 

    while (isspace(*p)) p++;

    if (*p == '(') 
    {
        tad_int_expr_t *sub_expr;
        int sub_expr_parsed = 0;
        p++;


        while (isspace(*p)) p++; 
        if (*p == '-')
        {
            (*expr)->n_type = EXP_U_MINUS;
            (*expr)->d_len = 1; 
            p++;
            while (isspace(*p)) p++; 
        }
        else
        { 
            (*expr)->n_type = EXP_ADD;
            (*expr)->d_len = 2;
        }

        (*expr)->exprs = calloc((*expr)->d_len, sizeof(tad_int_expr_t));

        rc = tad_int_expr_parse(p, &sub_expr, &sub_expr_parsed);
        if (rc)
            goto parse_error;

        p += sub_expr_parsed; 
        memcpy((*expr)->exprs, sub_expr, sizeof(tad_int_expr_t)); 
        free(sub_expr);
        while (isspace(*p)) p++; 

        if ((*expr)->d_len > 1)
        { 
            switch (*p)
            {
                case '+': (*expr)->n_type = EXP_ADD;    break;
                case '-': (*expr)->n_type = EXP_SUBSTR; break;
                case '*': (*expr)->n_type = EXP_MULT;   break;
                case '/': (*expr)->n_type = EXP_DIV;    break;
                default: goto parse_error;
            }
            p++;

            while (isspace(*p)) p++; 

            rc = tad_int_expr_parse(p, &sub_expr, &sub_expr_parsed);
            if (rc)
                goto parse_error;

            p += sub_expr_parsed; 
            memcpy((*expr)->exprs + 1, sub_expr, sizeof(tad_int_expr_t)); 
            free(sub_expr);
        }
        while (isspace(*p)) p++; 

        if (*p != ')')
            goto parse_error;
    }
    else if (isdigit(*p)) /* integer constant */
    {
        int base = 10;
        int64_t val = 0;
        char *endptr;

        (*expr)->n_type = CONSTANT;

        if (*p == '0') /* zero, or octal/hexadecimal number. */
        {
            p++;
            if (isdigit(*p))
                base = 8;
            else if (*p == 'x')
            {
                p++;
                base = 16;
            }
        }

        if (isxdigit(*p))
        { 
            val = strtoll(p, &endptr, base); 
            p = endptr;
        }

        if (val > LONG_MAX || val < LONG_MIN)
        {
            (*expr)->d_len = 8;
            (*expr)->val_i64 = val;
        }
        else
        {
            (*expr)->d_len = 4;
            (*expr)->val_i32 = val;
        }
        *syms = p - string; 
    }
    else if (*p == '$') 
    {
        p++;
        if (isdigit(*p))
        {
            char *endptr;
            (*expr)->n_type = ARG_LINK;
            (*expr)->arg_num = strtol(p, &endptr, 10);
            *syms = endptr - string; 
        }
        else
            goto parse_error;
    }
    else 
        goto parse_error;

    return 0;

parse_error:
    if (rc == 0)
        rc = ETADEXPRPARSE;

    tad_int_expr_free(*expr);
    *expr = NULL;

    if (*syms == 0)
        *syms = p - string;
    return rc;
}

static void
tad_int_expr_free_subtree(tad_int_expr_t *expr)
{
    if (expr->n_type != CONSTANT && 
        expr->n_type != ARG_LINK    ) 
    {
        unsigned i;
        for (i = 0; i < expr->d_len; i++)
            tad_int_expr_free_subtree(expr->exprs + i);

        free (expr->exprs);
    }
}

/**
 * Free data allocated for expression. 
 */
void 
tad_int_expr_free(tad_int_expr_t *expr)
{
    if(expr == NULL)
        return; 
    tad_int_expr_free_subtree(expr); 
    free(expr);
}

/**
 * Calculate value of expression as function of argument set
 *
 * @param expr          expression structure.
 * @param args          array with arguments.
 * @param result        location for result (OUT).
 *
 * @return status code.
 */
int 
tad_int_expr_calculate(const tad_int_expr_t *expr, 
                      const tad_template_arg_t *args, int64_t *result)
{
    int rc;

    if (expr == NULL || result == NULL)
        return ETEWRONGPTR; 

    switch (expr->n_type)
    {
        case CONSTANT:
            if (expr->d_len == 8)
                *result = expr->val_i64;
            else
                *result = expr->val_i32;
            break;

        case ARG_LINK:
            if (args == NULL)
                return ETEWRONGPTR;

            if (args[expr->arg_num].type != ARG_INT)
                return EINVAL;
            *result = args[expr->arg_num].arg_int;
            break;

        default: 
            {
                int64_t r1, r2;
                rc = tad_int_expr_calculate(expr->exprs, args, &r1);
                if (rc) 
                    return rc;

                if (expr->n_type != EXP_U_MINUS)
                {
                    rc = tad_int_expr_calculate(expr->exprs + 1, args, &r2);
                    if (rc) 
                        return rc;
                }

                switch (expr->n_type)
                {
                    case EXP_ADD:
                        *result = r1 + r2; break;
                    case EXP_SUBSTR:
                        *result = r1 - r2; break;
                    case EXP_MULT:
                        *result = r1 * r2; break;
                    case EXP_DIV:
                        *result = r1 / r2; break;
                    case EXP_U_MINUS:
                        *result = - r1; break;
                    default:
                        return EINVAL;
                }
            }
    }

    return 0;
}


/**
 * Initialize tad_int_expr_t structure with single constant value.
 *
 * @param n     value.
 *
 * @return pointer to new tad_int_expr_r structure.
 */
tad_int_expr_t *
tad_int_expr_constant(int64_t n)
{
    tad_int_expr_t *ret = calloc(1, sizeof(tad_int_expr_t));

    if (ret == NULL) 
        return NULL; 

    ret->n_type = CONSTANT;
    ret->d_len = 8;

    ret->val_i64 = n;

    return ret;
}

/**
 * Initialize tad_int_expr_t structure with single constant value, storing
 * binary array up to 8 bytes length. Array is assumed as "network byte order"
 * and converted to "host byte order" while saveing in 64-bit integer.
 *
 * @param arr   pointer to binary array.
 * @param len   length of array.
 *
 * @return  pointer to new tad_int_expr_r structure, or NULL if no memory 
 *          or too beg length passed.
 */
tad_int_expr_t *
tad_int_expr_constant_arr(uint8_t *arr, size_t len)
{
    tad_int_expr_t *ret;
    int64_t val = 0;

    if (len > sizeof(int64_t))
        return NULL;
            
    ret = calloc(1, sizeof(tad_int_expr_t));

    if (ret == NULL) 
        return NULL; 

    memcpy (((uint8_t *)&val) + sizeof(uint64_t) - len, arr, len);

    ret->n_type = CONSTANT;
    ret->d_len = 8;

    ret->val_i64 = tad_ntohll(val);

    return ret;

}


/**
 * Convert 64-bit integer from network order to the host and vise versa. 
 *
 * @param n     integer to be converted.
 *
 * @return converted integer
 */
uint64_t 
tad_ntohll(uint64_t n)
{ 
    uint16_t test_val = 0xff00;

    if (test_val != ntohs(test_val))
    { /* byte swap needed. */
        uint64_t ret = ntohl((uint32_t)(n & 0xffffffff));
        uint32_t h = ntohl((uint32_t)(n >> 32));

        ret <<= 32;
        ret |= h;

        return ret;
    }
    else 
        return n;
}



/**
 * Convert DATA-UNIT ASN field to plain C structure.
 * Memory need to store dynamic data, is allocated in this method and
 * should be freed by user. 
 *
 * @param pdu_val       ASN value with pdu, which DATA-UNIT field should be 
 *                      converted.
 * @param label         label of DATA_UNIT field in PDU.
 * @param location      location for converted structure (OUT). 
  *
 * @return zero on success or error code. 
 */ 
int 
tad_data_unit_convert(const asn_value *pdu_val, const char *label, 
                                 tad_data_unit_t *location)
{
    char choice[20]; 
    const asn_value *du_field;
    int rc;

    if (pdu_val == NULL || label == NULL || location == NULL)
        return ETEWRONGPTR;

    rc = asn_get_subvalue(pdu_val, &du_field, label);

    VERB("get subvalue %s rc %x", label, rc);

    if (rc)
    {
        const asn_type *type = asn_get_type(pdu_val);
        const asn_type *s_type = NULL; 
        char labels_buffer[200];
        asn_syntax_t plain_syntax;
     
        strcpy(labels_buffer, label);
        strcat(labels_buffer, ".#plain");
 
        rc = asn_get_subtype(type, &s_type, labels_buffer);
        if (rc)
        {
            ERROR(
                    "get subtype %s in pattern rc %x", labels_buffer, rc);
            return rc;
        }

        plain_syntax = asn_get_syntax_of_type(s_type); 
        switch (plain_syntax)
        {
            case BOOL:
            case INTEGER:
            case ENUMERATED:
                location->du_type = TAD_DU_INT_NM;
                break; 
            case BIT_STRING:
            case OCT_STRING:
            case CHAR_STRING:
                location->du_type = TAD_DU_DATA_NM;
                break; 
            default: 
                return ETENOSUPP;
        }
        return 0;
    } 

    rc = asn_get_choice(pdu_val, label, choice, sizeof(choice));
    if (rc)
    {
        F_ERROR("rc from get choice: %x", rc);
        return rc;
    }

    if (strcmp(choice, "plain") == 0)
    {
        asn_syntax_t plain_syntax = asn_get_syntax(du_field, "");

        switch (plain_syntax)
        {
            case BOOL:
            case INTEGER:
            case ENUMERATED:
                {
                    int val_len = sizeof (location->val_i32);
                    location->du_type = TAD_DU_I32; 
                    rc = asn_read_value_field(du_field, 
                            &location->val_i32, &val_len, "");
                    return rc;
                }
            case BIT_STRING:
            case OCT_STRING:
                location->du_type = TAD_DU_DATA;
                /* get data later */
                break;

            case CHAR_STRING:
                location->du_type = TAD_DU_STRING;
                /* get data later */
                break;

            case LONG_INT:
            case REAL:
            case OID:
                ERROR(
                        "No yet support for syntax %d", plain_syntax);
                return ETENOSUPP;

            default: 
                ERROR(
                        "Strange syntax %d", plain_syntax);
                return EINVAL;

        }
    }
    else if (strcmp(choice, "script") == 0)
    {
        const uint8_t *script;

        rc = asn_get_field_data(du_field, &script, "");
        if (rc == 0)
        {
            char expr_label[] = "expr:";
            if (strncmp(expr_label, script,
                        sizeof(expr_label)-1) == 0)
            {
                tad_int_expr_t *expression;
                int syms;

                rc = tad_int_expr_parse(
                        script + sizeof(expr_label) - 1,
                        &expression, &syms);
                if (rc)
                {
                    ERROR(
                        "expr script parse error %x, syms %d",
                        rc, syms);
                    return rc;
                }
                location->du_type = TAD_DU_EXPR;
                location->val_int_expr = expression;

                return 0;
            }
            else
            {
                ERROR("not supported type of script");
                return ETENOSUPP;
            }
        } 
        else
        {
            ERROR("rc from asn_get for 'script': %x", rc);
            return rc;
        }
    }
    else if (strcmp(choice, "mask") == 0)
    {
        int m_len = asn_get_length(du_field, "v");
        location->du_type = TAD_DU_MASK; 

        location->val_mask.length  = m_len;
        location->val_mask.mask    = calloc(1, m_len);
        location->val_mask.pattern = calloc(1, m_len);
        if (location->val_mask.mask == NULL || 
            location->val_mask.pattern == NULL)
            return ENOMEM;

        rc = asn_read_value_field(du_field, location->val_mask.mask, 
                                 &m_len, "m");
        if (rc == 0)
        { 
            rc = asn_read_value_field(du_field, location->val_mask.pattern, 
                                     &m_len, "v");
        }
        if (rc)
        {
            free(location->val_mask.mask);
            free(location->val_mask.pattern);
            ERROR("rc from asn_read for 'mask': %x", rc);
            return rc;
        }
        return 0;
    }
    else if (strcmp(choice, "intervals") == 0)
    {
        int i, label_len, num;
        size_t v_len = sizeof(location->val_intervals.begin[0]);
        char label[50];

        location->du_type = TAD_DU_INTERVALS; 
        num = location->val_intervals.length = asn_get_length(du_field, ""); 
        location->val_intervals.begin = calloc(num, v_len);
        location->val_intervals.end   = calloc(num, v_len);

        rc = 0;
        for (i = 0; i < num; i++)
        {
            label_len = snprintf (label, sizeof(label), "%d.b", i);
            rc = asn_read_value_field(du_field, 
                            location->val_intervals.begin + i, &v_len,
                            label);
            if (rc)
                break;

            label[label_len - 1] = 'e';

            rc = asn_read_value_field(du_field, 
                            location->val_intervals.end + i, &v_len,
                            label);
            if (rc)
                break;
        }

        if (rc)
        {
            ERROR(
                    "error reading intervals #%d: %x, label <%s>", 
                    i, rc, label);
            free(location->val_intervals.begin);
            free(location->val_intervals.end);
            location->du_type = TAD_DU_INT_NM;
        }

        return rc;
    }
    else
    {
        ERROR("No support for choice: %s", choice);
        return ETENOSUPP;
    }

    /* process string values */ 
    {
        int len = asn_get_length (du_field, "");
        void *d_ptr;

        if (len <= 0)
        {
            ERROR("wrong length");
            return EINVAL;
        }

        if ((d_ptr = calloc(len, 1)) == NULL)
            return ENOMEM; 

        rc = asn_read_value_field(du_field, d_ptr, &len, "");
        if (rc)
        {
            free(d_ptr);
            ERROR(
                    "rc from asn_read for some string: %x", rc);
            return rc;
        } 

        if (location->du_type == TAD_DU_DATA)
        {
            location->val_mask.length = len;
            location->val_mask.mask = NULL;
            location->val_mask.pattern = d_ptr;
        }
        else
        {
            location->val_string = d_ptr;
        }
    }

    return 0;
}


/**
 * Clear data_unit structure, e.g. free data allocated for internal usage. 
 * Memory block used by data_unit itself is not freed!
 * 
 * @param du    pointer to data_unit structure to be cleared. 
 */
void 
tad_data_unit_clear(tad_data_unit_t *du)
{
    if (du == NULL)
        return;

    switch (du->du_type)
    {
    case TAD_DU_STRING:
        free(du->val_string);
        break;
    case TAD_DU_MASK:
        free(du->val_mask.mask);
        /* fall through */
    case TAD_DU_DATA:
        free(du->val_mask.pattern);
        break;
    case TAD_DU_EXPR:
        tad_int_expr_free(du->val_int_expr);
        break;
    default:
        /* do nothing */
        break;
    }

    memset (du, 0, sizeof(*du));
}


/**
 * Constract data-unit structure from specified binary data for 
 * simple per-byte compare. 
 *
 * @param data          binary data which should be compared.
 * @param d_len         length of data.
 * @param location      location of data-unit structure to be initialized (OUT)
 *
 * @return error status.
 */
int 
tad_data_unit_from_bin(const uint8_t *data, size_t d_len, 
                       tad_data_unit_t *location)
{
    if (data == NULL || location == NULL)
        return ETEWRONGPTR;

    if ((location->val_mask.pattern = malloc(d_len)) == NULL)
        return ENOMEM;

    location->du_type = TAD_DU_DATA;
    location->val_mask.length = d_len;
    location->val_mask.mask = NULL;

    memcpy(location->val_mask.pattern, data, d_len);

    return 0;
}



