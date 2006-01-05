/** @file
 * @brief TAD Utils
 *
 * Traffic Application Domain Command Handler.
 * Implementation of some common useful utilities for TAD.
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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

#define TE_LGR_USER     "TAD Utils"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>

/* for ntohs, etc */
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netinet/tcp.h>



#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_utils.h"
#include "asn_usr.h" 
#include "ndn.h" 
#include "logger_api.h"
#include "logger_ta_fast.h"


/**
 * Description see in tad_utils.h
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
 * Description see in tad_utils.h
 */
tad_payload_type 
tad_payload_asn_tag_to_enum(uint16_t tag)
{
    switch (tag)
    {
        case NDN_PLD_BYTES:  return TAD_PLD_BYTES;
        case NDN_PLD_FUNC:   return TAD_PLD_FUNCTION;
        case NDN_PLD_LEN:    return TAD_PLD_LENGTH;
        case NDN_PLD_STREAM: return TAD_PLD_STREAM;
        default: return TAD_PLD_UNKNOWN;
    }
}

/**
 * Description see in tad_utils.h
 */
int 
tad_confirm_pdus(csap_p csap, te_bool recv, asn_value *pdus,
                 void **layer_opaque)
{
    unsigned int layer;
    te_errno     rc = 0;

    rc = tad_check_pdu_seq(csap, pdus);

#if 0
    /* FIXME */
    assert(layer_opaque != NULL);
#endif
    for (layer = 0; (rc == 0) && (layer < csap->depth); layer++)
    { 
        csap_layer_confirm_pdu_cb_t  confirm_cb;
        char                         label[40];
        asn_value                   *layer_pdu;

        snprintf(label, sizeof(label), "%d.#%s", 
                layer, csap->layers[layer].proto);

        rc = asn_get_subvalue(pdus, (const asn_value **)&layer_pdu, label);

        if (rc != 0) 
        {
            ERROR("%s(CSAP %d): asn_get_subvalue rc %r, "
                  "confirm layer %d, label %s",
                  __FUNCTION__, csap->id, rc, layer, label);
            break;
        }

        confirm_cb = (recv) ?
            csap_get_proto_support(csap, layer)->confirm_ptrn_cb :
            csap_get_proto_support(csap, layer)->confirm_tmpl_cb;
        if (confirm_cb != NULL)
        {
            /* FIXME */
            rc = confirm_cb(csap, layer, layer_pdu,
                            (layer_opaque == NULL) ?
                                NULL : (layer_opaque + layer));
            VERB("confirm rc: %d", rc);

            if (rc != 0)
            {
                ERROR("pdus do not confirm to CSAP; "
                      "rc: %r, csap id: %d, layer: %d", 
                      rc, csap->id, layer);
                break;
            }
        }
    }

    return rc;
}



#if 0 /* This function is not used at all now, leave it in sources
         until respective NDN method will be complete. */


/**
 * Generic method to match data in incoming packet with DATA_UNIT pattern
 * field. If data matches, it will be written into respective field in 
 * pkt_pdu. 
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
 *
 * This function is depricated, and leaved here only for easy backward 
 * rollbacks. Use 'ndn_match_data_units' instead. 
 * This function will be removed just when 'ndn_match_data_units' will 
 * be completed and debugged.
 */
int 
tad_univ_match_field(const tad_data_unit_t *pattern, asn_value *pkt_pdu, 
                     uint8_t *data, size_t d_len, const char *label)
{ 
    char labels_buffer[200];
    int  rc = 0;
    int  user_int;
 
    strcpy(labels_buffer, label);
    strcat(labels_buffer, ".#plain");

    if (data == NULL)
        return TE_EWRONGPTR; 

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
                    return TE_EOPNOTSUPP;
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
                rc = TE_ETADNOTMATCH;
            }
            break;

        case TAD_DU_INTERVALS:
            {
                unsigned int i;
                F_VERB("intervals");

                for (i = 0; i < pattern->val_intervals.length; i++)
                {
                    if (user_int >= pattern->val_intervals.begin[i] &&
                        user_int <= pattern->val_intervals.end[i] )
                        break;
                }
                F_VERB("after loop: i %d, u_i %d", 
                        i, user_int);
                if (i == pattern->val_intervals.length)
                    rc = TE_ETADNOTMATCH;
                else if (pkt_pdu)
                    rc = asn_write_value_field(pkt_pdu, &user_int,
                                      sizeof(user_int), labels_buffer);
                F_VERB("intervals rc %r", rc);
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
                rc = TE_ETADNOTMATCH;
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
                rc = TE_ETADNOTMATCH;
            }
            break;

        case TAD_DU_MASK:
            if (d_len != pattern->val_mask.length)
                rc = TE_ETADNOTMATCH;
            else
            {
                unsigned n;

                const uint8_t *d = data; 
                const uint8_t *m = pattern->val_mask.mask; 
                const uint8_t *p = pattern->val_mask.pattern;

                for (n = 0; n < d_len; n++, d++, p++, m++)
                    if ((*d & *m) != (*p & *m))
                    {
                        rc = TE_ETADNOTMATCH;
                        break;
                    }

                if (pkt_pdu)
                    rc = asn_write_value_field(pkt_pdu, data, d_len, 
                                               labels_buffer);
            }
            break;
        case TAD_DU_INT_NM:
            if (pkt_pdu)
                rc = asn_write_value_field(pkt_pdu, &user_int, 
                                           sizeof(user_int), labels_buffer);
            break;

        case TAD_DU_DATA_NM:
            if (pkt_pdu)
                rc = asn_write_value_field(pkt_pdu, data, d_len, 
                                           labels_buffer); 
            break;

        default:
            rc = TE_EOPNOTSUPP;
    }
    return rc;
}


#endif


/**
 * Parse textual presentation of expression. 
 * Syntax is very restricted Perl-like, references to template arguments 
 * noted as $1, $2, etc. All (sub)expressions except simple constants  and 
 * references to vaiables should be in parantheses, no priorities of 
 * operations are detected. 
 *
 * @param string        text with expression. 
 * @param expr          location for pointer to new expression (OUT).
 * @param syms          location for number of parsed symbols (OUT).
 *
 * @return status code.
 */
int 
tad_int_expr_parse(const char *string, tad_int_expr_t **expr, int *syms)
{
    const char *p = string;
    int rc = 0;

    if (string == NULL || expr == NULL || syms == NULL)
        return TE_EWRONGPTR;

    VERB("%s <%s> called", __FUNCTION__, string);

    if ((*expr = calloc(1, sizeof(tad_int_expr_t))) == NULL)
        return TE_ENOMEM;

    *syms = 0; 

    while (isspace(*p))
        p++;

    if (*p == '(') 
    {
        tad_int_expr_t *sub_expr;
        int             sub_expr_parsed = 0;

        p++;


        while (isspace(*p)) 
            p++; 
        if (*p == '-')
        {
            (*expr)->n_type = TAD_EXPR_U_MINUS;
            (*expr)->d_len = 1; 
            p++;
            while (isspace(*p)) 
                p++; 
        }
        else
        { 
            (*expr)->n_type = TAD_EXPR_ADD;
            (*expr)->d_len = 2;
        }

        (*expr)->exprs = calloc((*expr)->d_len, sizeof(tad_int_expr_t));

        rc = tad_int_expr_parse(p, &sub_expr, &sub_expr_parsed);
        VERB("first subexpr parsed, rc %r, syms %d", rc, sub_expr_parsed);
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
                case '+':
                    (*expr)->n_type = TAD_EXPR_ADD;    
                    break;
                case '-': 
                    (*expr)->n_type = TAD_EXPR_SUBSTR;
                    break;
                case '*':
                    (*expr)->n_type = TAD_EXPR_MULT;
                    break;
                case '/':
                    (*expr)->n_type = TAD_EXPR_DIV;
                    break;
                case '%':
                    (*expr)->n_type = TAD_EXPR_MOD;
                    break;
                default: 
                    WARN("%s(): unknown op %d", __FUNCTION__, (int)*p);
                    goto parse_error;
            }
            p++;

            while (isspace(*p))
                p++; 

            rc = tad_int_expr_parse(p, &sub_expr, &sub_expr_parsed);
            VERB("second subexpr parsed, rc %r, syms %d",
                 rc, sub_expr_parsed);
            if (rc)
                goto parse_error;

            p += sub_expr_parsed; 
            memcpy((*expr)->exprs + 1, sub_expr, sizeof(tad_int_expr_t)); 
            free(sub_expr);
        }
        while (isspace(*p))
            p++; 

        if (*p != ')')
            goto parse_error;
        p++;

        *syms = p - string; 
    }
    else if (isdigit(*p)) /* integer constant */
    {
        int base = 10;
        int64_t val = 0;
        char *endptr;

        (*expr)->n_type = TAD_EXPR_CONSTANT;

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
            (*expr)->n_type = TAD_EXPR_ARG_LINK;
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
        rc = TE_ETADEXPRPARSE;

    tad_int_expr_free(*expr);
    *expr = NULL;

    if (*syms == 0)
        *syms = p - string;
    return rc;
}

static void
tad_int_expr_free_subtree(tad_int_expr_t *expr)
{
    if (expr->n_type != TAD_EXPR_CONSTANT &&
        expr->n_type != TAD_EXPR_ARG_LINK) 
    {
        unsigned i;

        for (i = 0; i < expr->d_len; i++)
            tad_int_expr_free_subtree(expr->exprs + i);

        free(expr->exprs);
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
                       const tad_tmpl_arg_t *args, size_t num_args,
                       int64_t *result) 
{
    int rc;

    if (expr == NULL || result == NULL)
        return TE_EWRONGPTR; 

    switch (expr->n_type)
    {
        case TAD_EXPR_CONSTANT:
            if (expr->d_len == 8)
                *result = expr->val_i64;
            else
                *result = expr->val_i32;
            break;

        case TAD_EXPR_ARG_LINK:
            {
                int ar_n = expr->arg_num;

                if (args == NULL)
                    return TE_EWRONGPTR;

                if (ar_n < 0 || ((size_t)ar_n) >= num_args)
                {
                    ERROR("%s(): wrong arg ref: %d, num of iter. args: %d",
                          __FUNCTION__, ar_n, num_args); 
                    return TE_ETADWRONGNDS;
                }

                if (args[ar_n].type != TAD_TMPL_ARG_INT)
                {
                    ERROR("%s(): wrong arg %d type: %d, not integer",
                          __FUNCTION__, ar_n, args[ar_n].type);
                    return TE_ETADWRONGNDS;
                }
                *result = args[ar_n].arg_int;
            }
            break;

        default: 
        {
            int64_t r1, r2;

            rc = tad_int_expr_calculate(expr->exprs, args, num_args, &r1);
            if (rc != 0) 
                return rc;

            /* there is only one unary arithmetic operation */
            if (expr->n_type != TAD_EXPR_U_MINUS)
            {
                rc = tad_int_expr_calculate(expr->exprs + 1, args,
                                            num_args, &r2);
                if (rc != 0) 
                    return rc;
            }

            switch (expr->n_type)
            {
                case TAD_EXPR_ADD:
                    *result = r1 + r2; 
                    break;
                case TAD_EXPR_SUBSTR:
                    *result = r1 - r2; 
                    break;
                case TAD_EXPR_MULT:
                    *result = r1 * r2; 
                    break;
                case TAD_EXPR_DIV:
                    *result = r1 / r2; 
                    break;
                case TAD_EXPR_MOD:
                    *result = r1 % r2; 
                    break;
                case TAD_EXPR_U_MINUS:
                    *result = - r1; 
                    break;
                default:
                    ERROR("%s(): unknown type of expr node: %d",
                          __FUNCTION__, expr->n_type);
                    return TE_EINVAL;
            }
        } /* end of 'default' sub-block */
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

    ret->n_type = TAD_EXPR_CONSTANT;
    ret->d_len = 8;

    ret->val_i64 = n;

    return ret;
}

/**
 * Initialize tad_int_expr_t structure with single constant value, storing
 * binary array up to 8 bytes length. Array is assumed as "network byte 
 * order" and converted to "host byte order" while saveing 
 * in 64-bit integer.
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
    int64_t         val = 0;

    if (len > sizeof(int64_t))
        return NULL;
            
    ret = calloc(1, sizeof(tad_int_expr_t));

    if (ret == NULL) 
        return NULL; 

    memcpy(((uint8_t *)&val) + sizeof(uint64_t) - len, arr, len);

    ret->n_type = TAD_EXPR_CONSTANT;
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


/* See description in tad_utils.h */ 
int
tad_data_unit_convert_by_label(const asn_value *pdu_val, 
                               const char *label, 
                               tad_data_unit_t *location)
{
    int         rc;
    asn_tag_t   tag;

    const asn_value *clear_pdu_val;
    const asn_type  *clear_pdu_type;

    if (pdu_val == NULL || label == NULL || location == NULL)
        return TE_EWRONGPTR;
    
    if (asn_get_syntax(pdu_val, "") == CHOICE)
    {
        if ((rc = asn_get_choice_value(pdu_val, &clear_pdu_val, NULL, NULL))
             != 0)
            return rc;
    }
    else
        clear_pdu_val = pdu_val; 

    clear_pdu_type = asn_get_type(clear_pdu_val);
    rc = asn_label_to_tag(clear_pdu_type, label, &tag);

    if (rc != 0)
    {
        ERROR("%s(): wrong label %s, ASN type %s", 
             __FUNCTION__, label, asn_get_type_name(clear_pdu_type));
        return rc;
    }

    rc = tad_data_unit_convert(clear_pdu_val, tag.val, location);

    return rc;
}



/* See description in tad_utils.h */ 
int 
tad_data_unit_convert(const asn_value  *pdu_val,
                      asn_tag_value     tag_value,
                      tad_data_unit_t  *location)
{
    int rc;

    const asn_value *ch_du_field;

    if (pdu_val == NULL || location == NULL)
        return TE_EWRONGPTR;

    rc = asn_get_child_value(pdu_val, &ch_du_field, PRIVATE, tag_value);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
        {
            tad_data_unit_clear(location);
            return 0;
        }
        else
        {
            ERROR("%s(tag %d, pdu name %s): rc from get_child %r",
                  __FUNCTION__, tag_value, asn_get_name(pdu_val), rc);
            return rc;
        }
    } 

    rc = tad_data_unit_convert_simple(ch_du_field, location);
    if (rc != 0)
        ERROR("%s(tag %d, pdu name %s): rc from get choice label: %r",
              __FUNCTION__, tag_value, asn_get_name(pdu_val), rc);
    return rc;
}

int
tad_data_unit_convert_simple(const asn_value *ch_du_field, 
                             tad_data_unit_t *location)
{
    int  rc;

    const asn_value *du_field;
    asn_tag_class    t_class;
    uint16_t         t_val; 
    asn_syntax       plain_syntax;

    if (ch_du_field == NULL || location == NULL)
        return TE_EWRONGPTR;

    if (location->du_type != TAD_DU_UNDEF)
        tad_data_unit_clear(location);

    rc = asn_get_choice_value(ch_du_field, &du_field, &t_class, &t_val); 
    if (rc != 0)
    {
        ERROR("%s(field name %s): rc from get choice: %r",
              __FUNCTION__, asn_get_name(ch_du_field), rc);
        return rc;
    }

    switch (t_val)
    {
        case NDN_DU_PLAIN:
            plain_syntax = asn_get_syntax(du_field, "");

            switch (plain_syntax)
            {
                case BOOL:
                case INTEGER:
                case ENUMERATED:
                    location->du_type = TAD_DU_I32; 
                    rc = asn_read_int32(du_field, &(location->val_i32), "");
                    if (rc != 0)
                        ERROR("%s(): read integer rc %r", __FUNCTION__, rc);
                    break;

                case BIT_STRING:
                case OCT_STRING:
                    location->du_type = TAD_DU_OCTS;
                    /* get data later */
                    break;

                case CHAR_STRING:
                    location->du_type = TAD_DU_STRING;
                    /* get data later */
                    break;

                case LONG_INT:
                case REAL:
                case OID:
                    ERROR("No yet support for syntax %d", plain_syntax);
                    return TE_EOPNOTSUPP;

                default: 
                    ERROR("%s(field name %s): strange syntax %d",
                          __FUNCTION__, asn_get_name(ch_du_field),
                          plain_syntax);
                    return TE_EINVAL;

            }

            /* process string values */ 
            if (location->du_type != TAD_DU_I32)
            {
                size_t  len = asn_get_length(du_field, "");
                void   *d_ptr;

                if (len <= 0)
                {
                    ERROR("wrong length");
                    return TE_EINVAL;
                }

                if ((d_ptr = calloc(len, 1)) == NULL)
                    return TE_ENOMEM; 

                rc = asn_read_value_field(du_field, d_ptr, &len, "");
                if (rc)
                {
                    free(d_ptr);
                    ERROR("rc from asn_read for some string: %r", rc);
                    return rc;
                } 

                location->val_data.len = len;

                if (location->du_type == TAD_DU_OCTS)
                    location->val_data.oct_str = d_ptr;
                else
                    location->val_data.char_str = d_ptr;
            }
            break;

        case NDN_DU_SCRIPT:
            {
                const uint8_t *script;
                char expr_label[] = "expr:";

                rc = asn_get_field_data(du_field, &script, "");
                if (rc != 0)
                {
                    ERROR("rc from asn_get for 'script': %r", rc);
                    return rc;
                }

                if (strncmp(expr_label, script, sizeof(expr_label)-1) == 0)
                {
                    tad_int_expr_t *expr;
                    const char     *expr_string = script +
                                                  sizeof(expr_label) - 1;
                    int             syms;

                    rc = tad_int_expr_parse(expr_string, &expr, &syms);
                    if (rc != 0)
                    {
                        ERROR("expr script parse error %r,"
                              " script '%s', syms %d",
                              rc, expr_string, syms);
                        return rc;
                    }
                    location->du_type = TAD_DU_EXPR;
                    location->val_int_expr = expr;
                }
                else
                {
                    ERROR("not supported type of script");
                    return TE_EOPNOTSUPP;
                }
            }
            break;

        default:
            WARN("%s(): No support for choice: tag %d at sending",
                 __FUNCTION__, t_val);
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
            free(du->val_data.char_str);
            break;

        case TAD_DU_OCTS:
            free(du->val_data.oct_str);
            break;

        case TAD_DU_EXPR:
            tad_int_expr_free(du->val_int_expr);
            break;

        default:
            /* do nothing */
            break;
    }

    memset(du, 0, sizeof(*du));
    du->du_type = TAD_DU_UNDEF;
}


/* See description in tad_utils.h */
int 
tad_data_unit_from_bin(const uint8_t *data, size_t d_len, 
                       tad_data_unit_t *location)
{
    if (data == NULL || location == NULL)
        return TE_EWRONGPTR;

    if ((location->val_data.oct_str = malloc(d_len)) == NULL)
        return TE_ENOMEM;

    location->du_type = TAD_DU_OCTS;
    location->val_data.len = d_len; 
    memcpy(location->val_data.oct_str, data, d_len);

    return 0;
}

/* See description in tad_utils.h */
int
tad_data_unit_to_bin(const tad_data_unit_t *du_tmpl, 
                     const tad_tmpl_arg_t *args, size_t arg_num, 
                     uint8_t *data_place, size_t d_len)
{
    int rc = 0;

    if (du_tmpl == NULL || data_place == NULL)
        return TE_EWRONGPTR;
    if (d_len == 0)
        return TE_EINVAL;

    switch (du_tmpl->du_type)
    {
        case TAD_DU_EXPR:
        {               
            int64_t iterated, no_iterated;
            rc = tad_int_expr_calculate(du_tmpl->val_int_expr,
                                        args, arg_num, &iterated);
            if (rc != 0)                                    
                ERROR("%s(): int expr calc error %x", __FUNCTION__, rc);
            else
            {
                no_iterated = tad_ntohll(iterated);
                memcpy(data_place, ((uint8_t *)&no_iterated) +
                            sizeof(no_iterated) - d_len,
                       d_len);
            }
        }
            break;

        case TAD_DU_OCTS:
            if (du_tmpl->val_data.oct_str == NULL)
            {
                ERROR("Have no binary data to be sent");
                rc = TE_ETADLESSDATA;
            }
            else
                memcpy(data_place, du_tmpl->val_data.oct_str, d_len);
            break;
        case TAD_DU_I32:
            {
                int32_t no_int = htonl(du_tmpl->val_i32);
                memcpy(data_place,
                       ((uint8_t *)&no_int) + sizeof(no_int) - d_len,
                       d_len);
            }
            break;
        default:
            ERROR("%s(): wrong type %d of DU for send",
                  __FUNCTION__, du_tmpl->du_type);
            rc = TE_ETADLESSDATA;
    }

    return rc;
}

/**
 * Make hex dump of packet into log with RING log level.
 *
 * @param csap    CSAP descriptor structure
 * @param usr_param     string with some user parameter, not used 
 *                      in this callback
 * @param pkt           pointer to packet binary data
 * @param pkt_len       length of packet
 *
 * @return status code
 */
int
tad_dump_hex(csap_p csap, const char *usr_param,
             const uint8_t *pkt, size_t pkt_len)
{
    UNUSED(csap);
    UNUSED(usr_param);

    if (pkt == NULL || pkt_len == 0)
        return TE_EINVAL;

    RING("PACKET: %Tm", pkt, pkt_len);

    return 0;
}


/* See description in tad_utils.h */
int
tad_tcp_push_fin(int socket, const uint8_t *data, size_t length)
{
    int opt = 1;
    int rc;

    if (setsockopt(socket, SOL_TCP, TCP_CORK, &opt, sizeof(opt)) < 0)
    {
        F_ERROR("set CORK on socket %d failed, system errno %d",
                socket, errno);
        return errno;
    }

    if ((rc = send(socket, data, length, 0)) < 0)
    { 
        F_ERROR("Send last FIN & PUSH fail: errno %d", errno);
        return errno;
    }
    else if ((unsigned)rc < length)
    {
        F_ERROR("Send last FIN & PUSH fail: sent %d, less then asked %d",
                rc, length);
        return TE_ETOOMANY;
    }

    if (shutdown(socket, SHUT_WR) < 0)
    {
        F_ERROR("SHUT_WR of %d fail: errno %d", socket, errno);
        return errno;
    }

    return 0;
}



/**
 * Calculate, how much  ways are no insert nds_protos sequence
 * into csap protos sequence. 
 * If there are more then one way, they are not calculated accurately, 
 * just number, greater 1, is returned.
 *
 * @param csap_seq_len  length of array layers
 * @param layers        array with CSAP layers descriptors 
 * @param nds_seq_len   length of array nds_protos
 * @param nds_protos    array with NDS protos tags 
 *
 * @return calculate quantity (zero, 1 or more), or -1 on error. 
 */
static int 
tad_compare_seqs(size_t csap_seq_len, const csap_layer_t *layers,
                 size_t nds_seq_len, const te_tad_protocols_t *nds_protos)
{ 
    int csap_shift = 0;
    int both_shift = 0;

    if (layers == NULL || nds_protos == NULL)
        return -1;

    if (nds_seq_len == 0)
        return 1;

    if (csap_seq_len < nds_seq_len)
        return 0; 

    if (layers[0].proto_tag == nds_protos[0])
        both_shift = tad_compare_seqs(csap_seq_len - 1, layers + 1,
                                      nds_seq_len - 1, nds_protos + 1); 

    if (both_shift <= 1)
        csap_shift = tad_compare_seqs(csap_seq_len - 1, layers + 1,
                                      nds_seq_len, nds_protos);

    /* recursive calls cannot return -1 */
    return csap_shift + both_shift;
}


/* See description in tad_utils.h */
int
tad_check_pdu_seq(csap_p csap, asn_value *pdus)
{
    te_tad_protocols_t *nds_protos = NULL;
    int i;
    int rc = 0;
    int nds_len;

    if (csap == NULL || pdus == NULL)
    {
        ERROR("%s(): NULL ptrs passed", __FUNCTION__);
        return TE_EWRONGPTR;
    }

    nds_len = asn_get_length(pdus, "");
    nds_protos = calloc(nds_len, sizeof(nds_protos[0]));

    for (i = 0; i < nds_len; i++)
    {
        const asn_value *gen_pdu;
        uint16_t pdu_tag;

        rc = asn_get_indexed(pdus, &gen_pdu, i);
        if (rc != 0)
        {
            ERROR("%s(CSAP %d): asn_get_indexed failed %r", 
                  __FUNCTION__, csap->id, rc);
            break;
        }
        rc = asn_get_choice_value(gen_pdu, NULL, NULL, &pdu_tag);
        if (rc != 0)
        {
            ERROR("%s(CSAP %d): asn_get_choice failed %r", 
                  __FUNCTION__, csap->id, rc);
            break;
        } 
        nds_protos[i] = pdu_tag;
    } 

    if (rc == 0) 
    {
        int ways_insert = tad_compare_seqs(csap->depth, 
                                           csap->layers, 
                                           nds_len, 
                                           nds_protos);

        if (ways_insert < 1)
        {
            ERROR("%s(CSAP %d): There is no way to fix PDUs", 
                  __FUNCTION__, csap->id);
            rc = TE_ETADWRONGNDS;
        }
        else if (ways_insert > 1)
        {
            ERROR("%s(CSAP %d): There are many ways to fix PDUs", 
                  __FUNCTION__, csap->id);
            rc = TE_ETADWRONGNDS;
        }
        else /* Ohh, lets fix */
        {
            int pos_in_old_nds = 0;
            char buf[20];
            int syms;

            for (i = 0; i < (int)csap->depth; i++)
            {
                asn_value *new_pdu;

                if (nds_protos[pos_in_old_nds] == 
                    csap->layers[i].proto_tag)
                {
                    pos_in_old_nds++;
                    continue;
                }
                snprintf(buf, sizeof(buf), "%s:{}",
                         csap->layers[i].proto);
                syms = 0;
                rc = asn_parse_value_text(buf, ndn_generic_pdu,
                                          &new_pdu, &syms);
                if (rc != 0)
                {
                    ERROR("%s(CSAP %d) parse '%s' failed %r, sym %d",
                          __FUNCTION__, csap->id, 
                          buf, rc, syms);
                    break;

                }
                rc = asn_insert_indexed(pdus, new_pdu, i, "");
                if (rc != 0)
                {
                    ERROR("%s(CSAP %d) insert new value to %d failed %r",
                          __FUNCTION__, csap->id, i, rc);
                    break;
                }
            }
        }
    }

    free(nds_protos);

    return rc;
}

/* See description in tad_utils.h */
te_tad_protocols_t
te_proto_from_str(const char *proto_txt)
{
    if (proto_txt == NULL)
        return TE_PROTO_INVALID;

    switch (proto_txt[0])
    {
        case 'a':
            if (strcmp(proto_txt + 1, "rp") == 0)
                return TE_PROTO_ARP;
            break;

        case 'b':
            if (strcmp(proto_txt + 1, "ridge") == 0)
                return TE_PROTO_BRIDGE;
            break;

        case 'c':
            if (strcmp(proto_txt + 1, "li") == 0)
                return TE_PROTO_CLI;
            break;

        case 'd':
            if (strcmp(proto_txt + 1, "hcp") == 0)
                return TE_PROTO_DHCP;
            break;

        case 'e':
            if (strcmp(proto_txt + 1, "th") == 0)
                return TE_PROTO_ETH;
            break;

        case 'f':
            if (strcmp(proto_txt + 1, "ile") == 0)
                return TE_PROTO_FILE;
            break;

        case 'i':
            if (strcmp(proto_txt + 1, "p4") == 0)
                return TE_PROTO_IP4;
            if (strcmp(proto_txt + 1, "cmp4") == 0)
                return TE_PROTO_ICMP4;
            if (strcmp(proto_txt + 1, "scsi") == 0)
                return TE_PROTO_ISCSI;
            break;

        case 'p':
            if (strcmp(proto_txt + 1, "cap") == 0)
                return TE_PROTO_PCAP;
            break;

        case 's':
            if (strcmp(proto_txt + 1, "nmp") == 0)
                return TE_PROTO_SNMP;
            break;

        case 't':
            if (strcmp(proto_txt + 1, "cp") == 0)
                return TE_PROTO_TCP;
            break;

        case 'u':
            if (strcmp(proto_txt + 1, "dp") == 0)
                return TE_PROTO_UDP;
            break; 
    } 

    return TE_PROTO_INVALID;
}



const char *
te_proto_to_str(te_tad_protocols_t proto)
{
    switch (proto)
    {
         case TE_PROTO_INVALID:
            return NULL; 

         case TE_PROTO_ARP:
             return "arp";

         case TE_PROTO_BRIDGE:
             return "bridge";

         case TE_PROTO_CLI:
             return "cli";

         case TE_PROTO_DHCP:
             return "dhcp";

         case TE_PROTO_ETH:
             return "eth";

         case TE_PROTO_FILE:
             return "file";

         case TE_PROTO_ICMP4:
             return "icmp4";

         case TE_PROTO_IP4:
             return "ip4";

         case TE_PROTO_ISCSI:
             return "iscsi";

         case TE_PROTO_PCAP:
             return "pcap";

         case TE_PROTO_SNMP:
             return "snmp";

         case TE_PROTO_TCP:
             return "tcp";

         case TE_PROTO_UDP:
             return "udp";
    }
    return NULL;
}


/* See description tad_utils.h */
te_errno
tad_common_write_read_cb(csap_p csap, int timeout,
                         const tad_pkt *w_pkt,
                         char *r_buf, size_t r_buf_len)
{
    te_errno        rc;
    unsigned int    layer = csap_get_rw_layer(csap);

    rc = csap_get_proto_support(csap, layer)->write_cb(csap, w_pkt);
    
    if (rc == 0)  
        rc = csap_get_proto_support(csap, layer)->read_cb(csap, timeout,
                                                          r_buf, r_buf_len);

    return rc;
}
